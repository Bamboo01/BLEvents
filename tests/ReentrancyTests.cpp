// Subscribing, unsubscribing and clearing from inside callbacks while an event is being invoked.

#include "Event.h"
#include "TestHarness.h"

using namespace BLEvents;
using VoidCallback = EventCallback<void>;

TEST_CASE(CallbackCanUnsubscribeItself)
{
    struct State
    {
        Event<void> event;
        VoidCallback oneShot;
        int calls = 0;
    } state;

    state.oneShot = [&state] { ++state.calls; state.event -= state.oneShot; };
    state.event += state.oneShot;
    state.event.Invoke();
    state.event.Invoke();
    CHECK(state.calls == 1);
    CHECK(state.event.Empty());
}

TEST_CASE(RemovedCallbacksDontRunLaterInTheSameInvoke)
{
    struct State
    {
        Event<void> event;
        VoidCallback victim;
        int victimCalls = 0;
        int lastCalls = 0;
    } state;

    state.victim = [&state] { ++state.victimCalls; };
    state.event += [&state] { state.event -= state.victim; };
    state.event += state.victim;
    state.event += [&state] { ++state.lastCalls; };
    state.event.Invoke();
    CHECK(state.victimCalls == 0);
    CHECK(state.lastCalls == 1);
    CHECK(!state.event.Contains(state.victim));
}

TEST_CASE(CallbacksAddedDuringInvokeRunNextTime)
{
    Event<void> event;
    int added = 0;
    bool subscribed = false;
    event += [&] {
        if (!subscribed)
        {
            subscribed = true;
            for (int i = 0; i < 64; ++i) // enough to force the list to grow
            {
                event += [&added] { ++added; };
            }
        }
    };

    event.Invoke();
    CHECK(added == 0);
    event.Invoke();
    CHECK(added == 64);
}

TEST_CASE(NestedInvokeDoesNotRunCallbacksAddedDuringTheOuterOne)
{
    Event<void, int> event;
    int added = 0;
    event += [&](int depth) {
        if (depth == 0)
        {
            event += [&added](int) { ++added; };
            event.Invoke(1);
        }
    };

    event.Invoke(0);
    CHECK(added == 0);
    event.Invoke(1);
    CHECK(added == 1);
}

TEST_CASE(PendingCallbacksCanBeRemovedAndQueried)
{
    Event<void> event;
    VoidCallback pending = [] {};
    bool containedWhilePending = false;
    bool removedWhilePending = false;
    event += [&] {
        event += pending;
        containedWhilePending = event.Contains(pending);
        event -= pending;
        removedWhilePending = !event.Contains(pending);
    };

    event.Invoke();
    CHECK(containedWhilePending);
    CHECK(removedWhilePending);
    CHECK(!event.Contains(pending));
}

TEST_CASE(ClearDuringInvoke)
{
    Event<void> event;
    int first = 0;
    int second = 0;
    event += [&] { ++first; event.Clear(); };
    event += [&] { ++second; };
    event.Invoke();
    CHECK(first == 1);
    CHECK(second == 0);
    CHECK(event.Empty());
}

TEST_CASE(NestedInvokeRespectsRemovals)
{
    Event<void, int> event;
    EventCallback<void, int> victim;
    int outerCalls = 0;
    int victimCalls = 0;
    victim = [&victimCalls](int) { ++victimCalls; };
    event += [&](int depth) {
        ++outerCalls;
        if (depth == 0)
        {
            event.Invoke(1);    // runs the victim once
            event -= victim;    // so the outer Invoke must skip it
        }
    };
    event += victim;

    event.Invoke(0);
    CHECK(outerCalls == 2);
    CHECK(victimCalls == 1);
}

TEST_CASE(ThrowingCallbackLeavesTheEventUsable)
{
    Event<void, int> event;
    int calls = 0;
    EventCallback<void, int> counter = [&calls](int) { ++calls; };
    event += [](int) { throw 42; };
    event += counter;

    bool threw = false;
    try
    {
        event.Invoke(1);
    }
    catch (int)
    {
        threw = true;
    }
    CHECK(threw);
    CHECK(calls == 0);

    // The event isn't stuck deferring changes
    event -= counter;
    CHECK(!event.Contains(counter));
}

TEST_CASE(CopyMadeDuringInvokeIsUsable)
{
    Event<void> event;
    Event<void> copy;
    VoidCallback doomed = [] {};
    event += [&] { event -= doomed; copy = event; };
    event += doomed;
    event.Invoke();
    CHECK(!copy.Contains(doomed));

    VoidCallback later = [] {};
    copy += later;
    copy -= later;
    CHECK(!copy.Contains(later));
}
