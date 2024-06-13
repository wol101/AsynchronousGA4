#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>

class QString;

class Utilities
{
public:
    static std::string toString(const char * const printfFormatString, ...);
};

#endif // UTILITIES_H
