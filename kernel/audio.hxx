#ifndef AUDIO_HXX
#define AUDIO_HXX 1

#include <cstddef>
#include <vector>
#include <algorithm>
#include "vendor.hxx"

#define MAX_HZ 48000

namespace Audio
{
struct Sample
{
    enum Format
    {
        NONE, // No format
        PCM_U8, // 8-bit unsigned PCM
    } fmt = Format::NONE;
    char buffer[MAX_HZ] ALIGN(4096);
    size_t nBuffer = 0;
};

class Driver
{
    static std::vector<Driver*> drivers;
public:
    void *dummy = nullptr;

    Driver() = default;
    Driver(int _base, char inout, int bits);
    Driver(Driver&) = delete;
    Driver(Driver&&) = delete;
    Driver& operator=(Driver&) = delete;
    ~Driver()
    {
        this->RemoveSystem(*this);
    }

    /// @brief Get the default system driver
    static Driver& GetDefault()
    {
        return *drivers.front();
    }

    /// @brief Add driver to global system
    static void AddSystem(Driver& p)
    {
        drivers.push_back(&p);
    }

    /// @brief Remove driver from global system
    static void RemoveSystem(Driver& p)
    {
        drivers.erase(std::remove(std::begin(drivers), std::end(drivers), &p), std::end(drivers));
    }

    virtual void Start() {};
    virtual void Pause() {};
    virtual void Resume() {};
    virtual void Finish() {};
};
}

#endif
