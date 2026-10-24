#ifndef BLEVENTS_CROSS_FILE_SHARED_H
#define BLEVENTS_CROSS_FILE_SHARED_H

// Shared by CrossFileTests.cpp and CrossFileHelper.cpp, so callbacks are created in one
// source file and compared in another.

#include "Event.h"

struct CrossFileListener
{
    int total = 0;
    void Add(int x) { total += x; }
};

inline int crossFileTotal = 0;
inline void AddToCrossFileTotal(int x) { crossFileTotal += x; }

// Defined in CrossFileHelper.cpp
void SubscribeFromAnotherFile(BLEvents::Event<void, int>& event, CrossFileListener& listener);

#endif //BLEVENTS_CROSS_FILE_SHARED_H
