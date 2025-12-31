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
#define FLUSH std::cout.flush()

// TODO: convert some debugs into info logs to separate dev logs from 
// the logs that are a concern for the end-user

class Logger {
private:
  static Verbosity vb;

public:
  static void setVerbosity(Verbosity verbosity) { vb = verbosity; }

  static void debug(const std::string &message) {
    if (vb != Verbosity::verbose) return;
    std::cout << "\n[" << BLUE << "DEBUG" << RESET << "] " << message;
    FLUSH;
  }

  static void infoLog(const std::string &message) {
    if (vb != Verbosity::verbose) return;
    std::cout << "\n[" << CYAN << "INFO" << RESET << "] " << message;
  }

  static void successLog(const std::string &message) {
    std::cout << "\n[" << GREEN << "LOG" << RESET << "] " << message;
  }

  static void failLog(const std::string &message, const char *exception = nullptr) {
    std::cout << "\n[" << RED << "LOG" << RESET << "] " << message;
    if (vb == Verbosity::verbose) {
      std::cout << "\n[" << RED << "EXCEPTION" << RESET << "] ";
      std::cout << (exception ? exception : "unknown exception.") << std::endl;
    }
  }

  static void warningLog(const std::string &message) {
    std::cout << "\n[" << YELLOW << "LOG" << RESET << "] " << message;
  }

  static void printLogfile(const std::string& logfile_path) {
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
  }
};

Verbosity Logger::vb = Verbosity::normal;

#endif
