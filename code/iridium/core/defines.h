#pragma once

#if defined(__GNUC__) || defined(__clang__)
#    define IR_FORCEINLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#    define IR_FORCEINLINE __pragma(warning(suppress : 4714)) inline __forceinline
#else
#    define IR_FORCEINLINE inline
#endif

#if defined(__GNUC__) || defined(__clang__)
#    define IR_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#    define IR_NOINLINE __declspec(noinline)
#else
#    define IR_NOINLINE
#endif

#if defined(__GNUC__) || defined(__clang__)
#    define IR_LIKELY(x) __builtin_expect(static_cast<bool>(x), 1)
#    define IR_UNLIKELY(x) __builtin_expect(static_cast<bool>(x), 0)
#else
#    define IR_LIKELY(x) static_cast<bool>(x)
#    define IR_UNLIKELY(x) static_cast<bool>(x)
#endif
