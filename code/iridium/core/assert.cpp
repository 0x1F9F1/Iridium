#include "assert.h"

namespace Iridium
{
    [[noreturn]] void IrReportAssertion(const IrAssertData* data)
    {
        fmt::print("Assertion Failure: {} ({}) in {} ({}:{})", data->message, data->condition, data->function,
            data->filename, data->linenum);

        std::abort();
    }
} // namespace Iridium
