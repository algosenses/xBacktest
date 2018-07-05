#ifndef UTILS_RANDOM_H
#define UTILS_RANDOM_H

#include <time.h>
#include "Mutex.h"

namespace Utils
{

/// This union is used for conversation from 32-bits long integer random number
/// to single precision floating point number in interval (0, 1).
union UnsignedIntToFloat
{

    /// This field is used to store 32-bit long integer number.
    unsigned int bits;

    /// field is used to read 32-bit long integer number as mantissa of single precision floating point number.
    float number;

};

/// This union is used for conversation from 64-bits long integer random number
/// to double precision floating point number in interval (0, 1).
union UnsignedIntToDouble
{

    /// This field is used to store 64-bit long integer number.
    unsigned int bits[2];

    /// This field is used to read 64-bit long integer number as mantissa of single precision floating point number.
    double number;

};

/// RandomGenerator class implements algorithm for generating 64-bits wide random unsigned integers and floating-point numbers.
/// It takes care of architecture's endianness, but underlying CPU architecture must support floating-point by IEEE 754 standard.
/// RandomGenerator class does not implement interface. Primary purpose of this class is to provide service
/// for generating random numbers for classes which implement interface.
///
/// This class has built-in synchronizator so it is allowed to use LOCK_OBJECT and LOCK_THIS_OBJECT macros with instances of this class.
/// All public methods are thread-safe.
class RandomGenerator
{

private:

    /// Defines representations of random generator's state.
    struct State
    {

        /// The first part of random generator state.
        unsigned int _w;

        /// The second part of random generator state.
        unsigned int _z;

    };

    /// Current state of random generator.
    State _currentState;

    /// This attribute indicates endianness of architecture. If it is set to true, the architecture is little-endian,
    /// if the architecture is big-endian this attribute is set to false.
    bool _littleEndian;

    Mutex _lock;

public:

    /// This constructor initialize random generator with current time as seed.
    RandomGenerator()
    {
        unsigned long long x = (unsigned long long)time(NULL);
        Initalization((unsigned int)(x >> 16), (unsigned int)x);
    }

    /// This constructor initialize random generator with user-defined seed.
    /// @param seed: user-defined seed.
    RandomGenerator(unsigned int seed) { Initalization(seed, 0); }

    /// Generate method generates and returns 32-bit wide unsigned integer.
    ///
    /// This method is thread-safe.
    /// @returns: Method returns generated number.
    unsigned int Generate();

    /// GeneratrFloat method generates single precision floating point number i interval (0, 1).
    ///
    /// This method is thread-safe.
    /// @returns: Method returns generated number.
    float GenerateFloat();

    /// GeneratrFloat method generates double precision floating point number i interval (0, 1).
    ///
    /// This method is thread-safe.
    /// @returns: Method returns generated number.
    double GenerateDouble();

    /// Initializes random generator with specified seed. Initialization method is called by constructor.
    /// @param seed1: seed used to initialize the first part of generator's state.
    /// @param seed2: seed used to initialize the second part of generator's state.
    void Initalization(unsigned int seed1, unsigned int seed2);

};

/// Interface for random value generators.
/// @param TYPE: type of generated values.
template <typename TYPE>
class Random
{

public:

    /// This method generates random values of TYPE with no specific range.
    /// @returns: Returns generate random value.
    virtual TYPE Generate() = 0;

    /// This method generates random value of TYPE with specified maximum.
    /// @param max: maximal value which can be generated.
    /// @returns: Returns generate random value.
    virtual TYPE Generate(const TYPE& max) = 0;

    /// This method generates random value of TYPE within specified range of values.
    /// @param min: minimal value which can be generated.
    /// @param max: maximal value which can be generated.
    /// @returns: Returns generate random value.
    virtual TYPE Generate(const TYPE& min,
        const TYPE& max) = 0;

};

/// RandomInteger class generates random 32-bits wide integer numbers. The class implements interface.
/// This class has no built-in synchronizator, so LOCK_OBJECT and LOCK_THIS_OBJECT macros cannot be used with instances of this class,
/// but all public methods are thread-safe.
class RandomInteger : public Random<int>
{

private:

    /// Instance of algorithm for generating random numbers.
    RandomGenerator _generator;

public:

    /// This constructor initializes random generator with current time as seed.
    RandomInteger() { }

    /// This constructor initialize random generator with user-defined seed.
    /// @param seed: user-defined seed.
    RandomInteger(unsigned long seed) : _generator(seed) { }

    /// This method generates random values in interval(0, 2147483647).
    ///
    /// This method is thread-safe.
    /// @returns: Returns generate random value.
    virtual int Generate()
    {
        unsigned int w1 = _generator.Generate();
        return (int)w1 < 0 ? -(int)w1 : (int)w1;
    }

    /// This method generates random values in interval(0, max).
    ///
    /// This method is thread-safe.
    /// @param max: maximal value which can be generated.
    /// @returns: Returns generate random value.
    virtual int Generate(const int& max) { return (int)(_generator.GenerateDouble() * (max + 1)) % (max + 1); }

