#include <scn/scn.h>
#include <iostream>

int main() {
    std::string str{};
    auto ret = scn::input("{}", str);
    if (!ret) {
        return 1;
    }
    std::cout << str << '\n';
}
