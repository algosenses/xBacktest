#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif
#include <algorithm> 
#include <thread>
#include "Utils.h"

namespace Utils
{

void split(const std::string& s, const std::string& delim, std::vector<std::string>& result)
{
    size_t last = 0;
    size_t index = s.find_first_of(delim, last);
    while (index != std::string::npos) {
        result.push_back(s.substr(last, index - last));
        last = index + 1;
        index = s.find_first_of(delim, last);
    }

    if (index - last > 0) {
        result.push_back(s.substr(last, index - last));
    }
}

int getMachineCPUNum()
{
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    unsigned concurentThreadsSupported = std::thread::hardware_concurrency();
    if (concurentThreadsSupported == 0) {
        return 1;
    }

    return concurentThreadsSupported;
#endif

    return 1;
}

void getAllFilesNamesWithinFolder(const string& folder, vector<string>& list)
{
#ifdef _WIN32
    char search_path[200];
    sprintf(search_path, "%s/*.*", folder.c_str());
    WIN32_FIND_DATA fd;
    HANDLE hFind = ::FindFirstFile(search_path, &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // read all (real) files in current folder
            // , delete '!' read other 2 default folder . and ..
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                list.push_back(fd.cFileName);
            }
        } while (::FindNextFile(hFind, &fd));
        ::FindClose(hFind);
    }
#else
    struct dirent *entry;
    int ret = 1;
    DIR *dir;
    dir = opendir(folder.c_str());

    while ((entry = readdir(dir)) != NULL) {
        list.push_back(entry->d_name);
    }
#endif
}

string getFileExtension(const string& filename)
{
    std::string::size_type idx;
    idx = filename.rfind('.');

    if (idx != std::string::npos) {
        return filename.substr(idx + 1);
    } else {
        return "";
    }
}

#ifndef WIN32
struct MatchPathSeparator
{
    bool operator()(char ch) const
    {
        return ch == '/';
    }
};
#else
struct MatchPathSeparator
{
    bool operator()(char ch) const
    {
        return ch == '\\' || ch == '/';
    }
};
#endif
string getFileName(const string& pathname)
{
    return std::string(
        std::find_if(pathname.rbegin(), pathname.rend(),
        MatchPathSeparator()).base(),
        pathname.end());
}

std::string removeExtension(std::string const& filename)
{
    std::string::const_reverse_iterator
        pivot
        = std::find(filename.rbegin(), filename.rend(), '.');
    return pivot == filename.rend()
        ? filename
        : std::string(filename.begin(), pivot.base() - 1);
}

string getFileBaseName(const string& pathname)
{
    return removeExtension(getFileName(pathname));
}

} // namespace Utils