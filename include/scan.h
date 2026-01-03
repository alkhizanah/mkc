#ifndef SCAN_H_
#define SCAN_H_
#include "logger.h"
#include <filesystem> 
#include <algorithm>
#include "helpers.h"
#include "exceptions.h"
#include "containers.h"

namespace fs = std::filesystem;



// TODO: make this check before creating the dirs so we don't waste time
void init_working_dir(const fs::path &root) {
  if (!fs::exists(root)) {
    Logger::failLog("directory does not exist: " + root.string(), " function: init_working_dir() failed.");
    std::cout << std::endl;
    throw "init_working_dir()";
  }
  
  try {
    fs::create_directories(root / "build");
    fs::create_directories(root / "build/obj");
    std::ofstream(root / "build/log.out", std::ios::trunc).close();
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
  Logger::debug("hashed file: " + readable_path(p));
  Logger::debug("hash: " + std::to_string(hash));
  return hash;
}


void scan(const Config &conf) {
  fs::path root = fs::path(conf.root_dir);
  if (!fs::exists(root)) {
    Logger::failLog("directory does not exist: " + root.string(), "function: scan()");
    throw "scan()";
  }

  try {
    for (auto &p : fs::recursive_directory_iterator(root)) {
      if (!p.is_regular_file())
        continue;
      fs::path path = p.path();
      std::string ext = path.extension().string();
      std::string normalized = normalize_path(path);
      std::string pretty_path = readable_path(path);
     
      // NOTE: obviously this exclusion is a performance hit.
      // it would be better not to use this, but I made the 
      // option anyway, for special cases.
      // filtering excluded extensions
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
  } catch (const fs::filesystem_error &e) {
    Logger::failLog("error iterating over \""
                    + root.string()
                    + "\" directory, please check for permission denial, symlink loops, etc.",
                    e.what());
  }
}

////////////////////////////////////////////////////////////////////////////////////

// void parse_includes(SourceFile &src) {
//   if (!fs::exists(src.path)) {
//     Logger::failLog("file does not exist: " + readable_path(src.path), " function: parse_includes() failed.");
//   }
//   std::ifstream in(src.path);
//   ENABLE_EXCEPTIONS(in);
//   std::string line;
//   try {
//     while (std::getline(in, line)) {
//       if (line.find("#include \"") == 0) {
//         auto start = line.find('"') + 1;
//         auto end = line.find('"', start);
//         src.includes.emplace_back(line.substr(start, end - start));
//         Logger::debug("found include: " + line.substr(start, end - start) + " in source file: " + readable_path(src.path));
//       }
//     }
//   } catch (const std::ios_base::failure &e) {
//     Logger::failLog("parse_includes(): failed to parse includes for \"" + readable_path(src.path) + "\"", e.what());
//   }
// }


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


// TODO: we enable exceptions here but we aren't catching them
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

    if (!old_hashes.count(normalize_path(src.path))          // < if source file doesn't exist in our set of hashed files (it wasn't there last time we built)
        || old_hashes[normalize_path(src.path)] != src.hash // <  or it does exist, but the hash doesn't match the new one (the contents changed)
        || !fs::exists(src.object)) {               // < or it exists, and its hash exists, but its object file doesn't
      src.modified = true;                            // < then mark it as modified and move on to the next file
      Logger::debug("marked as modified: " + readable_path(src.path));
      continue;
    }

    // Logger::debug("checking source: " + readable_path(src.path) + " (includes: " +  std::to_string(src.includes.size()) + ")");

    // at this point we know the source didn't change in any way, we check if the headers did
    for (const fs::path &inc : src.includes) {
      fs::path resolved_include = src.path.parent_path() / inc;
      std::string normalized = normalize_path(resolved_include);
      std::string pretty_path = readable_path(resolved_include);

      Logger::debug("  checking include: " + inc.string() + " -> " + pretty_path);

      auto it = headers.find(normalized);

      if (it == headers.end()) { // if this is an external header that we aren't tracking
        if (!conf.track_external_headers) {
          Logger::debug("  skipping external header: " + pretty_path);
          continue;
        }

        Logger::debug("  tracking new external header: " + pretty_path);
        HeaderFile hf;
        hf.path = resolved_include;
        hf.hash = hash_file(resolved_include);
        it = headers.emplace(normalized, std::move(hf)).first;
      }

      // uint64_t current_hash = hash_file(it->second.path);
      // Logger::debug("mark_modified(): " + readable_path(it->second.path) +
      //               " current hash: " + std::to_string(current_hash));

      auto oh = old_hashes.find(normalized);
      if (oh == old_hashes.end() // if the hash isn't in the old hashes or..
          || oh->second != it->second.hash) { // if it is in the old hashes but
                                              // it has been modified.
        src.modified = true;
        Logger::debug("marked as modified due to: " + readable_path(it->second.path));
        break;
      }
    }
  }
}

#endif
