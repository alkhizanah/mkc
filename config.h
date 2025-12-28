#ifndef CONFIG_H_
#define CONFIG_H_

enum class Verbosity {
  silent,
  normal,
  verbose
};

struct Config {
  bool rebuild_all = false;
  bool track_external_headers = false;
  Verbosity log_verbosity = Verbosity::normal;
};

#endif
