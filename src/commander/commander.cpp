#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include "commander.hpp"

using namespace std;

static bool check_num(string str) {
    for (char c : str) {
        if (!isdigit(c)) 
            return false;
    }
    return true;
}

void Commander::parse_argv(char** argv) {

    string type_(argv[3]);

    if (type_ == "issueJob") {
        type = ISSUE_JOB;
        ASSERT_COND(argc > 0, "Usage: %s issueJob <command>\n", argv[0]);

        this->argv = &argv[4];
    }
    else if (type_ == "setConcurrency") {
        type = SET_CONCURRENCY;
        ASSERT_COND(argc == 1, "Usage: %s setConcurrency <natural number>\n", argv[0]);

        string str(argv[4]);
        ASSERT_COND(check_num(str), "'%s' is not a natural number!\n", argv[4]);
        number = atoi(str.data());
    }
    else if (type_ == "stop") {
        type = STOP_JOB;
        ASSERT_COND(argc == 1, "Usage: %s stop job_X\n", argv[0]);

        string str(argv[4]);
        ASSERT_COND(str.size() > 4 && str.substr(0, 4) == "job_" && check_num(str.substr(4)),
                    "'%s' is not of the correct format, with 'X' being a natural number!\n", argv[4]);
        number = atoi(str.substr(4).data());
    }
    else if (type_ == "poll") {
        type = POLL;
        ASSERT_COND(argc == 0, "Usage: %s poll\n", argv[0]);
    }
    else if (type_ == "exit") {
        type = EXIT;
        ASSERT_COND(argc == 0, "Usage: %s exit\n", argv[0]);
    }
    else {
        printf("Commands available: 'issueJob', 'setConcurrency', 'stop', 'poll', 'exit'\n");
        std::exit(0);
    }

}

Commander::Commander(size_t argc_, char** argv): argc(argc_ - 4), number(0) {
    ASSERT_COND(argc_ >= 4, "Usage: %s <serverName> <portNum> <jobCommanderInputCommand> <optionalArgs>\n", argv[0]);

    parse_argv(argv);

    ASSERT_NEQ(sock = socket(AF_INET, SOCK_STREAM, 0), -1);

    struct hostent *rem;
    ASSERT_NEQ(rem = gethostbyname(argv[1]), NULL);

    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);

    string str(argv[2]);
    ASSERT_COND(check_num(str), "'%s' is not a natural number!\n", argv[2]);

    port = atoi(str.data());
    server.sin_port = htons(port);
}

Commander::~Commander() {
    ASSERT_NEQ(close(sock), -1);
}

void Commander::communicate(char** argv) {
    ASSERT_NEQ(connect(sock, (struct sockaddr*) &server, sizeof(server)), -1);

    socket_write(sock, &type, sizeof(type));

    if (type == ISSUE_JOB)
        issue_job();
    else if (type == SET_CONCURRENCY)
        set_concurrency();
    else if (type == STOP_JOB)
        stop_job();
    else if (type == POLL)
        poll();
    else if (type == EXIT)
        this->exit();
}

void Commander::issue_job() {
    send_args(sock, argc, argv);
    receive_job(); 
}

void Commander::stop_job() {
    size_t size;
    fd_read(fdr, &size, sizeof(size)); // Size of job_XX removed/terminated

    char* msg = new char[size];
    fd_read(fdr, msg, size); // job_XX removed/terminated

    printf("%s\n", msg);

    delete [] msg;  
}

void Commander::poll(CommandType type) {

    printf("%s\n", type == POLL_QUEUED ? "+-----------+\n|Queued Jobs|\n+-----------+" : "+------------+\n|Running Jobs|\n+------------+");

    size_t size;
    fd_read(fdr, &size, sizeof(size)); // Nuber of jobs

    for (size_t i = 0; i < size; i++)
        receive_job();    
}

void Commander::exit() {    
    char msg[sizeof(TERMINATION_MSG)];
    fd_read(fdr, msg, sizeof(TERMINATION_MSG));

    if (string(msg) != TERMINATION_MSG) {
        printf("Termination message received from server was corrupted!\n");
        std::exit(EXIT_FAILURE);
    }

    printf("%s\n", TERMINATION_MSG);
}
