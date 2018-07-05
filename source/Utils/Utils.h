#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <cmath>

#include "Errors.h"
#include "Export.h"

using std::string;
using std::vector;

namespace Utils
{

#ifndef _MSC_VER
#define _stricmp  strcasecmp
#define _snprintf snprintf
#endif

DllExport void split(const std::string& s, const std::string& delim, std::vector<std::string>& result);
DllExport int  getMachineCPUNum();
DllExport void getAllFilesNamesWithinFolder(const string& folder, vector<string>& list);
DllExport string getFileExtension(const string& filename);
DllExport string getFileBaseName(const string& pathname);

template<typename T>
T roundCeilMultiple(T value, T multiple)
{
    if (multiple == 0) return value;
    return static_cast<T>(std::ceil(static_cast<double>(value) / static_cast<double>(multiple))*static_cast<double>(multiple));
}

template<typename T>
T roundFloorMultiple(T value, T multiple)
{
    if (multiple == 0) return value;
    return static_cast<T>(std::floor(static_cast<double>(value) / static_cast<double>(multiple))*static_cast<double>(multiple));
}

} // namespace Utils

#endif // UTILS_H
