#include <scn/ranges.h>

void do_scan()
{
    std::string source("42 3.14 999999999 string longerstring");

    int i;
    scn::ranges::scan(source, "{}", i);

    double d;
    scn::ranges::scan(source, "{}", d);

    long long ll;
    scn::ranges::scan(source, "{}", ll);

    char buf[7];
    auto buf_span = scn::make_span(buf, 7);
    scn::ranges::scan(source, "{}", buf_span);

    std::string str;
    scn::ranges::scan(source, "{}", str);
}
