#include "../include/file_watch.h"
#ifdef __linux__
#include <filesystem>
#include <sys/inotify.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <fcntl.h>
namespace fs = std::filesystem;

bool watch(const fs::path &root) {
  std::vector<int> watches;
  int fd = inotify_init1(0);
  if (fd == -1)
    return false;
  
  uint32_t mask = IN_MODIFY      |
                  IN_ATTRIB      | 
                  IN_CREATE      |
                  IN_DELETE      |
                  IN_DELETE_SELF |
                  IN_MOVED_FROM  | 
                  IN_MOVED_TO;

  auto add_watch = [&](const fs::path &p) {
    int wd = inotify_add_watch(fd, p.c_str(), mask);
    if (wd != -1)
      watches.push_back(wd);
  };

  add_watch(root);
  for (const auto &entry : fs::recursive_directory_iterator(root)) {
    if (entry.is_directory()) {
      add_watch(entry.path());
    }
  }

  char buf[4096];
  ssize_t len = read(fd, buf, sizeof(buf));

  if (len > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(DEBOUNCE_MS));
    fcntl(fd, F_SETFL, O_NONBLOCK);
    while (read(fd, buf, sizeof(buf)) > 0) {}
    fcntl(fd, F_SETFL, 0);
  }

  for (int wd : watches)
    inotify_rm_watch(fd, wd);
  close(fd);
  return len > 0;
}

bool file_watcher(const Config &config) {
  return watch(config.root_dir);
}
#else
#include <iostream>
bool file_watcher(const std::string &) {
  std::cerr << "\nfile-watch not implemented on this platform\n";
  return false;
}
#endif
