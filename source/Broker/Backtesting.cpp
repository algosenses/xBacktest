#include "Utils.h"
#include "Backtesting.h"
#include "Logger.h"
#include "Errors.h"

#define SECS_ONE_DAY            (60 * 60 * 24)
// 365 + 1/4 - 1/100 + 1/400 = 365.2425
#define AVERAGE_DAYS_PER_YEAR   (365.2425)
#define AVERAGE_DAYS_PER_MONTH  (AVERAGE_DAYS_PER_YEAR / 12)

namespace xBacktest
{

BacktestingBroker::BacktestingBroker()
{
}

void BacktestingBroker::init(double cash)
{
    REQUIRE(cash > 0, "Cash must be greater than 0");

    m_cash              = cash;
    m_availableCash     = cash;
    m_equity            = cash;
    m_posProfit         = 0;
    m_margin            = 0;
    m_maxMarginRequired = 0;
    m_totalCommissions  = 0;
    m_totalSlippages    = 0;

    m_barFillStrategy = nullptr;
    m_tickFillStrategy = nullptr;
    m_positions.clear();
    m_allowFractions = false;
    m_allowNegativeCash = false;

    m_activeOrders.clear();
    m_orderRecords.clear();

    m_firstBarDateTime.markInvalid();
    m_lastBarDateTime.markInvalid();

    m_notifyNewTradingDay = false;
    m_tradingDayEndTime = DEFAULT_TRADING_DAY_END_TIME;
}

void BacktestingBroker::initFillStrategy()
{
    delete m_tickFillStrategy;
    delete m_barFillStrategy;

    m_tickFillStrategy = new TickStrategy();
    m_barFillStrategy = new DefaultStrategy();
}

BacktestingBroker::~BacktestingBroker()
{
    delete m_tickFillStrategy;
    m_tickFillStrategy = nullptr;

    delete m_barFillStrategy;
    m_barFillStrategy = nullptr;

    m_orderRecords.clear();
    m_positions.clear();
}

void BacktestingBroker::setAllowFractions(bool allowFractions)
{
    m_allowFractions = allowFractions;
}

bool BacktestingBroker::getAllowFractions() const
{
    return m_allowFractions;
}

void BacktestingBroker::setAllowNegativeCash(bool allowNegativeCash)
{
    m_allowNegativeCash = allowNegativeCash;
}

double BacktestingBroker::getCash() const
{
    return m_cash;
}

double BacktestingBroker::getAvailableCash() const
{
    return m_availableCash;
}

double BacktestingBroker::getMargin() const
{
    return m_margin;
}

double BacktestingBroker::getPosProfit() const
{
    return m_posProfit;
}

double BacktestingBroker::getTotalCommission() const
{
    return m_totalCommissions;
}

double BacktestingBroker::getTotalSlippage() const
{
    return m_totalSlippages;
}

double BacktestingBroker::getMaxMarginRequired() const
{
    return m_maxMarginRequired;
}

void BacktestingBroker::setCash(double cash)
{
    m_cash          = cash;
    m_availableCash = cash;
    m_equity        = cash;
}

void BacktestingBroker::registerContract(const Contract& contract)
{
    if (contract.instrument[0] == '\0') {
        return;
    }
    
    m_contracts[contract.instrument] = contract;
}

FillStrategy& BacktestingBroker::getFillStrategy(int resolution)
{
    if (resolution != Bar::TICK) {
        ASSERT(m_barFillStrategy != nullptr, "Bar Fill Strategy is null.");
        return *m_barFillStrategy;
    } else {
        ASSERT(m_tickFillStrategy != nullptr, "Tick Fill Strategy is null.");
        return *m_tickFillStrategy;
    }
}

void BacktestingBroker::enableTradingDayNotification(bool enable)
{
    m_notifyNewTradingDay = enable;
}

void BacktestingBroker::setTradingDayEndTime(int timenum)
{
    if (timenum < 0 || timenum >= 240000) {
        return;
    }

    m_tradingDayEndTime = timenum;
}

void BacktestingBroker::getActiveOrders(const string& instrument)
{

}

int BacktestingBroker::getShares(const string& instrument) const
{
    map<string, BrokerPos>::const_iterator itor = m_positions.find(instrument);
    if (itor != m_positions.end()) {
        return itor->second.totalShares;
    }

    return 0;
}

int BacktestingBroker::getLongShares(const string& instrument) const
{
    int shares = 0;
    const auto& itor = m_positions.find(instrument);
    if (itor != m_positions.end()) {
        for (const auto& subpos : itor->second.subPositions) {
            if (subpos.shares > 0) {
                shares += subpos.shares;
            }
        }
        return shares;
    }

    return 0;
}

int BacktestingBroker::getShortShares(const string& instrument) const
{
    int shares = 0;
    const auto& itor = m_positions.find(instrument);
    if (itor != m_positions.end()) {
        for (const auto& subpos : itor->second.subPositions) {
            if (subpos.shares < 0) {
                shares += subpos.shares;
            }
        }
        return shares;
    }

    return 0;
}

void BacktestingBroker::getPositions()
{

}

const Contract& BacktestingBroker::getContract(const string& instrument) const
{
    static Contract defaultContract;
    // Performance optimization
    static Contract cachedContract;

    if (cachedContract.instrument != instrument) {
        const auto& itor = m_contracts.find(instrument);
        if (itor != m_contracts.end()) {
            cachedContract = itor->second;
            return cachedContract;
        } else {
            return defaultContract;
        }
    } else {
        return cachedContract;
    }
}

// Commission models, for implementing different commission schemes.
double BacktestingBroker::calculateCommission(const Order& order, double price, int quantity, double multiplier)
{
    const Contract& contract = getContract(order.getInstrument());

    switch (contract.commType) {
    // Always returns 0
    case CommissionType::NO_COMMISSION: 
        return 0;
        break;
    // Charges a fixed amount for the whole trade.
    case CommissionType::FIXED_PER_TRADE: {
        double ret = 0;
        // Only charge the first fill.
        if (order.getExecutionInfo().getQuantity() == 0) {
            ret = contract.commission;
        }

        return ret;
        break;
    }
    // Charges a percentage of the whole trade. 
    // The percentage to charge. 0.01 means 1%, and so on. It must be smaller than 1.
    case CommissionType::TRADE_PERCENTAGE: {
        double percentage = contract.commission;
        REQUIRE(percentage < 1 && percentage >= 0, "Commission percentage must be greater than 0 and smaller than 1.");
        return price * quantity * multiplier * percentage;
        break;
    }
    }

    return 0;
}

double BacktestingBroker::calculateSlippage(const Order& order, double price, int quantity, double multiplier)
{
    const Contract& contract = getContract(order.getInstrument());

    switch (contract.slippageType) {
    default:
    case SlippageType::NO_SLIPPAGE:
        return 0;
        break;

    case SlippageType::FIXED_PER_TRADE:
        return contract.slippage * multiplier * quantity;
        break;

    case  SlippageType::TRADE_PERCENTAGE:
        return contract.slippage * multiplier * price * quantity;
        break;

    }

    return 0;
}

void BacktestingBroker::updateEquityWithBar(const Bar& bar, bool onClose)
{
    const char* instrument = bar.getInstrument();
    m_posProfit = 0;
    m_portfolioValue = 0;

    for (auto& pos : m_positions) {
        double instrPrice = pos.second.lastPrice;
        if (pos.first == instrument) {
            // Update position's last price.
            pos.second.lastPrice = bar.getClose();
            // Use close price to calculate equity.
            if (onClose) {
                instrPrice = bar.getClose();
            } else {
                instrPrice = bar.getOpen();
            }
        }

        for (const auto& subpos : pos.second.subPositions) {
            int shares = subpos.shares;
            if (shares == 0) {
                continue;
            }

            double multiplier = getContract(bar.getInstrument()).multiplier;
            double marginRatio = getContract(bar.getInstrument()).marginRatio;

            double profit = 0.0;
            if (shares > 0) {
                profit = (instrPrice - subpos.price);
            } else {
                profit = (subpos.price - instrPrice);
            }

            double cost = subpos.price * abs(shares) * multiplier * marginRatio;
            profit = abs(shares) * profit * multiplier;
            m_posProfit += profit;
            m_portfolioValue += (cost + profit);
        }
    }

    m_equity = m_cash + m_portfolioValue;
    m_availableCash = m_equity - m_margin;
}

double BacktestingBroker::getEquity() const
{
    return m_equity;
}

long long BacktestingBroker::getTradingPeriod(int& year, int& month, int& day, int& hour, int& minute)
{
    long long ticks = (m_lastBarDateTime.ticks() - m_firstBarDateTime.ticks());

    long long secs = ticks / 1000;
    year = secs / (SECS_ONE_DAY * 365);
    if (year != 0) {
        secs = secs % (year * SECS_ONE_DAY * 365);
    }

    month = secs / (SECS_ONE_DAY * 30);
    if (month != 0) {
        secs = secs % (month * SECS_ONE_DAY * 30);
    }

    day = secs / SECS_ONE_DAY;
    if (day != 0) {
        secs = secs % SECS_ONE_DAY;
    }

    hour = secs / (60 * 60);
    if (hour > 0) {
        secs = secs % (hour * 60 * 60);
    }
    minute = secs / 60;

    return ticks;
}

void BacktestingBroker::printPositionList(Order* orderRef)
{
    std::cout << "BacktestingBroker: OrderID: " << orderRef->getId() << std::endl;
    for (const auto& pos : m_positions) {
        std::cout << pos.first << ":\n";
        for (const auto& subpos : pos.second.subPositions) {
            std::cout << "Quantity: " << subpos.shares << ", Price: " << subpos.price << std::endl;
        }
        std::cout << "\n\n";
    }
}

bool BacktestingBroker::commitOrderExecution(Order* orderRef, const Bar& bar, const FillInfo& fillInfo)
{
    double price = fillInfo.getPrice();
    int quantity = fillInfo.getQuantity();

    int sharesDelta = 0;

    bool   isOpen = orderRef->isOpen();
    bool   isBuy  = orderRef->isBuy();
    double availableCash = getAvailableCash();
    double resultingCash = getCash();
    double multiplier    = getContract(orderRef->getInstrument()).multiplier;
    double commission    = calculateCommission(*orderRef, price, quantity, multiplier);
    double slippage      = calculateSlippage(*orderRef, price, quantity, multiplier);
    double marginRatio   = getContract(orderRef->getInstrument()).marginRatio;

    double cost = price * quantity * multiplier;
    double margin = marginRatio * cost;
    
    if (isOpen) {
        resultingCash -= (margin + commission + slippage);
        availableCash -= (margin + commission + slippage);
    }

    sharesDelta = quantity;
    if (!isBuy) {
        sharesDelta *= -1;
    }

    // Check that we're ok on cash after the commission and margin.
    if (isBuy && (availableCash <= 0 || resultingCash <= 0) && !m_allowNegativeCash) {
        Logger_Warn() << "Not enough money ["
            << std::fixed << availableCash
            << "] to fill "
            << orderRef->getInstrument()
            << " order ["
            << orderRef->getId()
            << "] for "
            << orderRef->getRemaining()
            << " share/s.";

        orderRef->switchState(Order::State::REJECTED);
        OrderEvent evt(*orderRef, OrderEvent::REJECTED);
        evt.setEventInfo("Rejected");
        notifyOrderEvent(evt);

        ASSERT(false, "Not enough available cash.");

        return false;
    }

    // Update the order before updating internal state since addExecutionInfo may raise.
    // addExecutionInfo should switch the order state.
    OrderExecutionInfo orderExecutionInfo(bar.getDateTime(), price, quantity, commission, slippage);
    orderRef->addExecutionInfo(orderExecutionInfo);

    // Commit the order execution and update equity.
    map<string, BrokerPos>::iterator it = m_positions.find(orderRef->getInstrument());
    if (isOpen) {
        if (it == m_positions.end()) {
            BrokerPos pos = { 0 };
            pos.totalShares = sharesDelta;
            pos.avgPrice    = price;
            pos.lastPrice   = bar.getClose();

            SubPosItem item = { 0 };
            item.price = price;
            item.shares = sharesDelta;
            pos.subPositions.push_back(item);

            m_positions.insert(std::make_pair(orderRef->getInstrument(), pos));
        } else {
            SubPosItem item = { 0 };
            item.price = price;
            item.shares = sharesDelta;
            it->second.subPositions.push_back(item);

            cost = abs(it->second.totalShares) * it->second.avgPrice;
            cost += quantity * price;
            it->second.totalShares += sharesDelta;
            it->second.avgPrice = abs(cost / it->second.totalShares);
            it->second.lastPrice = bar.getClose();
        }

        m_cash = resultingCash;
        m_totalCommissions += commission;
        m_totalSlippages += slippage;
        m_margin += margin;

        if (m_margin > m_maxMarginRequired) {
            m_maxMarginRequired = m_margin;
        }

        updateEquityWithBar(bar);

    } else {
        double profit = 0;

        if (it == m_positions.end()) {
            ASSERT(false, "Position is not existed.");
        } else {
            double returnedMargin = 0;
            it->second.lastPrice = bar.getClose();

            if (isBuy) {  // Buy to cover
                // Close sub positions.
                int total = quantity;

                list<SubPosItem>& subpos = it->second.subPositions;
                list<SubPosItem>::reverse_iterator itor = subpos.rbegin();
                while (itor != subpos.rend()) {
                    // Skip long positions.
                    if (itor->shares >= 0) {
                        itor++;
                        continue;
                    }
                    int shares = abs(itor->shares);
                    if (total - shares >= 0) {
                        total -= shares;
                        profit += (itor->price - price) * shares * multiplier;
                        returnedMargin += itor->price * shares * multiplier * marginRatio;
                        itor++;
                        itor = list<SubPosItem>::reverse_iterator(subpos.erase(itor.base()));
                    } else {
                        itor->shares += total;
                        profit += (itor->price - price) * total * multiplier;
                        returnedMargin += itor->price * total * multiplier * marginRatio;
                        total = 0;
                        itor++;
                    }

                    profit -= commission;
                    profit -= slippage;

                    if (total <= 0) {
                        break;
                    }
                }
            } else {     // Sell
                // Close sub positions.
                int total = quantity;
                list<SubPosItem>& subpos = it->second.subPositions;
                list<SubPosItem>::reverse_iterator itor = subpos.rbegin();
                while (itor != subpos.rend()) {
                    // Skip short positions.
                    if (itor->shares <= 0) {
                        itor++;
                        continue;
                    }
                    int shares = abs(itor->shares);
                    if (total - shares >= 0) {
                        total -= shares;
                        profit = (price -  itor->price) * shares * multiplier;
                        returnedMargin += itor->price * shares * multiplier * marginRatio;
                        itor++;
                        itor = list<SubPosItem>::reverse_iterator(subpos.erase(itor.base()));
                    } else {
                        itor->shares -= total;
                        profit = (price -  itor->price) * total * multiplier;
                        returnedMargin += itor->price * total * multiplier * marginRatio;
                        total = 0;
                        itor++;
                    }

                    profit -= commission;
                    profit -= slippage;

                    if (total <= 0) {
                        break;
                    }
                }
            }

            m_totalCommissions += commission;
            m_totalSlippages += slippage;

            // current cash = last cash + returned margin + closed position profit
            m_margin -= returnedMargin;
            if (m_margin < 0) {
                ASSERT(false, "Margin must greater than 0.");
            }
            m_cash += (returnedMargin + profit);

            int shares = it->second.totalShares + sharesDelta;
            it->second.totalShares = shares;

            updateEquityWithBar(bar);
        }
    }

    // Let the strategy know that the order was filled.
    getFillStrategy(bar.getResolution()).onOrderFilled(*orderRef);

    // Notify the order update
    if (orderRef->isFilled()) {
        OrderEvent evt(*orderRef, OrderEvent::FILLED);
        evt.setExecInfo(orderExecutionInfo);
        notifyOrderEvent(evt);
    } else if (orderRef->isPartiallyFilled()) {
        OrderEvent evt(*orderRef, OrderEvent::PARTIALLY_FILLED);
        evt.setExecInfo(orderExecutionInfo);
        notifyOrderEvent(evt);
    } else {
        assert(false);
    }

    return true;    
}

void BacktestingBroker::placeOrder(const Order& order)
{
    if (order.isInitial()) {
        int sharesLong = getLongShares(order.getInstrument());
        if (order.getAction() == Order::Action::SELL && sharesLong < order.getQuantity()) {
            char error[64];
            sprintf(error, "Not enough shares to sell %s, shares: %d, required: %d.\n", 
                order.getInstrument().c_str(), sharesLong, order.getQuantity());
            ASSERT(false, error);
        }

        int sharesShort = getShortShares(order.getInstrument());
        if (order.getAction() == Order::Action::BUY_TO_COVER && sharesShort > order.getQuantity() * -1) {
            char error[64];
            sprintf(error, "Not enough shares to covert %s, shares: %d, required: %d.\n",
                order.getInstrument().c_str(), abs(sharesShort), order.getQuantity());
            ASSERT(false, error);
        }

        auto it = m_activeOrders.find(order.getId());
        assert(it == m_activeOrders.end());
        
        it = m_activeOrders.insert(make_pair(order.getId(), order)).first;
        // Switch from INITIAL -> SUBMITTED
        // IMPORTANT: Do not emit an event for this switch because when using the position interface
        // the order is not yet mapped to the position and Position::onOrderUpdated will get called.
        it->second.switchState(Order::State::SUBMITTED);
        it->second.setSubmittedDateTime(m_lastBarDateTime);

        m_orderRecords.insert(make_pair(order.getId(), order));

        OrderEvent evt(it->second, OrderEvent::SUBMITTED);
        evt.setEventInfo("Submitted");
        notifyOrderEvent(evt);

        // Process intra-bar order if needed.
        if (order.getExecTiming() == Order::ExecTiming::IntraBar) {
            Bar outBar;
            if (getLastBar(order.getInstrument(), outBar)) {
                processOrders(outBar);
            } else {
                char str[128];
                sprintf(str, "Backtesting broker has no bar to process order. instrument: %s\n", order.getInstrument().c_str());
                ASSERT(false, str);
            }
        }

    } else {
        ASSERT(false, "The order was already processed");
    }
}

void BacktestingBroker::saveCurrBar(const Bar& bar)
{
    if (!m_firstBarDateTime.isValid()) {
        m_firstBarDateTime = bar.getDateTime();
    }

    if (m_notifyNewTradingDay) {
        if (m_lastBarDateTime.isValid()) {
            bool newTradingDay = false;
            if (m_tradingDayEndTime == 0) {
                if (bar.getDateTime().date() > m_lastBarDateTime.date()) {
                    newTradingDay = true;
                }
            } else {
                // User specified trading period.
                int lastDateNum = m_lastBarDateTime.dateNum();
                int currDateNum = bar.getDateTime().dateNum();
                int currTimeNum = bar.getDateTime().timeNum();
                int lastTimeNum = m_lastBarDateTime.timeNum();
                if (currTimeNum > m_tradingDayEndTime) {
                    if (lastTimeNum <= m_tradingDayEndTime) {
                        newTradingDay = true;
                    } else {
                        if (lastDateNum < currDateNum) {
                            newTradingDay = true;
                        }
                    }
                } else {
                    if (lastDateNum < currDateNum && lastTimeNum <= m_tradingDayEndTime) {
                        newTradingDay = true;
                    }
                }
            }

            if (newTradingDay) {
                notifyNewTradingDayEvent(m_lastBarDateTime, bar.getDateTime());
            }
        }
    }

    if (bar.getDateTime() > m_lastBarDateTime || 
        !m_lastBarDateTime.isValid()) {
        m_lastBarDateTime = bar.getDateTime();
    }

    auto it = m_lastBars.find(bar.getInstrument());
    if (it == m_lastBars.end()) {
        it = m_lastBars.insert(std::make_pair(bar.getInstrument(), bar)).first;
    } else {
        it->second = bar;
    }
}

bool BacktestingBroker::getLastBar(const string& instrument, Bar& outBar) const
{
    unordered_map<string, Bar>::const_iterator it = m_lastBars.find(instrument);
    if (it != m_lastBars.end()) {
        outBar = it->second;
        return true;
    } else {
        return false;
    }
}

bool BacktestingBroker::preProcessOrder(Order* orderRef, const Bar& bar)
{
    if (checkExpired(orderRef, bar.getDateTime())) {
        return false;
    }

    bool rejected = false;

    Order::Action action    = orderRef->getAction();
    int  quantity           = orderRef->getQuantity();
    int  sharesLong         = getLongShares(orderRef->getInstrument());
    int  sharesShort        = getShortShares(orderRef->getInstrument());
    bool possibilityOfTrade = true;

    // If the bar's shape looks like a '-', there is no possibility of trading at this bar.
    if (bar.getResolution() == Bar::DAY && 
        fabs(bar.getHigh() - bar.getLow()) < 0.0000001) {
        Logger_Warn() << "There is no possibility of trading at this bar.[" 
                      << bar.getInstrument() << ", " 
                      << bar.getDateTime() << "]";

        possibilityOfTrade = false;
    }

    if (action == Order::Action::SELL &&
        sharesLong <= 0 &&
        abs(sharesLong) < quantity) {
        rejected = true;
    }
    
    if (action == Order::Action::BUY_TO_COVER &&
        sharesShort >= 0 &&
        abs(sharesShort) < quantity) {
        rejected = true;
    }

    if (!possibilityOfTrade) {
        rejected = true;
    }

    if (rejected) {
        orderRef->switchState(Order::State::REJECTED);
        OrderEvent evt(*orderRef, OrderEvent::REJECTED);
        evt.setEventInfo("Rejected");
        m_activeOrders.erase(orderRef->getId());
        orderRef = nullptr;
        notifyOrderEvent(evt);
        return false;
    }
    
    return true;
}

bool BacktestingBroker::postProcessOrder(Order* orderRef, const Bar& bar)
{
    // For non-GTC orders and daily (or greater) bars we need to check if orders should expire right now
    // before waiting for the next bar.
    if (!orderRef->getGoodTillCanceled()) {
        bool expired = false;
        Bar bar;
        if (getLastBar(orderRef->getInstrument(), bar)) {
            if (bar.getResolution() >= Bar::DAY) {
                expired = bar.getDateTime().date() >= orderRef->getAcceptedDateTime().date();
            }

            // Cancel the order if it will expire in the next bar.
            if (expired) {
                orderRef->switchState(Order::State::CANCELED);
                OrderEvent evt(*orderRef, OrderEvent::CANCELED);
                evt.setEventInfo("Expired");
                m_activeOrders.erase(orderRef->getId());
                orderRef = nullptr;
                notifyOrderEvent(evt);
            }
        }
    }

    return true;
}

bool BacktestingBroker::processOrder(Order* orderRef, const Bar& bar)
{
    bool ret = false;

    if (!preProcessOrder(orderRef, bar)) {
        return false;
    }

    //  Double dispatch to the fill strategy using the concrete order type.
    FillInfo fillInfo;
    switch (orderRef->getType()) {
    case Order::Type::MARKET:
        fillInfo = getFillStrategy(bar.getResolution()).fillMarketOrder(orderRef, *this, bar);
        break;
    case Order::Type::LIMIT:
        fillInfo = getFillStrategy(bar.getResolution()).fillLimitOrder(orderRef, *this, bar);
        break;
    case Order::Type::STOP:
        fillInfo = getFillStrategy(bar.getResolution()).fillStopOrder(orderRef, *this, bar);
        break;
    case Order::Type::STOP_LIMIT:
        fillInfo = getFillStrategy(bar.getResolution()).fillStopLimitOrder(orderRef, *this, bar);
        break;
    }

    if (fillInfo.getQuantity() > 0) {
        commitOrderExecution(orderRef, bar, fillInfo);
#ifdef _DEBUG
//        printPositionList(orderRef);
#endif
        ret = true;
    }

    if (orderRef->isActive()) {
        postProcessOrder(orderRef, bar);
    } else {
        m_activeOrders.erase(orderRef->getId());
        orderRef = nullptr;
    }

    return ret;
}

void BacktestingBroker::onBarImpl(Order* orderRef, const Bar& bar)
{
    // IF WE'RE DEALING WITH MULTIPLE INSTRUMENTS WE SKIP ORDER PROCESSING IF THERE IS NO BAR FOR THE ORDER'S
    // INSTRUMENT TO GET THE SAME BEHAVIOUR AS IF WERE BE PROCESSING ONLY ONE INSTRUMENT.
    if (bar.isValid()) {
        // Switch from SUBMITTED -> ACCEPTED
        if (orderRef->isSubmitted()) {
            orderRef->setAcceptedDateTime(bar.getDateTime());
            orderRef->switchState(Order::State::ACCEPTED);
            OrderEvent evt(*orderRef, OrderEvent::ACCEPTED);
            notifyOrderEvent(evt);
        }

        if (orderRef->isActive()) {
            // This may trigger orders to be added/removed from m_activeOrders.
            processOrder(orderRef, bar);
        } else {
            // If an order is not active it should be because it was canceled in this same loop and it should have been removed.
            assert(orderRef->isCanceled());
            const auto& it = m_activeOrders.find(orderRef->getId());
            assert(it == m_activeOrders.end());
        }
    }
}

void BacktestingBroker::onBarEvent(const Bar& bar)
{
    // 1: Save current bar for order processing.
    saveCurrBar(bar);

    // 2: Let the strategy know that new bar is being processed.
    getFillStrategy(bar.getResolution()).onBar(bar.getDateTime(), bar);

    // 3: Process remaining orders
    processOrders(bar);

    // 4: Update equity with bar.
    updateEquityWithBar(bar);

    // 5: Let analyzers process this bar.
    notifyAnalyzers(bar);
}

void BacktestingBroker::onPostBarEvent(const Bar& bar)
{
}

bool BacktestingBroker::checkExpired(Order* orderRef, const DateTime& datetime)
{
    // For non-GTC orders we need to check if the order has expired.
    if (!orderRef->getGoodTillCanceled()) {
        bool expired = false;
        if ((orderRef->isSubmitted() && datetime.yesterday() > orderRef->getSubmittedDateTime().date()) ||
            (orderRef->isAccepted() && datetime.date() > orderRef->getAcceptedDateTime().date())) {
            expired = true;
        }

        // Cancel the order if it is expired.
        if (expired) {
            orderRef->switchState(Order::State::CANCELED);
            OrderEvent evt(*orderRef, OrderEvent::CANCELED);
            evt.setEventInfo("Expired");
            m_activeOrders.erase(orderRef->getId());
            orderRef = nullptr;
            notifyOrderEvent(evt);
            return true;
        }
    }

    return false;
}

void BacktestingBroker::processOrders(const Bar& bar)
{
    if (m_activeOrders.size() == 0) {
        return;
    }

    vector<Order*> orderRefs;

    // Check if order has expired.
    for (auto& ord : m_activeOrders) {
        orderRefs.push_back(&(ord.second));
    }

    for (size_t i = 0; i < orderRefs.size(); i++) {
        checkExpired(orderRefs[i], bar.getDateTime());
    }

    orderRefs.clear();
    // This is to froze the orders that will be processed in this event, to avoid new getting orders introduced
    // and processed on this very same event.
    for (auto& ord : m_activeOrders) {
        if (ord.second.getInstrument() == bar.getInstrument()) {
            Order* ref = &(ord.second);
            orderRefs.push_back(ref);
        }
    }

    for (size_t i = 0; i < orderRefs.size(); i++) {
        // This may trigger orders to be added/removed from m_activeOrders.
        onBarImpl(orderRefs[i], bar);
    }
}

void BacktestingBroker::cancelOrder(unsigned long orderId)
{
    map<unsigned long, Order>::iterator it = m_activeOrders.find(orderId);
    if (it == m_activeOrders.end()) {
        return;
    }

    it->second.switchState(Order::State::CANCELED);
    OrderEvent evt(it->second, OrderEvent::CANCELED);
    evt.setEventInfo("Canceled");
    m_activeOrders.erase(orderId);
    notifyOrderEvent(evt);
}

void BacktestingBroker::attachAnalyzer(StgyAnalyzer& analyzer)
{
    attachAnalyzerEx(analyzer);
}

StgyAnalyzer* BacktestingBroker::getNamedAnalyzer(const string& name)
{
    const auto& it = m_namedAnalyzers.find(name);
    if (it != m_namedAnalyzers.end()) {
        return it->second;
    }

    return nullptr;
}

void BacktestingBroker::attachAnalyzerEx(StgyAnalyzer& strategyAnalyzer, const string& name)
{
    bool found = false;
    for (size_t i = 0; i < m_analyzers.size(); i++) {
        if (m_analyzers[i] == &strategyAnalyzer) {
            found = true;
            break;
        }
    }

    if (!found) {
        if (!name.empty()) {
            map<string, StgyAnalyzer*>::iterator it = m_namedAnalyzers.find(name);
            if (it != m_namedAnalyzers.end()) {
                throw std::runtime_error("A different analyzer named " + name + " was already attached");
            }
            m_namedAnalyzers[name] = &strategyAnalyzer;
        }
        strategyAnalyzer.beforeAttach(*this);
        m_analyzers.push_back(&strategyAnalyzer);
        strategyAnalyzer.attached(*this);
    }
}

void BacktestingBroker::notifyAnalyzers(const Bar& bar)
{
    for (size_t i = 0; i < m_analyzers.size(); i++) {
        if (m_analyzers[i] != nullptr) {
            m_analyzers[i]->beforeOnBar(*this, bar);
        }
    }
}

} // namespace xBacktest