#include <scn/scn.h>

void do_scan() {
    std::string source("42 answer");
    auto stream = scn::make_stream(source);

    int i;
    std::string str;
    scn::scan(stream, "{} {}", i, str);
}
