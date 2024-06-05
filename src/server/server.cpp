#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include<fcntl.h> 
#include <ctype.h>
#include <cstring>
#include <algorithm>
// #include <format>
#include "common.hpp"
#include "server.hpp"

#define BUFFER_SIZE 256

#define DASHES "-----"
#define WRAP(x) (DASHES + string(x) + DASHES + "\n")
#define JOB_(job) ("job_" + to_string(job.jid))
#define JOB(job) ("<" + JOB_(job) + ", " + string(args_concat(job)) + ">")

using namespace std;

struct ControllerArgs {
    Server* server;
    int socket;

    ControllerArgs(Server* server_, int socket_) : server(server_), socket(socket_) {}
};

struct Job {
    pid_t pid;
    size_t jid;
    size_t argc;
    char** argv;
    int socket;
    
    Job(pid_t pid_, size_t jid_, size_t argc_, char** argv_, int socket_)
    : pid(pid_), jid(jid_), argc(argc_), argv(argv_), socket(socket_) {}

    void delete_args() {
        for (size_t i = 0; i < argc; i++)
            delete [] argv[i];
        delete [] argv;
    }

    bool operator==(const Job& other) const { return jid == other.jid; }
    bool operator==(const size_t& other) const { return jid == other; }
};

static string args_concat(Job& job) {
    string ret(job.argv[0]);
    
    for (size_t i = 1; i < job.argc; i++)
        ret += string(" ") + job.argv[i];

    return ret;    
}

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

Job Server::pop() {
    ASSERT_EQ(pthread_mutex_lock(&lock), 0);

    while (running == concurrency || ready_queue.size() == 0)
        ASSERT_EQ(pthread_cond_wait(&cond_full, &lock), 0);

    Job job = ready_queue.front();
    ready_queue.pop_front();
    running++;

    ASSERT_EQ(pthread_mutex_unlock(&lock), 0);
	ASSERT_EQ(pthread_cond_signal(&cond_empty), 0);

    return job;
}

void* Server::worker(void* args) {
    Server* server = (Server*) args;

    while(1) {
        Job job = server->pop();
        server->exec_job(job);
    }
}

