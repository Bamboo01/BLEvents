// Built as its own program, because it replaces the global operator new to count allocations.
// Only checks what the library controls: whether lambdas or member functions allocate when
// they're bound is up to each standard library's std::function.

#include "Event.h"
#include "TestHarness.h"

#include <cstdlib>
#include <new>
#include <utility>

namespace
{
    int allocations = 0;
}

void* operator new(std::size_t size)
{
    ++allocations;
    if (void* memory = std::malloc(size))
    {
        return memory;
    }
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}

using namespace BLEvents;

namespace
{
    int total = 0;
    void AddToTotal(int x) { total += x; }

    struct Counter
    {
        int total = 0;
        void Add(int x) { total += x; }
    };
}

TEST_CASE(InvokeDoesNotAllocate)
{
    Counter counter;
    Event<void, int> event;
    event += AddToTotal;
    event += EventCallback(&counter, &Counter::Add);
    event += [&counter](int x) { counter.total += x; };

    allocations = 0;
    event.Invoke(1);
    event.Invoke(2);
    CHECK(allocations == 0);
}

TEST_CASE(MovingAnEventDoesNotAllocate)
{
    Counter counter;
    Event<void, int> event;
    for (int i = 0; i < 10; ++i)
    {
        event += EventCallback(&counter, &Counter::Add);
    }

    allocations = 0;
    Event<void, int> moved = std::move(event);
    Event<void, int> movedAgain;
    movedAgain = std::move(moved);
    CHECK(allocations == 0);
}
