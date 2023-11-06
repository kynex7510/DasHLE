#ifndef _DASHLE_TEST_H
#define _DASHLE_TEST_H

#include "Base.h"

#include <chrono>
#include <format>

using namespace dashle;

#define DASHLE_TEST(name)           \
    const char* TEST_NAME = #name;  \
    int runTest()

namespace dashle_test {

class Timer {
    double m_Start = 0;

    static double getNow() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
public:
    void start() { m_Start = getNow(); }
    double stop() { return getNow() - m_Start; }
};

}

template <usize MIN_SIZE, usize MAX_SIZE>
inline usize randomSize() requires(MAX_SIZE >= MIN_SIZE) {
    return MIN_SIZE + (rand() % (MAX_SIZE - MIN_SIZE));
}

#define TEST_PASSED() \
    return 0

#define TEST_FAILED(msg) \
    DASHLE_LOG(msg);     \
    return 1

extern const char* TEST_NAME;
int runTest();

int main() {
    srand(time(nullptr));
    dashle_test::Timer timer;
    DASHLE_LOG(std::format("Running test \"{}\"...", TEST_NAME));
    timer.start();
    const auto ret = runTest();
    const auto time = timer.stop();

    if (ret) {
        DASHLE_LOG("FAILED");
    } else {
        DASHLE_LOG("Passed!");
    }

    DASHLE_LOG(std::format("Execution time: {}ms", time));
    return ret;
}

#endif /* _DASHLE_TEST_H */