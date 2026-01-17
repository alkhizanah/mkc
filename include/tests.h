#define BENCHMARK1
#define BENCHMARK2
#define BENCHMARK3
#define BENCHMARK4
#define BENCHMARK5
#ifdef FINE_GRAINED_BENCHMARK
#define BENCHMARK1 BuildTimer test("init_and_scan: ", "test")
#define BENCHMARK2 BuildTimer test("load cache: ", "test")
#define BENCHMARK3 BuildTimer test("gen and load compiler deps: ", "test")
#define BENCHMARK4 BuildTimer test("mark_modified: ", "test")
#define BENCHMARK5 BuildTimer test("compile_and_link: ", "test")
#endif
