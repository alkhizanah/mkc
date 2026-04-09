#ifndef BENCHMARK_H_ 
#define BENCHMARK_H_
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#define BLUE            "\033[34m"
#define CYAN            "\033[36m"
#define RESET           "\033[0m"
#ifndef BENCH_TAG
#define BENCH_TAG ""
#endif

struct BuildTimer {
  std::chrono::steady_clock::time_point start;
  const char* label;
  const char* message;

  explicit BuildTimer(const char* lbl = "Build", const char* msg = " ")
      : start(std::chrono::steady_clock::now()), label(lbl), message(msg) {}

  ~BuildTimer() noexcept {
    using namespace std::chrono;
    const auto end = steady_clock::now();
    const duration<double> elapsed = end - start;
    std::cout << CYAN << "\n" << label << BLUE
          << std::fixed << std::setprecision(3)
          << elapsed.count() << CYAN
          << " seconds " << RESET;

    try {
      std::ofstream out("build/logs/benchmark_log.out", std::ios::app);
      if (!out.is_open()) return;
      out << BENCH_TAG << " | " << message << " | " << label << " "
          << elapsed.count() << " seconds\n";
    } catch (...) {}
  }
};

#endif
