#pragma once

#include <hedley.h>

#define IR_FORCEINLINE HEDLEY_ALWAYS_INLINE
#define IR_NOINLINE HEDLEY_NEVER_INLINE

#define IR_LIKELY HEDLEY_LIKELY
#define IR_UNLIKELY HEDLEY_UNLIKELY

#define IR_ASSUME HEDLEY_ASSUME

#define IR_CONCAT HEDLEY_CONCAT
#define IR_STRINGIFY HEDLEY_STRINGIFY

#define IR_RESTRICT HEDLEY_RESTRICT

#ifndef IR_FUNCTION
#    if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#        define IR_FUNCTION __func__
#    elif ((__GNUC__ >= 2) || defined(_MSC_VER) || defined(__WATCOMC__))
#        define IR_FUNCTION __FUNCTION__
#    else
#        define IR_FUNCTION "???"
#    endif
#endif

#ifndef IR_FILE
#    define IR_FILE __FILE__
#endif

#ifndef IR_LINE
#    define IR_LINE IR_CONCAT(__LINE__, L) // Workaround Edit and Continue
#endif
