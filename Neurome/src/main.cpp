#include "NeuromeGUI.h"

int main(int argc, const char *argv[])
{
    if (argc != 1)
    {
        Messenger::error("Command line arguments are not supported");
        return 1;
    }

    if (ProcessHandler::requestAdmin() == 1)
    {
        return 0;
    }

    NeuromeGUI app;
    try
    {
        app.render();
    }
    catch (const std::exception &exp)
    {
        Messenger::error(exp.what());
        return 1;
    }

    return 0;
}

