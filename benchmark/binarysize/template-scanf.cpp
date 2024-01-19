#include <cstdio>
#include <string>

void do_scan()
{
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#endif
    int i;
    scanf("%d", &i);

    double d;
    scanf("%lf", &d);

    long long ll;
    scanf("%lld", &ll);

    std::string str(13, '\0');
    scanf("%12s", &str[0]);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}
