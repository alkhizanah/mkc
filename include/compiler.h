#ifndef COMPILER_H_
#define COMPILER_H_
#include "compiler_unity.h"

// NOTE TO SELF: compile flags MUST be the same as the ones we pass to dep_gen func.
bool compile_objects(const Config &conf, int &modified) {
  if (conf.unity_b) {
    return compile_unity(conf, modified);
  }

  for (auto &[_, src] : sources) {
    if (!src.modified && !conf.rebuild_all)
      continue;

    std::string cmd = conf.compiler;
    cmd += " -c " + src.path.string();
    cmd += " -o " + src.object.string();

    for (const auto &inc : conf.include_dirs)
      cmd += " -I" + inc.string();

    for (const auto &flag : conf.compile_flags)
      cmd += " " + flag;
    // TODO: as a matter of design choice here.. taking the compiler output
    // into the log file then back out of the log file 
    // takes away the colors of the compiler output, which isn't 
    // particularly nice.
    std::string cmd_no_log = cmd;
    cmd += " >> build/log.out 2>&1";

    if (std::system(cmd.c_str()) != 0)
      return false;

    Logger::successLog("compiled: " + readable_path(src.path));
    Logger::infoLog("compile command was: " + cmd_no_log);
    modified++;
  }
  return true;
}


bool link_executable(const Config &conf) {
  std::string cmd = conf.compiler;
  if (conf.make_shared)  cmd += " -shared";
  if (conf.unity_b) {
    cmd += " " + conf.unity_obj.string();
  } else {
    for (const auto &[_, src] : sources) {
      cmd += " " + src.object.string();
    }
  }

  for (const auto &flag : conf.link_flags) { cmd += " " + flag; }
  cmd += " -o build/" + conf.executable_name;
  std::string cmd_no_log = cmd;
  cmd += " >> build/log.out 2>&1";
  Logger::infoLog("linking command was: " + cmd_no_log);

  return std::system(cmd.c_str()) == 0;
}


int compile_and_link(const Config &conf) {
  int modif_count = 0;
  if (!compile_objects(conf, modif_count)) {
    Logger::failLog("compilation failed.", "see build/log.out");
    throw conf.unity_b ? "compile_unity()" : "compile_objects()";
  }
  
  if (!conf.unity_b) {
    if (modif_count == 1) {
      Logger::successLog("recompiled: " + std::to_string(modif_count) + " file");
    } else {
      Logger::successLog("recompiled: " + std::to_string(modif_count) + " files");
    }
  }

  if (!link_executable(conf)) {
    Logger::failLog("linking failed.", "see build/log.out");
    throw "link_executable()";
  }

  return modif_count;
}




// compile flags for hyprland plugin
    // // note: remove this later
    // cmd += " -std=c++23";
    // cmd += " -D WITH_GZFILEOP";
    // cmd += " -fPIC";
    // //////////////////
// link flags for hyprland plugin 
  // // note: remove later
  // cmd +=  " -Wl,--unresolved-symbols=ignore-all";
  // // ////


#endif
