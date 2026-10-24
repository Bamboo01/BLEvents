# Bamboo's Lightweight Events

A small header-only C++17 library for C#-style events

Example Use Case
---
```cpp
#include "Event.h"

using namespace BLEvents;

struct Player
{
    Event<void, int> onDamaged;
};

class HealthBar
{
public:
    explicit HealthBar(Player& player) : player(player)
    {
        player.onDamaged += EventCallback(this, &HealthBar::OnDamaged);
    }

    ~HealthBar()
    {
        player.onDamaged -= EventCallback(this, &HealthBar::OnDamaged);
    }

    void OnDamaged(int amount) { shown -= amount; }

    int shown = 100;

private:
    Player& player;
};

int main()
{
    Player player;
    HealthBar bar(player);
    player.onDamaged.Invoke(25); // bar.shown is now 75
}
```

## Features

- A type-friendly `Callback` type for free functions, member functions (including const, `noexcept` and inherited ones) and lambdas
- C# flavoured syntax: 
  - Subscribe by value : '+=' 
  - Unsubscribe by matching value: `-=`

## Getting started

Simply add `Callback.h` and `Event.h` into your project, or add this folder to your include path!

## Usage

An `Event<Return, Params...>` holds callbacks with a signature:

```cpp
Event<void, int> onDamaged;

onDamaged += LogDamage;                                      // free function
onDamaged += EventCallback(this, &Player::OnDamaged);        // member function
onDamaged += EventCallback(&constPlayer, &Player::Describe); // const member function on a const object
onDamaged += [](int amount) { /* ... */ };                   // lambda

onDamaged.Invoke(25);
```

### Unsubscribing

`-=` removes the first registered callback that is equal to the one you pass:

| Callback | Equal when |
| --- | --- |
| Free function | it's the same function |
| Member function | it's the same method on the same instance |
| Lambda or other callable | it's a copy of the same `EventCallback` |

**To remove a lambda later, keep the callback you subscribed with:**

```cpp
EventCallback<void, int> onHit = [this](int amount) { hp -= amount; };
onDamaged += onHit;
// ...
onDamaged -= onHit;
```

Writing the lambda out again doesn't work, because every lambda is a new callback. `onDamaged -= [this](int amount) { hp -= amount; };` removes nothing.

### Other operations

| `Event` | |
| --- | --- |
| `Contains(callback)` | whether an equal callback is registered |
| `Empty()` | whether no callbacks are registered |
| `Clear()` | removes every callback |

| `EventCallback` | |
| --- | --- |
| `Invoke(args...)` | calls the callback; throws `std::bad_function_call` if it's empty |
| `Set(...)` | points the callback at something else, taking the same arguments as the constructors |
| `Clear()` | empties the callback |
| `IsValid()`, `explicit operator bool` | whether the callback holds something to call |

A callback bound to a null instance or a null function pointer is empty, and `+=` ignores empty callbacks.

### Changing an event while it's being invoked

Callbacks may call `+=`, `-=` and `Clear()` on the event that is invoking them:

- A callback removed during `Invoke` doesn't run if it hasn't run yet.
- A callback added during `Invoke` is added once the outermost `Invoke` returns, so it first runs on the next call. A nested `Invoke` from inside a callback won't run it either.

## Performance

- Callbacks are stored in `std::function`. Whether a callback allocates when it's created depends on your standard library. With libstdc++, a lambda capturing one or two pointers doesn't allocate, and a member function callback allocates once.
- `Invoke` doesn't copy or allocate callbacks. Arguments are passed by value to each callback, so use a reference parameter type for large payloads, for example `Event<void, const DamageInfo&>`.
- Moving an `Event` doesn't allocate.

## Limitations

- **Not thread-safe.** Don't use one event from several threads at once.
- **No lifetime tracking.** Unsubscribe member functions before their object is destroyed, for example in the destructor as in the example above.
- **Don't destroy, move or assign to an event from inside one of its own callbacks.**
- **Virtual member functions:** the C++ standard leaves comparing pointers to virtual member functions unspecified. The tests cover it with GCC.
- **DLLs and shared libraries:** callbacks created in different modules may not compare equal.

## Building the tests

The tests need GNU Make and a C++17 compiler. On Windows, run `make` from Git Bash or MSYS2.

```sh
make test               # build and run the tests (C++17)
make test STD=c++20     # use another language standard
make test-all           # C++17, C++20 and C++23
make test CXX=clang++   # use another compiler
make clean
```

Tested with GCC 15.2 (MinGW-w64) on Windows, as C++17, C++20 and C++23.

# AI Usage Disclaimer
Test cases, makefile and doxygen comments were written using Opus 5.5. Readme examples and usage cases also generated with Opus 5.5. Events used to have an issue with mid-invoke deletion and was resolved using Opus 5.5.
Most of the code was written since my polytechnic days over the years!