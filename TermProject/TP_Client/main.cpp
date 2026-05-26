#include "GameApp.h"

#include <Windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int commandShow)
{
    GameApp app;

    return app.Run(instance, commandShow);
}
