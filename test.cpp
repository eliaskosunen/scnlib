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
    std::string str;
    scn::scan(file, "{}", str);
    std::fclose(f);
    std::cout << str << '\n';
}
