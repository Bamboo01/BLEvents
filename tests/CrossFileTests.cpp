// Callbacks created in one source file must compare equal to the same callbacks created in another.

#include "CrossFileShared.h"
#include "TestHarness.h"

using namespace BLEvents;

TEST_CASE(UnsubscribeCallbacksCreatedInAnotherFile)
{
    Event<void, int> event;
    CrossFileListener listener;
    SubscribeFromAnotherFile(event, listener);

    crossFileTotal = 0;
    event.Invoke(1);
    CHECK(listener.total == 1);
    CHECK(crossFileTotal == 1);

    CHECK(event.Contains(EventCallback(&listener, &CrossFileListener::Add)));
    CHECK(event.Contains(AddToCrossFileTotal));
    event -= EventCallback(&listener, &CrossFileListener::Add);
    event -= AddToCrossFileTotal;
    CHECK(event.Empty());
}
