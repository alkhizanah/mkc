#ifndef LOGGER_H_
#define LOGGER_H_
#include <iostream>
#include <fstream>
#include <string>
#include "config.h"
#include "containers.h"
#include "helpers.h"

#define BLUE            "\033[34m"
#define MAGENTA         "\033[35m"
#define CYAN            "\033[36m"
#define WHITE           "\033[37m"
#define BLACK           "\033[30m"
#define RED             "\033[31m"
#define GREEN           "\033[32m"
#define YELLOW          "\033[33m"
#define RESET           "\033[0m"

void f_flush(bool force) {
  if (!force) return;
  std::cout.flush();
}

// TODO: convert some debugs into info logs to separate dev logs from 
// the logs that are a concern for the end-user
class Logger {
private:
  static Verbosity vb;
  static bool force_f;

public:
  static void set_log_verbosity(Verbosity verbosity) { vb = verbosity; }
  static void set_log_immediacy(bool immediate) { force_f = immediate; }

  static void debug(const std::string &message) {
    if (vb != Verbosity::debug) return;
    std::cout << "\n[" << BLUE << "DEBUG" << RESET << "] " << message;
    f_flush(true);
  }

  static void infoLog(const std::string &message) {
    if (vb != Verbosity::verbose && vb != Verbosity::debug) return;
    std::cout << "\n[" << CYAN << "INFO" << RESET << "]  " << message;
    f_flush(force_f);
  }

  static void successLog(const std::string &message) {
    if (vb == Verbosity::silent) return;
    std::cout << "\n[" << GREEN << "LOG" << RESET << "]   " << message;
    f_flush(force_f);
  }

  static void failLog(const std::string &message, const char *exception = nullptr) {
    std::cout << "\n[" << RED << "LOG" << RESET << "]   " << message;
    if (vb == Verbosity::verbose || vb == Verbosity::debug) {
      std::cout << "\n[" << RED << "EXCEPTION" << RESET << "] ";
      std::cout << (exception ? exception : "unknown exception.") << std::endl;
    }
    f_flush(force_f);
  }

  static void warningLog(const std::string &message) {
    if (vb == Verbosity::silent) return;
    std::cout << "\n[" << YELLOW << "LOG" << RESET << "]   " << message;
    f_flush(force_f);
  }

  static void printLogfile(const std::string& logfile_path, const Config& conf) {
    if (vb == Verbosity::silent) return;
    if (!fs::exists(logfile_path)) {
      Logger::failLog("Log file does not exist at: " + logfile_path);
        return;
    }

    std::ifstream file(logfile_path);
    if (!file) {
      Logger::failLog("Error opening file at: " + logfile_path);
        return;
    }

    std::string line;
    std::cout << std::endl;
    if (conf.error_nums) {
      int i = 1;
      while (std::getline(file, line)) {
        std::cout << "[" << RED << i << RESET << "] " << line << std::endl;
        i++;
      }
    } else {
      while (std::getline(file, line)) {
        std::cout << line << std::endl;
      }
    }

    file.close();
    f_flush(force_f);
  }

  static void print_src_and_hdr(bool print_as_toml_array) {
    int i = 1;
    int j = 1;
    int width = 3;
    if (print_as_toml_array) {
      std::cout << BLUE << "\n######## sources ########" << RESET << std::endl;
      for (const auto& [_, src] : sources) {
        std::cout << "  \"" << readable_path(src.path) << "\", " << std::endl;
      }
      if (!headers.empty()) {
      std::cout << BLUE << "\n######## headers ########" << RESET << std::endl;
        for (const auto& [_, hdr] : headers) {
          std::cout << "  \"" << readable_path(hdr.path) << "\", " << std::endl;
        }
      }
    } else {
      std::cout << "\n[" << CYAN << "sources" << RESET << "] " << std::endl;
      for (const auto& [_, src] : sources) {
        std::string num = std::to_string(i);
        std::cout << "[" << BLUE << num << RESET << "]" << std::string(width - num.length(), ' ') << " " << readable_path(src.path) << std::endl;
        i++;
      }
      std::cout << "[" << CYAN << "LOG" << RESET << "] " << " " << (i - 1) << " sources." << std::endl;
      if (!headers.empty()) {
        std::cout << "[" << CYAN << "headers" << RESET << "] " << std::endl;
        for (const auto& [_, hdr] : headers) {
          std::string num = std::to_string(j);
          std::cout << "[" << BLUE << num << RESET << "]" << std::string(width - num.length(), ' ') << " " << readable_path(hdr.path) << std::endl;
          j++;
        }
        std::cout << "[" << CYAN << "LOG" << RESET << "] " << " " << (j - 1) << " headers." << std::endl;
      }
      std::cout << "[" << CYAN << "LOG" << RESET << "] " << " total: " << (i + j - 2) << " files." << std::endl;
    }
  }
};

Verbosity Logger::vb = Verbosity::normal;
bool Logger::force_f = false;

#endif
