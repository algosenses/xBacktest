#ifndef UTILS_ERRORS_H
#define UTILS_ERRORS_H

#include <exception>
#include <iostream>
#include <sstream>
#include <string>

namespace xBacktest
{

class Error : public std::exception
{
public:
    Error(const std::string& file,
          long line,
          const std::string& functionName,
          const std::string& message = "");
    ~Error() throw() {}
    const char* what() const throw ();

private:
    std::string m_message;
};

} // namespace xBacktest

/* Fix C4127: conditional expression is constant when wrapping macros
    with do { ... } while(false); on MSVC
    */
#define MULTILINE_MACRO_BEGIN do {

#if defined(_MSC_VER) && _MSC_VER >= 1500 
/* __pragma is available from VC++9 */
#define MULTILINE_MACRO_END \
    __pragma(warning(push)) \
    __pragma(warning(disable:4127)) \
} while (false) \
    __pragma(warning(pop))
#else
#define MULTILINE_MACRO_END } while(false)
#endif

/*! \def ASSERT
    \brief throw an error if the given condition is not verified
    */
#define ASSERT(condition,message) \
if (!(condition)) { \
    std::ostringstream msg_stream; \
    msg_stream << message; \
    throw Error(__FILE__, __LINE__, \
    __FUNCTION__, msg_stream.str()); \
} else


/*! \def REQUIRE
    \brief throw an error if the given pre-condition is not verified
    */
#define REQUIRE(condition,message) \
if (!(condition)) { \
    std::ostringstream msg_stream; \
    msg_stream << message; \
    throw Error(__FILE__, __LINE__, \
    __FUNCTION__, msg_stream.str()); \
} else

#endif // UTILS_ERRORS_H