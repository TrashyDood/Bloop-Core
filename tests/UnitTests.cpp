#include "LogHandler.h"
#include "EventHandler.h"
#include "ThreadPool.h"
#include "Transform.h"
#ifdef ENGINE_ENABLED
#include "InputHandler.h"
#include "Scene.h"
#endif
#ifdef GRAPHICS_ENABLED
#include "GlfwInput.h"
#include "ResourceLoading.h"
#endif

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace
{
#ifdef GRAPHICS_ENABLED
    using Input::KeyCode;
    using Input::MouseKeyCode;
    using Input::Held;
    using Input::Pressed;
    using Input::Released;
#endif

    constexpr float Tolerance = 1e-4f;

    bool NearlyEqual(float actual, float expected)
    {
        return std::abs(actual - expected) <= Tolerance;
    }

    bool NearlyEqual(const Vector3f &actual, const Vector3f &expected)
    {
        return NearlyEqual(actual.x, expected.x) &&
               NearlyEqual(actual.y, expected.y) &&
               NearlyEqual(actual.z, expected.z);
    }

    void Require(bool condition, const char *message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    int GetFileDescriptor(FILE *file)
    {
#ifdef _WIN32
        return _fileno(file);
#else
        return fileno(file);
#endif
    }

    int DuplicateDescriptor(int descriptor)
    {
#ifdef _WIN32
        return _dup(descriptor);
#else
        return dup(descriptor);
#endif
    }

    int ReplaceDescriptor(int source, int target)
    {
#ifdef _WIN32
        return _dup2(source, target);
#else
        return dup2(source, target);
#endif
    }

    void CloseDescriptor(int descriptor)
    {
#ifdef _WIN32
        _close(descriptor);
#else
        close(descriptor);
#endif
    }

    void TestLogHandlerFormatting()
    {
        LOG_SCOPE;
        LogHandler &logHandler = StaticLogHandler();
        logHandler.ClearLogEntries();

        FILE *capture = std::tmpfile();
        Require(capture != nullptr, "Log handler test creates a temporary file");

        std::fflush(stderr);
        const int stderrDescriptor = GetFileDescriptor(stderr);
        const int savedStderr = DuplicateDescriptor(stderrDescriptor);
        if (savedStderr == -1)
        {
            std::fclose(capture);
            throw std::runtime_error("Log handler test duplicates stderr");
        }

        if (ReplaceDescriptor(GetFileDescriptor(capture), stderrDescriptor) == -1)
        {
            CloseDescriptor(savedStderr);
            std::fclose(capture);
            throw std::runtime_error("Log handler test redirects stderr");
        }

        LOG_INFO("message %d\n", 7);
        const std::vector<LogHandler::LogEntry> defaultPriorityEntries = logHandler.GetLogEntries();
        LOG_ERROR_WITH_PRIORITY(5, "priority %d\n", 8);
        const std::vector<LogHandler::LogEntry> explicitPriorityEntries = logHandler.GetLogEntries();
        logHandler.ClearLogEntries();

        for (int i = 0; i <= 1000; ++i)
            LOG_WARNING("entry %d\n", i);

        const std::vector<LogHandler::LogEntry> entries = logHandler.GetLogEntries();
        logHandler.ClearLogEntries();

        std::fflush(stderr);
        const int restoreResult = ReplaceDescriptor(savedStderr, stderrDescriptor);
        CloseDescriptor(savedStderr);
        if (restoreResult == -1)
        {
            std::fclose(capture);
            throw std::runtime_error("Log handler test restores stderr");
        }

        std::rewind(capture);
        char buffer[256];
        const std::size_t outputSize = std::fread(buffer, 1, sizeof(buffer), capture);
        std::fclose(capture);

        const std::string output(buffer, outputSize);
        Require(!output.empty() && output[0] == '[' &&
                    output.find("] [INFO] [thread ") != std::string::npos &&
                    output.find("message 7\n") != std::string::npos,
                "Log handler formats timestamp, level, depth, and message");
        Require(defaultPriorityEntries.size() == 1 && defaultPriorityEntries[0].priority == 0 &&
                    defaultPriorityEntries[0].depth == 1 &&
                    explicitPriorityEntries.back().priority == 5 &&
                    explicitPriorityEntries.back().depth == 1,
                "Log macros use scope depth and support explicit priorities");
        Require(entries.size() == 1000 && entries.front().message == "entry 1\n" &&
                    entries.back().message == "entry 1000\n",
                "Log handler retains the most recent 1,000 formatted entries");
        Require(entries.back().level == LogHandler::Warning && entries.back().priority == 0 &&
                    entries.back().timestamp != std::chrono::system_clock::time_point{} &&
                    !entries.back().timestampText.empty(),
                "Stored log entries preserve metadata and timestamp");
        Require(logHandler.GetLogEntries().empty(), "Log handler clears stored entries");
    }

    void TestTransformDefaultsAndComposition()
    {
        Engine::Transform transform;
        Require(NearlyEqual(transform.GetPosition(), Vector3f{0.0f}), "Transform starts at the origin");
        Require(NearlyEqual(transform.GetScale(), Vector3f{1.0f}), "Transform starts with unit scale");
        Require(NearlyEqual(transform.GetUp(), Vectors::v3fUp),
                "Transform up direction matches the engine up vector");

        transform.SetPosition({2.0f, 3.0f, 4.0f});
        transform.SetScale({2.0f, 3.0f, 4.0f});
        Require(NearlyEqual(transform.TransformPoint({1.0f, 1.0f, 1.0f}),
                            Vector3f{4.0f, 6.0f, 8.0f}),
                "TransformPoint applies scale and translation");
        Require(NearlyEqual(transform.TransformVector({1.0f, 1.0f, 1.0f}),
                            Vector3f{2.0f, 3.0f, 4.0f}),
                "TransformVector applies scale without translation");
        Require(NearlyEqual(transform.TransformVector({1.0f, 1.0f, 1.0f}, false),
                            Vector3f{1.0f, 1.0f, 1.0f}),
                "TransformVector can omit scale");
    }

    void TestTransformRotation()
    {
        Engine::Transform transform;
        transform.SetRotation(glm::angleAxis(glm::half_pi<float>(), Vector3f{0.0f, 0.0f, 1.0f}));

        Require(NearlyEqual(transform.GetRight(), Vector3f{0.0f, 1.0f, 0.0f}),
                "Rotation updates the transform's right direction");
        Require(NearlyEqual(glm::length(transform.GetRotation()), 1.0f),
                "Transform stores a normalized rotation");
    }

    void TestTransformRejectsParallelLookDirection()
    {
        Engine::Transform transform;
        transform.SetRotation(glm::angleAxis(0.5f, Vector3f{0.0f, 1.0f, 0.0f}));
        const Quaternionf originalRotation = transform.GetRotation();

        transform.SetLookDirection({0.0f, 1.0f, 0.0f}, {0.0f, 2.0f, 0.0f});

        Require(NearlyEqual(glm::dot(originalRotation, transform.GetRotation()), 1.0f),
                "Parallel look direction and up vectors leave rotation unchanged");
    }

#ifdef GRAPHICS_ENABLED
    void TestGlfwInputTranslation()
    {
        Require(Graphics::ToSharedKeyCode(GLFW_KEY_A) == KeyCode::A,
                "GLFW letters map to engine key codes");
        Require(Graphics::ToSharedKeyCode(GLFW_KEY_7) == KeyCode::Num7,
                "GLFW digits map to engine key codes");
        Require(Graphics::ToSharedKeyCode(GLFW_KEY_F25) == KeyCode::F25,
                "GLFW function keys map to engine key codes");
        Require(Graphics::ToSharedKeyCode(GLFW_KEY_KP_8) == KeyCode::Keypad8,
                "GLFW keypad digits map to engine key codes");
        Require(Graphics::ToSharedKeyCode(GLFW_KEY_ESCAPE) == KeyCode::Escape,
                "GLFW special keys map to engine key codes");
        Require(Graphics::ToSharedKeyCode(-999) == KeyCode::Unknown,
                "Unrecognized GLFW keys map to Unknown");

        Require(Graphics::ToSharedMouseButton(GLFW_MOUSE_BUTTON_1) == MouseKeyCode::Button1,
                "GLFW mouse buttons map to engine button codes");
        Require(Graphics::ToSharedMouseButton(GLFW_MOUSE_BUTTON_8) == MouseKeyCode::Button8,
                "GLFW's highest mouse button maps to the engine code");
        Require(Graphics::ToSharedMouseButton(99) == MouseKeyCode::Unknown,
                "Unrecognized GLFW mouse buttons map to Unknown");

        Require(Graphics::ToSharedKeyAction(GLFW_PRESS) == Pressed,
                "GLFW press maps to Engine::Pressed");
        Require(Graphics::ToSharedKeyAction(GLFW_RELEASE) == Released,
                "GLFW release maps to Engine::Released");
        Require(Graphics::ToSharedKeyAction(GLFW_REPEAT) == Held,
                "GLFW repeat maps to Engine::Held");
        Require(!Graphics::ToSharedKeyAction(-999), "Unrecognized GLFW actions are rejected");
    }

    class TestResourceOwner final : public Graphics::ResourceOwner
    {
        int id;
        bool succeeds;
        bool initialized = false;

    public:
        inline static int releaseCount = 0;

        TestResourceOwner(int id, bool succeeds) : id(id), succeeds(succeeds) {}
        TestResourceOwner(TestResourceOwner &&) noexcept = default;
        TestResourceOwner &operator=(TestResourceOwner &&) noexcept = default;

        bool IsInitialized() const override { return initialized; }
        void ReleaseResources() noexcept override
        {
            initialized = false;
            ++releaseCount;
        }
        bool InitializeResources() override
        {
            initialized = succeeds;
            return initialized;
        }
        int GetId() const { return id; }
    };

    class TemporaryDirectory
    {
        std::filesystem::path path;

    public:
        TemporaryDirectory()
        {
            const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
            path = std::filesystem::temp_directory_path() / ("engine-tests-" + std::to_string(nonce));
            std::filesystem::create_directories(path);
        }

        ~TemporaryDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }

        const std::filesystem::path &GetPath() const { return path; }
    };

    void WriteTextFile(const std::filesystem::path &path, const std::string &contents)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path, std::ios::binary);
        Require(static_cast<bool>(file), "Test creates an asset file");
        file << contents;
        Require(static_cast<bool>(file), "Test writes an asset file");
    }

    void TestResourceInitializationDropsFailures()
    {
        TestResourceOwner::releaseCount = 0;
        std::vector<TestResourceOwner> owners;
        owners.emplace_back(1, false);
        owners.emplace_back(2, true);
        owners.emplace_back(3, false);
        owners.emplace_back(4, true);
        std::vector<std::string> names{"one", "two", "three", "four"};
        Graphics::LoadedResources<TestResourceOwner> results{owners, names};
        results.uninitializedCount = static_cast<UInt32>(owners.size());

        Graphics::InitializeResources(results);
        Require(owners.size() == 2 && owners[0].GetId() == 2 && owners[1].GetId() == 4,
                "Failed resources are removed while successful resources are retained");
        Require(names == std::vector<std::string>{"two", "four"},
                "Names remain aligned with retained resources");
        Require(owners[0].IsInitialized() && owners[1].IsInitialized(),
                "Initialization continues after a resource fails");
        Require(TestResourceOwner::releaseCount == 2,
                "Failed resources are released before removal");
    }

    void TestResourceLoadersGenerateRelativeNames()
    {
        TemporaryDirectory temporaryDirectory;
        const std::filesystem::path root = temporaryDirectory.GetPath();

        const auto shaderRoot = root / "shaders";
        WriteTextFile(shaderRoot / "nested" / "program.vert.glsl", "vertex shader");
        WriteTextFile(shaderRoot / "nested" / "program.frag.glsl", "fragment shader");
        WriteTextFile(shaderRoot / "other" / "program.vert.glsl", "another vertex shader");
        WriteTextFile(shaderRoot / "other" / "program.unknown.glsl", "unsupported shader stage");

        std::vector<Graphics::Shader> shaders;
        std::vector<std::string> shaderNames;
        Graphics::LoadedResources<Graphics::Shader> shaderResults{shaders, shaderNames};
        Graphics::LoadShaderResources_Parallel(shaderResults, IO::Path::FromAbsolute(shaderRoot, root));
        Require(shaderResults.filesFound == 4 && shaderResults.filesFailed == 1,
                "Shader loader reports discovered and discarded files");
        Require(shaderResults.resourcesLoaded == 3 && shaderResults.resourcesFailed == 1,
                "Shader loader reports loaded and failed shader sources");
        Require(shaderNames == std::vector<std::string>{"nested/program", "other/program"},
                "Shader names include their path relative to the root");
        Require(shaders.size() == 2 && shaders[0].GetSourceCount() == 2 &&
                    shaders[1].GetSourceCount() == 1,
                "Shader files are grouped by relative directory and program name");
        Require(shaderResults.uninitializedCount == shaders.size(),
                "Shader initialization count tracks grouped shader programs");

        const auto meshRoot = root / "models";
        WriteTextFile(meshRoot / "nested" / "unnamed.obj",
            "v 0 0 0\n"
            "v 1 0 0\n"
            "v 0 1 0\n"
            "vt 0 0\n"
            "vt 1 0\n"
            "vt 0 1\n"
            "vn 0 0 1\n"
            "f 1/1/1 2/2/1 3/3/1\n");

        std::vector<Graphics::Mesh> meshes;
        std::vector<std::string> meshNames;
        Graphics::LoadedResources<Graphics::Mesh> meshResults{meshes, meshNames};
        Graphics::LoadMeshResources_Parallel(meshResults, IO::Path::FromAbsolute(meshRoot, root));
        Require(meshResults.filesFound == 1 && meshResults.filesFailed == 0,
                "Mesh loader reports successfully loaded and discarded files");
        Require(meshResults.resourcesLoaded == 1 && meshResults.resourcesFailed == 0,
                "Mesh loader reports loaded and failed mesh objects");
        Require(meshNames == std::vector<std::string>{"nested/unnamed"},
                "Unnamed mesh objects use their root-relative file stem");

        const auto textureSource = std::filesystem::current_path() / "assets" / "textures" / "test.png";
        Require(std::filesystem::is_regular_file(textureSource), "Texture test fixture exists");
        const auto texturePath = root / "textures" / "nested" / "sample.png";
        std::filesystem::create_directories(texturePath.parent_path());
        std::filesystem::copy_file(textureSource, texturePath);
        WriteTextFile(root / "textures" / "nested" / "broken.png", "not a texture");

        std::vector<Graphics::Texture2D> textures;
        std::vector<std::string> textureNames;
        Graphics::LoadedResources<Graphics::Texture2D> textureResults{textures, textureNames};
        Graphics::LoadTextureResources_Parallel(textureResults, IO::Path::FromAbsolute(root / "textures", root));
        Require(textureResults.filesFound == 2 && textureResults.filesFailed == 1,
                "Texture loader reports discovered and discarded files");
        Require(textureResults.resourcesLoaded == 1 && textureResults.resourcesFailed == 1,
                "Texture loader reports loaded and failed textures");
        Require(textureNames == std::vector<std::string>{"nested/sample"},
                "Texture names include their path relative to the root");

        std::vector<Graphics::Shader> allShaders;
        std::vector<Graphics::Mesh> allMeshes;
        std::vector<Graphics::Texture2D> allTextures;
        std::vector<std::string> allShaderNames, allMeshNames, allTextureNames;
        Graphics::LoadedResources<Graphics::Shader> allShaderResults{allShaders, allShaderNames};
        Graphics::LoadedResources<Graphics::Mesh> allMeshResults{allMeshes, allMeshNames};
        Graphics::LoadedResources<Graphics::Texture2D> allTextureResults{allTextures, allTextureNames};
        Graphics::LoadResources_Parallel(
            allShaderResults, IO::Path::FromAbsolute(shaderRoot, root),
            allMeshResults, IO::Path::FromAbsolute(meshRoot, root),
            allTextureResults, IO::Path::FromAbsolute(root / "textures", root));
        Require(allShaderNames == shaderNames && allMeshNames == meshNames && allTextureNames == textureNames,
                "Combined resource loader preserves names from each supplied directory");
    }
