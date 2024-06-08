#pragma once
#include <netinet/in.h>
#include "common.hpp"

class Commander {
    private:
        int sock;
        uint16_t port;
        struct sockaddr_in server;

        size_t argc;
        char** argv;
        CommandType type;
        size_t number;

        void parse_argv(char** argv);

        void issue_job();
        void set_concurrency();
        void stop_job();
        void poll();
        void exit();
    public:
        Commander(size_t argc_, char** argv);
        ~Commander();
        void communicate();
};