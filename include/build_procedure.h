#ifndef BUILDPROCEDURE_H_
#define BUILDPROCEDURE_H_
#include "benchmark.h"
#include "scan.h"
#include "cache.h" 
#include "compiler.h"
#include "tests.h"
// TODO: can we do "*.c" in our sources or use that kinda regex generally?
// TODO: consistency with this macro should be achieved with the rest of the
// functions, something that is currently not yet done.
#define CACHE_PATH config.root_dir + "/build/.cache"
#define LOG_PATH config.root_dir + "/build/logs/log.out"

void build_procedure(const Config& config, bool init_only = false) {
  int modifications = 0;
  try {
    BENCHMARK1;
    init_working_dir(config);
    if (init_only) return;
    scan(config);
  } catch (const std::exception &e) {
    Logger::debug("failed at stage: " + std::string(e.what()));
    throw;
  }

  { BENCHMARK2; load_cache(CACHE_PATH); }

  { BENCHMARK3;
    for (auto &[_, src] : sources) {
      try {
        #ifdef DEP_GEN
        generate_deps(LOG_PATH, config, src);
        load_compiler_deps(src); 
        #endif
      } catch (const char *msg) {
        Logger::printLogfile(LOG_PATH, config);
        Logger::debug("failed at stage: " + std::string(msg));
        throw;
      }
    }
  }
  

  { BENCHMARK4; mark_modified(config); }

  try {
    BENCHMARK5;
    modifications = compile_and_link(config);
  } catch (const char *msg) {
    Logger::printLogfile(LOG_PATH, config);
    Logger::debug("failed at stage: " + std::string(msg));
    throw;
  }

  save_cache(CACHE_PATH);
      
  if (config.run_mode && modifications != 0) {
    std::string executable_path = config.root_dir + "/build/" + config.executable_name;
    std::system(executable_path.c_str());
  }
}
#endif
