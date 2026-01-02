#ifndef LOGGER_H_
#define LOGGER_H_
#include <iostream>
#include <fstream>
#include <string>
#include "config.h"

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
    std::cout << "\n[" << CYAN << "INFO" << RESET << "] " << message;
    f_flush(force_f);
  }

  static void successLog(const std::string &message) {
    if (vb == Verbosity::silent) return;
    std::cout << "\n[" << GREEN << "LOG" << RESET << "] " << message;
    f_flush(force_f);
  }

  static void failLog(const std::string &message, const char *exception = nullptr) {
    std::cout << "\n[" << RED << "LOG" << RESET << "] " << message;
    if (vb == Verbosity::verbose) {
      std::cout << "\n[" << RED << "EXCEPTION" << RESET << "] ";
      std::cout << (exception ? exception : "unknown exception.") << std::endl;
    }
    f_flush(force_f);
  }

  static void warningLog(const std::string &message) {
    if (vb == Verbosity::silent) return;
    std::cout << "\n[" << YELLOW << "LOG" << RESET << "] " << message;
    f_flush(force_f);
  }

  static void printLogfile(const std::string& logfile_path) {
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
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }

    file.close();
    f_flush(force_f);
  }
};

Verbosity Logger::vb = Verbosity::normal;
bool Logger::force_f = false;

#endif
