#include <stdarg.h>
#include "Simulator.h"
#include "SimulatorImpl.h"

namespace xBacktest
{

Simulator* Simulator::m_instance = nullptr;

Simulator* Simulator::instance()
{
    if (m_instance == nullptr) {
        m_instance = new Simulator();
    }

    return m_instance;
}

void Simulator::release()
{
    delete m_instance;
    m_instance = nullptr;
}

Simulator::Simulator()
{
    m_implementor = new SimulatorImpl();
}

Simulator::~Simulator()
{
    delete m_implementor;
    m_implementor = nullptr;
}

BrokerConfig& Simulator::getBrokerConfig()
{
    return m_implementor->getBrokerConfig();
}

EnvironmentConfig& Simulator::getEnvironmentConfig()
{
    return m_implementor->getEnvironmentConfig();
}

DataFeedConfig& Simulator::getDataFeedConfig()
{
    return m_implementor->getDataFeedConfig();
}

StrategyConfig Simulator::createNewStrategyConfig()
{
    return m_implementor->createNewStrategyConfig();
}

void Simulator::setRunningMode(int mode)
{
    return m_implementor->setRunningMode(mode);
}

bool Simulator::loadScenario(const char* file)
{
    if (file == nullptr || file[0] == '\0') {
        return false;
    }

    return m_implementor->loadScenario(file);
}

bool Simulator::loadStrategy(StrategyConfig& config)
{
    return m_implementor->loadStrategy(config);
}

bool Simulator::loadStrategy(StrategyCreator* creator)
{
    return m_implementor->loadStrategy(creator);
}

void Simulator::writeLogMsg(int level, const char* msgFmt, ...) const
{
    char sMsg[1024] = { 0 };
    int bufLen = sizeof(sMsg);

    va_list args;
    va_start(args, msgFmt);
    int len = vsnprintf(NULL, 0, msgFmt, args);
    if (len > 0) {
        len = (len >= bufLen - 1 ? bufLen - 1 : len);
        vsnprintf(sMsg, len, msgFmt, args);
    }
    va_end(args);

    if (len > 0) {
        return writeLogMsg(level, sMsg);
    }
}

void Simulator::run()
{
    return m_implementor->run();
}

int Simulator::version() const
{
    return m_implementor->version();
}

// helper functions
void Simulator::setMachineCPUNum(int num)
{
    return m_implementor->setMachineCPUNum(num);
}

void Simulator::setDataSeriesMaxLength(int length)
{
    return m_implementor->setDataSeriesMaxLength(length);
}

bool Simulator::registerDataStream(const char* symbol, int resolution, int interval, const char* filename, int format)
{
    if (symbol == nullptr || symbol[0] == '\0') {
        return false;
    }

    if (filename == nullptr || filename[0] == '\0') {
        return false;
    }

    return m_implementor->registerDataStream(symbol, resolution, interval, filename, format);
}

void Simulator::setMultiplier(const char* symbol, double multiplier)
{
    if (symbol == nullptr || symbol[0] == '\0') {
        return;
    }

    return m_implementor->setMultiplier(symbol, multiplier);
}

void Simulator::setTickSize(const char* symbol, double size)
{
    if (symbol == nullptr || symbol[0] == '\0') {
        return;
    }

    return m_implementor->setTickSize(symbol, size);
}

void Simulator::setMarginRatio(const char* symbol, double ratio)
{
    if (symbol == nullptr || symbol[0] == '\0') {
        return;
    }

    return m_implementor->setMarginRatio(symbol, ratio);
}

void Simulator::setCommissionType(const char* symbol, int type)
{
    if (symbol == nullptr || symbol[0] == '\0') {
        return;
    }

    return m_implementor->setCommissionType(symbol, type);
}

void Simulator::setCommission(const char* symbol, double comm)
{
    if (symbol == nullptr || symbol[0] == '\0') {
        return;
    }

    return m_implementor->setCommission(symbol, comm);
}

void Simulator::setSlippageType(const char* symbol, int type)
{
    if (symbol == nullptr || symbol[0] == '\0') {
        return;
    }

    return m_implementor->setSlippageType(symbol, type);
}

void Simulator::setSlippage(const char* symbol, double slippage)
{
    if (symbol == nullptr || symbol[0] == '\0') {
        return;
    }

    return m_implementor->setSlippage(symbol, slippage);
}

void Simulator::setCash(double cash)
{
    return m_implementor->setCash(cash);
}

void Simulator::useTickDataToStopPos(bool useTickData)
{
    return m_implementor->useTickDataToStopPos(useTickData);
}

void Simulator::enableReport(int mask)
{
    return m_implementor->enableReport(mask);
}

void Simulator::disableReport(int mask)
{
    return m_implementor->disableReport(mask);
}

void Simulator::setPositionsReportFile(const char* filename)
{
    if (filename == nullptr || filename[0] == '\0') {
        return;
    }

    return m_implementor->setPositionsReportFile(filename);
}

} // namespace xBacktest