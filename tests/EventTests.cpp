// Event: subscribing, unsubscribing, querying, copying and invoking.

#include "Event.h"
#include "TestHarness.h"

#include <string>
#include <utility>
#include <vector>

using namespace BLEvents;
using IntCallback = EventCallback<void, int>;

namespace
{
    int firstTotal = 0;
    int secondTotal = 0;
    void AddToFirst(int x) { firstTotal += x; }
    void AddToSecond(int x) { secondTotal += x; }

    struct Counter
    {
        int total = 0;
        void Add(int x) { total += x; }
    };
}

TEST_CASE(InvokesInSubscriptionOrder)
{
    std::vector<int> order;
    Event<void> event;
    event += [&order] { order.push_back(1); };
    event += [&order] { order.push_back(2); };
    event += [&order] { order.push_back(3); };
    event.Invoke();
    CHECK((order == std::vector<int>{ 1, 2, 3 }));
}

TEST_CASE(UnsubscribeRemovesOnlyTheMatchingFunction)
{
    firstTotal = 0;
    secondTotal = 0;
    Event<void, int> event;
    event += AddToFirst;
    event += AddToSecond;
    event -= AddToSecond;
    event.Invoke(1);
    CHECK(firstTotal == 1);
    CHECK(secondTotal == 0);
}

TEST_CASE(UnsubscribeMemberFunction)
{
    Counter a;
    Counter b;
    Event<void, int> event;
    event += EventCallback(&a, &Counter::Add);
    event += EventCallback(&b, &Counter::Add);
    event -= EventCallback(&a, &Counter::Add);
    event.Invoke(1);
    CHECK(a.total == 0);
    CHECK(b.total == 1);
}

TEST_CASE(UnsubscribeLambdaWithItsCallback)
{
    int a = 0;
    int b = 0;
    IntCallback addA = [&a](int x) { a += x; };
    IntCallback addB = [&b](int x) { b += x; };
    Event<void, int> event;
    event += addA;
    event += addB;
    event -= addB;
    event.Invoke(1);
    CHECK(a == 1);
    CHECK(b == 0);
}

TEST_CASE(RewritingALambdaUnsubscribesNothing)
{
    int calls = 0;
    Event<void, int> event;
    event += [&calls](int) { ++calls; };
    event -= [&calls](int) { ++calls; }; // a new lambda is a new callback
    event.Invoke(0);
    CHECK(calls == 1);
    CHECK(!event.Empty());
}

TEST_CASE(DuplicatesAreRemovedOneAtATime)
{
    firstTotal = 0;
    Event<void, int> event;
    event += AddToFirst;
    event += AddToFirst;
    event.Invoke(1);
    CHECK(firstTotal == 2);
    event -= AddToFirst;
    event.Invoke(1);
    CHECK(firstTotal == 3);
    CHECK(event.Contains(AddToFirst));
}

TEST_CASE(UnsubscribingAMissingCallbackDoesNothing)
{
    Event<void, int> event;
    event += AddToFirst;
    event -= AddToSecond;
    event -= IntCallback();
    CHECK(event.Contains(AddToFirst));
}

TEST_CASE(ContainsAndEmpty)
{
    Counter counter;
    Event<void, int> event;
    CHECK(event.Empty());
    event += EventCallback(&counter, &Counter::Add);
    CHECK(!event.Empty());
    CHECK(event.Contains(EventCallback(&counter, &Counter::Add)));
    CHECK(!event.Contains(AddToFirst));
}

TEST_CASE(ClearRemovesEverything)
{
    Event<void, int> event;
    event += AddToFirst;
    event += [](int) {};
    event.Clear();
    CHECK(event.Empty());
}

TEST_CASE(EmptyCallbacksAreIgnored)
{
    Counter* noCounter = nullptr;
    Event<void, int> event;
    event += IntCallback();
    event += IntCallback(noCounter, &Counter::Add);
    CHECK(event.Empty());
    event.Invoke(1); // nothing to call, nothing thrown
}

TEST_CASE(EveryListenerGetsTheArguments)
{
    std::vector<std::string> received;
    Event<void, std::string, int> event;
    event += [&received](std::string name, int) { received.push_back(name); };
    event += [&received](const std::string& name, int count) { received.push_back(name + std::to_string(count)); };
    event.Invoke("hit", 2);
    CHECK((received == std::vector<std::string>{ "hit", "hit2" }));
}

TEST_CASE(CopyingAndMovingEvents)
{
    firstTotal = 0;
    Event<void, int> original;
    original += AddToFirst;

    Event<void, int> copy = original;
    copy.Invoke(1);
    CHECK(firstTotal == 1);
    CHECK(original.Contains(AddToFirst));

    Event<void, int> moved = std::move(copy);
    moved.Invoke(1);
    CHECK(firstTotal == 2);

    Event<void, int> assigned;
    assigned = original;
    CHECK(assigned.Contains(AddToFirst));
}
