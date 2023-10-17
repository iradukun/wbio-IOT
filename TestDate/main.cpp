#include <iostream>
#include <cstdint>
#include <cstdio>
#include <chrono>
#include <string>
#include <fmt/format.h>
#include <fmt/chrono.h>

int main()
{
#if 0
    fmt::print("{:%S}\n", std::chrono::system_clock::now().time_since_epoch());

#elif 1
    const uint64_t timestamp = 1636368540200LL;
    auto now_milli = std::chrono::milliseconds(timestamp);
    std::chrono::time_point<std::chrono::system_clock> now(now_milli);
    const std::string zDate = fmt::format("{0:%FT%H:%M}:{1:%S}Z", now, std::chrono::duration_cast<std::chrono::duration<double, std::milli> >(now_milli));

    std::cout << zDate << std::endl;
#else
    auto t = tm();
    t.tm_year = 2010 - 1900;
    t.tm_mon = 7;
    t.tm_mday = 4;
    t.tm_hour = 12;
    t.tm_min = 15;
    t.tm_sec = 58;
    fmt::print("{:%Y-%m-%d %H:%M:%S}", t);
#endif
    return 0;
}