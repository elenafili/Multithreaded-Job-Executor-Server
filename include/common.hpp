#pragma once
#include <iostream>
#include <unistd.h>

#define ASSERT_COND(cond, ...) {    \
    if (!(cond)) {                  \
        printf(__VA_ARGS__);        \
        std::exit(0);               \
    }                               \
}

#define ASSERT_EQ(call, value) {    \
    if ((call) != value) {          \
        perror(#call);              \
        std::exit(EXIT_FAILURE);    \
    }                               \
}

#define ASSERT_NEQ(call, value) {   \
    if ((call) == value) {          \
        perror(#call);              \
        std::exit(EXIT_FAILURE);    \
    }                               \
}

#define ASSERT_GEQ(call, value) {   \
    if ((call) < value) {           \
        perror(#call);              \
        std::exit(EXIT_FAILURE);    \
    }                               \
}

#define TRY_CATCH(x, y) { try { x } catch (Exception& e) { y } }     

typedef enum {
    ISSUE_JOB,
    SET_CONCURRENCY,
    STOP_JOB,
    POLL,
    EXIT,
} CommandType;

class Exception : public std::exception
{
	private:
		const std::string msg;

    public:
        Exception(const std::string& msg_);
        virtual const char* what() const throw ();
};

bool check_num(std::string str);

void socket_read(int sock, void* address, int size);
void socket_write(int sock, void* address, int size);

void send_msg(int sock, std::string msg);
std::string receive_msg(int sock);

void send_args(int sock, size_t argc, char** argv);
char** receive_args(int sock, size_t* argc);
