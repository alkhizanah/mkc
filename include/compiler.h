#ifndef COMPILER_H_
#define COMPILER_H_
#include "containers.h"
#include "helpers.h"
#include "logger.h"


// NOTE TO SELF: compile flags MUST be the same as the ones we pass to dep_gen func.
bool compile_objects(const Config &conf, int &modified) {
  for (auto &[_, src] : sources) {
    if (!src.modified && !conf.rebuild_all)
      continue;

    std::string cmd = conf.compiler;
    cmd += " -c " + src.path.string();
    cmd += " -o " + src.object.string();

    for (const auto &inc : conf.include_dirs)
      cmd += " -I" + inc.string();

    cmd += " >> build/log.out 2>&1";

    if (std::system(cmd.c_str()) != 0)
      return false;

    Logger::successLog("compiled: " + readable_path(src.path));
    modified++;
  }
  return true;
}



// TODO: what if we compile but linking fails for some reason? gotta manage that.
bool link_executable(const Config &conf) {
  std::string cmd = conf.compiler;

  for (const auto &[_, src] : sources) {
    cmd += " " + src.object.string();
  }

  cmd += " -o build/" + conf.executable_name;
  cmd += " >> build/log.out 2>&1";

  return std::system(cmd.c_str()) == 0;
}



void compile_and_link(const Config &conf) {
  int modif_count = 0;
  if (!compile_objects(conf, modif_count)) {
    Logger::failLog("compilation failed.", "see build/log.out");
    throw "compile_objects()";
  }

  if (modif_count == 1) {
    Logger::successLog("recompiled: " + std::to_string(modif_count) + " file.");
  } else {
    Logger::successLog("recompiled: " + std::to_string(modif_count) + " files.");
  }

  if (!link_executable(conf)) {
    Logger::failLog("linking failed.", "see build/log.out");
    throw "link_executable()";
  }
}


#endif