#endif

    void Increment(int &value)
    {
        ++value;
    }

    void TestEventHandlerRegistrationAndRemoval()
    {
        EventHandler<int &> event;
        int value = 0;

        const auto callbackId = event.BindCallback(Increment);
        Require(callbackId.has_value(), "Event accepts a callback");
        Require(!event.BindCallback(Increment).has_value(), "Event rejects duplicate free-function callbacks");

        event.Invoke(value);
        Require(value == 1, "Event invokes registered callbacks");
        Require(event.UnbindCallback(*callbackId), "Event removes a callback by ID");
        Require(!event.UnbindCallback(*callbackId), "Event reports when a callback ID is not registered");

        event.Invoke(value);
        Require(value == 1, "Removed callbacks are not invoked");
    }

    void TestEventHandlerAllowsDuplicateLambdas()
    {
        EventHandler<int> event;
        int value = 0;
        const auto callback = [&value](int increment) { value += increment; };

        const auto firstId = event.BindCallback(callback);
        const auto secondId = event.BindCallback(callback);
        Require(firstId.has_value() && secondId.has_value(), "Event allows duplicate lambda callbacks");

        event.Invoke(2);
        Require(value == 4, "Event invokes each registered lambda");

        Require(event.UnbindCallback(*firstId), "Event removes one duplicate callback by ID");
        event.Invoke(3);
        Require(value == 7, "Removing one duplicate leaves the other registered");
    }

    void TestEventHandlerConcurrentInvocation()
    {
        EventHandler<int> event;
        std::atomic<int> value = 0;
        event.BindCallback([&value](int increment) { value.fetch_add(increment, std::memory_order_relaxed); });

        std::vector<std::thread> threads;
        for (int i = 0; i < 4; ++i)
        {
            threads.emplace_back([&event] {
                for (int j = 0; j < 1000; ++j)
                    event.Invoke(1);
            });
        }

        for (auto &thread : threads)
            thread.join();

        Require(value == 4000, "Event supports concurrent invocation");
    }

    void TestThreadPoolHelpsNestedTasks()
    {
        ThreadPool threadPool(1);
        auto parentTask = threadPool.SubmitTask([&threadPool]
        {
            auto childTask = threadPool.SubmitTask([] { return 41; });
            return childTask.Get() + 1;
        });

        Require(parentTask.Get() == 42, "Pool workers execute queued work while waiting");
    }

