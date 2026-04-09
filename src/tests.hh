#define BENCHMARK
#ifdef FINE_GRAINED_BENCHMARK
#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define BENCHMARK(msg) BuildTimer CONCAT(timer_, __LINE__)(msg, "test")
#endif
