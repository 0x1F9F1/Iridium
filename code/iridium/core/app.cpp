#include "app.h"

namespace Iridium
{
    Application* Application::s_App {nullptr};

    Application::Application()
    {
        IrAssert(s_App == nullptr, "Cannot have multiple applications");

        s_App = this;
    }

    Application::~Application()
    {
        IrAssert(s_App == this, "Cannot have multiple applications");

        s_App = nullptr;
    }

    int Application::Run(int argc, char** argv)
    {
        int result = Init(argc, argv);

        if (result == 0)
        {
            while (Update())
                ;

            result = Shutdown();
        }
        else
        {
            Shutdown();
        }

        return result;
    }

    int Application::Init(int, char**)
    {
        return 0;
    }

    bool Application::Update()
    {
        return false;
    }

    int Application::Shutdown()
    {
        return 0;
    }
} // namespace Iridium
