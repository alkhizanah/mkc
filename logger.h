#ifndef LOGGER_H_
#define LOGGER_H_
#include <iostream>
#include <string>

#define WHITE           "\033[37m"
#define BLACK           "\033[30m"
#define RED             "\033[31m"
#define GREEN           "\033[32m"
#define YELLOW          "\033[33m"
#define RESET           "\033[0m"

struct Logger {
  void failLog(std::string message) {
    std::cout << "\n[" << RED << "LOG" << "] " << RESET << message << "\n";
  }
  void successLog(std::string message) {
    std::cout << "\n[" << GREEN << "LOG" << "] " << RESET << message << "\n";
  }
  void warningLog(std::string message) {
    std::cout << "\n[" << YELLOW << "LOG" << "] " << RESET << message << "\n";
  }
};

#endif
