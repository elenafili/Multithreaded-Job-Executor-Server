#pragma once
#include <iostream>
#include <list>
#include <pthread.h>

typedef struct Job Job;

class Server {
    private:
        int sock;
        uint16_t port;
        size_t buffer_sz;
        size_t thread_pool_sz;
        size_t concurrency;

        pthread_mutex_t lock;
        pthread_cond_t cond;

        size_t jids;
        std::list<Job> ready_queue;

        void parse_argv(char** argv);
        void send_job(int sock, Job& job, std::string prefix="", std::string suffix="");
        static void* handle_command(void* args); 

        void issue_job(int sock);
        void set_concurrency(int sock);
        void stop_job(int sock);
        void poll(int sock);
        void exit(int sock);
    public:
        Server(size_t argc_, char** argv);
        ~Server();
        void run();
};