    /// This method generates random values in interval(min, max).
    ///
    /// This method is thread-safe.
    /// @param min: minimal value which can be generated.
    /// @param max: maximal value which can be generated.
    /// @returns: Returns generate random value.
    virtual int Generate(const int& min,
        const int& max) {
        return min + Generate(max - min);
    }

};

/// RandomFloat class generates random single  precision floating-point numbers. Targeted architecture must support IEEE 754 standard.
/// The class implements interface. This class has no built-in synchronizator, so LOCK_OBJECT and LOCK_THIS_OBJECT
/// macros cannot be used with instances of this class, but all public methods are thread-safe.
class RandomFloat : public Random<float>
{

private:

    /// Instance of algorithm for generating random numbers.
    RandomGenerator _generator;

public:

    /// This constructor initializes random generator with current time as seed.
    RandomFloat() { }

    /// This constructor initialize random generator with user-defined seed.
    /// @param seed: user-defined seed.
    RandomFloat(unsigned long seed) : _generator(seed) { }

    /// This method generates random values in interval(0, 1).
    ///
    /// This method is thread-safe.
    /// @returns: Returns generate random value.
    virtual float Generate() { return _generator.GenerateFloat(); }

    /// This method generates random values in interval(0, max).
    ///
    /// This method is thread-safe.
    /// @param max: maximal value which can be generated.
    /// @returns: Returns generate random value.
    virtual float Generate(const float& max) { return max * _generator.GenerateFloat(); }

    /// This method generates random values in interval(min, max).
    ///
    /// This method is thread-safe.
    /// @param min: minimal value which can be generated.
    /// @param max: maximal value which can be generated.
    /// @returns: Returns generate random value.
    virtual float Generate(const float& min,
        const float& max) {
        return min + Generate(max - min);
    }

};

/// RandomDouble class generates random double precision floating-point numbers. Class takes care about endianness of the architecture.
/// Targeted architecture must support IEEE 754 standard. The class implements interface. This class has no built-in synchronizator,
/// so LOCK_OBJECT and LOCK_THIS_OBJECT macros cannot be used with instances of this class, but all public methods are thread-safe.
class RandomDouble : public Random<double>
{

private:

    /// Instance of algorithm for generating random numbers.
    RandomGenerator _generator;

public:

    /// This constructor initializes random generator with current time as seed.
    RandomDouble() { }

    /// This constructor initialize random generator with user-defined seed.
    /// @param seed: user-defined seed.
    RandomDouble(unsigned long seed) : _generator(seed) { }

    /// This method generates random values in interval(0, 1).
    ///
    /// This method is thread-safe.
    /// @returns: Returns generate random value.
    virtual double Generate() { return _generator.GenerateDouble(); }

    /// This method generates random values in interval(0, max).
    ///
    /// This method is thread-safe.
    /// @param max: maximal value which can be generated.
    /// @returns: Returns generate random value.
    virtual double Generate(const double& max) { return max * _generator.GenerateDouble(); }

    /// This method generates random values in interval(min, max).
    ///
    /// This method is thread-safe.
    /// @param min">minimal value which can be generated.
    /// @param name="max">maximal value which can be generated.
    /// @returns: Returns generate random value.
    virtual double Generate(const double& min,
        const double& max) {
        return min + Generate(max - min);
    }

};

/// RandomBool class generates random boolean values. It supports generating boolean with defined probabilities of 
/// true and false states. The class implements interface. This class has no built-in synchronizator,
/// so LOCK_OBJECT and LOCK_THIS_OBJECT macros cannot be used with instances of this class, but all public methods are thread-safe.</summary>
class RandomBool : public Random<bool>
{

private:

    /// Instance of algorithm for generating random numbers.
    RandomGenerator _generator;

public:

    /// This constructor initializes random generator with current time as seed.
    RandomBool() { }

    /// This constructor initialize random generator with user-defined seed.
    /// @param seed: user-defined seed.
    RandomBool(unsigned long seed) : _generator(seed) { }

    /// This method generates random Boolean values.
    ///
    /// This method is thread-safe.
    /// @returns: Returns generate random value.
    virtual bool Generate() { return (_generator.Generate() & 1) == 1; }

    /// This method generates random Boolean values.
    ///
    /// This method is thread-safe.
    /// @param max: this parameter is ignored.
    /// @Returns generate random value.
    virtual bool Generate(const bool& max) { return Generate(); }

    /// This method generates random Boolean values.
    ///
    /// This method is thread-safe.
    /// @param min: this parameter is ignored.
    /// @param max: this parameter is ignored.
    /// @returns: Returns generate random value.
    virtual bool Generate(const bool& min,
        const bool& max) {
        return Generate();
    }

    // Generates boolean with p probability of TRUE and 1-p probability of FALSE
    /// This method generates Boolean value with p probability of true value.
    ///
    /// This method is thread safe.
    /// @param p: probability of <c>true</c> value (0, 1).
    /// @returns: Returns generate random value.
    inline bool Generate(double p) { return _generator.GenerateFloat() < p; }

    // Generates boolean with p probability of TRUE and 100-p probability of FALSE. p is expressed in 
    /// This method generates Boolean value with p probability of true and 100-p of false value.
    ///
    /// This method is thread safe.
    /// @param p: probability in percents of true value (0 - 100).
    /// @returns: Returns generate random value.
    inline bool Generate(int p) { return (int)(_generator.GenerateDouble() * 100) < p; }

};

} // namespace Utils

#endif // UTILS_RANDOM_H