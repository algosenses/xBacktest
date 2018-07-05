#include "Export.h"
#include "HighLow.h"

namespace xBacktest
{

/*
class SlidingWindow
{
public:
  SlidingWindow(int size)
  {
    m_winSize = size;
    count = 0;
  }

  void append(int value)
  {
    while (!m_minWindow.empty() && m_minWindow.back().first >= value) {
       m_minWindow.pop_back();
     }
     m_minWindow.push_back(std::make_pair(value, count));
     while(m_minWindow.front().second <= count - m_winSize) {
       m_minWindow.pop_front();
     }
     m_minimun = m_minWindow.front().first;

     while (!m_maxWindow.empty() && m_maxWindow.back().first <= value) {
       m_maxWindow.pop_back();
     }
     m_maxWindow.push_back(std::make_pair(value, count));
     while(m_maxWindow.front().second <= count - m_winSize) {
       m_maxWindow.pop_front();
     }
     m_maximum = m_maxWindow.front().first;

     count++;
  }

  int minimun() const
  {
    return m_minimun;
  }

  int maximum() const
  {
    return m_maximum;
  }

private:
  int m_winSize;
  int m_minimun;
  int m_maximum;
  int count;
  std::deque< std::pair<int, int> > m_minWindow;
  std::deque< std::pair<int, int> > m_maxWindow;
};

int main()
{
  int input[] = { 1, 3, -1, -3, 5, 3, 6, 7, 11, 2, -19, 99, -101 };

  SlidingWindow window(3);

  for (int i = 0; i < sizeof(input) / sizeof(input[0]); i++) {
    window.append(input[i]);
    std::cout << window.maximum() << "\t" << window.minimun() << std::endl;
  }
  
  return 0;
}
*/
void HighLowEventWindow::init(int period, bool useMin)
{
    EventWindow<double, double>::init(period);
    m_useMin = useMin;
    m_winSize = period;
    count = 0;
    m_window.clear();
    m_minimun = 0;
    m_maximum = 0;
}

void HighLowEventWindow::onNewValue(const DateTime& datetime, const double& value)
{
    if (m_useMin) {
        while (!m_window.empty() && 
               (m_window.back().first > value || fabs(m_window.back().first - value) < 0.0000001)) {
            m_window.pop_back();
        }
        m_window.push_back(std::make_pair(value, count));
        while (m_window.front().second <= count - m_winSize) {
            m_window.pop_front();
        }
        m_minimun = m_window.front().first;
    } else {
        while (!m_window.empty() && 
               (m_window.back().first < value || fabs(m_window.back().first - value) < 0.0000001)) {
            m_window.pop_back();
        }
        m_window.push_back(std::make_pair(value, count));
        while (m_window.front().second <= count - m_winSize) {
            m_window.pop_front();
        }
        m_maximum = m_window.front().first;
    }

    EventWindow::pushNewValue(datetime, value);

    count++;
}

double HighLowEventWindow::getValue() const
{
    if (windowFull()) {
        if (m_useMin) {
            return m_minimun;
        } else {
            return m_maximum;
        }
    }

    return std::numeric_limits<double>::quiet_NaN();
}

void High::init(DataSeries& dataSeries, int period, int maxLen)
{
    EventBasedFilter<double, double>::init(&dataSeries, &m_eventWindow, maxLen);
    m_eventWindow.init(period, false);
}

void Low::init(DataSeries& dataSeries, int period, int maxLen)
{
    EventBasedFilter<double, double>::init(&dataSeries, &m_eventWindow, maxLen);
    m_eventWindow.init(period, true);
}

static int pivot(const DataSeries& ds, bool hilo, int len, int leftStrength, int rightStrength)
{
    return 0;
}

/*
  DEFINITION of 'Swing High'
  A term used in technical analysis that refers to the peak reached by an indicator 
  or an asset's price. A swing high is formed when the high of a price is greater 
  than a given number of highs positioned around it. A series of consecutively higher
  swing highs indicates that the given asset is in an uptrend.

  BREAKING DOWN 'Swing High'
  Swing highs can be used by traders to identify possible areas of support and resistance, 
  which can then be used to determine optimal positions for stop-loss orders. If an 
  indicator fails to create a new swing high while the price of the security does reach 
  a new high, there is a divergence between price and indicator, which could be a signal
  that the trend is reversing.
*/
int SwingHigh(const DataSeries& dataSeries, int leftStrength, int rightStrength)
{
    int len = leftStrength + rightStrength + 1;
    if (dataSeries.length() < len) {
        return 0;
    }

    SequenceDataSeries<double>& ds = (SequenceDataSeries<double>&)dataSeries;
    double pivot = ds[rightStrength];

    for (int i = rightStrength - 1; i >= 0; i--) {
        if (pivot <= ds[i]) {
            return 0;
        }
    }

    for (int i = rightStrength + 1; i < len; i++) {
        if (pivot < ds[i]) {
            return 0;
        }
    }

    return 1;
}

int SwingLow(const DataSeries& dataSeries, int leftStrength, int rightStrength)
{
    int len = leftStrength + rightStrength + 1;
    if (dataSeries.length() < len) {
        return 0;
    }

    SequenceDataSeries<double>& ds = (SequenceDataSeries<double>&)dataSeries;
    double pivot = ds[rightStrength];

    for (int i = rightStrength - 1; i >= 0; i--) {
        if (pivot >= ds[i]) {
            return 0;
        }
    }

    for (int i = rightStrength + 1; i < len; i++) {
        if (pivot > ds[i]) {
            return 0;
        }
    }

    return 1;    
}

} // namespace xBacktest