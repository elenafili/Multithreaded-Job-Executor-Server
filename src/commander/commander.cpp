#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <signal.h>
#include "commander.hpp"

using namespace std;

// Assert correctness of command line arguments given to the commander
void Commander::parse_argv(char** argv) {
    string type_(argv[3]);

    if (type_ == "issueJob") {
        type = ISSUE_JOB;
        ASSERT_COND(argc > 0, "Usage: %s <serverName> <portNum> issueJob <command>\n", argv[0]);

        this->argv = &argv[4];
    }
    else if (type_ == "setConcurrency") {
        type = SET_CONCURRENCY;
        ASSERT_COND(argc == 1, "Usage: %s <serverName> <portNum> setConcurrency <natural number>\n", argv[0]);

        string str(argv[4]);
        ASSERT_COND(check_num(str), "'%s' is not a natural number!\n", argv[4]);
        number = atoi(str.data());
    }
    else if (type_ == "stop") {
        type = STOP_JOB;
        ASSERT_COND(argc == 1, "Usage: %s <serverName> <portNum> stop job_X\n", argv[0]);

        string str(argv[4]);
        ASSERT_COND(str.size() > 4 && str.substr(0, 4) == "job_" && check_num(str.substr(4)),
                    "'%s' is not of the correct format, with 'X' being a natural number!\n", argv[4]);
        number = atoi(str.substr(4).data());
    }
    else if (type_ == "poll") {
        type = POLL;
        ASSERT_COND(argc == 0, "Usage: %s <serverName> <portNum> poll\n", argv[0]);
    }
    else if (type_ == "exit") {
        type = EXIT;
        ASSERT_COND(argc == 0, "Usage: %s <serverName> <portNum> exit\n", argv[0]);
    }
    else {
        cout << "Commands available: 'issueJob', 'setConcurrency', 'stop', 'poll', 'exit'" << endl;
        std::exit(0);
    }
}

Commander::Commander(size_t argc_, char** argv) : argc(argc_ - 4), number(0) {
    ASSERT_COND(argc_ >= 4, "Usage: %s <serverName> <portNum> <jobCommanderInputCommand> <optionalArgs>\n", argv[0]);

    parse_argv(argv);

    // Setup the socket which will be used to connect to the server
    ASSERT_NEQ(sock = socket(AF_INET, SOCK_STREAM, 0), -1);

    struct hostent* rem;
    ASSERT_NEQ(rem = gethostbyname(argv[1]), NULL);

    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);

    string str(argv[2]);
    ASSERT_COND(check_num(str), "'%s' is not a natural number!\n", argv[2]);

    port = atoi(str.data());
    server.sin_port = htons(port);
}

Commander::~Commander() {
    ASSERT_NEQ(shutdown(sock, SHUT_WR), -1 && errno != ENOTCONN);
    ASSERT_NEQ(close(sock), -1);
}

void Commander::communicate() {
    ASSERT_NEQ(connect(sock, (struct sockaddr*) &server, sizeof(server)), -1);

    TRY_CATCH(
        // Send the type of command
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
        ,
        cout << "Connection to the server was unexpectedly terminated!" << endl;
    )
}

void Commander::issue_job() {
    send_args(sock, argc, argv);

    while (1) {
        size_t len;
        socket_read(sock, &len, sizeof(len));

        if (len == 0)
            break;

        char* str = new char[len];
        TRY_CATCH(
            socket_read(sock, str, len);
            cout << str << flush;,
        )
        delete [] str;        
    }
}

void Commander::set_concurrency() {
    socket_write(sock, &number, sizeof(number));
    cout << receive_msg(sock) << endl;
}

void Commander::stop_job() {
    socket_write(sock, &number, sizeof(number));
    cout << receive_msg(sock) << endl;
}

void Commander::poll() {
    size_t queue_sz;
    socket_read(sock, &queue_sz, sizeof(queue_sz));

    for (size_t i = 0; i < queue_sz; i++)
        cout << receive_msg(sock) << endl;
}

void Commander::exit() {
    cout << receive_msg(sock) << endl;
}
