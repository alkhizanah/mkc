#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <unordered_map>
#include <vector>
#include "config.h"
#include "logger.h"

#define ENABLE_EXCEPTIONS(stream) (stream).exceptions(std::ifstream::badbit)
namespace fs = std::filesystem;
#define PRINT_ENABLED_EXCEPTIONS                                   \
    {                                                              \
        std::ios_base::iostate exceptions = (in).exceptions();     \
        if (exceptions & std::ifstream::failbit) {                 \
            std::cout << "\nfailbit is enabled";                   \
        }                                                          \
        if (exceptions & std::ifstream::badbit) {                  \
            std::cout << "\nbadbit is enabled";                    \
        }                                                          \
        if (exceptions & std::ifstream::eofbit) {                  \
            std::cout << "\neofbit is enabled";                    \
        }                                                          \
    }


struct SourceFile {
  fs::path path;
  fs::path object;
  std::vector<fs::path> includes;
  uint64_t hash = 0;
  bool modified = false;
};

struct HeaderFile {
  fs::path path;
  uint64_t hash = 0;
};

std::unordered_map<std::string, SourceFile> sources;
std::unordered_map<std::string, HeaderFile> headers;
uint64_t hash_file(const fs::path &p);


// helper function to fix the issue of paths being "./logger.h" instead of "logger.h" which mismatches lookups
std::string normalize_path(const fs::path& p) {
  try {
    return fs::weakly_canonical(p).string();
  } catch (...) {
    return p.string();
  }
}

// relative instead of absolute for readability in the logs
std::string readable_path(const fs::path& p) {
  try {
    return fs::relative(p, fs::current_path()).string();
  } catch (...) {
    return p.string();
  }
}



void scan(const fs::path &root) {
  if (!fs::exists(root)) {
    Logger::failLog("directory does not exist: " + root.string(), " function: scan() failed.");
    std::cout << std:: endl;
    exit(EXIT_FAILURE);
  }

  try {
    fs::recursive_directory_iterator iter(root);
    for (auto &p : iter) {
      if (!p.is_regular_file())
        continue;

      auto ext = p.path().extension().string();
      std::string normalized = normalize_path(p.path());
      std::string pretty_path = readable_path(p.path());

      if (ext == ".cpp" || ext == ".c" || ext == ".cc") {
        sources[normalized] = {p.path(), fs::path("build/obj") / p.path().filename().replace_extension(".o")};
        Logger::successLog("found source file: " + pretty_path);
      } else if (ext == ".h") {
        HeaderFile hf;
        hf.path = p.path();
        hf.hash = hash_file(p.path());
        headers[normalized] = hf;
        Logger::successLog("found header file: " + pretty_path);
        Logger::debug("hashed header on scan: " + pretty_path + " with hash: " + std::to_string(hf.hash));
      }
    }
  } catch (const fs::filesystem_error &e) {
    Logger::failLog("error iterating over \"" + root.string() + "\" directory, please check for permission denial, symlink loops, etc.", e.what());
  }
}

