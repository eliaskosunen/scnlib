#include <scn/ranges.h>

void do_scan() {
    int i;
    std::string str;
    scn::ranges::scan("42 answer", "{} {}", i, str);
}
