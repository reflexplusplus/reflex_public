#pragma once




//
//std includes

#include <type_traits>
#include <functional>
#include <cmath>
#include <memory>
#include <atomic>
#include <source_location>




//
//atomic intrinsics

#define REFLEX_ATOMIC_READ(var) var.load(std::memory_order_acquire)

#define REFLEX_ATOMIC_READ_UNORDERED(var) var.load(std::memory_order_relaxed)

#define REFLEX_ATOMIC_WRITE(var, value) var.store(value, std::memory_order_release)

#define REFLEX_ATOMIC_WRITE_UNORDERED(var, value) var.store(value, std::memory_order_relaxed)

#define REFLEX_ATOMIC_INC(var, n) std::atomic_fetch_add_explicit(&var, n, std::memory_order_relaxed)

#define REFLEX_ATOMIC_DEC(var, n) std::atomic_fetch_sub_explicit(&var, n, std::memory_order_acq_rel)

#define REFLEX_ATOMIC_OR(var, flags) var.fetch_or(flags, std::memory_order_release)

#define REFLEX_ATOMIC_POLL(var) var.exchange(0, std::memory_order_acquire)

#define REFLEX_ATOMIC_SET_FILTERED(var, value) (var.exchange(value, std::memory_order_acq_rel) != value)

//#define REFLEX_ATOMIC_NOTIFY(var) var.notify_all()

#define REFLEX_ATOMIC_WAIT(var, expected_value) var.wait(expected_value, std::memory_order_acquire)
