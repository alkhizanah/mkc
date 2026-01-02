#ifndef BUILDPROCEDURE_H_
#define BUILDPROCEDURE_H_
#include "../include/scan.h"
#include "../include/cache.h" 
#include "../include/compiler.h"

// TODO: consistency with this macro should be achieved with the rest of the
// functions, something that is currently not yet done.
#define CACHE_PATH config.root_dir + "/build/.cache"
#define LOG_PATH config.root_dir + "/build/log.out"

void build_procedure(const Config& config) {
  int modifications = 0;
  try {
    init_working_dir(config.root_dir);
    scan(config.root_dir);
  } catch (const char *msg) {
    Logger::debug("failed at stage: " + std::string(msg));
    throw;
  }

  load_cache(CACHE_PATH);
  for (auto &[_, src] : sources) {
      try {
        generate_deps(LOG_PATH, config, src);
        load_compiler_deps(src);
      } catch (const char *msg) {
        Logger::printLogfile(LOG_PATH);
        Logger::debug("failed at stage: " + std::string(msg));
        throw;
      }
  }

  mark_modified(config);
  try {
    modifications = compile_and_link(config);
  } catch (const char *msg) {
    Logger::printLogfile(LOG_PATH);
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
