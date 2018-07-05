#include <cassert>
#include <algorithm>
#include <functional>
#include "Errors.h"
#include "Dispatcher.h"
#include "Event.h"

namespace xBacktest
{

volatile unsigned long Dispatcher::m_nextId = 0;

Dispatcher::Dispatcher()
	: m_startEvent(Event::EvtDispatcherStart)
	, m_idleEvent(Event::EvtDispatcherIdle)
    , m_timeElapsedEvent(Event::EvtDispatcherTimeElapsed)
{
    m_id = ++m_nextId;

    m_subjects.clear();
    m_stop = false;
    m_eof  = false;

    m_currDateTime.markInvalid();
    m_prevDateTime.markInvalid();
}

unsigned long Dispatcher::getId() const
{
    return m_id;
}

const DateTime& Dispatcher::getCurrDateTime() const
{
    return m_currDateTime;
}

const DateTime& Dispatcher::getPrevDateTime() const
{
    return m_prevDateTime;
}

Event& Dispatcher::getStartEvent()
{
    return m_startEvent;
}

Event& Dispatcher::getIdleEvent()
{
    return m_idleEvent;
}

Event& Dispatcher::getTimeElapsedEvent()
{
    return m_timeElapsedEvent;
}

bool Dispatcher::dispatchSubject(Subject *subject)
{
	// Dispatch if the datetime is currEventDateTime of if its a realtime subject.
	return subject->dispatch();
}

bool Dispatcher::dispatch()
{
	bool eventsDispatched = false;
    long index = -1;
    size_t i = 0;

    m_eof = true;

    DateTime smallestDateTime;
    smallestDateTime.markInvalid();
    // Scan for the lowest datetime.
    for (i = 0; i < m_subjects.size(); i++) {
        if (!m_subjects[i]->eof()) {
            if (!smallestDateTime.isValid()) {
                smallestDateTime = m_subjects[i]->peekDateTime();
            }

            DateTime dt = m_subjects[i]->peekDateTime();
            if (dt.isValid() && smallestDateTime > dt) {
                smallestDateTime = dt;
            }
        }
    }

    // Collect subjects with the same lowest datetime.
    size_t subjectCount = 0;
    for (i = 0; i < m_subjects.size(); i++) {
        if (m_subjects[i]->eof()) {
            continue;
        }

        DateTime dt = m_subjects[i]->peekDateTime();
        if (!dt.isValid()) {
            continue;
        }

        if (dt == smallestDateTime) {
            m_subjectIdxs[subjectCount] = i;
            subjectCount++;
        }
    }

    if (subjectCount > 0) {
        m_eof = false;
        // Notify time elapsed event.
        m_currDateTime = smallestDateTime;
        if (m_prevDateTime.isValid()) {
            if (m_prevDateTime > m_currDateTime) {
                char str[128];
                sprintf(str, "\r\nFATAL: Timeline wrap back, please verify the correctness of your input data. \r\nPrevious: %s \r\nCurrent:  %s", 
                            m_prevDateTime.toString().c_str(), 
                            m_currDateTime.toString().c_str());
                ASSERT(false, str);
            }
            m_timeElapsedEvent.emit(m_currDateTime, &m_prevDateTime);
        } else {
            m_timeElapsedEvent.emit(m_currDateTime, &m_currDateTime);
        }

        m_prevDateTime = m_currDateTime;
    }

    // Dispatch subjects for this round.
    for (i = 0; i < subjectCount; i++) {
        int idx = m_subjectIdxs[i];

        if (dispatchSubject(m_subjects[idx])) {
            eventsDispatched = true;
        }
    }

    return eventsDispatched;
}

void Dispatcher::run()
{
    if (m_subjects.size() == 0) {
        return;
    }

    m_subjectIdxs.resize(m_subjects.size());

    for (size_t i = 0; i < m_subjects.size(); i++) {
        m_subjects[i]->start();
    }

    m_startEvent.emit(DateTime(), nullptr);

    bool eventsDispatched = false;

    while (!m_stop) {
        eventsDispatched = dispatch();
        if (m_eof) {
            m_stop = true;
        } else if (!eventsDispatched) {
            m_idleEvent.emit(DateTime(), nullptr);
        }
    }

    for (size_t i = 0; i < m_subjects.size(); i++) {
        m_subjects[i]->stop();
    }

    for (size_t i = 0; i < m_subjects.size(); i++) {
        m_subjects[i]->join();
    }
}

void Dispatcher::stop()
{
}

bool Dispatcher::lowerPriority(const Subject* s1, const Subject* s2)
{
    return s1->getDispatchPriority() < s2->getDispatchPriority();
}

void Dispatcher::addSubject(Subject* subject)
{
    if (subject == nullptr) {
        return;
    }

    size_t size = m_subjects.size();
    for (size_t i = 0; i < size; i++) {
        if (m_subjects[i] == subject) {
            return;
        }
    }

    m_subjects.push_back(subject);
    sort(m_subjects.begin(), m_subjects.end(), lowerPriority);
}

} // namespace xBacktest