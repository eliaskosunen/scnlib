#include <scn/scn.h>

void do_scan()
{
    auto i = scn::scan_value<int>(scn::cstdin().lock());
    auto d = scn::scan_value<double>(scn::cstdin().lock());
    auto ll = scn::scan_value<long long>(scn::cstdin().lock());

    auto str = scn::scan_value<std::string>(scn::cstdin().lock());
}
