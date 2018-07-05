#include "boost/python.hpp"
#include "Framework.h"
#include "Strategy.h"

using namespace boost::python;
using namespace xBacktest;

struct StrategyWrap : Strategy, wrapper<Strategy>
{
};

class DummyFramework { };

BOOST_PYTHON_MODULE(xBacktest)
{
    // Change the current scope 
    scope xBacktest
        = class_<DummyFramework>("xBacktest")
        ;

    class_<xBacktest::Framework, shared_ptr<xBacktest::Framework>, boost::noncopyable >("Framework", no_init)
        .def("instance", &xBacktest::Framework::instance, return_value_policy<reference_existing_object>())
        .staticmethod("instance")
        .def("release", &xBacktest::Framework::release)
        .def("loadScenario", &xBacktest::Framework::loadScenario)
        .def("run", &xBacktest::Framework::run)
        ;

    class_<xBacktest::Strategy>("Strategy")
        .def("getId", &xBacktest::Strategy::getId)
        ;
}