#ifdef ENGINE_ENABLED
    void TestInputHandlerKeyStateTransitions()
    {
        Engine::InputHandler input;
        std::vector<Engine::KeyAction> actions;
        const auto callbackId = input.BindKey(
            [&actions](const Engine::InputCode &, const Engine::KeyAction &action) { actions.push_back(action); },
            Engine::KeyCode::A);

        Require(callbackId.has_value(), "Input handler accepts a key callback");
        Require(!input.IsKeyHeld(Engine::KeyCode::A), "Unseen keys are not held");

        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::A), Engine::Pressed);
        Require(input.IsKeyHeld(Engine::KeyCode::A), "Pressed key becomes held");
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::A), Engine::Pressed);
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::A), Engine::Held);
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::A), Engine::Released);
        Require(!input.IsKeyHeld(Engine::KeyCode::A), "Released key is no longer held");
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::A), Engine::Pressed);

        const std::vector<Engine::KeyAction> expected{
            Engine::Pressed, Engine::Held, Engine::Held, Engine::Released, Engine::Pressed};
        Require(actions == expected, "Input handler emits press, hold, and release transitions");
    }

    void TestInputHandlerUnbindsKeyCallback()
    {
        Engine::InputHandler input;
        int calls = 0;
        const auto callbackId = input.BindKey(
            [&calls](const Engine::InputCode &, const Engine::KeyAction &) { ++calls; },
            Engine::KeyCode::B);

        Require(callbackId.has_value(), "Input handler returns a callback ID");
        Require(!input.UnbindKey(*callbackId, Engine::KeyCode::A), "Callback cannot be removed from another key");
        Require(input.UnbindKey(*callbackId, Engine::KeyCode::B), "Input handler removes callback by key and ID");
        Require(!input.UnbindKey(*callbackId, Engine::KeyCode::B), "Removed callback cannot be removed twice");

        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::B), Engine::Pressed);
        Require(calls == 0, "Unbound input callback is not invoked");
    }

    void TestInputHandlerMouseBindingAndState()
    {
        Engine::InputHandler input;
        std::vector<std::pair<Engine::InputCode, Engine::KeyAction>> events;
        const auto callbackId = input.BindKey(
            [&events](const Engine::InputCode &code, const Engine::KeyAction &action)
            {
                events.emplace_back(code, action);
            },
            Engine::MouseKeyCode::Left);

        Require(callbackId.has_value(), "Input handler accepts a typed mouse-button callback");
        Require(!input.IsKeyHeld(Engine::MouseKeyCode::Left), "Unseen mouse buttons are not held");

        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::MouseKeyCode::Left), Engine::Pressed);
        Require(input.IsKeyHeld(Engine::MouseKeyCode::Left), "Pressed mouse button becomes held");
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::MouseKeyCode::Left), Engine::Released);
        Require(!input.IsKeyHeld(Engine::MouseKeyCode::Left), "Released mouse button is no longer held");
        Require(events.size() == 2, "Mouse callback receives press and release events");
        Require(events[0] == std::make_pair(static_cast<Engine::InputCode>(Engine::MouseKeyCode::Left), Engine::Pressed) &&
                    events[1] == std::make_pair(static_cast<Engine::InputCode>(Engine::MouseKeyCode::Left), Engine::Released),
                "Mouse callback receives the button code and actions");

        Require(input.UnbindKey(*callbackId, Engine::MouseKeyCode::Left), "Mouse callback can be unbound by typed button");
    }

    void TestInputHandlerAnyInputBinding()
    {
        Engine::InputHandler input;
        std::vector<Engine::InputCode> codes;
        const auto callbackId = input.BindKey(
            [&codes](const Engine::InputCode &code, const Engine::KeyAction &) { codes.push_back(code); },
            Engine::AnyInput);

        Require(callbackId.has_value(), "Input handler accepts an any-input callback");
        input.OnKeyStateChanged(Engine::KeyCode::A, Engine::Pressed);
        input.OnKeyStateChanged(Engine::MouseKeyCode::Right, Engine::Pressed);

        Require(codes == std::vector<Engine::InputCode>{
                             static_cast<Engine::InputCode>(Engine::KeyCode::A),
                             static_cast<Engine::InputCode>(Engine::MouseKeyCode::Right)},
                "Any-input callback receives keyboard and mouse events");
        Require(input.IsKeyHeld(Engine::AnyInput), "Any-input state is held while any input is down");

        Require(input.UnbindKey(*callbackId, Engine::AnyInput), "Any-input callback can be unbound");
        input.OnKeyStateChanged(Engine::KeyCode::A, Engine::Released);
        input.OnKeyStateChanged(Engine::MouseKeyCode::Right, Engine::Released);
        Require(!input.IsKeyHeld(Engine::AnyInput), "Any-input state clears when all inputs are released");
    }

    void TestInputHandlerMouseAnyBinding()
    {
        Engine::InputHandler input;
        std::vector<Engine::InputCode> codes;
        const auto callbackId = input.BindKey(
            [&codes](const Engine::InputCode &code, const Engine::KeyAction &) { codes.push_back(code); },
            Engine::MouseKeyCode::Any);

        Require(callbackId.has_value(), "Input handler accepts a mouse-any callback");
        const std::vector<Engine::MouseKeyCode> buttons{
            Engine::MouseKeyCode::Button1, Engine::MouseKeyCode::Button2,
            Engine::MouseKeyCode::Button3, Engine::MouseKeyCode::Button4,
            Engine::MouseKeyCode::Button5, Engine::MouseKeyCode::Button6,
            Engine::MouseKeyCode::Button7, Engine::MouseKeyCode::Button8};

        for (const auto button : buttons)
            input.OnKeyStateChanged(button, Engine::Pressed);

        Require(codes.size() == buttons.size(), "MouseKeyCode::Any receives all mouse buttons");
        for (std::size_t i = 0; i < buttons.size(); ++i)
            Require(codes[i] == static_cast<Engine::InputCode>(buttons[i]),
                    "MouseKeyCode::Any receives each pressed button code");
    }

    void TestInputHandlerTypedAnyState()
    {
        Engine::InputHandler input;

        input.OnKeyStateChanged(Engine::KeyCode::A, Engine::Pressed);
        input.OnKeyStateChanged(Engine::MouseKeyCode::Left, Engine::Pressed);

        Require(input.IsKeyHeld(Engine::KeyCode::Any), "KeyCode::Any reports held keys");
        Require(input.IsKeyHeld(Engine::MouseKeyCode::Any), "MouseKeyCode::Any reports held mouse buttons");
        Require(input.IsKeyHeld(Engine::AnyInput), "AnyInput reports held inputs of any type");

        input.OnKeyStateChanged(Engine::KeyCode::A, Engine::Released);
        Require(!input.IsKeyHeld(Engine::KeyCode::Any), "KeyCode::Any clears when no keys are held");
        Require(input.IsKeyHeld(Engine::MouseKeyCode::Any), "MouseKeyCode::Any remains held while a button is held");
        Require(input.IsKeyHeld(Engine::AnyInput), "AnyInput remains held while an input is held");

        input.OnKeyStateChanged(Engine::MouseKeyCode::Left, Engine::Released);
        Require(!input.IsKeyHeld(Engine::MouseKeyCode::Any), "MouseKeyCode::Any clears when no buttons are held");
        Require(!input.IsKeyHeld(Engine::AnyInput), "AnyInput clears when no inputs are held");
    }

    void TestInputHandlerAxis2()
    {
        Engine::InputHandler input;
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::D), Engine::Pressed);
        input.OnKeyStateChanged(static_cast<Engine::InputCode>(Engine::KeyCode::S), Engine::Pressed);

        float x = 0.0f;
        float y = 0.0f;
        input.GetAxis2(
            Engine::KeyCode::D, Engine::KeyCode::A,
            Engine::KeyCode::W, Engine::KeyCode::S,
            x, y);

        Require(NearlyEqual(x, 1.0f) && NearlyEqual(y, 1.0f),
                "Axis2 combines horizontal and vertical input");
    }

    class LifecycleScene : public Engine::Scene
    {
    public:
        int starts = 0;
        int ticks = 0;
        int stops = 0;
        int keyEvents = 0;
        bool tickObservedRunning = false;

    protected:
        Int32 OnStart() override
        {
            ++starts;
            GetInput().BindKey(
                [this](const Engine::InputCode &, const Engine::KeyAction &) { ++keyEvents; },
                Engine::KeyCode::Space);
            return EXIT_SUCCESS;
        }

        Int32 OnMainTick(const Engine::Scene::SceneTime &) override
        {
            ++ticks;
            tickObservedRunning = GetIsRunning();
            return EXIT_SUCCESS;
        }

        void OnStop(const Int32) override { ++stops; }
    };

    void TestSceneLifecycleAndRunHooks()
    {
        LifecycleScene scene;
        Engine::Scene::SceneConfig config;
        int eventPumps = 0;
        int presents = 0;
        int continueChecks = 0;
        const Engine::Scene::RunHooks hooks{
            [&]
            {
                ++eventPumps;
                scene.GetInput().OnKeyStateChanged(Engine::KeyCode::Space, Engine::Pressed);
            },
            [&] { ++presents; },
            [&] { ++continueChecks; return false; }
        };

        Require(scene.Run(config, hooks) == EXIT_SUCCESS, "Scene runner returns the tick result");
        Require(scene.starts == 1 && scene.ticks == 1 && scene.stops == 1,
                "Scene runner owns scene lifecycle and runs one frame");
        Require(eventPumps == 1 && presents == 1 && continueChecks == 1,
                "Scene runner invokes platform hooks around the frame");
        Require(scene.keyEvents == 1, "Scene receives input during the pre-tick hook");
        Require(scene.tickObservedRunning && scene.GetState() == Engine::Scene::State::Stopped,
                "Scene transitions through running to stopped");
    }
