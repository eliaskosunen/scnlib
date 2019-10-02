#include "include/scn/scn.h"

#include <iostream>
#include <vector>

namespace rng = scn::detail::ranges;

template <typename T>
struct debug;

int main()
{
    auto view = scn::make_view("123");

    std::string word;
    auto ret = scn::scan("Hello 42", "{}", word);
    if (!ret) {
        std::cerr << "Whoops: " << ret.error().msg() << '\n';
        return 1;
    }
    int i;
    ret = scn::scan(ret.range(), "{}", i);
    if (!ret) {
        std::cerr << "Whoops: " << ret.error().msg() << '\n';
        return 1;
    }

    std::cout << word << ' ' << i;
}
