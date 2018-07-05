#ifndef TECHNICAL_H
#define TECHNICAL_H

#include <limits>
#include <math.h>
#include "DateTime.h"
#include "DataSeries.h"
#include "Errors.h"
#include "circular.h"

namespace xBacktest
{

// An EventWindow class is responsible for making calculation over a moving window of values.
// Note: This is a base class and should not be used directly.
// @param: FilterType: Event window accept this type of data
// @param: ValueType: Event window output this type of data
template<typename FilterType = double, typename ValueType = double>
class EventWindow
{
public:

    class Values
    {
    public:
        void setDataBuffer(circular_buffer<ValueType>* buf)
        {
            m_pDataBuf = buf;
        }
        
        ValueType operator[](int pos) const
        {
            if (m_pDataBuf != nullptr) {
                circular_buffer<ValueType>& buf = *m_pDataBuf;
                int size = buf.size();

                if (pos >= size ||
                    (pos < 0 && abs(pos) > size)) {
                    throw std::out_of_range("circular buffer out of range.");
                }

                if (pos >= 0) {
                    return buf[pos];
                } else {
                    return buf[size + pos];
                }
            }

            return 0;
        }

        ValueType mean() const
        {
            ValueType mean = 0.0;
            if (m_pDataBuf != nullptr) {
                circular_buffer<ValueType>& buf = *m_pDataBuf;
                size_t size = buf.size();
                for (size_t i = 0; i < size; i++) {
                    mean += buf[i];
                }
                mean /= size;
            }

            return mean;
        }

        ValueType minimum() const
        {
            double ret = std::numeric_limits<double>::quiet_NaN();
            if (m_pDataBuf != nullptr) {
                circular_buffer<ValueType>& buf = *m_pDataBuf;
                if (buf.size() == 0) {
                    return ret;
                }

                ret = buf[0];
                for (size_t i = 1; i < buf.size(); i++) {
                    if (buf[i] < ret) {
                        ret = buf[i];
                    }
                }
            }

            return ret;
        }

        ValueType maximum() const
        {
            double ret = std::numeric_limits<double>::quiet_NaN();
            if (m_pDataBuf != nullptr) {
                circular_buffer<ValueType>& buf = *m_pDataBuf;
                if (buf.size() == 0) {
                    return ret;
                }

                ret = buf[0];
                for (size_t i = 1; i < buf.size(); i++) {
                    if (buf[i] > ret) {
                        ret = buf[i];
                    }
                }
            }

            return ret;
        }

        int size() const
        {
            if (m_pDataBuf != nullptr) {
                circular_buffer<ValueType>& buf = *m_pDataBuf;
                return buf.size();
            }

            return 0;
        }

    private:
        circular_buffer<ValueType>* m_pDataBuf;
    };

    // It is important to initialize circular_buffer to zero size
    EventWindow()
        : m_values(0)
    {
        m_values.clear();
    }
	// @param: windowSize: The size of the window. Must be greater than 0.
	// @param: dtype: The desired data-type for the array.
	// @param: skipNone: True if None values should not be included in the window.
    void init(int windowSize, int dtype = 0, bool skipNone = true)
    {
        REQUIRE(windowSize > 0, "Period length must been greater than 0, please check your indicator parameters.");

        m_windowSize = windowSize;
        m_skipNone = skipNone;
        m_values.clear();
        m_values.reserve(windowSize);

        m_outRefValues.setDataBuffer(&m_values);
    }

    // Default implementation be used in the scene where FilterType equals to EvtWinType.
    virtual void onNewValue(const DateTime& datetime, const FilterType& value)
    {
        pushNewValue(datetime, (ValueType&)value);
    }

    void pushNewValue(const DateTime& datetime, const ValueType& value)
    {
        if (!m_skipNone) {
        }

        m_values.push_back(value);
    }

	const Values& getValues() const
    {
        return m_outRefValues;
    }

	// Returns the window size.
	int getWindowSize() const
    {
        return m_windowSize;
    }

	// Return True if window is full.
	bool windowFull() const
    {
        return m_values.size() >= m_windowSize;
    }

	// Override to calculate a value using the values in the window.
	virtual ValueType getValue() const = 0;

    void clear()
    {
        m_values.clear();
    }

private:
	circular_buffer<ValueType> m_values;
	int    m_windowSize;
	bool   m_skipNone;
    Values m_outRefValues;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// An EventBasedFilter class is responsible for capturing new values in a DataSeries
// and using an EventWindow to calculate new values.
// @param: FilterType: Filter capturing this type of data stream.
// @param: ValueType: Filter output this type of data item.
template<typename FilterType = double, typename ValueType = double>
class EventBasedFilter : public SequenceDataSeries<ValueType>, public IEventHandler
{
public:
    EventBasedFilter() {}
	// @param: dataSeries: The DataSeries instance being filtered.
	// @param: eventWindow: The EventWindow instance to use to calculate new values.
	// @param: maxLen: The maximum number of values to hold.
	// Once a bounded length is full, when new items are added, a corresponding number of items are discarded from the opposite end.
	void init(DataSeries* dataSeries, EventWindow<FilterType, ValueType>* eventWindow, int maxLen = DataSeries::DEFAULT_MAX_LEN)
    {
        SequenceDataSeries<ValueType>::setMaxLen(maxLen);
        m_dataSeries  = dataSeries;
        m_eventWindow = eventWindow;
        m_dataSeries->getNewValueEvent().subscribe(this);
    }

	DataSeries* getDataSeries() const;

	EventWindow<FilterType, ValueType>* getEventWindow() const;

    void reset()
    {
        m_eventWindow->clear();
        SequenceDataSeries<ValueType>::clear();
    }

private:
    void onEvent(int type, const DateTime& datetime, const void* context)
    {
        if (type == Event::EvtDataSeriesNewValue) {
            Event::Context& ctx = *((Event::Context *)context);
            // Let the event window perform calculations.
            m_eventWindow->onNewValue(ctx.datetime, *(FilterType *)ctx.data);
            // Get the resulting value.
            // If event window is not full yet, resulting value is None.
            ValueType newValue = m_eventWindow->getValue();
            // Add the new value.
            SequenceDataSeries<ValueType>::appendWithDateTime(ctx.datetime, &newValue);
        }
    }

	DataSeries* m_dataSeries;
	EventWindow<FilterType, ValueType>* m_eventWindow;
};

} // namespace xBacktest

#endif // TECHNICAL_H
