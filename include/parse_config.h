#ifndef PARSECONF_H_
#define PARSECONF_H_
#include <toml++/toml.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include "config.h"
#include "logger.h"

void generate_example_config(const fs::path &path);
void validate_config(const Config &c);

// NOTE: currently, this function overrides cli.
void load_toml_config(const fs::path &path, Config &config) {
  toml::table tbl;
  try {
    tbl = toml::parse_file(path.string());
  } catch (...) {
    throw std::runtime_error(
      "mkc_config.toml not found or can't be read."
    );
  }

  if (auto tool = tbl["tool"].as_table()) {
    if (auto n = tool->get("compiler"))
      if (auto v = n->value<std::string>())
        config.compiler = *v;

    if (auto n = tool->get("parallel_jobs"))
      if (auto v = n->value<int64_t>())
        config.parallel_jobs = *v;

    if (auto n = tool->get("watch"))
      if (auto v = n->value<bool>())
        config.watch_mode = *v;

    if (auto n = tool->get("verbosity"))
      if (auto v = n->value<std::string>()) {
        if (*v == "silent")       config.log_verbosity = Verbosity::silent;
        else if (*v == "normal")  config.log_verbosity = Verbosity::normal;
        else if (*v == "verbose") config.log_verbosity = Verbosity::verbose;
        else if (*v == "debug")   config.log_verbosity = Verbosity::debug;
      }
  }

  if (auto build = tbl["build"].as_table()) {
    if (auto n = build->get("root"))
      if (auto v = n->value<std::string>())
        config.root_dir = *v;

    if (auto n = build->get("mode"))
      if (auto v = n->value<std::string>()) {
        if (*v == "debug")        config.build_mode = BuildMode::debug;
        else if (*v == "release") config.build_mode = BuildMode::release;
      }
  }

  if (auto paths = tbl["paths"].as_table()) {
    if (auto arr = paths->get("include"); arr && arr->is_array())
      for (auto &&v : *arr->as_array())
        if (auto s = v.value<std::string>())
          config.include_dirs.emplace_back(*s);

    if (auto arr = paths->get("exclude_dirs"); arr && arr->is_array())
      for (auto &&v : *arr->as_array())
        if (auto s = v.value<std::string>())
          config.exclude_dirs.emplace_back(*s);

    if (auto arr = paths->get("exclude_exts"); arr && arr->is_array())
      for (auto &&v : *arr->as_array())
        if (auto s = v.value<std::string>())
          config.exclude_exts.emplace_back(*s);
    ////////////////////////////////////////////////////
    /// static libs
    ///////////////////////////////////////////////////
    if (auto arr = paths->get("static_lib"); arr && arr->is_array()) {
      for (auto &&elem : *arr->as_array()) {
        if (auto lib_tbl = elem.as_table()) {
          StaticLib lib;

          if (auto n = lib_tbl->get("name"))
            if (auto v = n->value<std::string>()) {
              lib.name = *v;
              fs::path tmp = lib.name;
              lib.archive = tmp.replace_extension(".a");
            }

          if (auto src_arr = lib_tbl->get("sources");
              src_arr && src_arr->is_array())
            for (auto &&v : *src_arr->as_array())
              if (auto s = v.value<std::string>())
                lib.sources.emplace_back(*s);

          if (auto inc_arr = lib_tbl->get("include_dirs");
              inc_arr && inc_arr->is_array())
            for (auto &&v : *inc_arr->as_array())
              if (auto s = v.value<std::string>())
                lib.include_dirs.emplace_back(*s);

          config.static_libs.push_back(std::move(lib));
        }
      }
    }
    ///////////////////////////////////////////////////
  }

  if (auto flags = tbl["flags"].as_table()) {
    if (auto arr = flags->get("compile"); arr && arr->is_array())
      for (auto &&v : *arr->as_array())
        if (auto s = v.value<std::string>())
          config.compile_flags.emplace_back(*s);

    if (auto arr = flags->get("link"); arr && arr->is_array())
      for (auto &&v : *arr->as_array())
        if (auto s = v.value<std::string>())
          config.link_flags.emplace_back(*s);
  }

  if (auto u = tbl["unity"].as_table()) {
    if (auto n = u->get("enabled"))
      if (auto v = n->value<bool>())
        config.unity_b = *v;

    if (auto n = u->get("source"))
      if (auto s = n->value<std::string>()) {
        config.unity_src_name = *s;
        config.unity_obj =
          fs::path("build/obj") /
          config.unity_src_name.filename().replace_extension(".o");
      }
  }

  if (auto proj = tbl["project"].as_table()) {
    if (auto n = proj->get("name"))
      if (auto v = n->value<std::string>())
        config.executable_name = *v;

    if (auto n = proj->get("shared"))
      if (auto v = n->value<bool>())
        config.make_shared = *v;
  }

  if (auto src = tbl["sources"].as_table()) {
    if (auto arr = src->get("files")->as_array()) {
      for (auto &&v : *arr) {
        if (auto s = v.value<std::string>())
          config.explicit_sources.emplace_back(*s);
      }
    }
  }

  if (auto pkg = tbl["pkg"].as_table()) {
    if (auto arr = pkg->get("deps"); arr && arr->is_array()) {
      for (auto &&v : *arr->as_array())
        if (auto s = v.value<std::string>())
          config.pkg_deps.push_back({*s});
    }
  }

  // try {
  //   validate_config(config);
  // } catch(const std::exception &e) {
  //   Logger::failLog("Configuration errors");
  //   std::cerr << e.what();
  //   throw 1;
  // }
}


