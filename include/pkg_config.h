#ifndef PKGCONF_H_
#define PKGCONF_H_
#include <string>
#include <array>
#include "config.h"
#include "logger.h"

std::string cmd_output(const std::string& cmd) {
  if (cmd.empty())
    throw std::runtime_error("cmd_output(): empty command");

  std::array<char, 256> buffer{};
  std::string result;

  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) throw std::runtime_error("cmd_output(): popen failed for command: " + cmd);

  while (fgets(buffer.data(), buffer.size(), pipe)) {
    result += buffer.data();
  }

  int rc = pclose(pipe);
  if (rc == -1) throw std::runtime_error("cmd_output(): pclose failed");
  if (rc != 0) throw std::runtime_error("cmd_output(): command returned non-zero: " + cmd);
  if (result.empty()) throw std::runtime_error("cmd_output(): command produced no output: " + cmd);

  return result;
}

void resolve_pkg_config(Config& conf) {
  if (conf.pkg_deps.empty()) return;

  std::string dep_list;
  for (auto& d : conf.pkg_deps) {
    if (d.name.empty()) {
      Logger::failLog("empty pkg-config dependency name", "failed at: resolve_pkg_config()");
      throw "failed while resolving pkg-config";
    }
    dep_list += d.name + " ";
  }

  try {
    ///////// CFLAGS
    {
      std::string cmd = "pkg-config --cflags " + dep_list;
      auto out = cmd_output(cmd);
      std::istringstream iss(out);
      for (std::string flag; iss >> flag; ) conf.compile_flags.push_back(flag);
    }
    ///////// LIBS
    {
      std::string cmd = "pkg-config --libs " + dep_list;
      auto out = cmd_output(cmd);
      std::istringstream iss(out);
      for (std::string flag; iss >> flag; ) conf.link_flags.push_back(flag);
    }
  } catch (const std::exception& e) {
    Logger::failLog(std::string("failed to resolve dependencies [") + dep_list + "]: " + e.what());
    throw "failed while resolving pkg-config";
  }
}
#endif
