#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <vector>
#include "Event.h"
#include "Observer.h"

namespace xBacktest
{

// This class is responsible for dispatching events from multiple subjects, synchronizing them if necessary.
class Dispatcher 
{
public:
    Dispatcher();
    unsigned long getId() const;
    // Returns the current event datetime. It may be None for events from real-time subjects.
    const DateTime& getCurrDateTime() const;
    const DateTime& getPrevDateTime() const;
    Event& getStartEvent();
    Event& getIdleEvent();
    Event& getTimeElapsedEvent();
    
	void run();
	void stop();
    void addSubject(Subject* subject);

private:
    // True if all subjects hit eof
	// True if at least one subject dispatched events.
	bool dispatch();

    //  Return True if events were dispatched.
    bool dispatchSubject(Subject *subject);

    static bool lowerPriority(const Subject* s1, const Subject* s2);

private:
    static volatile unsigned long m_nextId;
    
    unsigned long m_id;

    std::vector<Subject*> m_subjects;   
    std::vector<int> m_subjectIdxs;

    bool m_stop;
    Event m_startEvent;
    Event m_idleEvent;
    Event m_timeElapsedEvent;
    DateTime m_currDateTime;
    DateTime m_prevDateTime;
    bool m_eof;
};

} // namespace xBacktest

#endif	// DISPATCHER_H