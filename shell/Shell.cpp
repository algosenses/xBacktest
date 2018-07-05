#include <iostream>
#include "Simulator.h"

using namespace xBacktest;

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <scenario file>" << std::endl;
        return -1;
    }

    Simulator* simulator = Simulator::instance();

    if (!simulator->loadScenario(string(argv[1]))) {
        simulator->release();
        return -1;
    }

    simulator->run();

    std::cin.get();

    simulator->release();

    return 0;
}