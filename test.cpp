#include "include/scn/scn.h"

#include <iostream>
#include <vector>

namespace rng = scn::detail::ranges;

template <typename T>
struct debug;

int main()
{
    auto f = scn::file(stdin);
    auto w = scn::wrap(f);

    std::string word;
    auto ret = scn::scan(w, "{}", word);
    w = ret.range();
    if (!ret) {
        std::cerr << "Whoops: " << ret.error().msg() << '\n';
        return 1;
    }

    int i;
#if 0
    ret = scn::scan(w, "{}", i);
    if (!ret) {
        std::cerr << "Whoops: " << ret.error().msg() << '\n';
        return 1;
    }
#else
    ret.range().sync();
    std::cin >> i;
#endif

    std::cout << i << ' ' << word;
}
