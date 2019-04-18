#include <cstdio>
#include <string>

void do_scan() {
    int i;
    std::string str(7, '\0');
    sscanf("42 answer", "%d %s", &i, &str[0]);
}
