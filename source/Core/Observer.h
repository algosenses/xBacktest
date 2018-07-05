#ifndef OBSERVER_H
#define OBSERVER_H

#include "DateTime.h"

namespace xBacktest
{

class Subject
{
public:
    Subject();

    virtual ~Subject();

    virtual void start();

    virtual void stop();
    
	virtual void join();
    
	// Return True if there are not more events to dispatch.
    virtual bool eof();
    
	// Dispatch events. If True is returned, it means that at least one event was dispatched.
	virtual bool dispatch() = 0;
    
	// Return the datetime for the next event.
    // This is needed to properly synchronize non-realtime subjects.
    virtual const DateTime peekDateTime() const;
    
	// Returns a number (or None) used to sort subjects within the dispatch queue.
    // The return value should never change.
    virtual int getDispatchPriority() const;

    // Set dispatch priority. Smaller numbers have a higher priority.
    void setDispatchPriority(int priority);

private:
    int      m_priority;
};

} // namespace xBacktest

#endif // OBSERVER_H
