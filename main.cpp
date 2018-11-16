#include "scn/scn.h"

#include <iostream>
#include <numeric>

int main()
{
    std::vector<char> data{'4', '2', 0};
    std::cout << "Data before scan: '" << data.data() << "'\n";

    int i{0};
    auto ret =
        scn::scan(scn::basic_stream<char, std::vector<char>>(data), "{}", i);

    std::cout << "Data after scan: '" << data.data() << "'\n";
    std::cout << "Scanned integer: " << i << '\n';
    std::cout << "Returned value is an error: " << !ret.has_value() << '\n';
    if (!ret.has_value()) {
        std::cout << "Error code: " << static_cast<int>(ret.error()) << '\n';
    }

    return 0;
}