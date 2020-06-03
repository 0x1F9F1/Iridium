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

    [[noreturn]] void IrReportAssertion(const IrAssertData& data);

    struct IrCheckData
    {
        const char* message;
        const char* filename;
        unsigned int linenum;
    };

    [[noreturn]] void IrReportCheck(const IrCheckData& data);
} // namespace Iridium

#define IrEnabledAssert(CONDITION, MESSAGE)                                                     \
    do                                                                                          \
    {                                                                                           \
        if (IR_UNLIKELY(!(CONDITION)))                                                          \
        {                                                                                       \
            static const ::Iridium::IrAssertData ir_assert_data {                               \
                #CONDITION, MESSAGE, IR_FILE, IR_FUNCTION, static_cast<unsigned int>(IR_LINE)}; \
            ::Iridium::IrReportAssertion(ir_assert_data);                                       \
        }                                                                                       \
    } while (false)

#define IrEnabledCheck(CONDITION, MESSAGE)               \
    (IR_LIKELY((CONDITION)) ? void()                     \
                            : (::Iridium::IrReportCheck( \
                                  ::Iridium::IrCheckData {MESSAGE, IR_FILE, static_cast<unsigned int>(IR_LINE)})))

#define IrDisabledAssert(CONDITION, MESSAGE) void(sizeof(!(CONDITION)))

#define IrDisabledCheck(CONDITION, MESSAGE) void(sizeof(!(CONDITION)))

#ifdef IRIDIUM_DEBUG
#    define IrDebugAssert IrEnabledAssert
#    define IrDebugCheck IrEnabledCheck
#else
#    define IrDebugAssert IrDisabledAssert
#    define IrDebugCheck IrDisabledCheck
#endif

#define IrAssert IrEnabledAssert
#define IrCheck IrEnabledCheck