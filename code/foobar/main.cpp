#include "core/app.h"

#include "core/meta/metadefine.h"

using namespace Iridium;

class FooBarApplication final : public Application
{
public:
    int Run(int argc, char** argv) override;

    StringView GetName() override;
};

#include <chrono>

class ProfilingTimer
{
public:
    ProfilingTimer(StringView name);
    ~ProfilingTimer();

    void Stop();

private:
    using Clock = std::chrono::high_resolution_clock;
    using Timepoint = Clock::time_point;

    String name_ {};
    Timepoint start_ {};
    bool done_ {false};
};

ProfilingTimer::ProfilingTimer(StringView name)
    : name_(name)
    , start_(Clock::now())
{}

ProfilingTimer::~ProfilingTimer()
{
    if (!done_)
    {
        Stop();
    }
}

void ProfilingTimer::Stop()
{
    Timepoint end = Clock::now();

    fmt::print("{} took {} us\n", name_, std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count());

    done_ = true;
}

#include "asset/local.h"
#include "asset/stream.h"

#include "asset/device/zip.h"

#include "asset/glob.h"

int FooBarApplication::Run(int, char**)
{
    Rc<FileDevice> archive;

    if (Rc<Stream> input = LocalFiles()->Open("C:/Users/Sam/Downloads/boost_1_71_0.zip", true))
    {
        for (usize i = 0; i < 10; ++i)
        {
            ProfilingTimer _("Load");

            archive = nullptr;

            archive = MakeRc<ZipArchive>(input);
        }
    }

    if (false)
    {
        ProfilingTimer _("Glob");

        if (auto files = Glob(archive, "", "**"))
        {
            for (const auto& file : *files)
            {
                fmt::print("{}\n", file.Name);
            }
        }
    }

    return 0;
}

StringView FooBarApplication::GetName()
{
    return "FooBar";
}

Ptr<Application> Iridium::CreateApplication()
{
    return MakeUnique<FooBarApplication>();
}
