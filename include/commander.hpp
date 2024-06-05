#include <netinet/in.h>
#include "common.hpp"

class Commander {
    private:
        size_t argc;
        char** argv;
        CommandType type;
        size_t number;
        uint16_t port;
        int sock;
        struct sockaddr_in server;

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