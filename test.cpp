#include "include/scn/scn.h"

#include <iostream>
#include <vector>

namespace rng = scn::detail::ranges;

template <typename T>
struct debug;

int main()
{
    auto wrapped = scn::detail::wrap("42");
    auto rewrapped = scn::detail::wrap(wrapped);
}