void validate_config(const Config &c) {
  // std::vector<std::string> errors;
  // std::vector<std::string> warnings;
  //////////////////////////////////////////////////////////
  // NOTE: FIX UP AND TEST THOSE ERROR MESSAGES AND WARNINGS
  /////////////////////////////////////////////////////////
  //
  // //[tool] validation
  // if (c.compiler.empty()) {
  //   errors.push_back("[tool] compiler cannot be empty");
  // } else if (c.compiler != "g++" && c.compiler != "gcc") {
  //  warnings.push_back("[tool] compiler '" + c.compiler + "' is not supported (expected g++ or gcc)");
  // }
  // if (c.parallel_jobs < 1) {
  //   errors.push_back("[tool] parallel_jobs must be >= 1 (got " + std::to_string(c.parallel_jobs) + ")");
  // } else if (c.parallel_jobs > 64) {
  //   warnings.push_back("[tool] parallel_jobs = " + std::to_string(c.parallel_jobs) + " is very high");
  // }

  // //[build] validation
  // if (c.root_dir.empty()) {
  //   errors.push_back("[build] root directory cannot be empty");
  // } else if (!fs::exists(c.root_dir)) {
  //  errors.push_back("[build] root directory '" + c.root_dir + "' does not exist");
  // }
  // //[paths] validation
  // for (const auto &inc : c.include_dirs) {
  //   if (!fs::exists(inc)) {
  //     warnings.push_back("[paths] include directory '" + inc.string() + "' does not exist");
  //   }
  // }
  // for (const auto &ext : c.exclude_exts) {
  //   if (ext.empty() || ext[0] != '.') {
  //     warnings.push_back("[paths] exclude_exts should start with '.' (got '" + ext + "')");
  //   }
  // }
  // //[unity] validation
  // if (c.unity_b) {
  //   if (c.unity_src_name.empty()) {
  //     errors.push_back("[unity] enabled but unity name not set");
  //   } else if (!fs::exists(c.unity_src_name)) {
  //     errors.push_back("[unity] source file '" + c.unity_src_name.string() + "' does not exist");
  //   } else if (c.unity_src_name.extension() != ".cpp" && c.unity_src_name.extension() != ".cc") {
  //     warnings.push_back("[unity] source file should be .cpp or .cc (got '" + c.unity_src_name.extension().string() + "')");
  //   }
  // }
  //
  // //[project] validation
 //  if (c.executable_name.empty()) {
 //    errors.push_back("[project] name cannot be empty");
 //  }
 //
 //  //[sources] validation
 //  if (!c.explicit_sources.empty()) {
 //    for (const auto &src : c.explicit_sources) {
 //      if (!fs::exists(src)) {
 //        warnings.push_back("[sources] file '" + src.string() + "' does not exist");
 //      } else if (src.extension() != ".cpp" && src.extension() != ".cc" && src.extension() != ".c") {
 //        warnings.push_back("[sources] file '" + src.string() + "' has unusual extension");
 //      }
 //    }
 //  }
 //
 // // [flags] validation
 //  for (const auto &flag : c.compile_flags) {
 //    if (flag.empty()) {
 //      warnings.push_back("[flags] empty compile flag detected");
 //    }
 //  }
 //  for (const auto &flag : c.link_flags) {
 //    if (flag.empty()) {
 //      warnings.push_back("[flags] empty link flag detected");
 //    }
 //  }
 //
 //  //[pkg] validation
 //    // NOTE: could add: check if pkg-config knows about this package
 //    // but I think this is already handeled by the pkg function
 //    // system("pkg-config --exists " + dep.name);
 //  for (const auto &dep : c.pkg_deps) {
 //    if (dep.name.empty()) {
 //      errors.push_back("[pkg] dependency with empty name");
 //    }
 //  }

  ////////////////////////////////////////////////////
  ////  WARNINGS / ERRORS
  ////////////////////////////////////////////////////

  // if (!warnings.empty()) {
  //   Logger::warningLog("Config: warning!");
  //   std::cerr << std::endl;
  //   for (const auto &w : warnings) {
  //     std::cerr << "  - " << w << "\n";
  //   }
  //   std::cerr << "\n";
  // }
  //
  // if (!errors.empty()) {
  //   std::stringstream ss;
  //   for (const auto &e : errors) {
  //     ss << "  - " << e << "\n";
  //   }
  //   ss << "\nFix these errors in mkc_config.toml. Run:\n";
  //   ss << "  mkc init\n";
  //   ss << "to generate an example config.\n";
  //   throw std::runtime_error(ss.str());
  // }
}




void generate_example_config(const fs::path &path) {
  if (fs::exists(path)) {
    Logger::failLog(" Warning: mkc_config.toml already exists. Overwrite? (y/N): ");
    std::string response;
    std::getline(std::cin, response);
    if (response != "y" && response != "Y") {
      std::cout << "Aborted example config creation.\n";
      return;
    }
  }

  std::ofstream file(path);
  if (!file.is_open()) {
    Logger::failLog("Failed to create mkc_config.toml");
    return;
  }

  file << R"(# example mkc_config.toml file
# Generated by command: mkc init
# For details, check the readme at: 
# https://github.com/Nytril-ark/mkc

[tool]
compiler = "g++"
parallel_jobs = 1
watch = false
verbosity = "normal"

[build]
mode = "debug"

[paths]
include = []
exclude_dirs = []
exclude_exts = []

[[paths.static_lib]]
name = "lib.a"
sources = []
include_dirs = []

[flags]
compile = []
link = []

[unity]
enabled = false
source = "oneapp.cpp"

[project]
name = "app" 
shared = false

[sources]
files = []

[pkg]
deps = []
)";
  file.close();
}
#endif 
