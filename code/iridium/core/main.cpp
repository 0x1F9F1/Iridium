#include "main.h"

#include "app.h"

int Iridium::Main(int argc, char** argv)
{
    Ptr<Application> app = CreateApplication();

    return app->Run(argc, argv);
}
