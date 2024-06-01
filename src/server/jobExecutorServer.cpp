#include "server.hpp"

int main(int argc, char* argv[]) {

    Server server((size_t)argc, argv);
    server.run();
    
    return 0;
}
