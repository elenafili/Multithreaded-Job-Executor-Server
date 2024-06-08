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
        size_t active_workers;
        size_t active_controllers;
        bool flag;

        pthread_t main_thread;
        pthread_t* worker_tids;

        pthread_mutex_t lock;
        pthread_cond_t cond_full;
        pthread_cond_t cond_empty;
        pthread_cond_t cond_workers;
        pthread_cond_t cond_controllers;

        size_t jids;
        std::list<Job> ready_queue;

        void parse_argv(char** argv);

        static void* handle_command(void* args);
        void issue_job(int sock);
        void set_concurrency(int sock);
        void stop_job(int sock);
        void poll(int sock);
        void exit(int sock);

        static void* worker(void* args);
        Job pop();
        void exec_job(Job& job);
        
    public:
        Server(size_t argc_, char** argv);
        ~Server();
        void run();
};