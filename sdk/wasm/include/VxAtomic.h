#ifndef VXATOMIC_H
#define VXATOMIC_H

#include "VxMathDefines.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

/**
 * @brief Atomically loads an integer value.
 * @param value Pointer to the integer to load.
 * @return The current value.
 *
 * @remarks
 * Uses acquire semantics on supported platforms.
 */
inline int VxAtomicLoadInt(volatile int *value) {
#if defined(_WIN32)
    return InterlockedCompareExchange(reinterpret_cast<volatile LONG *>(value), 0, 0);
#elif defined(__GNUC__) || defined(__clang__)
    return __atomic_load_n(value, __ATOMIC_ACQUIRE);
#else
    return *value;
#endif
}

/**
 * @brief Atomically stores an integer value.
 * @param value Pointer to the integer to update.
 * @param next Value to store.
 *
 * @remarks
 * Uses release semantics on supported platforms.
 */
inline void VxAtomicStoreInt(volatile int *value, int next) {
#if defined(_WIN32)
    InterlockedExchange(reinterpret_cast<volatile LONG *>(value), next);
#elif defined(__GNUC__) || defined(__clang__)
    __atomic_store_n(value, next, __ATOMIC_RELEASE);
#else
    *value = next;
#endif
}

/**
 * @brief Atomically increments an integer value.
 * @param value Pointer to the integer to increment.
 * @return The incremented value.
 *
 * @remarks
 * Uses acquire-release semantics on supported platforms.
 */
inline int VxAtomicIncrementInt(volatile int *value) {
#if defined(_WIN32)
    return InterlockedIncrement(reinterpret_cast<volatile LONG *>(value));
#elif defined(__GNUC__) || defined(__clang__)
    return __atomic_add_fetch(value, 1, __ATOMIC_ACQ_REL);
#else
    return ++(*value);
#endif
}

/**
 * @brief Atomically loads a pointer value.
 * @param value Pointer to the pointer to load.
 * @return The current pointer value.
 *
 * @remarks
 * Uses acquire semantics on supported platforms.
 */
inline void *VxAtomicLoadPtr(void *volatile *value) {
#if defined(_WIN32)
    return InterlockedCompareExchangePointer(reinterpret_cast<void *volatile *>(value), nullptr, nullptr);
#elif defined(__GNUC__) || defined(__clang__)
    return __atomic_load_n(value, __ATOMIC_ACQUIRE);
#else
    return *value;
#endif
}

/**
 * @brief Atomically stores a pointer value.
 * @param value Pointer to the pointer to update.
 * @param next Pointer value to store.
 *
 * @remarks
 * Uses release semantics on supported platforms.
 */
inline void VxAtomicStorePtr(void *volatile *value, void *next) {
#if defined(_WIN32)
    InterlockedExchangePointer(reinterpret_cast<void *volatile *>(value), next);
#elif defined(__GNUC__) || defined(__clang__)
    __atomic_store_n(value, next, __ATOMIC_RELEASE);
#else
    *value = next;
#endif
}

#endif // VXATOMIC_H
