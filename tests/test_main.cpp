#include "check.h"

int g_checks = 0;
int g_failures = 0;

void TestConductor();
void TestOcean();
void TestFloater();
void TestWeather();

int main()
{
    TestConductor();
    TestOcean();
    TestFloater();
    TestWeather();
    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
