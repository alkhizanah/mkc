#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "config.h"
#include "logger.h"

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

void scan(const fs::path &root) {
  for (auto &p : fs::recursive_directory_iterator(root)) {
    if (!p.is_regular_file())
      continue;

    auto ext = p.path().extension().string();

    if (ext == ".cpp" || ext == ".c") {
      sources[p.path().string()] = {
          p.path(),
          fs::path("build/obj") / p.path().filename().replace_extension(".o")};
    } else if (ext == ".h") {
      headers[p.path().string()] = {p.path()};
    }
  }
}

void parse_includes(SourceFile &src) {
  std::ifstream in(src.path);
  std::string line;

  while (std::getline(in, line)) {
    if (line.find("#include \"") == 0) {
      auto start = line.find('"') + 1;
      auto end = line.find('"', start);
      src.includes.emplace_back(line.substr(start, end - start));
    }
  }
}

uint64_t hash_file(const fs::path &p) {
  std::ifstream in(p, std::ios::binary);
  uint64_t hash = 1469598103934665603ULL;
  char c;

  while (in.get(c)) {
    hash ^= static_cast<unsigned char>(c);
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::unordered_map<std::string, uint64_t> old_hashes;

void load_cache() {
  std::ifstream in("build/.buildcache");
  std::string path;
  uint64_t h;

  while (in >> path >> h) {
    old_hashes[path] = h;
  }
}

void mark_dirty(const Config& conf) {
  for (auto &[_, src] : sources) {
    src.hash = hash_file(src.path);

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
        hf.hash = hash_file(inc);
        it = headers.emplace(pathOfInclude, std::move(hf)).first;
      }

      it->second.hash = hash_file(it->second.path);

      if (old_hashes[pathOfInclude] != it->second.hash) { // if the hash that we just computed isn't the same as the one in the cached hashes
        src.dirty = true;
        break; // we break once any of the headers triggers recompilation rather
               // than checking all of them
      }
    }
  }
}

int main() {
  Config config;
  Logger logger;
  logger.successLog("this is a message that means we compiled correctly");
  logger.failLog("this is a message that means we compiled incorrectly");
  logger.warningLog("this is a warning, but it's not about the warning.. it's about.. sending a message");
  scan("src");
  load_cache();

  for (auto &[_, s] : sources)
    parse_includes(s);

  mark_dirty(config);
  return 0;
}
