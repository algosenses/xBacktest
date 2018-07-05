#ifndef STOCK_MOMENTUM_H
#define STOCK_MOMENTUM_H

#include <iostream>
#include <unordered_map>
#include "Strategy.h"
#include "HighLow.h"
#include "MA.h"

class MomentumStrategy : public Strategy
{
public:
    void onCreate()
    {
        m_currMonth = -1;
        m_indicators.clear();
    }

    void onSetParameter(const string& name, int type, const string& value)
    {
        if (name == "period") {
            m_params.period = atoi(value.c_str());
        } else if (name == "maLength") {
            m_params.maLength = atoi(value.c_str());
        } else if (name == "poolSize") {
            m_params.poolSize = atoi(value.c_str());
        } else if (name == "amount") {
            m_params.amount = atof(value.c_str());
        } else if (name == "newHighLookbackDays") {
            m_params.newHighLookbackDays = atoi(value.c_str());
        } else if (name == "stopLossPct") {
            m_params.stopLossPct = atof(value.c_str());
        } else if (name == "expired") {
            m_params.expired = atoi(value.c_str());
        }
    }

    void onStart()
    {
        /*
        m_params.period = 52 * 5;
        m_params.maLength = 20;
        m_params.poolSize = 300;
        m_params.stopLossPct = 0.1; // 10%
        m_params.amount = 30000000;
        m_params.newHighLookbackDays = 1;
        m_params.expired = 8;
        */
        m_params.moneyUnit = m_params.amount / m_params.poolSize;
    }

    void onBar(const Bar& bar)
    {
        const string& instrument = bar.getInstrument();
        auto itor = m_indicators.find(instrument);
        if (itor == m_indicators.end()) {
            Indicator indicator = { 0 };
            indicator.high = new High();
            indicator.high->init(getBarSeries(instrument).getCloseDataSeries(), m_params.period);
            indicator.sma = new SMA();
            indicator.sma->init(getBarSeries(instrument).getCloseDataSeries(), m_params.maLength);
            m_indicators.insert(make_pair(instrument, indicator));
        }
    }

    void onTimeElapsed(const DateTime& prevDateTime, const DateTime& nextDateTime)
    {
        bool adjustStockPool   = false;

        int month = prevDateTime.month();
        if (month != m_currMonth && m_currMonth != -1) {
            adjustStockPool = true;
        }

        m_currMonth = month;

        if (adjustStockPool) {
            m_shoppingCart.clear();
            m_removeList.clear();
        }

        if (!adjustStockPool) {
            return;
        }

        vector<Position*> allPositions;
        getAllPositions(allPositions);
        int total = allPositions.size();

        if (adjustStockPool) {
//            std::cout << "\nAdjust stock pool[" << total << "] @ " << prevDateTime << std::endl;

            map<string, Position*> checkList;
            for (auto p : allPositions) {
                checkList.insert(std::make_pair(p->getInstrument(), p));
            }

            // 
            for (auto it = m_indicators.begin(); it != m_indicators.end(); it++) {
                High& high = *(it->second.high);
                if (high.length() < m_params.period || isnan(high[0])) {
                    continue;
                }

                double highest = high[0];
                const string& inst = it->first;
                BarSeries& series = getBarSeries(inst);
                double closePrice = series[0].getClose();

                bool hitNewHigh = false;
                DateTime dt = series[0].getDateTime();
                // can trading within today.
//                if (dt >= prevDateTime) {
                    for (int i = 0; i < m_params.newHighLookbackDays; i++) {
                        double close = series[i].getClose();
                        // Hit a new high
                        if (fabs(close - highest) < 0.000001) {
                            hitNewHigh = true;
                            break;
                        }
                    }
//                }

                if (hitNewHigh) {
                    if (getPositionSize(inst) > 0) {
                        continue;
                    }

                    int shares = int(m_params.moneyUnit / (closePrice*100)) * 100;
                    if (shares <= 0) {
                        continue;
                    }

                    if (total < m_params.poolSize) {
                        StockItem item;
                        item.instrument = inst;
                        item.datetime = prevDateTime;
                        item.shares = shares;
                        item.touch = false;
                        m_shoppingCart.insert(std::make_pair(inst, item));
                    
                        total++;
                    } else {
                        // Remove worst stock.
                        Position* pos = nullptr;
                        if (checkList.size() > 0) {
                            pos = checkList.begin()->second;
                            double pnl = pos->getPnL();
                            for (auto p : checkList) {
                                if (pnl > p.second->getPnL()) {
                                    pnl = p.second->getPnL();
                                    pos = p.second;
                                }
                            }

                            checkList.erase(pos->getInstrument());
                            StockItem item;
                            item.instrument = inst;
                            item.datetime = prevDateTime;
                            item.shares = 0;
                            item.touch = false;
                            m_removeList.insert(std::make_pair(inst, item));

                            if (pos->getInstrument() != inst) {
                                StockItem item;
                                item.instrument = inst;
                                item.datetime = prevDateTime;
                                item.shares = shares;
                                m_shoppingCart.insert(std::make_pair(inst, item));
                            }
                        }

                    }
                }

                // Price cross below SMA line, exit
                SMA& sma = *(it->second.sma);
                if (sma.length() < m_params.maLength || isnan(sma[0])) {
                    continue;
                }

                if (Cross::crossBelow(series.getCloseDataSeries(), sma)) {
                    m_shoppingCart.erase(inst);
                    Position* pos = getCurrentPosition(inst);

                    if (pos != nullptr) {
                        StockItem item;
                        item.instrument = inst;
                        item.datetime = prevDateTime;
                        item.shares = 0;
                        item.touch = false;
                        m_removeList.insert(std::make_pair(inst, item));

//                        std::cout << "--- Cross Below, exit!" << std::endl;
                    }
                }
            }

            for (auto item : m_removeList) {
                Position* pos = getCurrentPosition(item.second.instrument);
                if (pos && !pos->exitActive()) {
//                    std::cout << "---Sell stock:" << pos->getInstrument() << " @ " << prevDateTime << std::endl;
                    pos->close();
                    item.second.touch = true;
                }
            }

            int size = allPositions.size();
            int remain = m_params.poolSize - size;

//            std::cout << "shoping cart: " << m_shoppingCart.size() << ", remain: " << remain << std::endl;

            for (auto item : m_shoppingCart) {
                if (remain > 0) {
//                    std::cout << "+++Buy stock:" << item.second.instrument << " @ " << prevDateTime << std::endl;
                    Buy(item.second.instrument, item.second.shares);
                    item.second.touch = true;
                    remain--;
                }
            }
        }
    }

