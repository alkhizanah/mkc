#include <toml++/toml.h>

auto cfg = toml::parse_file("mkc.toml");

config.root_dir = cfg["root"].value_or(".");
for (auto&& v : *cfg["exclude_dirs"].as_array())
  config.exclude_dirs.push_back(v.value<std::string>().value());

for (auto&& v : *cfg["exclude_fmt"].as_array())
  config.exclude_fmt.push_back(v.value<std::string>().value());
