#include <algorithm>
#include <cassert>
#include "Event.h"

namespace xBacktest
{

Event::Event(EvtType type)
	: m_type(type)
{
    m_handlers.clear();
    m_toSubscribe.clear();
    m_toUnsubscribe.clear();

    m_emitting     = false;
    m_applyChanges = false;
}

void Event::setType(EvtType type)
{
	m_type = type;
}

Event::EvtType Event::getType() const
{
	return m_type;
}

void Event::applyChanges()
{
    if (m_toSubscribe.size() > 0) {
        for (std::size_t i = 0; i < m_toSubscribe.size(); i++) {
            if (std::find(m_handlers.begin(), m_handlers.end(), m_toSubscribe[i]) == m_handlers.end()) {
                m_handlers.push_back(m_toSubscribe[i]);
            }
        }
        m_toSubscribe.clear();
    }

    if (m_toUnsubscribe.size() > 0) {
        for (std::size_t i = 0; i < m_toUnsubscribe.size(); i++) {
            std::vector<IEventHandler*>::iterator it;
            it = std::find(m_handlers.begin(), m_handlers.end(), m_toUnsubscribe[i]);
            if (it != m_handlers.end()) {
                m_handlers.erase(it);
            }
        }
        m_toUnsubscribe.clear();
    }
}

void Event::subscribe(IEventHandler* handler)
{
    if (m_emitting) {
        m_toSubscribe.push_back(handler);
        m_applyChanges = true;
    } else {
        for (std::size_t i = 0; i < m_handlers.size(); i++) {
            if (m_handlers[i] == handler) {
                return;
            }
        }
        m_handlers.push_back(handler);
    }
}

void Event::unsubscribe()
{

}

void Event::emit(const DateTime& datetime, const void *context)
{
    m_emitting = true;
    for (std::size_t i = 0; i < m_handlers.size(); i++) {
        m_handlers[i]->onEvent(m_type, datetime, context);
    }
    m_emitting = false;

    if (m_applyChanges) {
        applyChanges();
        m_applyChanges = false;
    }
}

} // namespace xBacktest