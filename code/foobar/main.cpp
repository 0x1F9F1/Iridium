#include "core/app.h"

using namespace Iridium;

class FooBarApplication final : public Application
{
public:
    int Run(int argc, char** argv) override;

    StringView GetName() override;
};

int FooBarApplication::Run(int, char**)
{
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
