#include <sys/wait.h>
#include <netdb.h>
#include <fcntl.h>
#include <algorithm>
#include "common.hpp"
#include "server.hpp"

#define BUFFER_SIZE 256

#define DASHES "-----"
#define WRAP(x) (DASHES + string(x) + DASHES + "\n")
#define JOB_(job) ("job_" + to_string(job.jid))
#define JOB(job) ("<" + JOB_(job) + ", " + string(args_concat(job)) + ">")

#define SEND_EOF(sock) {size_t num = 0; socket_write(sock, &num, sizeof(num));}

using namespace std;


struct ControllerArgs {
    Server* server;
    int socket;

    ControllerArgs(Server* server_, int socket_) : server(server_), socket(socket_) {}
};

struct Job {
    size_t jid;
    size_t argc;
    char** argv;
    int socket;
    
    Job(size_t jid_, size_t argc_, char** argv_, int socket_)
    : jid(jid_), argc(argc_), argv(argv_), socket(socket_) {}

    void delete_args() {
        for (size_t i = 0; i < argc; i++)
            delete [] argv[i];
        delete [] argv;
    }

    bool operator==(const Job& other) const { return jid == other.jid; }
    bool operator==(const size_t& other) const { return jid == other; }
};

typedef void (*Handler)(int);


// +----------------------+
// |   Helper Functions   |
// +----------------------+

static bool stop_server = false; 
static int public_sock = 0;

static void handler(int) {
    stop_server = true;
    ASSERT_NEQ(close(public_sock), -1);
}

static void set_handler(int signum, Handler handler) {
    struct sigaction sa;

    sa.sa_handler = handler;

    ASSERT_NEQ(sigemptyset(&sa.sa_mask), -1);
    sa.sa_flags = SA_RESTART;

    ASSERT_NEQ(sigaction(signum, &sa, NULL), -1);
}

static string args_concat(Job& job) {
    string ret(job.argv[0]);
    
    for (size_t i = 1; i < job.argc; i++)
        ret += string(" ") + job.argv[i];

    return ret;    
}


// +------------+
// |   Server   |
// +------------+

void Server::parse_argv(char** argv) {
    string str1(argv[1]);
    ASSERT_COND(check_num(str1), "'%s' is not a natural number!\n", argv[1]);
    ASSERT_GEQ(port = atoi(str1.data()), 1);
    
    string str2(argv[2]);
    ASSERT_COND(check_num(str2), "'%s' is not a natural number!\n", argv[2]); 
    ASSERT_GEQ(buffer_sz = atoi(str2.data()), 1);

    string str3(argv[3]);
    ASSERT_COND(check_num(str3), "'%s' is not a natural number!\n", argv[3]); 
    ASSERT_GEQ(thread_pool_sz = atoi(str3.data()), 1);
}

Server::Server(size_t argc_, char** argv) 
: concurrency(1), active_workers(0), active_controllers(0), flag(false), 
  main_thread(pthread_self()), jids(1) {
    set_handler(SIGUSR1, handler);
    ASSERT_COND(argc_ == 4, "Usage: %s <portNum> <bufferSize> <threadPoolSize>\n", argv[0]);
    parse_argv(argv);

    ASSERT_NEQ(public_sock = sock = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0), -1);

    int temp = 1;
    ASSERT_NEQ(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &temp, sizeof(int)), -1);

    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    ASSERT_NEQ(bind(sock, (struct sockaddr*) &server, sizeof(server)), -1)
    ASSERT_NEQ(listen(sock, 1 << 8) , -1);

    ASSERT_EQ(pthread_mutex_init(&lock, NULL), 0);
    ASSERT_EQ(pthread_cond_init(&cond_full, NULL), 0);
    ASSERT_EQ(pthread_cond_init(&cond_empty, NULL), 0);
    ASSERT_EQ(pthread_cond_init(&cond_workers, NULL), 0);
    ASSERT_EQ(pthread_cond_init(&cond_controllers, NULL), 0);

    worker_tids = new pthread_t[thread_pool_sz];

    for (size_t i = 0; i < thread_pool_sz; i++)
        ASSERT_EQ(pthread_create(&(worker_tids[i]), NULL, worker, this), 0);
}

