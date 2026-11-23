#include "Shared.h"
#include "Graphics.h"
#include "Engine.h"
#include "Application.h"
#include "DemoScene.h"

int main()
{
    using namespace Engine;
    using namespace Graphics;
    using namespace Input;

    Application app;
    app.SetRunningLayer<DemoSceneLayer>();

    return app.Run();
}