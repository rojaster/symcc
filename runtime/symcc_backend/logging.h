#ifndef SYMCC_LOGGING_H_
#define SYMCC_LOGGING_H_

#include "compiler.h"
#include "pin.H"
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <string>

namespace symcc {

#define UNREACHABLE()                                                          \
    LOG_FATAL(std::string(__FILE__) + ":" + std::to_string(__LINE__) +         \
              ": Unreachable");

#define SYMCC_ASSERT(x)                                                        \
    if (!(x))                                                                  \
        LOG_FATAL(std::string(__FILE__) + ":" + std::to_string(__LINE__) +     \
                  ": " #x);

void log(const char* tag, const std::string& msg);

#define LOG_DEBUG(msg)                                                         \
    do {                                                                       \
        if (isDebugMode())                                                     \
            log("DEBUG", msg);                                                 \
    } while (0);

bool isDebugMode();
void LOG_FATAL(const std::string& msg);
void LOG_INFO(const std::string& msg);
void LOG_STAT(const std::string& msg);
void LOG_WARN(const std::string& msg);

} // namespace symcc

#endif // SYMCC_LOGGING_H_
