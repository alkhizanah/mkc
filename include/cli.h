#ifndef CLI_H_
#define CLI_H_
#include <iostream> 
#include "config.h"


void printHelp() {
  std::cout << R"(
Usage: mkc [options]

Config:
  --config <file>         Load configuration from file
  --watch                 Watch project directory for changes and rebuild.
  --run                   Run the executable after compilation
  --exclude <file>        Exclude directory or specific file
  --exclude-fmt           Exclude a file extension (eg: .c)
  -r, --root <dir>        Set project root directory (default: .)
  -o, --output <name>     Specify output executable name
  --track-external        Track external header dependencies
  
Compiler Options:
  --compiler <compiler>   Specify compiler (default: g++)
  -I <dir>                Add include directory
  -D <define>             Add preprocessor define
  -O <level>              Optimization level (0, 1, 2, 3, s)
  -f <flag>               Add compiler flag
  -l <lib>                Link library
  -L <dir>                Add library search path
  --link-flags            Add arbitrary flags for the linker
  --shared                Use when creating a shared object.

Log Options:
  -h, --help              Show this help message
  -s, --silent            Suppress all non-error output
  -v, --verbose           Enable verbose logging
  -d, --debug-log         Enable debug logging (highest verbosity)
  --immediate             Force flushing all logs

Examples:
  mkc                           # Normal build
  mkc --clean --debug           # Clean debug build
  mkc -j4 -O3                   # Parallel release build with -O3
  mkc --watch --run             # Watch, rebuild and run on changes
  mkc -o myapp -I./external     # Custom output name and includes
)";
}

Config parse_cli_args(int argc, char *argv[]) {
  Config config;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      config.show_help = true;
      return config;
    }

    else if (arg == "-v" || arg == "--verbose") {
      config.log_verbosity = Verbosity::verbose;
    } else if (arg == "-d" || arg == "--debug-log") {
      config.log_verbosity = Verbosity::debug;
    } else if (arg == "-s" || arg == "--silent") {
      config.log_verbosity = Verbosity::silent;
    }

    else if (arg == "-c" || arg == "--clean") {
      config.rebuild_all = true;
      // TODO: force rebuild all
    } else if (arg == "--debug") {
      config.build_mode = BuildMode::debug;
      // TODO: add debug flags `-g, -O0` to compile_flags
      // NOTE: must see who overrides who in terms of -O and the debug / release build types
    } else if (arg == "--release") {
      config.build_mode = BuildMode::release;
      // TODO: add release flags `-O2, -DNDEBUG` to compile_flags
    } else if (arg == "-o" || arg == "--output") {
      if (i + 1 < argc) {
        config.executable_name = argv[++i];
      } else {
        throw std::runtime_error("--output requires an argument");
      }
    } else if (arg == "-j" || arg == "--jobs") {
      if (i + 1 < argc) {
        config.parallel_jobs = std::stoi(argv[++i]);
      } else {
        throw std::runtime_error("--jobs requires an argument");
      }
    }

    else if (arg == "--compiler") {
      if (i + 1 < argc) {
        config.compiler = argv[++i];
      } else {
        throw std::runtime_error("--compiler requires an argument");
      }
    } else if (arg == "-I") {
      if (i + 1 < argc) {
        config.include_dirs.push_back(argv[++i]);
      } else {
        throw std::runtime_error("-I requires an argument");
      }
    } else if (arg == "-D") {
      if (i + 1 < argc) {
        config.compile_flags.push_back("-D" + std::string(argv[++i]));
      } else {
        throw std::runtime_error("-D requires an argument");
      }
    } else if (arg == "-O") {
      if (i + 1 < argc) {
        config.compile_flags.push_back("-O" + std::string(argv[++i]));
      } else {
        throw std::runtime_error("-O requires an argument");
      }
    } else if (arg == "-f") {
      if (i + 1 < argc) {
        config.compile_flags.push_back(argv[++i]);
      } else {
        throw std::runtime_error("-f requires an argument");
      }
    } else if (arg == "-l") {
      if (i + 1 < argc) {
        config.link_flags.push_back("-l" + std::string(argv[++i]));
      } else {
        throw std::runtime_error("-l requires an argument");
      }
    } else if (arg == "--link-flags") {
      if (i + 1 < argc) {
        config.link_flags.push_back(std::string(argv[++i]));
      } else {
        throw std::runtime_error("--link-flags requires an argument");
      }
    } else if (arg == "--exclude") {
      if (i + 1 < argc) {
        config.exclude_dirs.push_back(std::string(argv[++i]));
      } else {
        throw std::runtime_error("--exclude requires an argument");
      }
    } else if (arg == "--exclude-fmt") {
      if (i + 1 < argc) {
        config.exclude_exts.push_back(std::string(argv[++i]));
      } else {
        throw std::runtime_error("--exclude-fmt requires an argument");
      }
    } else if (arg == "-L") {
      if (i + 1 < argc) {
        config.link_flags.push_back("-L" + std::string(argv[++i]));
      } else {
        throw std::runtime_error("-L requires an argument");
      }
    }

    else if (arg == "--config") {
      if (i + 1 < argc) {
        config.config_file = argv[++i];
        // TODO: config file parsing
      } else {
        throw std::runtime_error("--config requires an argument");
      }
    } else if (arg == "--track-external") {
      config.track_external_headers = true;
    } else if (arg == "--watch") {
      config.watch_mode = true;
    } else if (arg == "--run") {
      config.run_mode = true;
    } else if (arg == "--immediate") {
      config.log_immediately = true;
    } else if (arg == "--shared") {
      config.make_shared = true;
    } else if (arg == "-r" || arg == "--root") {
      if (i + 1 < argc) {
        config.root_dir = argv[++i];
      } else {
        throw std::runtime_error("--root requires an argument");
      }
    }

    else {
      throw std::runtime_error("Unknown option: " + arg);
    }
  }

  return config;
}

// void printHelp() {
//   std::cout << R"(
// Usage: mkc [options]
//
// Config:
//   --config <file>         Load configuration from file
//   --watch                 Watch project directory for changes and rebuild.
//   --run                   Run the executable after compilation
//   -r, --root <dir>        Set project root directory (default: .)
//   --track-external        Track external header dependencies
//
// Build Options:
//   -o, --output <name>     Specify output executable name
//   -j, --jobs <count>      Parallel compilation jobs (default: 1)
//   -c, --clean             Clean build (rebuild all)
//   --debug                 Debug build (build with debug symbols)
//   --release               Release build (optimized, default)
//
// Compiler Options:
//   --compiler <compiler>   Specify compiler (default: g++)
//   -I <dir>                Add include directory
//   -D <define>             Add preprocessor define
//   -O <level>              Optimization level (0, 1, 2, 3, s)
//   -f <flag>               Add compiler flag
//   -l <lib>                Link library
//   -L <dir>                Add library search path
//   --link-flags            Add arbitrary flags for the linker
//   --shared                Use when creating a shared object.
//
// Log Options:
//   -h, --help              Show this help message
//   -s, --silent            Suppress all non-error output
//   -v, --verbose           Enable verbose logging
//   -d, --debug-log         Enable debug logging (highest verbosity)
//   --immediate             Force flushing all logs
//
// Examples:
//   mkc                           # Normal build
//   mkc --clean --debug           # Clean debug build
//   mkc -j4 -O3                   # Parallel release build with -O3
//   mkc --watch --run             # Watch, rebuild and run on changes
//   mkc -o myapp -I./external     # Custom output name and includes
// )";
// }

#endif
