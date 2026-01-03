#include <toml++/toml.h>
#include "config.h"

void validate_config(const Config &c);
void load_toml_config(const fs::path &path, Config &config) {
  toml::table tbl = toml::parse_file(path.string());

  //////// scalars
  config.root_dir = tbl["root"].value_or(config.root_dir);
  config.compiler = tbl["compiler"].value_or(config.compiler);
  config.executable_name = tbl["output"].value_or(config.executable_name);

  config.watch_mode = tbl["watch"].value_or(config.watch_mode);
  config.make_shared = tbl["shared"].value_or(config.make_shared);
  config.parallel_jobs = tbl["parallel_jobs"].value_or(config.parallel_jobs);

  ///////// enums
  if (auto v = tbl["verbosity"].value<std::string>()) {
    if (*v == "silent")
      config.log_verbosity = Verbosity::silent;
    else if (*v == "normal")
      config.log_verbosity = Verbosity::normal;
    else if (*v == "verbose")
      config.log_verbosity = Verbosity::verbose;
    else if (*v == "debug")
      config.log_verbosity = Verbosity::debug;
  }

  if (auto v = tbl["build_mode"].value<std::string>()) {
    if (*v == "debug")
      config.build_mode = BuildMode::debug;
    else if (*v == "release")
      config.build_mode = BuildMode::release;
  }

  ///////// vectors
  auto read_string_array = [&](const char *key, auto &out) {
    if (auto arr = tbl[key].as_array()) {
      for (auto &&v : *arr) {
        if (auto s = v.value<std::string>())
          out.emplace_back(*s);
      }
    }
  };

  auto read_path_array = [&](const char *key, auto &out) {
    if (auto arr = tbl[key].as_array()) {
      for (auto &&v : *arr) {
        if (auto s = v.value<std::string>())
          out.emplace_back(fs::path(*s));
      }
    }
  };

  read_path_array("include_dirs", config.include_dirs);
  read_path_array("exclude_dirs", config.exclude_dirs);
  read_string_array("exclude_exts", config.exclude_exts);
  read_string_array("compile_flags", config.compile_flags);
  read_string_array("link_flags", config.link_flags);

  //////// unity build block
  if (auto u = tbl["unity"].as_table()) {
    config.unity_b = u->get("enabled")->value_or(false);
    if (auto s = u->get("source")->value<std::string>()) {
      config.unity_src_name = *s;
      config.unity_obj = fs::path("build/obj") / config.unity_src_name.filename().replace_extension(".o");
    }
  }

  validate_config(config);
}


void validate_config(const Config &c) {
  // TODO: extend config validation;
  // and add auto-gen config
  if (c.unity_b && c.unity_src_name.empty())
    throw std::runtime_error("unity enabled but unity.source not set");

  if (c.parallel_jobs < 1)
    throw std::runtime_error("parallel_jobs must be >= 1");
}
