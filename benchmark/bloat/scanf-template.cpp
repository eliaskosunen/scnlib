#include <cstdio>
#include <string>

void do_scan()
{
    int i;
    double d;
    long long ll;
    char buf[7] = {0};
    std::string str(13, '\0');
    scanf("%d %lf %lld %6s %12s", &i, &d, &ll, buf, &str[0]);
}
