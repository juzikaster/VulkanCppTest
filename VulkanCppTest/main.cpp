// Enable the WSI extensions
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif

// Tell SDL not to mess with main()
#define SDL_MAIN_HANDLED

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <vector>


class TriangleApp {
public:
    void run() {
        initSDL();
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    SDL_Window* window;
    unsigned extension_count;
    std::vector<const char*> extensions;
    std::vector<const char*> layers;

    vk::Instance instance;
    VkSurfaceKHR c_surface;
    vk::SurfaceKHR surface;

    void initSDL() {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            std::cout << "Could not initialize SDL." << std::endl;
            return;
        }
    }

    void initWindow() {
        // Create an SDL window that supports Vulkan rendering.
        window = SDL_CreateWindow("Vulkan Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN);
        if (window == NULL) {
            std::cout << "Could not create SDL window." << std::endl;
            return;
        }

        // Get WSI extensions from SDL (we can add more if we like - we just can't remove these)
        if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, NULL)) {
            std::cout << "Could not get the number of required instance extensions from SDL." << std::endl;
            return;
        }

        extensions.resize(extension_count);
        if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, extensions.data())) {
            std::cout << "Could not get the names of required instance extensions from SDL." << std::endl;
            return;
        }

        // Use validation layers if this is a debug build
#if defined(_DEBUG)
        layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
    }

    void initVulkan() {
        // vk::ApplicationInfo allows the programmer to specifiy some basic information about the
        // program, which can be useful for layers and tools to provide more debug information.
        vk::ApplicationInfo appInfo = vk::ApplicationInfo()
            .setPApplicationName("Vulkan C++ Windowed Program Template")
            .setApplicationVersion(1)
            .setPEngineName("LunarG SDK")
            .setEngineVersion(1)
            .setApiVersion(VK_API_VERSION_1_0);

        // vk::InstanceCreateInfo is where the programmer specifies the layers and/or extensions that
        // are needed.
        vk::InstanceCreateInfo instInfo = vk::InstanceCreateInfo()
            .setFlags(vk::InstanceCreateFlags())
            .setPApplicationInfo(&appInfo)
            .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
            .setPpEnabledExtensionNames(extensions.data())
            .setEnabledLayerCount(static_cast<uint32_t>(layers.size()))
            .setPpEnabledLayerNames(layers.data());

        try {
            instance = vk::createInstance(instInfo);
        }
        catch (const std::exception& e) {
            std::cout << "Could not create a Vulkan instance: " << e.what() << std::endl;
            return;
        }

        if (!SDL_Vulkan_CreateSurface(window, static_cast<VkInstance>(instance), &c_surface)) {
            std::cout << "Could not create a Vulkan surface." << std::endl;
            return;
        }
    }

    void mainLoop() {
        bool stillRunning = true;
        while (stillRunning) {

            SDL_Event event;
            while (SDL_PollEvent(&event)) {

                switch (event.type) {

                case SDL_QUIT:
                    stillRunning = false;
                    break;

                default:
                    // Do nothing.
                    break;
                }
            }

            SDL_Delay(10);
        }
    }

    void cleanup() {
        instance.destroySurfaceKHR(surface);
        SDL_DestroyWindow(window);
        SDL_Quit();
        instance.destroy();
    }
};

int main()
{   

    TriangleApp app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
