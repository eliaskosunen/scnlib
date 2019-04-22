#include <scn/scn.h>

void do_scan()
{
    int i;
    scn::input("{}", i);

    double d;
    scn::input("{}", d);

    long long ll;
    scn::input("{}", ll);

    char buf[7];
    auto buf_span = scn::make_span(buf, 7);
    scn::input("{}", buf_span);

    std::string str;
    scn::input("{}", str);
}
