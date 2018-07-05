#include <cassert>
#include "Observer.h"

namespace xBacktest
{

Subject::Subject()
{
    m_priority = 0;
}

Subject::~Subject()
{
}

void Subject::start()
{
}

void Subject::stop()
{
}

void Subject::join()
{
}

bool Subject::eof()
{
	return true;
}

int Subject::getDispatchPriority() const
{
    return m_priority;
}

void Subject::setDispatchPriority(int priority)
{
    assert(priority >= 0);
    m_priority = priority;
}

const DateTime Subject::peekDateTime() const
{
    DateTime dt;
    dt.markInvalid();
    return dt;
}

} // namespace xBacktest
