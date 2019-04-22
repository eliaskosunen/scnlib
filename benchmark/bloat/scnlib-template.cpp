#include <scn/scn.h>

void do_scan()
{
    auto source = std::string{"42 3.14 999999999 abcdefg test_string"};
    auto stream = scn::make_erased_stream(source);

    int i;
    scn::scan(stream, "{}", i);

    double d;
    scn::scan(stream, "{}", d);

    long long ll;
    scn::scan(stream, "{}", ll);

    char buf[7];
    auto buf_span = scn::make_span(buf, 7);
    scn::scan(stream, "{}", buf_span);

    std::string str;
    scn::scan(stream, "{}", str);
}