#endif

    using Test = std::pair<const char *, void (*)()>;
}

int main()
{
    const std::vector<Test> tests{
        {"log handler formatting", TestLogHandlerFormatting},
        {"transform defaults and composition", TestTransformDefaultsAndComposition},
        {"transform rotation", TestTransformRotation},
        {"transform rejects parallel look direction", TestTransformRejectsParallelLookDirection},
#ifdef GRAPHICS_ENABLED
        {"GLFW input translation", TestGlfwInputTranslation},
        {"resource initialization drops failures", TestResourceInitializationDropsFailures},
        {"resource loaders generate relative names", TestResourceLoadersGenerateRelativeNames},
#endif
        {"event registration and removal", TestEventHandlerRegistrationAndRemoval},
        {"event duplicate lambdas", TestEventHandlerAllowsDuplicateLambdas},
        {"event concurrent invocation", TestEventHandlerConcurrentInvocation},
        {"thread pool helps nested tasks", TestThreadPoolHelpsNestedTasks},
#ifdef ENGINE_ENABLED
        {"input key state transitions", TestInputHandlerKeyStateTransitions},
        {"input callback unbinding", TestInputHandlerUnbindsKeyCallback},
        {"input mouse binding and state", TestInputHandlerMouseBindingAndState},
        {"input any-input binding", TestInputHandlerAnyInputBinding},
        {"input mouse-any binding", TestInputHandlerMouseAnyBinding},
        {"input typed any state", TestInputHandlerTypedAnyState},
        {"input 2D axis", TestInputHandlerAxis2},
        {"scene lifecycle and run hooks", TestSceneLifecycleAndRunHooks},
#endif
    };

    int failures = 0;
    for (const auto &[name, run] : tests)
    {
        try
        {
            run();
            std::cout << "[PASS] " << name << '\n';
        }
        catch (const std::exception &error)
        {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
        }
    }

    return failures == 0 ? 0 : 1;
}
