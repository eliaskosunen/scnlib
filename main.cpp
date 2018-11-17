#include "scn/scn.h"
#include "scn/istream.h"

#include <iostream>
#include <numeric>

int main()
{
    std::string data{"42 3.14 foobar true"};
    std::cout << "Data before scan: '" << data.data() << "'\n";

    int i{0};
    double d{};
    std::string s(6, '\0');
    auto span = scn::make_span(&s[0], &s[0] + s.size());
    bool b{};
    auto ret = scn::scan(scn::make_stream(data), "{} {} {} {}", i, d, span, b);

    std::cout << "Data after scan: '" << data.data() << "'\n";
    std::cout << "Scanned integer: " << i << '\n';
    std::cout << "Scanned double: " << d << '\n';
    std::cout << "Scanned string: '" << s << "'\n";
    std::cout << "Scanned boolean: " << b << '\n';
    std::cout << "Returned value is an error: " << !ret.has_value() << '\n';
    if (!ret.has_value()) {
        std::cout << "Error code: " << static_cast<int>(ret.error()) << '\n';
    }

    return 0;
}