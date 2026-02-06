#ifndef SCAN_H_
#define SCAN_H_
#include "logger.h"
#include <filesystem> 
#include <algorithm>
#include "helpers.h"
#include "exceptions.h"
#include "containers.h"

namespace fs = std::filesystem;

void init_working_dir(const Config& conf) {
  fs::path root = conf.root_dir;
  if (!fs::exists(root)) {
    Logger::failLog("directory does not exist: " + root.string(), " function: init_working_dir() failed.");
    std::cout << std::endl;
    throw std::runtime_error("init_working_dir()");
  }
  
  try {
    fs::create_directories(root / "build");
    fs::create_directories(root / "build/obj");
    fs::create_directories(root / "build/logs");
    fs::create_directories(root / "build/lib");
    for (const auto& lib : conf.static_libs) fs::create_directories(root / "build/lib" / lib.name);
    std::ofstream(root / "build/logs/log.out", std::ios::trunc).close();
    std::ofstream(root / "build/logs/benchmark_log.out", std::ios::app).close();
  } catch (...) {
    Logger::failLog("error initializing root directory \"" + root.string() + "\"");
  }
}


uint64_t hash_file(const fs::path &p) {
  if (!fs::exists(p)) {
    Logger::failLog("file does not exist: " + readable_path(p), " function: hash_file() failed.");
  }

  std::ifstream in(p, std::ios::binary); 
  ENABLE_EXCEPTIONS(in);
  uint64_t hash = 1469598103934665603ULL;
  char c;

  try {
    while (in.get(c)) {
      hash ^= static_cast<unsigned char>(c);
      hash *= 1099511628211ULL;
    }
  } catch (const std::ios_base::failure &e) {
    Logger::failLog("hash_file(): failed to read \"" + readable_path(p) + "\"", e.what());
  }
  // Logger::debug("hashed file: " + readable_path(p));
  // Logger::debug("hash: " + std::to_string(hash));
  return hash;
}


void scan_explicit_sources(const Config &conf) {
  for (const auto &rel : conf.explicit_sources) {
    fs::path abs = fs::path(conf.root_dir) / rel;
    if (!fs::exists(abs) || !fs::is_regular_file(abs)) {
      Logger::failLog("explicit source not found: " + abs.string());
      throw std::runtime_error("scan_explicit_sources()");
    }

    std::string ext = abs.extension().string();
    if (ext != ".c" && ext != ".cpp" && ext != ".cc") continue;

    std::string normalized = normalize_path(abs);
    sources[normalized] = {abs, fs::path("build/obj") / abs.filename().replace_extension(".o")};
    Logger::infoLog("found explicit source: " + readable_path(abs));
  }
}

void scan(const Config &conf) {
  fs::path root = fs::path(conf.root_dir);
  if (!fs::exists(root)) {
    Logger::failLog("directory does not exist: " + root.string(), "function: scan()");
    throw std::runtime_error("scan()");
  }

  if (!conf.explicit_sources.empty()) {
    try {
      scan_explicit_sources(conf);
    } catch (const std::exception& e) {
      Logger::debug("failed at stage: " + std::string(e.what()));
    }
    if (conf.dry_run) { 
      Logger::print_src_and_hdr(conf.dry_run_toml);
      throw 1;
    }
    return;
  }

  try {
    for (auto &p : fs::recursive_directory_iterator(root)) {
      if (!p.is_regular_file())
        continue;
      fs::path path = p.path();
      std::string ext = path.extension().string();
      std::string normalized = normalize_path(path);
      std::string pretty_path = readable_path(path);
     
      if (is_excluded(conf, path, ext)) continue;

      if (ext == ".cpp" || ext == ".c" || ext == ".cc") {
        sources[normalized] = {p.path(), fs::path("build/obj") / p.path().filename().replace_extension(".o")};
        Logger::infoLog("found source file: " + pretty_path);
      } else if (ext == ".h" || ext == ".hpp") {
        HeaderFile hf;
        hf.path = p.path();
        hf.hash = hash_file(p.path());
        headers[normalized] = hf;
        Logger::infoLog("found header file: " + pretty_path);
        Logger::debug("hashed header on scan: " + pretty_path +
                      " with hash: " + std::to_string(hf.hash));
      }
    }

    if (conf.dry_run) { 
      Logger::print_src_and_hdr(conf.dry_run_toml);
      throw 1;
    }
  } catch (const fs::filesystem_error &e) {
    Logger::failLog("error iterating over \""
                    + root.string()
                    + "\" directory, please check for permission denial, symlink loops, etc.",
                    e.what());
  } catch (int& i) {
    throw i;
  }
}


