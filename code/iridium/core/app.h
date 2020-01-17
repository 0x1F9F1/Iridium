#pragma once

namespace Iridium
{
    class Application
    {
    public:
        Application();
        virtual ~Application();

        virtual int Run(int argc, char** argv);
        virtual StringView GetName() = 0;

        virtual int Init(int argc, char** argv);
        virtual bool Update();
        virtual int Shutdown();

    private:
        static Application* s_App;

        friend Application& App();
    };

    extern Ptr<Application> CreateApplication();

    inline Application& App()
    {
        return *Application::s_App;
    }
} // namespace Iridium