#ifndef EVENT_H
#define EVENT_H

#include <vector>
#include "Export.h"
#include "DateTime.h"

namespace xBacktest
{

class IEventHandler
{
public:
	virtual void onEvent(
        int             type, 
        const DateTime& datetime,
        const void *    context) = 0;
};

class DllExport Event
{
public:
    enum EvtType {
        EvtNone,
		EvtDispatcherStart,
		EvtDispatcherIdle,
        EvtDispatcherTimeElapsed,
		EvtDataSeriesNewValue,
        EvtDataSeriesReset,
		EvtOrderUpdate,
		EvtBarProcessed,
		EvtNewBar,
        EvtNewReturns,
        EvtNewTradingDay
    };

    typedef struct  {
        DateTime datetime;
        void* data;
    } Context;

    Event(EvtType type = EvtNone);
	void    setType(EvtType type);
	EvtType getType() const;
    void    subscribe(IEventHandler* handler);
    void    unsubscribe();
    void    emit(const DateTime& datetime, const void *context);

private:
    void applyChanges();

private:
    std::vector<IEventHandler*> m_handlers;
    std::vector<IEventHandler*> m_toSubscribe;
    std::vector<IEventHandler*> m_toUnsubscribe;
    volatile bool m_emitting;
    bool m_applyChanges;
	EvtType m_type;
};

} // namespace xBacktest

#endif // EVENT_H