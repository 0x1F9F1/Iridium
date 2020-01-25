#pragma once

/* Don't include intrin.h here because it contains C++ code */
extern void __cdecl __debugbreak(void);

#define IrDebugBreak() __debugbreak()

namespace Iridium
{
    struct IrAssertData
    {
        const char* condition;
        const char* message;
        const char* filename;
        const char* function;
        unsigned int linenum;
    };

    [[noreturn]] void IrReportAssertion(const IrAssertData* data);
} // namespace Iridium

#ifndef IRIDIUM_FUNCTION
#    if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#        define IRIDIUM_FUNCTION __func__
#    elif ((__GNUC__ >= 2) || defined(_MSC_VER) || defined(__WATCOMC__))
#        define IRIDIUM_FUNCTION __FUNCTION__
#    else
#        define IRIDIUM_FUNCTION "???"
#    endif
#endif

#ifndef IRIDIUM_FILE
#    define IRIDIUM_FILE __FILE__
#endif

#ifndef IRIDIUM_LINE
#    define IRIDIUM_LINE __LINE__
#endif

#define IrEnabledAssert(CONDITION, MESSAGE)                                                                    \
    do                                                                                                         \
    {                                                                                                          \
        if (IR_UNLIKELY(!(CONDITION)))                                                                         \
        {                                                                                                      \
            static const ::Iridium::IrAssertData ir_assert_data {                                              \
                #CONDITION, MESSAGE, IRIDIUM_FILE, IRIDIUM_FUNCTION, static_cast<unsigned int>(IRIDIUM_LINE)}; \
            ::Iridium::IrReportAssertion(&ir_assert_data);                                                     \
        }                                                                                                      \
    } while (false);

#define IrDisabledAssert(CONDITION, MESSAGE) static_cast<void>(sizeof(!(CONDITION)));

#ifdef IRIDIUM_DEBUG
#    define IrDebugAssert IrEnabledAssert
#else
#    define IrDebugAssert IrDisabledAssert
#endif

#define IrAssert IrEnabledAssert
