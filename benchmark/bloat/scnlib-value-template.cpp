#include <scn/scn.h>

void do_scan()
{
    auto i = scn::scan_value<int>(scn::cstdin());
    auto d = scn::scan_value<double>(scn::cstdin());
    auto ll = scn::scan_value<long long>(scn::cstdin());

    char buf[7];
    auto buf_span = scn::make_span(buf, 7);
    auto ret = scn::input("{}", buf_span);

    auto str = scn::scan_value<std::string>(scn::cstdin());
}
