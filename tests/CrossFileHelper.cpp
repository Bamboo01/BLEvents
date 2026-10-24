#include "CrossFileShared.h"

void SubscribeFromAnotherFile(BLEvents::Event<void, int>& event, CrossFileListener& listener)
{
    event += BLEvents::EventCallback(&listener, &CrossFileListener::Add);
    event += AddToCrossFileTotal;
}
