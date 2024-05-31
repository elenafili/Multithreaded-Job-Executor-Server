#pragma once
#include <iostream>

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

typedef enum {
    ISSUE_JOB,
    SET_CONCURRENCY,
    STOP_JOB,
    POLL,
    EXIT,
} CommandType;

void socket_read(int sock, void* address, int size);
void socket_write(int sock, void* address, int size);

void send_args(int sock, size_t argc, char** argv);
