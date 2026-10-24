#ifndef BLEVENT_EVENT_H
#define BLEVENT_EVENT_H


/******************************************************************************/
/*!
\file   Event.h
\brief
Defines the Event class for the event system.
This file provides the Event class template that manages collections of Callbacks
and allows for broadcasting events to multiple listeners. It implements a publisher-subscriber
pattern where objects can register and unregister callbacks, and events can be triggered
to invoke all registered callbacks with the appropriate parameters.
*/
/******************************************************************************/

#include "Callback.h"
#include <cstddef>
#include <utility>
#include <vector>

namespace BLEvents
{
    /**
     * @class Event
     * @brief Manages a collection of event callbacks and handles event broadcasting
     *
     * @tparam FunctionType The return type of the callback functions
     * @tparam params Parameter pack for function arguments
     *
     * @details The Event class implements a publisher-subscriber pattern for event handling.
     * It maintains a list of callbacks and provides methods to register, unregister, and
     * invoke these callbacks. When an event is triggered via Invoke(), all registered
     * callbacks are executed with the provided parameters.
     *
     * Callbacks may subscribe and unsubscribe (including themselves) while the event is being
     * invoked: callbacks added during Invoke() first run on the next Invoke(), and callbacks
     * removed during Invoke() don't run if they haven't run yet. Destroying, moving or assigning
     * to an event from inside one of its own callbacks is not supported.
     */
    template<typename FunctionType, typename... params>
    class Event
    {
    private:
        /** @brief A registered callback */
        struct Delegate
        {
            /** @brief The callback to invoke */
            Callback<FunctionType, params...> callback;

            /** @brief Set when unsubscribed during Invoke(); erased once the outermost Invoke() returns */
            bool removed = false;
        };

        /** @brief Applies changes made during Invoke() once the outermost Invoke() returns, even if a callback throws */
        struct InvokeScope
        {
            explicit InvokeScope(Event& event) : event(event) { ++event.invokeDepth; }
            ~InvokeScope() { if (--event.invokeDepth == 0) event.ApplyPendingChanges(); }

            Event& event;
        };

        /** @brief List of registered callback delegates */
        std::vector<Delegate> delegateList;

        /** @brief Callbacks subscribed during Invoke(), added to delegateList once it returns */
        std::vector<Callback<FunctionType, params...>> pendingList;

        /** @brief Number of Invoke() calls currently running on this event */
        std::size_t invokeDepth = 0;

        /** @brief Whether delegateList holds delegates removed during Invoke() */
        bool hasRemovals = false;

        /**
         * @brief Erases delegates removed during Invoke() and adds callbacks subscribed during it
         */
        void ApplyPendingChanges()
        {
            if (hasRemovals)
            {
                std::size_t kept = 0;
                for (std::size_t i = 0; i < delegateList.size(); ++i)
                {
                    if (!delegateList[i].removed)
                    {
                        if (kept != i)
                        {
                            delegateList[kept] = std::move(delegateList[i]);
                        }
                        ++kept;
                    }
                }
                delegateList.erase(delegateList.begin() + static_cast<std::ptrdiff_t>(kept), delegateList.end());
                hasRemovals = false;
            }

            for (auto& callback : pendingList)
            {
                delegateList.push_back({ std::move(callback) });
            }
            pendingList.clear();
        }

    public:
        /**
         * @brief Default constructor
         * @details Creates an empty event with no registered callbacks
         */
        Event() = default;

        /**
         * @brief Default destructor
         */
        ~Event() = default;

        /**
         * @brief Copy constructor
         * @details A copy made during Invoke() starts out with that Invoke()'s changes already applied
         */
        Event(const Event& other)
            : delegateList(other.delegateList), pendingList(other.pendingList), hasRemovals(other.hasRemovals)
        {
            ApplyPendingChanges();
        }

        /**
         * @brief Copy assignment operator
         */
        Event& operator=(const Event& other)
        {
            if (this != &other)
            {
                *this = Event(other);
            }
            return *this;
        }

        /**
         * @brief Default move constructor
         */
        Event(Event&&) noexcept = default;

        /**
         * @brief Default move assignment operator
         */
        Event& operator=(Event&&) noexcept = default;

        /**
         * @brief Adds a callback to the event
         * @param a The callback to register
         * @details Registers a new callback to be invoked when the event is triggered.
         *          Empty callbacks are ignored.
         */
        void operator+=(Callback<FunctionType, params...> a)
        {
            if (!a)
            {
                return;
            }

            if (invokeDepth > 0)
            {
                pendingList.push_back(std::move(a));
            }
            else
            {
                delegateList.push_back({ std::move(a) });
            }
        }

        /**
         * @brief Removes a callback from the event
         * @param a The callback to unregister
         * @details Finds and removes the first callback equal to a.
         *          Does nothing if the callback is not found.
         */
        void operator-=(const Callback<FunctionType, params...>& a)
        {
            for (std::size_t i = 0; i < delegateList.size(); ++i)
            {
                if (!delegateList[i].removed && delegateList[i].callback == a)
                {
                    if (invokeDepth > 0)
                    {
                        // The running Invoke() may be iterating over this delegate, so erase it later
                        delegateList[i].removed = true;
                        hasRemovals = true;
                    }
                    else
                    {
                        delegateList.erase(delegateList.begin() + static_cast<std::ptrdiff_t>(i));
                    }
                    return;
                }
            }

            for (auto it = pendingList.begin(); it != pendingList.end(); ++it)
            {
                if (*it == a)
                {
                    pendingList.erase(it);
                    return;
                }
            }
        }

        /**
         * @brief Removes all registered callbacks
         * @details Clears the list of callbacks, effectively unregistering all listeners
         */
        void Clear()
        {
            pendingList.clear();
            if (invokeDepth == 0)
            {
                delegateList.clear();
                return;
            }

            for (Delegate& delegate : delegateList)
            {
                delegate.removed = true;
            }
            hasRemovals = true;
        }

        /**
         * @brief Checks if a callback is registered with this event
         * @param a The callback to check
         * @return True if the callback is registered, false otherwise
         * @details Searches the delegate list for the specified callback
         */
        [[nodiscard]] bool Contains(const Callback<FunctionType, params...>& a) const
        {
            for (const Delegate& delegate : delegateList)
            {
                if (!delegate.removed && delegate.callback == a)
                {
                    return true;
                }
            }

            for (const auto& callback : pendingList)
            {
                if (callback == a)
                {
                    return true;
                }
            }
            return false;
        }

        /**
         * @brief Checks if the event has no registered callbacks
         * @return True if no callbacks are registered, false otherwise
         */
        [[nodiscard]] bool Empty() const
        {
            if (!pendingList.empty())
            {
                return false;
            }

            for (const Delegate& delegate : delegateList)
            {
                if (!delegate.removed)
                {
                    return false;
                }
            }
            return true;
        }

        /**
         * @brief Triggers the event, invoking all registered callbacks
         * @param a The arguments to pass to each callback
         * @details Iterates through all registered callbacks and invokes each one
         *          with the provided arguments
         */
        void Invoke(params... a)
        {
            // Changes made by callbacks are deferred, so delegateList can't reallocate while we loop
            InvokeScope scope(*this);
            for (Delegate& delegate : delegateList)
            {
                if (!delegate.removed)
                {
                    delegate.callback.Invoke(a...);
                }
            }
        }
    };
}

#endif //BLEVENT_EVENT_H
