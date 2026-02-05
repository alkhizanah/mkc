#ifndef STATICLIB_H_
#define STATICLIB_H_
#include "config.h"
#include "tests.h"
#include "scan.h"

fs::path lib_cache_path(const StaticLib& lib) {
  return fs::path("build/lib") / lib.name / ".mkc.hash";
}


uint64_t hash_lib( const Config& conf, const StaticLib& lib) {
  std::vector<fs::path> incs = lib.include_dirs;
  std::sort(incs.begin(), incs.end());
  std::vector<std::string> flags = conf.compile_flags;
  std::sort(flags.begin(), flags.end());

  uint64_t h = 1469598103934665603ULL;
  auto mix = [&](uint64_t v) {
    h ^= v;
    h *= 1099511628211ULL;
  };

  for (unsigned char c : conf.compiler) mix(c);
  for (const auto& f : flags)
    for (unsigned char c : f) mix(c);

  for (const auto& inc : incs)
    for (unsigned char c : inc.string()) mix(c);

  std::vector<fs::path> srcs = lib.sources;
  std::sort(srcs.begin(), srcs.end());
  for (const auto& src : srcs)
    mix(hash_file(src));

  return h;
}


bool lib_unmodified(const Config& conf, const StaticLib& lib) {
  fs::path cache = lib_cache_path(lib);
  fs::path archive_path = fs::path("build/lib") / lib.name / lib.archive;
  if (!fs::exists(archive_path)) return false;
  if (!fs::exists(cache)) return false;
  std::ifstream in(cache);
  ENABLE_EXCEPTIONS(in);
  uint64_t cached;
  in >> cached;
  return cached == hash_lib(conf, lib);
}


void save_lib_cache(const Config& conf, const StaticLib& lib) {
  fs::path cache = lib_cache_path(lib);
  fs::create_directories(cache.parent_path());
  std::ofstream out(cache);
  ENABLE_EXCEPTIONS(out);
  out << hash_lib(conf, lib);
}


bool build_static_lib(const Config& conf, const StaticLib& lib) { 
  BENCHMARK("build_static_lib: ");
  try {
    if (!conf.rebuild_all && lib_unmodified(conf, lib)) {
      Logger::debug("static lib up-to-date: " + lib.name.string());
      return true;
    }
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
    if (std::system(ar.c_str()) == 0) {
      save_lib_cache(conf, lib);
      return true;
    } else {
      return false;
    }
  } catch (const std::ios_base::failure &e) {
    Logger::failLog("build_static_lib(): static lib cache i/o error", e.what());
    return false;
  }
}

#endif
