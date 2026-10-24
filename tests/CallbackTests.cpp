// EventCallback: binding each kind of callable, equality, Set and empty callbacks.

#include "Callback.h"
#include "TestHarness.h"

#include <string>
#include <type_traits>

using namespace BLEvents;
using IntCallback = EventCallback<void, int>;

namespace
{
    int freeTotal = 0;
    void AddToTotal(int x) { freeTotal += x; }
    void SubtractFromTotal(int x) { freeTotal -= x; }
    void AddConstRef(const int& x) { freeTotal += x; }
    int Double(int x) { return x * 2; }

    struct Counter
    {
        int total = 0;
        mutable int constCalls = 0;
        void Add(int x) { total += x; }
        void AddTwice(int x) { total += 2 * x; }
        void Peek(int) const { ++constCalls; }
        void AddNoexcept(int x) noexcept { total += x; }
    };

    struct Base
    {
        int baseTotal = 0;
        void AddToBase(int x) { baseTotal += x; }
        virtual void Virtual(int x) { baseTotal += x; }
        virtual ~Base() = default;
    };

    struct Derived : Base
    {
        int derivedTotal = 0;
        void Virtual(int x) override { derivedTotal += x; }
    };
}

TEST_CASE(FreeFunctionsCompareByAddress)
{
    freeTotal = 0;
    IntCallback add(AddToTotal);
    add.Invoke(3);
    CHECK(freeTotal == 3);
    CHECK(add == IntCallback(AddToTotal));
    CHECK(add != IntCallback(SubtractFromTotal));
}

TEST_CASE(CompatibleFreeFunctionsCompareByAddress)
{
    // A function whose signature differs but is callable with the event's arguments
    IntCallback add(AddConstRef);
    CHECK(add == IntCallback(AddConstRef));
    CHECK(add != IntCallback(AddToTotal));
}

TEST_CASE(MemberFunctionsCompareByInstanceAndMethod)
{
    Counter a;
    Counter b;
    IntCallback addA(&a, &Counter::Add);
    addA.Invoke(5);
    CHECK(a.total == 5);
    CHECK(addA == IntCallback(&a, &Counter::Add));
    CHECK(addA != IntCallback(&b, &Counter::Add));
    CHECK(addA != IntCallback(&a, &Counter::AddTwice));
}

TEST_CASE(ConstMemberFunctionsWorkOnConstInstances)
{
    Counter counter;
    const Counter& constCounter = counter;
    IntCallback peek(&constCounter, &Counter::Peek);
    peek.Invoke(0);
    CHECK(counter.constCalls == 1);
    CHECK(peek == IntCallback(&counter, &Counter::Peek));
}

TEST_CASE(NoexceptInheritedAndVirtualMethods)
{
    Counter counter;
    IntCallback(&counter, &Counter::AddNoexcept).Invoke(4);
    CHECK(counter.total == 4);

    Derived derived;
    Base* base = &derived;
    IntCallback(&derived, &Derived::AddToBase).Invoke(2);
    CHECK(derived.baseTotal == 2);

    IntCallback virtualCall(base, &Base::Virtual);
    virtualCall.Invoke(7);
    CHECK(derived.derivedTotal == 7);
    CHECK(virtualCall == IntCallback(base, &Base::Virtual));
}

TEST_CASE(LambdasEqualOnlyCopiesOfTheSameCallback)
{
    int a = 0;
    int b = 0;
    IntCallback addA = [&a](int x) { a += x; };
    IntCallback addB = [&b](int x) { b += x; };
    IntCallback copyOfA = addA;
    CHECK(addA == copyOfA);
    CHECK(addA != addB);

    auto sameLambda = [](int) {};
    CHECK(IntCallback(sameLambda) != IntCallback(sameLambda)); // separately created callbacks differ
}

TEST_CASE(SetReplacesTheTarget)
{
    Counter counter;
    IntCallback callback(&counter, &Counter::Add);

    callback.Set(AddToTotal);
    freeTotal = 0;
    callback.Invoke(3);
    CHECK(freeTotal == 3);
    CHECK(callback == IntCallback(AddToTotal));

    callback.Set(&counter, &Counter::Add);
    callback.Invoke(2);
    CHECK(counter.total == 2);
    CHECK(callback == IntCallback(&counter, &Counter::Add));
    CHECK(callback != IntCallback(AddToTotal));

    callback.Set([](int) {});
    CHECK(callback.IsValid());
    CHECK(callback != IntCallback(&counter, &Counter::Add));

    callback.Clear();
    CHECK(!callback);
    CHECK(callback == IntCallback());
}

TEST_CASE(NullTargetsMakeEmptyCallbacks)
{
    Counter* noCounter = nullptr;
    void (*noFunction)(int) = nullptr;
    CHECK(!IntCallback());
    CHECK(!IntCallback(noCounter, &Counter::Add));
    CHECK(!IntCallback(noFunction));

    bool threw = false;
    try
    {
        IntCallback().Invoke(1);
    }
    catch (const std::bad_function_call&)
    {
        threw = true;
    }
    CHECK(threw);
}

TEST_CASE(ReturnValuesAndReferenceParameters)
{
    EventCallback<int, int> doubler(Double);
    CHECK(doubler.Invoke(21) == 42);

    EventCallback<void, int> discardsResult(Double);
    discardsResult.Invoke(1);

    EventCallback<void, int&> increment = [](int& x) { ++x; };
    int value = 1;
    increment.Invoke(value);
    CHECK(value == 2);
}

TEST_CASE(MutableLambdasAndOwningCaptures)
{
    int last = 0;
    IntCallback count = [calls = 0, &last](int) mutable { last = ++calls; };
    count.Invoke(0);
    count.Invoke(0);
    CHECK(last == 2);

    std::string greeting = "hello";
    std::string received;
    EventCallback<void> greet = [greeting, &received] { received = greeting; };
    greet.Invoke();
    CHECK(received == "hello");
}

TEST_CASE(DeductionGuides)
{
    Counter counter;
    const Counter& constCounter = counter;
    EventCallback fromFunction(AddToTotal);
    EventCallback fromMember(&counter, &Counter::Add);
    EventCallback fromConstMember(&constCounter, &Counter::Peek);
    static_assert(std::is_same_v<decltype(fromFunction), IntCallback>);
    static_assert(std::is_same_v<decltype(fromMember), IntCallback>);
    static_assert(std::is_same_v<decltype(fromConstMember), IntCallback>);
    CHECK(fromMember == IntCallback(&counter, &Counter::Add));
}