Server::~Server() {
    for (size_t i = 0; i < thread_pool_sz; i++)
        pthread_join(worker_tids[i], NULL);
    
    delete [] worker_tids;

    ASSERT_EQ(pthread_mutex_destroy(&lock), 0);
    ASSERT_EQ(pthread_cond_destroy(&cond_full), 0);
    ASSERT_EQ(pthread_cond_destroy(&cond_empty), 0);
    ASSERT_EQ(pthread_cond_destroy(&cond_workers), 0);
    ASSERT_EQ(pthread_cond_destroy(&cond_controllers), 0);
}

void Server::run() {
    int private_sock;

    while(1) {
        ASSERT_NEQ(private_sock = accept4(sock, NULL, NULL, SOCK_CLOEXEC), -1 && stop_server == false);

        if (private_sock == -1 && stop_server)
            break;
        
        ASSERT_EQ(pthread_mutex_lock(&lock), 0);
        active_controllers++;
        ASSERT_EQ(pthread_mutex_unlock(&lock), 0);

        pthread_t controller;
        ASSERT_EQ(pthread_create(&controller, NULL, handle_command, new ControllerArgs(this, private_sock)), 0);
    }

    ASSERT_EQ(pthread_mutex_lock(&lock), 0);

    ASSERT_EQ(pthread_cond_broadcast(&cond_full), 0);
    ASSERT_EQ(pthread_cond_broadcast(&cond_empty), 0);

    for (auto& job : ready_queue) {
        send_msg(job.socket, "SERVER TERMINATED BEFORE EXECUTION\n");
        SEND_EOF(job.socket);

        job.delete_args();
        ASSERT_NEQ(close(job.socket), -1);
    }
    ready_queue.clear();

    while (active_workers > 0)
        ASSERT_EQ(pthread_cond_wait(&cond_workers, &lock), 0);    

    while (active_controllers > 0)
        ASSERT_EQ(pthread_cond_wait(&cond_controllers, &lock), 0);

    ASSERT_EQ(pthread_mutex_unlock(&lock), 0);
}


// +------------------------+
// |   Controller Threads   |
// +------------------------+

void* Server::handle_command(void* args) {
    ASSERT_EQ(pthread_detach(pthread_self()), 0);

    ControllerArgs* cargs = (ControllerArgs*) args;
    Server* server = cargs->server;
    int sock = cargs->socket;

    int temp = 0;
    socket_write(sock, &temp, sizeof(temp));

    CommandType type;
    socket_read(sock, &type, sizeof(type));

    if (type == ISSUE_JOB)
        server->issue_job(sock);
    else if (type == SET_CONCURRENCY)
        server->set_concurrency(sock);
    else if (type == STOP_JOB)
        server->stop_job(sock);
    else if (type == POLL)
        server->poll(sock);
    else 
        server->exit(sock);

    if (type != ISSUE_JOB)
        ASSERT_NEQ(close(sock), -1);
    
    ASSERT_EQ(pthread_mutex_lock(&server->lock), 0);
    server->active_controllers--;
    ASSERT_EQ(pthread_cond_signal(&server->cond_controllers), 0);
    ASSERT_EQ(pthread_mutex_unlock(&server->lock), 0);

    delete cargs;

    return NULL;
}

void Server::issue_job(int sock) {
    size_t argc;
    char** argv = receive_args(sock, &argc);

    ASSERT_EQ(pthread_mutex_lock(&lock), 0);

    Job job(jids++, argc, argv, sock);

    while (ready_queue.size() == buffer_sz && flag == false)
        ASSERT_EQ(pthread_cond_wait(&cond_empty, &lock), 0);
    
    if (flag == false)
        ready_queue.push_back(job);
    else
        job.delete_args();

    ASSERT_EQ(pthread_mutex_unlock(&lock), 0);

    send_msg(job.socket, "JOB " + JOB(job) + (flag ? " NOT" : "") + " SUBMITTED\n");
    ASSERT_EQ(pthread_cond_signal(&cond_full), 0);
}

void Server::set_concurrency(int sock) {
    size_t concurrency_;
    socket_read(sock, &concurrency_, sizeof(concurrency_));

    ASSERT_EQ(pthread_mutex_lock(&lock), 0);
    concurrency = concurrency_;
    ASSERT_EQ(pthread_mutex_unlock(&lock), 0);
    ASSERT_EQ(pthread_cond_broadcast(&cond_full), 0);

    send_msg(sock, "CONCURRENCY SET AT " + to_string(concurrency_));
}

