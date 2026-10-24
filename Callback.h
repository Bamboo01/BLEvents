#ifndef BLEVENT_CALLBACK_H
#define BLEVENT_CALLBACK_H


/******************************************************************************/
/*!
\file   Callback.h
\brief
Defines a type-safe event callback system.
This file provides a flexible event handling system that supports binding free functions,
member functions, and lambda expressions with proper type safety. The system allows
for efficient callback management with equality comparison and safe invocation.
The Callback class serves as the core of this system, implementing delegate
pattern functionality with support for various callable types.
*/
/******************************************************************************/

#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <type_traits>
#include <utility>

namespace BLEvents
{
    /**
     * @class Callback
     * @brief A type-safe callback system for event handling
     *
     * @tparam FunctionType The return type of the callback function
     * @tparam params Parameter pack for callback function arguments
     *
     * @details The Callback class provides a unified interface for storing and invoking
     * different types of callables (free functions, member functions, lambdas) with proper
     * type safety. It implements the delegate pattern, allowing for safe invocation and equality
     * comparison between callbacks to support event subscription and unsubscription.
     * Free functions are equal when they are the same function, member functions when they are
     * the same method on the same instance, and lambdas only to copies of the same callback.
     */
    template<typename FunctionType, typename... params>
    class Callback
    {
    private:
        /** @brief Type alias for member function pointers */
        template <typename ClassType>
        using MemberFunctionPointer = FunctionType(ClassType::*)(params...);

        /** @brief Type alias for const member function pointers */
        template <typename ClassType>
        using ConstMemberFunctionPointer = FunctionType(ClassType::*)(params...) const;

        /** @brief Compares two keys of the same type */
        using KeyComparer = bool(*)(const unsigned char*, const unsigned char*);

        /**
         * @brief Compares two keys of type Key with Key's own ==
         * @details Copying the bytes back into real Key objects keeps this well-defined;
         *          comparing the raw bytes directly is not guaranteed to work for member function pointers.
         */
        template <typename Key>
        static bool CompareKeys(const unsigned char* lhs, const unsigned char* rhs)
        {
            Key a{};
            Key b{};
            std::memcpy(&a, lhs, sizeof(Key));
            std::memcpy(&b, rhs, sizeof(Key));
            return a == b;
        }

        /** @brief Hands out a unique id to every lambda callback */
        static std::uint64_t NextLambdaId()
        {
            static std::atomic<std::uint64_t> lastId{ 0 };
            return ++lastId;
        }

        /**
         * @brief Records what this callback calls, for equality comparisons
         * @param object The bound instance, or null
         * @param value A function pointer, member function pointer or lambda id
         */
        template <typename Key>
        void SetKey(const void* object, Key value)
        {
            static_assert(sizeof(Key) <= sizeof(funcptr), "Function pointer is too large for Callback");
            objectptr = object;
            std::memcpy(funcptr, &value, sizeof(Key));
            compareKeys = &CompareKeys<Key>;
        }

        /** @brief Shared implementation of the member function Set overloads */
        template <typename ClassType, typename MemberPointer>
        void SetMember(ClassType* instance, MemberPointer memberFunction)
        {
            if (instance == nullptr || memberFunction == nullptr)
            {
                Clear();
                return;
            }

            func = [instance, memberFunction](params... args) -> FunctionType
            {
                return (instance->*memberFunction)(std::forward<params>(args)...);
            };
            SetKey(instance, memberFunction);
        }

    protected:
        /** @brief Function object storing the callable */
        std::function<FunctionType(params...)> func;

        /** @brief The bound instance for member functions; null for free functions and lambdas */
        const void* objectptr = nullptr;

        /** @brief Bytes of the function pointer, member function pointer or lambda id, for comparisons */
        unsigned char funcptr[sizeof(void*) * 4]{};

        /** @brief Compares funcptr using its real type; null when the callback is empty */
        KeyComparer compareKeys = nullptr;

    public:
        /**
         * @brief Default constructor
         * @details Creates an empty callback
         */
        Callback() = default;

        /**
         * @brief Constructor for free functions
         * @param function Pointer to a free function
         * @details Creates a callback that will invoke the provided free function
         */
        Callback(FunctionType(*function)(params...))
        {
            Set(function);
        }

        /**
         * @brief Constructor for member functions
         * @tparam ClassType Type of class instance
         * @tparam MemberClass Class declaring the member function; ClassType or one of its bases
         * @param instance Pointer to the class instance
         * @param memberFunction Pointer to the member function
         * @details Creates a callback that will invoke the member function on the given instance
         */
        template <typename ClassType, typename MemberClass>
        Callback(ClassType* instance, MemberFunctionPointer<MemberClass> memberFunction)
        {
            Set(instance, memberFunction);
        }

