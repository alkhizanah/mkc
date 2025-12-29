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

      if (ext == ".cpp" || ext == ".c" || ext == ".cc") {
        sources[p.path().string()] = {
            p.path(), fs::path("build/obj") /
                          p.path().filename().replace_extension(".o")}; // TODO: add checks for if p.path has no filename somehow
        Logger::successLog("found source file: " + p.path().string());
      } else if (ext == ".h") {
        headers[p.path().string()] = {p.path()};
        Logger::successLog("found header file: " + p.path().string());
      }
    }
  } catch (const fs::filesystem_error &e) {
    Logger::failLog("error iterating over \"" + root.string() + "directory, please check for permission denial, symlink loops, etc.", e.what());
  }
}

void parse_includes(SourceFile &src) {
  if (!fs::exists(src.path)) {
    Logger::failLog("file does not exist: " + src.path.string(), " function: parse_includes() failed.");
  }
  std::ifstream in(src.path); ENABLE_EXCEPTIONS(in);
  std::string line;
  try {
    while (std::getline(in, line)) {
      if (line.find("#include \"") == 0) {
        auto start = line.find('"') + 1;
        auto end = line.find('"', start);
        src.includes.emplace_back(line.substr(start, end - start));
        Logger::debug("found include: " + line.substr(start, end - start) + " in source file: " + src.path.string());
      }
    }
  } catch (const std::ios_base::failure &e) {
    Logger::failLog("parse_includes(): failed to parse includes for \"" + src.path.string() + "\"", e.what());
  }
}

uint64_t hash_file(const fs::path &p) {
  if (!fs::exists(p)) {
    Logger::failLog("file does not exist: " + p.string(), " function: hash_file() failed.");
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
    Logger::failLog("hash_file(): failed to read \"" + p.string() + "\"", e.what()); // if something fails it could be HERE
  }
  Logger::debug("hashed file: " + p.string());
  Logger::debug("hash: " + std::to_string(hash));
  return hash;
}

std::unordered_map<std::string, uint64_t> old_hashes;


void cleanTemp(const fs::path &tempPath);
void save_cache(const fs::path &cachePath) {
  // // Create parent directory if it doesn't exist
  // fs::path parentDir = cachePath.parent_path();
  // if (!parentDir.empty() && !fs::exists(parentDir)) {
  //   try {
  //     fs::create_directories(parentDir);
  //   } catch (const fs::filesystem_error &e) {
  //     Logger::failLog("save_cache(): failed to create directory \"" + parentDir.string() + "\"", e.what());
  //     return;
  //   }
  // }
  
  fs::path tempPath = cachePath;
  tempPath += ".tmp";
  std::ofstream out(tempPath);
  ENABLE_EXCEPTIONS(out);

  try {
    for (auto &[_, s] : sources)
      out << s.path.string() << " " << s.hash << "\n";

    for (auto &[_, h] : headers)
      out << h.path.string() << " " << h.hash << "\n"; // HERE: maybe we don't need to convert to string, but my intuition says this for now

    out.flush();
    out.close();

    if (!out.good()) {
      throw std::ios_base::failure("Failed to write all data");
    }

    fs::rename(tempPath, cachePath); // atomic replace

  } catch (const std::ios_base::failure &e) {
    Logger::failLog("save_cache(): failed to write \"" + cachePath.string() + "\"", e.what());
    cleanTemp(tempPath);
  } catch (const fs::filesystem_error &e) {
    Logger::failLog("save_cache(): failed to rename temporary file", e.what());
    cleanTemp(tempPath);
  }
}

void cleanTemp(const fs::path &tempPath) {
  try {
    if (fs::exists(tempPath)) {
      fs::remove(tempPath);
    }
  } catch (...) {
    Logger::warningLog("clean up error while removing: \"" + tempPath.string() + "\"");
  }
}

void load_cache(const fs::path &cachePath) {
  if (!fs::exists(cachePath) || fs::is_empty(cachePath)) {
    Logger::warningLog("cache missing or empty, full rebuild required");
    return;
  }

  std::ifstream in(cachePath); 
  ENABLE_EXCEPTIONS(in);
  PRINT_ENABLED_EXCEPTIONS;
  std::string path;
  uint64_t h;

  try {
    while (in >> path >> h) {
      old_hashes[path] = h;
    }
  } catch (const std::ios_base::failure &e) {
    Logger::failLog("load_cache(): failed to read \"" + cachePath.string() + "\"", e.what());
  }
}

void mark_modified(const Config& conf) {
  for (auto &[_, src] : sources) {
    src.hash = hash_file(src.path);

    if (!old_hashes.count(src.path.string())          // < if file doesn't exist in our set of hashed files (it wasn't there last time we built)
        || old_hashes[src.path.string()] != src.hash // <  or it does exist, but the hash doesn't match the new one (the contents changed)
       /* || !fs::exists(src.object) */) {               // < or it exists, and its hash exists, but its object file doesn't
      src.modified = true;                            // < then mark it as modified and move on to the next file
      Logger::debug("mark_modified(): " + src.path.string() + " marked as modified");
      continue;
    }

    Logger::debug("mark_modified(): source " + src.path.string() + " has include count: " + std::to_string(src.includes.size()));
    // at this point we know the source didn't change in any way, we check if the headers did
    for (const fs::path &inc : src.includes) {
      Logger::debug("mark_modified(): inside for loop");
      auto pathOfInclude = inc.string();
      auto it = headers.find(pathOfInclude);
      Logger::debug("mark_modified(): " + it->second.path.string());
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
      Logger::debug("mark_modified(): " + it->second.path.string() + " has been hashed.");

      if (old_hashes[pathOfInclude] != it->second.hash) { // if the hash that we just computed isn't the same as the one in the cached hashes
        src.modified = true;
        Logger::debug("mark_modified(): " + src.path.string() + " marked as modified");
        break; // we break once any of the headers triggers recompilation rather
               // than checking all of them
      }
    }
  }
}


int compile() {
  for (auto &[_, src] : sources) {
    if (src.modified) {
      // TODO: if any compilation fails, abort, do not write cache
      Logger::successLog("compiled source: \"" + src.path.string() + "\"");
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
