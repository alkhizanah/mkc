#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <unordered_map>
#include <vector>
#include "config.h"
#include "logger.h"

#define ENABLE_EXCEPTIONS in.exceptions(std::ifstream::badbit)
#define LOGGER const Logger& logger
namespace fs = std::filesystem;
#define PRINT_ENABLED_EXCEPTIONS                                   \
    // {                                                              \
    //     std::ios_base::iostate exceptions = (in).exceptions();     \
    //     if (exceptions & std::ifstream::failbit) {                 \
    //         std::cout << "\nfailbit is enabled";                   \
    //     }                                                          \
    //     if (exceptions & std::ifstream::badbit) {                  \
    //         std::cout << "\nbadbit is enabled";                    \
    //     }                                                          \
    //     if (exceptions & std::ifstream::eofbit) {                  \
    //         std::cout << "eofbit is enabled\n";                    \
    //     }                                                          \
    // }


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

void scan(const fs::path &root, LOGGER) {
  if (!fs::exists(root)) {
    logger.failLog("directory does not exist: " + root.string(), " function: scan() failed.");
    std::cout << std:: endl;
    exit(EXIT_FAILURE);
  }

  try {
    fs::recursive_directory_iterator iter(root);
    for (auto &p : iter) {
      if (!p.is_regular_file())
        continue;

      auto ext = p.path().extension().string();

      if (ext == ".cpp" || ext == ".c" || ext == ".cc") {
        sources[p.path().string()] = {
            p.path(), fs::path("build/obj") /
                          p.path().filename().replace_extension(".o")}; // TODO: add checks for if p.path has no filename somehow
        logger.successLog("found source file: " + p.path().string());
      } else if (ext == ".h") {
        headers[p.path().string()] = {p.path()};
        logger.successLog("found header file: " + p.path().string());
      }
    }
  } catch (const fs::filesystem_error &e) {
    logger.failLog("error iterating over \"" + root.string() + "directory, please check for permission denial, symlink loops, etc.", e.what());
  }
}

void parse_includes(SourceFile &src, LOGGER) {
  if (!fs::exists(src.path)) {
    logger.failLog("file does not exist: " + src.path.string(), " function: parse_includes() failed.");
  }
  std::ifstream in(src.path); ENABLE_EXCEPTIONS;
  std::string line;
  try {
    while (std::getline(in, line)) {
      if (line.find("#include \"") == 0) {
        auto start = line.find('"') + 1;
        auto end = line.find('"', start);
        src.includes.emplace_back(line.substr(start, end - start));
        debug("found include: " + line.substr(start, end - start) + " in source file: " + src.path.string());
      }
    }
  } catch (const std::ios_base::failure &e) {
    logger.failLog("parse_includes(): failed to parse includes for \"" + src.path.string() + "\"", e.what());
  }
}

uint64_t hash_file(const fs::path &p, LOGGER) {
  if (!fs::exists(p)) {
    logger.failLog("file does not exist: " + p.string(), " function: hash_file() failed.");
  }

  std::ifstream in(p, std::ios::binary); 
  ENABLE_EXCEPTIONS;
  uint64_t hash = 1469598103934665603ULL;
  char c;
  try {
    while (in.get(c)) {
      hash ^= static_cast<unsigned char>(c);
      hash *= 1099511628211ULL;
    }
  } catch (const std::ios_base::failure &e) {
    logger.failLog("hash_file(): failed to read \"" + p.string() + "\"", e.what()); // if something fails it could be HERE
  }
  debug("hashed file: " + p.string());
  debug("hash: " + std::to_string(hash));
  return hash;
}

std::unordered_map<std::string, uint64_t> old_hashes;