        /**
         * @brief Constructor for const member functions
         * @tparam ClassType Type of class instance
         * @tparam MemberClass Class declaring the member function; ClassType or one of its bases
         * @param instance Pointer to the class instance, which may be const
         * @param memberFunction Pointer to the const member function
         * @details Creates a callback that will invoke the const member function on the given instance
         */
        template <typename ClassType, typename MemberClass>
        Callback(const ClassType* instance, ConstMemberFunctionPointer<MemberClass> memberFunction)
        {
            Set(instance, memberFunction);
        }

        /**
         * @brief Constructor for callable objects (like lambdas)
         * @tparam Callable Type of the callable object
         * @param callable The callable object
         * @details Creates a callback that will invoke the provided callable object
         */
        template <typename Callable,
            typename = std::enable_if_t<std::is_invocable_r_v<FunctionType, Callable&, params...>>>
        Callback(Callable&& callable)
        {
            Set(std::forward<Callable>(callable));
        }

        /**
         * @brief Helper function to clear all internal state of an Callback
         */
        void Clear()
        {
            *this = Callback();
        }

        /**
         * @brief Invokes the callback
         * @param a The arguments to pass to the callback
         * @return Whatever the callback returns
         * @throws std::bad_function_call if the callback is empty
         */
        FunctionType Invoke(params... a) const
        {
            return func(std::forward<params>(a)...);
        }

        /**
         * @brief Sets the callback to a member function
         * @tparam ClassType Type of class instance
         * @tparam MemberClass Class declaring the member function; ClassType or one of its bases
         * @param a Pointer to the class instance
         * @param b Pointer to the member function
         * @details Configures the callback to invoke the specified member function on the given instance.
         *          A null instance or member function leaves the callback empty.
         */
        template <typename ClassType, typename MemberClass>
        void Set(ClassType* a, MemberFunctionPointer<MemberClass> b)
        {
            SetMember(a, b);
        }

        /**
         * @brief Sets the callback to a const member function
         * @tparam ClassType Type of class instance
         * @tparam MemberClass Class declaring the member function; ClassType or one of its bases
         * @param a Pointer to the class instance, which may be const
         * @param b Pointer to the const member function
         * @details Configures the callback to invoke the specified const member function on the given instance.
         *          A null instance or member function leaves the callback empty.
         */
        template <typename ClassType, typename MemberClass>
        void Set(const ClassType* a, ConstMemberFunctionPointer<MemberClass> b)
        {
            SetMember(a, b);
        }

        /**
         * @brief Sets the callback to a free function or other callable object (like a lambda)
         * @tparam Callable Type of the callable object
         * @param callable The free function or callable object
         * @details Free functions compare equal by address. Any other callable gets a unique id,
         *          so it only compares equal to copies of this callback.
         */
        template <typename Callable,
            typename = std::enable_if_t<std::is_invocable_r_v<FunctionType, Callable&, params...>>>
        void Set(Callable&& callable)
        {
            using Stored = std::decay_t<Callable>;
            if constexpr (std::is_pointer_v<Stored>)
            {
                SetKey(nullptr, static_cast<Stored>(callable));
            }
            else
            {
                SetKey(nullptr, NextLambdaId());
            }

            func = std::forward<Callable>(callable);
            if (!func)
            {
                Clear();
            }
        }

        /**
         * @brief Equality comparison operator
         * @param rhs The right-hand side Callback to compare with
         * @return True if both callbacks call the same thing, or both are empty
         */
        [[nodiscard]] bool operator==(const Callback& rhs) const
        {
            if (compareKeys == nullptr || rhs.compareKeys == nullptr)
            {
                return compareKeys == rhs.compareKeys;
            }
            return compareKeys == rhs.compareKeys && objectptr == rhs.objectptr &&
                compareKeys(funcptr, rhs.funcptr);
        }

        /**
         * @brief Inequality comparison operator
         * @param rhs The right-hand side Callback to compare with
         * @return The inverse of operator==
         */
        [[nodiscard]] bool operator!=(const Callback& rhs) const
        {
            return !(*this == rhs);
        }

        /**
         * @brief Boolean conversion operator
         * @return True if the callback is valid (has a non-null function), false otherwise
         */
        explicit operator bool() const noexcept
        {
            return static_cast<bool>(func);
        }

        /**
         * @brief Checks if this callback is valid (contains a callable function)
         * @return True if the callback is valid, false otherwise
         */
        [[nodiscard]] bool IsValid() const noexcept
        {
            return static_cast<bool>(func);
        }
    };

    // Deduction guide for member functions
    template<typename ClassType, typename MemberClass, typename FunctionType, typename... params>
    Callback(ClassType*, FunctionType(MemberClass::*)(params...)) -> Callback<FunctionType, params...>;

    // Deduction guide for const member functions
    template<typename ClassType, typename MemberClass, typename FunctionType, typename... params>
    Callback(const ClassType*, FunctionType(MemberClass::*)(params...) const) -> Callback<FunctionType, params...>;
}


#endif //BLEVENT_CALLBACK_H
