#include <scn/scn.h>

void do_scan()
{
    int i;
    auto ret = scn::input("{}", i);

    double d;
    ret = scn::input("{}", d);

    long long ll;
    ret = scn::input("{}", ll);

    char buf[7];
    auto buf_span = scn::make_span(buf, 7);
    ret = scn::input("{}", buf_span);

    std::string str;
    ret = scn::input("{}", str);
}
