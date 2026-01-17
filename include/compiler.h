#ifndef COMPILER_H_
#define COMPILER_H_
#include <atomic>
#include <future>
#include <mutex>
#include <thread>
#include "compiler_unity.h"


bool build_static_lib(const Config& conf, const StaticLib& lib) {
  // TODO: increment rebuild and account for those in cache
  std::vector<fs::path> objects;
  std::string lib_dir = "build/lib/" + lib.name.string() + "/";
  for (const auto& src : lib.sources) {
    fs::path obj = lib_dir + src.filename().replace_extension(".o").string();
    objects.push_back(obj);

    std::string cmd = conf.compiler;
    cmd += " -c " + src.string();
    cmd += " -o " + obj.string();

    for (auto& inc : lib.include_dirs) cmd += " -I" + inc.string();
    for (const auto &flag : conf.compile_flags) cmd += " " + flag;

    std::string cmd_no_log = cmd;
    std::string logfile = "build/logs/log_" + src.filename().stem().string() + ".out";
    cmd += " >> " + logfile + " 2>&1";
    if (std::system(cmd.c_str()) != 0) return false;
    Logger::successLog("compiled: " + readable_path(src));
    Logger::infoLog("compile command was: " + cmd_no_log);
  }
  fs::remove(lib_dir + lib.archive.string());
  std::string ar = "ar rcs ";
  ar += lib_dir + lib.archive.string();
  for (auto& obj : objects) ar += " " + obj.string();
  return std::system(ar.c_str()) == 0;
}



// NOTE TO SELF: compile flags MUST be the same as the ones we pass to dep_gen func.
bool compile_objects(const Config &conf, int &modified) {
  for (const auto& lb : conf.static_libs) {
    if (!build_static_lib(conf, lb)) return false;
  }
  if (conf.unity_b) {
    return compile_unity(conf, modified);
  }

  std::vector<SourceFile *> jobs;
  for (auto &[_, src] : sources) {
    if (src.modified || conf.rebuild_all) {
      jobs.push_back(&src);
    }
  }
  if (jobs.empty()) return true;

  unsigned max_jobs = conf.parallel_jobs > 0
      ? conf.parallel_jobs
      : std::thread::hardware_concurrency();

  if (max_jobs == 0) max_jobs = 1;
  std::atomic<int> modified_atomic{0};
  std::mutex log_mutex;
  bool failed = false;
  auto compile_one = [&](SourceFile *src) -> bool {
    std::string cmd = conf.compiler;
    cmd += " -c " + src->path.string();
    cmd += " -o " + src->object.string();

    for (const auto &inc : conf.include_dirs) cmd += " -I" + inc.string();
    for (const auto &flag : conf.compile_flags) cmd += " " + flag;

    std::string cmd_no_log = cmd;
    // TODO: as a matter of design choice here.. taking the compiler output
    // into the log file then back out of the log file
    // takes away the colors of the compiler output, which isn't
    // particularly nice.
    std::string logfile = "build/logs/log_" + src->object.filename().stem().string() + ".out";
    cmd += " >> " + logfile + " 2>&1";
    if (std::system(cmd.c_str()) != 0) return false;
    {
      std::lock_guard<std::mutex> lock(log_mutex);
      Logger::successLog("compiled: " + readable_path(src->path));
      Logger::infoLog("compile command was: " + cmd_no_log);
    }
    modified_atomic.fetch_add(1, std::memory_order_relaxed);
    return true;
  };

  std::vector<std::future<bool>> running;
  running.reserve(max_jobs);

  for (SourceFile *src : jobs) {
    while (running.size() >= max_jobs) {
      for (auto it = running.begin(); it != running.end();) {
        if (it->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
          if (!it->get()) failed = true;
          it = running.erase(it);
        } else {
          ++it;
        }
      }
    }
    if (!failed) running.emplace_back(std::async(std::launch::async, compile_one, src));
  }

  for (auto &f : running) {
    if (!f.get()) failed = true;
  }

  modified = modified_atomic.load();
  return !failed;
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
  for (const auto& lib : conf.static_libs) 
  cmd += " build/lib/" + lib.name.string() + "/" + lib.archive.string();
  if (conf.make_shared) {
    cmd += " -o build/lib" + conf.executable_name + ".so";
  } else {
    cmd += " -o build/" + conf.executable_name;
  }
  std::string cmd_no_log = cmd;
  cmd += " >> build/logs/log.out 2>&1";
  Logger::infoLog("linking command was: " + cmd_no_log);

  return std::system(cmd.c_str()) == 0;
}


int compile_and_link(const Config &conf) {
  int modif_count = 0;
  if (!compile_objects(conf, modif_count)) {
    Logger::failLog("compilation failed.", "see build/logs/log.out");
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
    Logger::failLog("linking failed.", "see build/logs/log.out");
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
