#include <sstream>

void do_scan() {
    std::istringstream source("42 answer");

    int i;
    std::string str;
    source >> i >> str;
}
