#include <scn/scn.h>
#include <cstdio>

int main()
{
    int i{};
    if (!scn::input("{}", i)) {
        return 1;
    }
    std::printf("%d", i);
}
