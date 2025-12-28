#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <unordered_map>
#include <vector>
#include "config.h"
#include "logger.h"

#define ENABLE_EXCEPTIONS in.exceptions(std::ifstream::failbit | std::ifstream::badbit)
#define LOGGER const Logger& logger
namespace fs = std::filesystem;



struct SourceFile {
  fs::path path;
  fs::path object;
  std::vector<fs::path> includes;
  uint64_t hash = 0;
  bool dirty = false;
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

      if (ext == ".cpp" || ext == ".c") {
        sources[p.path().string()] = {
            p.path(), fs::path("build/obj") /
                          p.path().filename().replace_extension(".o")}; // TODO: add checks for if p.path has no filename somehow
      } else if (ext == ".h") {
        headers[p.path().string()] = {p.path()};
      }
    }
  } catch (const fs::filesystem_error &e) {
    logger.failLog("error iterating over \"" + root.string() + "directory, please check for permission denial, symlink loops, etc.", e.what());
  }
}

void parse_includes(SourceFile &src, LOGGER) {
  std::ifstream in(src.path); ENABLE_EXCEPTIONS;
  if (!fs::exists(src.path)) {
    logger.failLog("file does not exist: " + src.path.string(), " function: parse_includes() failed.");
  }
  std::string line;
try {
  while (std::getline(in, line)) {
    if (line.find("#include \"") == 0) {
      auto start = line.find('"') + 1;
      auto end = line.find('"', start);
      src.includes.emplace_back(line.substr(start, end - start));
    }
  }
  } catch (const std::ios_base::failure &e) {
    logger.failLog("failed to parse includes for \"" + src.path.string() + "\"", e.what());
  }

}

uint64_t hash_file(const fs::path &p, LOGGER) {
  std::ifstream in(p, std::ios::binary); ENABLE_EXCEPTIONS;
  if (!fs::exists(p)) {
    logger.failLog("file does not exist: " + p.string(), " function: hash_file() failed.");
  }
  uint64_t hash = 1469598103934665603ULL;
  char c;
  try {
    while (in.get(c)) {
      hash ^= static_cast<unsigned char>(c);
      hash *= 1099511628211ULL;
    }
  } catch (const std::ios_base::failure &e) {
    logger.failLog("failed to read" + p.string(), e.what()); // if something fails it could be HERE
  }
  return hash;
}

std::unordered_map<std::string, uint64_t> old_hashes;


// TODO: for some reason this function errors out when I test it with some bogus cache file 
// that i made. It fails after reading the first line, throwing [EXCEPTION] basic_ios::clear: iostream error 
// I need to fix that, but I'll try generating real hashes first
void load_cache(std::string cachePath, LOGGER) {
  if (!fs::exists(cachePath)) {
    logger.failLog("file does not exist: " + cachePath, "function: load_cache() failed");
    return;
  }

  std::ifstream in(cachePath); 
  ENABLE_EXCEPTIONS;
  std::string path;
  uint64_t h;

  try {
    while (in >> path >> h) {
      old_hashes[path] = h;
    }
  } catch (const std::ios_base::failure &e) {
    logger.failLog("failed to read \"" + cachePath + "\"", e.what());
  }
}

void mark_dirty(const Config& conf, LOGGER) {
  for (auto &[_, src] : sources) {
    src.hash = hash_file(src.path, logger);

    if (!old_hashes.count(src.path.string())          // < if file doesn't exist in our set of hashed files (it wasn't there last time we built)
        || old_hashes[src.path.string()] != src.hash // <  or it does exist, but the hash doesn't match the new one (the contents changed)
        || !fs::exists(src.object)) {               // < or it exists, and its hash exists, but its object file doesn't
      src.dirty = true;                            // < then mark it as dirty and move on to the next file
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
        src.dirty = true;
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
    mark_dirty(config, logger);
    load_cache("build/.cache", logger);

    for (auto &[_, s] : sources)
      parse_includes(s, logger);

    mark_dirty(config, logger);
    std::cout << std::endl;
    return 0;
  }
  // // ==========================================

  scan("src", logger);
  load_cache("build/.cache", logger);

  for (auto &[_, s] : sources)
    parse_includes(s, logger);

  mark_dirty(config, logger);

  std::cout << std::endl;
  return 0;
}
