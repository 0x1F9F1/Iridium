#include "core/app.h"

#include "core/meta/metadefine.h"

using namespace Iridium;

class FooBarApplication final : public Application
{
public:
    int Run(int argc, char** argv) override;

    StringView GetName() override;
};

struct FooBar : AtomicRefCounted
{
    VIRTUAL_META_DECLARE;

    i32 foo = 0;
    i32 bar = 1;
    struct Frob* frob = nullptr;
};

VIRTUAL_META_DEFINE_CHILD("FooBar", FooBar, AtomicRefCounted)
{
    META_FIELD("foo", foo);
    META_FIELD("bar", bar);
    META_FIELD("frob", frob);
}

struct Frobber
{
    int frobber;

    META_DECLARE;
};

META_DEFINE("Frobber", Frobber)
{}

struct Frob
    : AtomicRefCounted
    , Frobber
{
    VIRTUAL_META_DECLARE;
};

VIRTUAL_META_DEFINE_CHILD("Frob", Frob, AtomicRefCounted)
{}

int FooBarApplication::Run(int, char**)
{
    FooBar f;

    fmt::print("{}, {}, {}\n", f.IsA<FooBar>(), f.IsA<Base>(), f.IsA<Frob>());

    auto a = f.GetClass()->GetField("frob")->Type->GetClass()->GetField("TargetType")->Type;
    auto b = GetMetaType<const MetaType*>();

    fmt::print("{}, {}, {}\n", static_cast<const void*>(a), static_cast<const void*>(b), a == b);

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
