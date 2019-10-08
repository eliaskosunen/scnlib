#include <scn/scn.h>

#include <iostream>
#include <vector>

namespace rng = scn::detail::ranges;

template <typename T>
struct debug;

int main()
{
    auto f = std::fopen("test.cpp", "r");
    auto file = scn::file(f);

    int i;
    auto ret = scn::scan(file, "{}", i);
    SCN_ENSURE(!ret);

    std::string str;
    ret = scn::scan(file, "{}", str);
    std::cout << str << '\n';

    std::fclose(f);
}
