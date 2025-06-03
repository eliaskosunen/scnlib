#include <scn/scan.h>
#include <cstdio>

int main()
{
    auto result = scn::input<int>("{}");
    if (!result) {
        return 1;
    }
    std::printf("%d", result->value());
}