void parse_includes(SourceFile &src) {
  if (!fs::exists(src.path)) {
    Logger::failLog("file does not exist: " + readable_path(src.path), " function: parse_includes() failed.");
  }
  std::ifstream in(src.path);
  ENABLE_EXCEPTIONS(in);
  std::string line;
  try {
    while (std::getline(in, line)) {
      if (line.find("#include \"") == 0) {
        auto start = line.find('"') + 1;
        auto end = line.find('"', start);
        src.includes.emplace_back(line.substr(start, end - start));
        Logger::debug("found include: " + line.substr(start, end - start) + " in source file: " + readable_path(src.path));
      }
    }
  } catch (const std::ios_base::failure &e) {
    Logger::failLog("parse_includes(): failed to parse includes for \"" + readable_path(src.path) + "\"", e.what());
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

std::unordered_map<std::string, uint64_t> old_hashes;


void cleanTemp(const fs::path &tempPath);
void save_cache(const fs::path &cachePath) {
  fs::path tempPath = cachePath;
  tempPath += ".tmp";
  std::ofstream out(tempPath);
  ENABLE_EXCEPTIONS(out);

  try {
    for (auto &[_, s] : sources)
      out << normalize_path(s.path) << " " << s.hash << "\n";

    for (auto &[_, h] : headers)
      out << normalize_path(h.path) << " " << h.hash << "\n";

    out.flush();
    out.close();

    if (!out.good()) {
      throw std::ios_base::failure("Failed to write all data");
    }

    fs::rename(tempPath, cachePath);

  } catch (const std::ios_base::failure &e) {
    Logger::failLog("save_cache(): failed to write \"" + readable_path(cachePath) + "\"", e.what());
    cleanTemp(tempPath);
  } catch (const fs::filesystem_error &e) {
    Logger::failLog("save_cache(): failed to rename temporary file", e.what());
    cleanTemp(tempPath);
  }

  for (const auto& inc : headers) {
    Logger::debug("cached header: " + readable_path(inc.second.path) + " | hash: " + std::to_string(inc.second.hash));
  }
}

void cleanTemp(const fs::path &tempPath) {
  try {
    if (fs::exists(tempPath)) {
      fs::remove(tempPath);
    }
  } catch (...) {
    Logger::warningLog("clean up error while removing: \"" + readable_path(tempPath) + "\"");
  }
}

void load_cache(const fs::path &cachePath) {
  if (!fs::exists(cachePath) || fs::is_empty(cachePath)) {
    Logger::warningLog("cache missing or empty, full rebuild required");
    return;
  }

  std::ifstream in(cachePath); 
  ENABLE_EXCEPTIONS(in);
  std::string path;
  uint64_t h;

  try {
    while (in >> path >> h) {
      old_hashes[path] = h;
    }
  } catch (const std::ios_base::failure &e) {
    Logger::failLog("load_cache(): failed to read cache at \"" + readable_path(cachePath) + "\"", e.what());
  }
}

void mark_modified(const Config& conf) {
  for (auto &[_, src] : sources) {
    src.hash = hash_file(src.path);

    if (!old_hashes.count(normalize_path(src.path))          // < if source file doesn't exist in our set of hashed files (it wasn't there last time we built)
        || old_hashes[normalize_path(src.path)] != src.hash // <  or it does exist, but the hash doesn't match the new one (the contents changed)
      // NOTE: we are momentarily disabling 
      // obj file check because we don't compile yet
       /*|| !fs::exists(src.object) */) {               // < or it exists, and its hash exists, but its object file doesn't
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


int compile() {
  for (auto &[_, src] : sources) {
    if (src.modified) {
      // TODO: if any compilation fails, abort, do not write cache
      Logger::successLog("compiled source: \"" + readable_path(src.path) + "\"");
    }
  }
  return 0;
}

void printHelp() {
  std::cerr << R"(
Usage: mkc [option]

Options:
  verbose          Enable verbose logging
  clean            Rebuild all files
  track-external   Rebuild files affected by external headers outside the working dir
)" << std::endl;
}



Verbosity Logger::vb = Verbosity::normal;
int main(int argc, char *argv[]) {
  Config config;
  if (argc == 2) {
    if (std::string(argv[1]) == "verbose") {
      config.log_verbosity = Verbosity::verbose;
    } else if (std::string(argv[1]) == "clean") {
      config.rebuild_all = true;
    } else if (std::string(argv[1]) == "track-external") {
      config.rebuild_all = true;
    } else {
      printHelp();
      return 1;
    }
  } else if (argc > 2) {
    printHelp();
    return 1;
  }
  Logger::setVerbosity(config.log_verbosity);
  // if (config.rebuild_all) {
  //   return 0;
  // }

  scan(".");
  load_cache("build/.cache");

  for (auto &[_, s] : sources)
    parse_includes(s);

  mark_modified(config);
  if (compile() == 0) {
    save_cache("build/.cache");
  }

  std::cout << std::endl;
  return 0;
}


// leverage this to create a cachefile on mkc init command
  // // Create parent directory if it doesn't exist
  // fs::path parentDir = cachePath.parent_path();
  // if (!parentDir.empty() && !fs::exists(parentDir)) {
  //   try {
  //     fs::create_directories(parentDir);
  //   } catch (const fs::filesystem_error &e) {
  //     Logger::failLog("save_cache(): failed to create directory \"" + readable_path(parentDir) + "\"", e.what());
  //     return;
  //   }
  // }

