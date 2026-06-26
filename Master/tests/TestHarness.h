#ifndef TestHarness_h
#define TestHarness_h

#include <cstdio>
#include <cstdlib>

static int g_testsRun = 0;
static int g_testsFailed = 0;

#define TEST(name)                                                             \
  static void name();                                                          \
  struct name##_Runner {                                                       \
    name##_Runner() {                                                          \
      ++g_testsRun;                                                            \
      std::printf("  %s ... ", #name);                                         \
      name();                                                                  \
      std::printf("ok\n");                                                     \
    }                                                                          \
  };                                                                           \
  static name##_Runner name##_instance;                                        \
  static void name()

#define EXPECT_TRUE(expr)                                                      \
  do {                                                                         \
    if (!(expr)) {                                                             \
      std::printf("\nFAIL %s:%d: expected true: %s\n", __FILE__, __LINE__,   \
                  #expr);                                                      \
      ++g_testsFailed;                                                         \
      return;                                                                  \
    }                                                                          \
  } while (0)

#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))

#define EXPECT_EQ(a, b)                                                        \
  do {                                                                         \
    auto _a = (a);                                                             \
    auto _b = (b);                                                             \
    if (_a != _b) {                                                            \
      std::printf("\nFAIL %s:%d: expected %s == %s\n", __FILE__, __LINE__,     \
                  #a, #b);                                                     \
      ++g_testsFailed;                                                         \
      return;                                                                  \
    }                                                                          \
  } while (0)

inline int run_test_harness() {
  std::printf("Running %d test(s)...\n", g_testsRun);
  if (g_testsFailed == 0) {
    std::printf("All %d test(s) passed.\n", g_testsRun);
    return 0;
  }
  std::printf("%d of %d test(s) FAILED.\n", g_testsFailed, g_testsRun);
  return 1;
}

#endif