void Server::exec_job(Job& job) {
    int child_pid;
    ASSERT_NEQ(child_pid = fork(), -1);

    if (child_pid == 0) {
        string name = "output/" + to_string(getpid()) + ".output";
    
        int fd;
        ASSERT_NEQ(fd = open(name.data(), O_WRONLY | O_CREAT, 0644), -1);
        ASSERT_NEQ(dup2(fd, STDOUT_FILENO), -1);

        ASSERT_NEQ(execvp(job.argv[0], job.argv), -1);
    } 
    else {
        job.pid = child_pid;
        int status;
        do {
            ASSERT_NEQ(waitpid(child_pid, &status, 0), -1);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        
        running--;

        string name = "output/" + to_string(child_pid) + ".output";
        FILE* file = fopen(name.data(), "r");

        send_msg(job.socket, WRAP(JOB_(job) + " output start"));

        char buffer[BUFFER_SIZE + 1];
        size_t bytes = 0;

        while ( (bytes = fread(buffer, sizeof(char), BUFFER_SIZE, file)) > 0) {
            buffer[bytes] = 0;
            send_msg(job.socket, buffer);
        }

        send_msg(job.socket, WRAP(JOB_(job) + " output end"));

        size_t num = 0;
        socket_write(job.socket, &num, sizeof(num));

        ASSERT_EQ(fclose(file), 0);
        ASSERT_NEQ(close(job.socket), -1);
    }
}

Server::Server(size_t argc_, char** argv) : concurrency(1), running(0), jids(1) {
    ASSERT_COND(argc_ == 4, "Usage: %s <portNum> <bufferSize> <threadPoolSize>\n", argv[0]);
    parse_argv(argv);

    ASSERT_NEQ(sock = socket(AF_INET, SOCK_STREAM, 0), -1);

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

    pthread_t* worker_tids = new pthread_t[thread_pool_sz];

    for (size_t i = 0; i < thread_pool_sz; i++) {
        ASSERT_EQ(pthread_create(&(worker_tids[i]), NULL, worker, this), 0);
    }
}

// WORKER TIDS join + delete + destroy files
Server::~Server() {
    ASSERT_NEQ(close(sock), -1);
    ASSERT_EQ(pthread_mutex_destroy(&lock), 0);
    ASSERT_EQ(pthread_cond_destroy(&cond_full), 0);
    ASSERT_EQ(pthread_cond_destroy(&cond_empty), 0);
}

void Server::run() {
    int newsock;

    while(1) {

        ASSERT_NEQ(newsock = accept(sock, NULL, NULL), -1)

        pthread_t controller;
        ASSERT_EQ(pthread_create(&controller, NULL, handle_command, new ControllerArgs(this, newsock)), 0);
        ASSERT_EQ(pthread_detach(controller), 0);
    }
}

void* Server::handle_command(void* args) {
    ControllerArgs* cargs = (ControllerArgs*) args;
    CommandType type;
    socket_read(cargs->socket, &type, sizeof(type));

    if (type == ISSUE_JOB)
        cargs->server->issue_job(cargs->socket);
    else if (type == SET_CONCURRENCY)
        cargs->server->set_concurrency(cargs->socket);
    else if (type == STOP_JOB)
        cargs->server->stop_job(cargs->socket);
    else if (type == POLL)
        cargs->server->poll(cargs->socket);
    else 
        cargs->server->exit(cargs->socket);

    if (type != ISSUE_JOB)
        ASSERT_NEQ(close(cargs->socket), -1);
    delete cargs;

    return NULL;
}

void Server::issue_job(int sock) {

    size_t argc;
    char** argv = receive_args(sock, &argc);

    Job job(-1, jids++, argc, argv, sock);

    ASSERT_EQ(pthread_mutex_lock(&lock), 0);

    while (ready_queue.size() == buffer_sz)
        ASSERT_EQ(pthread_cond_wait(&cond_empty, &lock), 0);
   
    ready_queue.push_back(job);
    ASSERT_EQ(pthread_mutex_unlock(&lock), 0);

    send_msg(job.socket, "JOB " + JOB(job) + " SUBMITTED\n");
	// send_msg(job.socket, format("JOB <job_{}, {}> SUBMITTED\n", job.jid, args_concat(job));
    ASSERT_EQ(pthread_cond_signal(&cond_full), 0);

}

void Server::set_concurrency(int sock) {
    size_t concurrency_;
    socket_read(sock, &concurrency_, sizeof(concurrency_));

    ASSERT_EQ(pthread_mutex_lock(&lock), 0);
    concurrency = concurrency_;
    ASSERT_EQ(pthread_mutex_unlock(&lock), 0);

    string msg = "CONCURRENCY SET AT " + to_string(concurrency_);
    size_t len = msg.length() + 1;

    socket_write(sock, &len, sizeof(len));
    socket_write(sock, msg.data(), len);
}

void Server::stop_job(int sock) {
    size_t job_id;
    socket_read(sock, &job_id, sizeof(job_id));

    Job dummy(-1, job_id, 0, NULL, 0);

    pthread_mutex_lock(&lock);
    auto it = find(ready_queue.begin(), ready_queue.end(), (size_t)job_id);
    int job_sock = -1;

    if (it != ready_queue.end()) {
        job_sock = (*it).socket;
        (*it).delete_args();
    }

    ready_queue.remove(dummy);
    pthread_mutex_unlock(&lock);

    string ret = "JOB <" + JOB_(dummy) + "> ";
    ret += (job_sock != -1) ? "REMOVED" : "NOT FOUND";

    send_msg(sock, ret);

    if (job_sock > 0) {
        string term_msg = "JOB <" + JOB_(dummy) + "> WAS TERMINATED\n";
        send_msg(job_sock, term_msg);

        size_t num = 0;
        socket_write(job_sock, &num, sizeof(num));
    }
}

void Server::poll(int sock) {
    pthread_mutex_lock(&lock);

    size_t queue_sz = ready_queue.size();
    socket_write(sock, &queue_sz, sizeof(queue_sz));

    for (auto job : ready_queue)
        send_msg(sock, JOB(job));
    
    pthread_mutex_unlock(&lock);
}

void Server::exit(int sock) {    
    printf("exit iou\n");
}
