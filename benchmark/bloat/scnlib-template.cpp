#include <scn/scn.h>

void do_scan()
{
    auto source = std::string{"42 3.14 999999999 abcdefg test_string"};
    auto range = scn::wrap(source);

    int i;
    auto ret = scn::scan(range, "{}", i);
    range = ret.range();

    double d;
    ret = scn::scan(range, "{}", d);
    range = ret.range();

    long long ll;
    ret = scn::scan(range, "{}", ll);
    range = ret.range();

    char buf[7];
    auto buf_span = scn::make_span(buf, 7);
    ret = scn::scan(range, "{}", buf_span);
    range = ret.range();

    std::string str;
    ret = scn::scan(range, "{}", str);
    range = ret.range();
}
