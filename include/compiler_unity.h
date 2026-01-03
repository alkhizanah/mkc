#ifndef COMPILEUNITY_H_
#define COMPILEUNITY_H_
#include "containers.h"
#include "helpers.h"
#include "logger.h"
  

fs::path generate_unity_file(const Config& conf) {
  std::ofstream out(conf.unity_src_name);
  if (!out)
    throw "generate_unity_file()";

  for (auto& [_, src] : sources) {
    out << "#include \"" << fs::absolute(src.path).string() << "\"\n";
  }

  return conf.unity_src_name;
}




bool compile_unity(const Config& conf, int &modified) {
  bool need = conf.rebuild_all;
  if (!need) {
    for (auto &[_, src] : sources) {
      if (src.modified) {
        need = true;
        break;
      }
    }
  }
  if (!need) return true;
  fs::path unity_src = generate_unity_file(conf);
  std::string cmd = conf.compiler;
  cmd += " -c " + unity_src.string();
  cmd += " -o " + conf.unity_obj.string();
  for (const auto &inc : conf.include_dirs)
    cmd += " -I" + inc.string();
  for (const auto &flag : conf.compile_flags)
    cmd += " " + flag;
  std::string cmd_no_log = cmd;
  cmd += " >> build/log.out 2>&1";
  if (std::system(cmd.c_str()) != 0) return false;
  Logger::successLog("compiled unity: " + unity_src.string());
  Logger::infoLog("compile command was: " + cmd_no_log);
  modified = 1;
  return true;
}


#endif
