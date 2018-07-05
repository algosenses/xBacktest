#include "Errors.h"
#include "Logger.h"

namespace xBacktest
{

#define ERROR_FUNCTIONS

#if defined(_MSC_VER) || defined(__BORLANDC__)
    // allow Visual Studio integration
    std::string format(
#ifdef ERROR_LINES
        const std::string& file, long line,
#else
        const std::string&, long,
#endif
#ifdef ERROR_FUNCTIONS
        const std::string& function,
#else
        const std::string&,
#endif
        const std::string& message) {
        std::ostringstream msg;
#ifdef ERROR_FUNCTIONS
        if (function != "(unknown)")
            msg << function << ": ";
#endif
#ifdef ERROR_LINES
        msg << "\n  " << file << "(" << line << "): \n";
#endif
        msg << message;
        return msg.str();
    }
#else
    // use gcc format (e.g. for integration with Emacs)
    std::string format(const std::string& file, long line,
        const std::string& function,
        const std::string& message) {
        std::ostringstream msg;
#ifdef ERROR_LINES
        msg << "\n" << file << ":" << line << ": ";
#endif
#ifdef ERROR_FUNCTIONS
        if (function != "(unknown)")
            msg << "In function `" << function << "': \n";
#endif
        msg << message;
        return msg.str();
    }
#endif

Error::Error(const std::string& file, 
        long line, 
        const std::string& functionName, 
        const std::string& message /* = "" */)
{
    m_message = format(file, line, functionName, message);
    Logger_Err() << "ERROR: " << m_message;
}

const char* Error::what() const throw () 
{
    return m_message.c_str();
}

} // namespace xBacktest
