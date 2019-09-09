#include "include/scn/detail/scan.h"

#include <iostream>

namespace rng = scn::detail::ranges;

template <typename T>
struct debug;

int main()
{
    int i;
    std::string str;
    scn::scan("42 foo", "{} {}", i, str);
    std::cout << i << ' ' << str << '\n';
}
