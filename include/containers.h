#ifndef CONTAINERS_H_
#define CONTAINERS_H_ 
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <unordered_map>
#include <vector>
namespace fs = std::filesystem;


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
std::unordered_map<std::string, uint64_t> old_hashes;
#endif
