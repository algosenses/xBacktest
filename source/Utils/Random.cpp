#include "Random.h"

namespace Utils
{

// Generator of random bits
unsigned int RandomGenerator::Generate()
{
    _lock.Lock();

    // calculate new state of the generator
    _currentState._z = 0x9069 * (_currentState._z & 0xffff) + (_currentState._z >> 16);
    _currentState._w = 0x4650 * (_currentState._w & 0xffff) + (_currentState._w >> 16);

    // generate new random value
    return (_currentState._z << 16) + _currentState._w;
}

// Generate random single precision floating point number in interval 0..1
float RandomGenerator::GenerateFloat()
{
    UnsignedIntToFloat converter;

    // generate random bits
    converter.bits = Generate();

    // covert to double
    converter.bits = (converter.bits & 0x007FFFFF) | 0x3F800000;

    return converter.number - 1;
}

// Generate random double precision floating point number in interval 0..1
double RandomGenerator::GenerateDouble()
{
    UnsignedIntToDouble converter;

    // generate random bits
    converter.bits[0] = Generate();
    converter.bits[1] = Generate();

    // covert to double
    converter.bits[_littleEndian] = (converter.bits[0] & 0x000FFFFF) | 0x3FF00000;

    return converter.number - 1;
}

// Initialization of random generator
void RandomGenerator::Initalization(unsigned int seed1,
    unsigned int seed2)
{
    // initialize seed
    _currentState._w = seed1 ? seed1 : 0x1f123bb5;
    _currentState._z = seed2 ? seed2 : 0x159a55e5;

    // detecting big or little endian
    UnsignedIntToDouble converter;
    converter.number = 1;
    _littleEndian = converter.bits[1] == 0x3FF00000;
}

} // namespace Utils