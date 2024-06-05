#include "commander.hpp"

int main(int argc, char* argv[]) {

    Commander commander((size_t)argc, argv);
    commander.communicate();
    return 0;
}