#include "assert.h"

namespace Iridium
{
    [[noreturn]] IR_NOINLINE void IrReportAssertion(const IrAssertData& data)
    {
        fmt::print("Assertion Failure: '{}' ({}) in {} ({}:{})", data.message, data.condition, data.function,
            data.filename, data.linenum);

        std::abort();
    }

    [[noreturn]] IR_NOINLINE void IrReportCheck(const IrCheckData& data)
    {
        fmt::print("Check Failure: '{}' in {}:{}", data.message, data.filename, data.linenum);

        std::abort();
    }
} // namespace Iridium
