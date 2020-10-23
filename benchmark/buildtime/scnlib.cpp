#include <scn/scn.h>

int main()
{
    int i;
    auto ret = scn::input("{}", i);

    double d;
    ret = scn::input("{}", d);

    long long ll;
    ret = scn::input("{}", ll);

    std::string str;
    ret = scn::input("{}", str);
}