    // ReBuy or ReSell
    void onOrderFailed(const Order& order)
    {
//        std::cout << "***Order Failed: " << order.getInstrument() << " @ " << order.getSubmittedDateTime() << endl;

        const string& instrument = order.getInstrument();
        Order::Action action = order.getAction();

        for (auto it = m_removeList.begin(); it != m_removeList.end(); ) {
            const string& inst = it->second.instrument;
            if (inst == instrument) {
                DateTime dt = order.getSubmittedDateTime();
                if (dt.days() - it->second.datetime.days() > m_params.expired) {
//                    std::cout << "---Sell operation expired: " << inst << " @ " << order.getSubmittedDateTime() << std::endl;
                    it = m_removeList.erase(it);
                } else {
                    Position* pos = getCurrentPosition(inst);
                    if (pos && !pos->exitActive()) {
//                        std::cout << "---ReSell stock:" << pos->getInstrument() << " @ " << order.getSubmittedDateTime() << std::endl;
                        pos->close();
                    }
                    it++;
                }
            } else {
                it++;
            }
        }


        for (auto it = m_shoppingCart.begin(); it != m_shoppingCart.end(); ) {
            const string& inst = it->second.instrument;
            if (inst == instrument) {
                DateTime dt = order.getSubmittedDateTime();
                if (dt.days() - it->second.datetime.days() > m_params.expired) {
//                    std::cout << "+++Buy operation expired: " << inst << " @ " << order.getSubmittedDateTime() << std::endl;
                    it = m_shoppingCart.erase(it);
                } else {
//                    std::cout << "+++ReBuy stock:" << it->first << " @ " << order.getSubmittedDateTime() << std::endl;
                    Buy(it->first, it->second.shares);
                    it++;
                }
            } else {
                it++;
            }
        }
    }

    void onPositionOpened(Position& position)
    {
        position.setStopLossPct(m_params.stopLossPct);
        const string& inst = position.getInstrument();
//        std::cout << "Position opened: " << position.getInstrument() << " @ " << position.getEntryDateTime() << std::endl;
        auto it = m_shoppingCart.find(inst);
        if (it != m_shoppingCart.end()) {
            m_shoppingCart.erase(it);
        }
    }

    void onPositionClosed(Position& position)
    {
        const string inst = position.getInstrument();
        auto it = m_removeList.find(inst);
        if (it != m_removeList.end()) {
            m_removeList.erase(it);
        }
        
//        std::cout << "Close " << position.getInstrument() << " @ " << position.getExitDateTime() << std::endl;
    }

    void onDestroy()
    {
        for (auto itor = m_indicators.begin(); itor != m_indicators.end(); itor++) {
            delete itor->second.high;
            itor->second.high = nullptr;

            delete itor->second.sma;
            itor->second.sma = nullptr;
        }

        m_indicators.clear();
    }

private:
    typedef struct {
        int    poolSize;
        int    period;
        int    maLength;
        double stopLossPct;
        double amount;
        double moneyUnit;
        int    newHighLookbackDays;
        int    expired;
    } Parameters;

    typedef struct {
        High* high;
        Low*  low;
        SMA*  sma;
    } Indicator;

    std::unordered_map<string, Indicator> m_indicators;
    Parameters m_params;
    int m_currMonth;

    typedef struct {
        string   instrument;
        DateTime datetime;
        int      shares;
        bool     touch;
    } StockItem;

    map<string, StockItem> m_shoppingCart;
    map<string, StockItem> m_removeList;
};

#endif // STOCK_MOMENTUM_H