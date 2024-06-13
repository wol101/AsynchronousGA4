#include "Utilities.h"

#include <stdarg.h>
#include <memory>

// convert to string using printf style formatting and variable numbers of arguments
std::string Utilities::toString(const char * const printfFormatString, ...)
{
    // initialize use of the variable argument array
    va_list vaArgs;
    va_start(vaArgs, printfFormatString);

    // reliably acquire the size
    // from a copy of the variable argument array
    // and a functionally reliable call to mock the formatting
    va_list vaArgsCopy;
    va_copy(vaArgsCopy, vaArgs);
    const int iLen = std::vsnprintf(nullptr, 0, printfFormatString, vaArgsCopy);
    va_end(vaArgsCopy);

    // return a formatted string without risking memory mismanagement
    // and without assuming any compiler or platform specific behavior
    std::unique_ptr<char[]> zc = std::make_unique<char[]>(iLen + 1);
    std::vsnprintf(zc.get(), size_t(iLen + 1), printfFormatString, vaArgs);
    va_end(vaArgs);
    return std::string(zc.get(), size_t(iLen));
}
