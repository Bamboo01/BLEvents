#ifndef BLEVENTS_TEST_HARNESS_H
#define BLEVENTS_TEST_HARNESS_H

// A minimal test harness: TEST_CASE registers a test, CHECK records a failure without stopping it.

#include <cstdio>
#include <exception>
#include <vector>

namespace TestHarness
{
    struct TestCase
    {
        const char* name;
        void (*run)();
    };

    inline std::vector<TestCase>& Tests()
    {
        static std::vector<TestCase> tests;
        return tests;
    }

    inline int& Failures()
    {
        static int failures = 0;
        return failures;
    }

    struct Register
    {
        Register(const char* name, void (*run)()) { Tests().push_back({ name, run }); }
    };

    inline int RunAll()
    {
        int failedTests = 0;
        for (const TestCase& test : Tests())
        {
            const int failuresBefore = Failures();
            try
            {
                test.run();
            }
            catch (const std::exception& e)
            {
                std::printf("  unexpected exception: %s\n", e.what());
                ++Failures();
            }
            catch (...)
            {
                std::printf("  unexpected exception\n");
                ++Failures();
            }

            const bool passed = Failures() == failuresBefore;
            if (!passed)
            {
                ++failedTests;
            }
            std::printf("[%s] %s\n", passed ? "PASS" : "FAIL", test.name);
        }

        std::printf("%zu tests, %d failed\n", Tests().size(), failedTests);
        return failedTests == 0 ? 0 : 1;
    }
}

#define TEST_CASE(name) \
    static void name(); \
    static const TestHarness::Register name##Registration(#name, &name); \
    static void name()

#define CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            std::printf("  %s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #condition); \
            ++TestHarness::Failures(); \
        } \
    } while (false)

#endif //BLEVENTS_TEST_HARNESS_H
