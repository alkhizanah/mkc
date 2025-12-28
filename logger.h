#ifndef LOGGER_H_
#define LOGGER_H_
#include <iostream>
#include <string>
#include "config.h"

#define WHITE           "\033[37m"
#define BLACK           "\033[30m"
#define RED             "\033[31m"
#define GREEN           "\033[32m"
#define YELLOW          "\033[33m"
#define RESET           "\033[0m"


static void debug(std::string message) {
  std::cout << "\n[" << BLACK << "DEBUG" << RESET << "] " << message;
}
class Logger {
private: 
  Verbosity vb;
public:
  Logger(Verbosity verbosity) : vb(verbosity) {};

  void successLog(std::string message) const {
    std::cout << "\n[" << GREEN << "LOG" << RESET << "] " << message;
  }
  void failLog(std::string message) const {
    std::cout << "\n[" << RED << "LOG" << RESET << "] " << message;
  }
  void failLog(std::string message, const char * exception) const {
    std::cout << "\n[" << RED << "LOG" << RESET << "] " << message;
    if (vb == Verbosity::verbose) {
      if (exception) {
        std::cout << "\n[" << RED << "EXCEPTION" << RESET << "] " << exception;
      } else {
        std::cout << "\n[" << RED << "EXCEPTION" << RESET << "] " << "unknown exception.";
      }
    }
  }
  void warningLog(std::string message) const {
    std::cout << "\n[" << YELLOW << "LOG" << RESET << "] " << message;
  }
};

#endif