void cleanTemp(const fs::path &tempPath);
void save_cache(const fs::path &cachePath, LOGGER) {
  // // Create parent directory if it doesn't exist
  // fs::path parentDir = cachePath.parent_path();
  // if (!parentDir.empty() && !fs::exists(parentDir)) {
  //   try {
  //     fs::create_directories(parentDir);
  //   } catch (const fs::filesystem_error &e) {
  //     logger.failLog("save_cache(): failed to create directory \"" + parentDir.string() + "\"", e.what());
  //     return;
  //   }
  // }
  
  fs::path tempPath = cachePath;
  tempPath += ".tmp";
  
  std::ofstream out(tempPath);
  // ENABLE_EXCEPTIONS; // TODO: does ostream have the same kind of exceptions? otherwise we should keep this commented out

  try {
    for (const auto &[path, hash] : old_hashes) {
      out << path << " " << hash << "\n";
    }

    out.flush();
    out.close();

    if (!out.good()) {
      throw std::ios_base::failure("Failed to write all data");
    }

    fs::rename(tempPath, cachePath); // atomic replace

  } catch (const std::ios_base::failure &e) {
    logger.failLog("save_cache(): failed to write \"" + cachePath.string() + "\"", e.what());
    cleanTemp(tempPath);
  } catch (const fs::filesystem_error &e) {
    logger.failLog("save_cache(): failed to rename temporary file", e.what());
    cleanTemp(tempPath);
  }
}

void cleanTemp(const fs::path &tempPath) {
  // clean up temp file on failure
  try {
    if (fs::exists(tempPath)) {
      fs::remove(tempPath);
    }
  } catch (...) {
    // ignore cleanup errors (?!)
  }
}

void load_cache(const fs::path &cachePath, LOGGER) {
  if (!fs::exists(cachePath)) {
    logger.failLog("file does not exist: " + cachePath.string(), "function: load_cache() failed");
    return;
  }

  std::ifstream in(cachePath); 
  ENABLE_EXCEPTIONS;
  PRINT_ENABLED_EXCEPTIONS;
  std::string path;
  uint64_t h;

  try {
    while (in >> path >> h) {
      old_hashes[path] = h;
    }
  } catch (const std::ios_base::failure &e) {
    logger.failLog("load_cache(): failed to read \"" + cachePath.string() + "\"", e.what());
  }
}

void mark_modified(const Config& conf, LOGGER) {
  for (auto &[_, src] : sources) {
    src.hash = hash_file(src.path, logger);

    if (!old_hashes.count(src.path.string())          // < if file doesn't exist in our set of hashed files (it wasn't there last time we built)
        || old_hashes[src.path.string()] != src.hash // <  or it does exist, but the hash doesn't match the new one (the contents changed)
        || !fs::exists(src.object)) {               // < or it exists, and its hash exists, but its object file doesn't
      src.modified = true;                            // < then mark it as modified and move on to the next file
      debug("mark_modified: " + src.path.string());
      continue;
    }

    // at this point we know the source didn't change in any way, we check if the headers did
    for (auto &inc : src.includes) {
      auto pathOfInclude = inc.string();
      auto it = headers.find(pathOfInclude);
      if (it == headers.end()) { // if this is an external header that we aren't tracking
        if (!conf.track_external_headers) {
          continue;
        }

        HeaderFile hf;
        hf.path = inc;
        hf.hash = hash_file(inc, logger);
        it = headers.emplace(pathOfInclude, std::move(hf)).first;
      }

      it->second.hash = hash_file(it->second.path, logger);

      if (old_hashes[pathOfInclude] != it->second.hash) { // if the hash that we just computed isn't the same as the one in the cached hashes
        src.modified = true;
        debug("mark_modified: " + src.path.string());
        break; // we break once any of the headers triggers recompilation rather
               // than checking all of them
      }
    }
  }
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

  Logger logger(config.log_verbosity);

  // // =============== TODO: this is to be removed later
  if (config.rebuild_all) {
    scan("src", logger);
    mark_modified(config, logger);
    load_cache("build/.cache", logger);

    for (auto &[_, s] : sources)
      parse_includes(s, logger);

    mark_modified(config, logger);
    std::cout << std::endl;
    return 0;
  }
  // // ==========================================

  scan("src", logger);
  load_cache("build/.cache", logger);

  for (auto &[_, s] : sources)
    parse_includes(s, logger);

  mark_modified(config, logger);

  std::cout << std::endl;
  return 0;
}