void generate_deps(const std::string& log_path, const Config& conf, const SourceFile &src) {
  fs::path dep_file = src.object;
  dep_file.replace_extension(".d");

  std::string cmd = conf.compiler;
  cmd += " -MM -MF " + dep_file.string();
  cmd += " -MT " + src.object.string();
  cmd += " " + src.path.string();

  for (const auto &inc : conf.include_dirs) {
    cmd += " -I" + inc.string();
  }

  cmd += " >> " + log_path + " 2>&1";

  int ret = std::system(cmd.c_str());
  if (ret != 0) {
    Logger::failLog("\"#Include\" dependency generation failed.", ("dependency generation command was:\n            " + cmd).c_str());
    std::cout << std::endl;
    throw "generate_deps()";
  }
}

fs::path depfile_path(const fs::path& src) {
  fs::path p = src;
  p.replace_extension(".d");
  return fs::path("build/obj") / p.filename();
}

bool need_regen_deps(const fs::path& src, const fs::path& depfile) {
  if (!fs::exists(depfile)) {
    return true;
  }
  auto src_time = fs::last_write_time(src);
  auto dep_time = fs::last_write_time(depfile);
  return src_time > dep_time;
}

std::vector<fs::path> parse_dep_file(const fs::path &dep_path) {
  std::ifstream in(dep_path);
  ENABLE_EXCEPTIONS(in);

  std::vector<fs::path> deps;
  std::string line, full;

  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\\') {
      line.pop_back();
      full += line;
    } else {
      full += line;
      break;
    }
  }

  auto colon = full.find(':');
  if (colon == std::string::npos)
    return deps;

  std::istringstream iss(full.substr(colon + 1));
  std::string tok;

  while (iss >> tok) {
    deps.emplace_back(normalize_path(tok));
  }

  return deps;
}

void load_compiler_deps(SourceFile &src) {
  fs::path dep = src.object;
  dep.replace_extension(".d");

  auto deps = parse_dep_file(dep);

  src.includes.clear();
  for (const auto &d : deps) {
    if (d != src.path) {
      src.includes.push_back(d);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////
void mark_modified(const Config &conf) {
  for (auto &[_, src] : sources) {
    src.hash = hash_file(src.path);

    bool hash_changed = old_hashes[normalize_path(src.path)] != src.hash;
    if (hash_changed) Logger::warningLog("file modified: " + readable_path(src.path));
    if (!old_hashes.count(normalize_path(src.path))          // < if source file doesn't exist in our set of hashed files (it wasn't there last time we built)
        || hash_changed // <  or it does exist, but the hash doesn't match the new one (the contents changed)
        || !fs::exists(src.object)) {               // < or it exists, and its hash exists, but its object file doesn't
      src.modified = true;                            // < then mark it as modified
      continue;
    }

    // at this point we know the source didn't change in any way, we check if the headers did
    for (const fs::path &inc : src.includes) {
      fs::path resolved_include = src.path.parent_path() / inc;
      std::string normalized = normalize_path(resolved_include);
      std::string pretty_path = readable_path(resolved_include);

      auto it = headers.find(normalized);

      if (it == headers.end()) {
        if (!conf.track_external_headers) {
          // Logger::debug("  skipping external header: " + pretty_path);
          continue;
        }
        // Logger::debug("  tracking new external header: " + pretty_path);
        HeaderFile hf;
        hf.path = resolved_include;
        hf.hash = hash_file(resolved_include);
        it = headers.emplace(normalized, std::move(hf)).first;
      }

      auto oh = old_hashes.find(normalized);
      if (oh == old_hashes.end() // if the hash isn't in the old hashes or..
          || oh->second != it->second.hash) { // if it is in the old hashes but
                                              // it has been modified.
        src.modified = true;
        Logger::warningLog("file modified: " + readable_path(it->second.path));
        break;
      }
    }
  }
}

#endif