void Server::stop_job(int sock) {
    size_t job_id;
    socket_read(sock, &job_id, sizeof(job_id));

    Job dummy(job_id, 0, NULL, 0);

    ASSERT_EQ(pthread_mutex_lock(&lock), 0);
    auto it = find(ready_queue.begin(), ready_queue.end(), (size_t)job_id);
    int job_sock = -1;

    if (it != ready_queue.end()) {
        job_sock = (*it).socket;
        (*it).delete_args();
    }

    ready_queue.remove(dummy);
    ASSERT_EQ(pthread_mutex_unlock(&lock), 0);

    string ret = "JOB <" + JOB_(dummy) + "> ";
    ret += (job_sock != -1) ? "REMOVED" : "NOT FOUND";

    send_msg(sock, ret);

    if (job_sock > 0) {
        send_msg(job_sock, "JOB <" + JOB_(dummy) + "> WAS TERMINATED\n");
        SEND_EOF(job_sock); 

        ASSERT_NEQ(close(job_sock), -1);
    }
}

void Server::poll(int sock) {
    ASSERT_EQ(pthread_mutex_lock(&lock), 0);

    size_t queue_sz = ready_queue.size();
    socket_write(sock, &queue_sz, sizeof(queue_sz));

    for (auto job : ready_queue)
        send_msg(sock, JOB(job));
    
    ASSERT_EQ(pthread_mutex_unlock(&lock), 0);
}

void Server::exit(int sock) {
    ASSERT_EQ(pthread_mutex_lock(&lock), 0);

    if (!flag) {
        flag = true;
        ASSERT_EQ(pthread_kill(main_thread, SIGUSR1), 0);
    }

    ASSERT_EQ(pthread_mutex_unlock(&lock), 0);

    send_msg(sock, "SERVER TERMINATED");
}


// +--------------------+
// |   Worker Threads   |
// +--------------------+

void* Server::worker(void* args) {
    Server* server = (Server*) args;

    while(1) {
        Job job = server->pop();
        if (job.jid == 0)
            break;

        server->exec_job(job);
    }

    return NULL;
}

Job Server::pop() {
    Job job(0, 0, NULL, -1);

    ASSERT_EQ(pthread_mutex_lock(&lock), 0);

    while ((active_workers == concurrency || ready_queue.size() == 0) && flag == false)
        ASSERT_EQ(pthread_cond_wait(&cond_full, &lock), 0);

    if (flag == false) {
        job = ready_queue.front();
        ready_queue.pop_front();
        active_workers++;
    }

    ASSERT_EQ(pthread_mutex_unlock(&lock), 0);
	ASSERT_EQ(pthread_cond_signal(&cond_empty), 0);

    return job;
}

void Server::exec_job(Job& job) {
    int child_pid;
    ASSERT_NEQ(child_pid = fork(), -1);

    if (child_pid == 0) {
        string name = "out/" + to_string(getpid()) + ".output";
    
        int fd;
        ASSERT_NEQ(fd = open(name.data(), O_WRONLY | O_CREAT | O_CLOEXEC, 0644), -1);
        ASSERT_NEQ(dup2(fd, STDOUT_FILENO), -1);

        ASSERT_NEQ(execvp(job.argv[0], job.argv), -1);
    } 
    else {
        int status;
        do {
            ASSERT_NEQ(waitpid(child_pid, &status, 0), -1);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));

        string name = "out/" + to_string(child_pid) + ".output";
        FILE* file;
        ASSERT_NEQ(file = fopen(name.data(), "r"), NULL);

        send_msg(job.socket, WRAP(JOB_(job) + " output start"));

        char buffer[BUFFER_SIZE + 1];
        size_t bytes = 0;

        while ( (bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file)) > 0) {
            buffer[bytes] = 0;
            send_msg(job.socket, buffer);
        }

        send_msg(job.socket, WRAP(JOB_(job) + " output end"));

        SEND_EOF(job.socket); 

        ASSERT_EQ(fclose(file), 0);
        ASSERT_EQ(remove(name.data()), 0);
        ASSERT_NEQ(close(job.socket), -1);

        job.delete_args();
        
        ASSERT_EQ(pthread_mutex_lock(&lock), 0);
        active_workers--;
        ASSERT_EQ(pthread_cond_signal(&cond_workers), 0);
        ASSERT_EQ(pthread_mutex_unlock(&lock), 0);
    }
}