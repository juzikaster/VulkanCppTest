#pragma region Pre-processing
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
#pragma endregion

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <vector>
#include <cstring>
#include <optional>


class TriangleApp {
#pragma region public
public:
    void run() {
        initSDL();
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }
#pragma endregion

#pragma region private
private:
#pragma region Structs
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;

        bool isComplete() {
            return graphicsFamily.has_value();
        }
    };
#pragma endregion
#pragma region Constants

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif // NDEBUG

#pragma endregion

    SDL_Window* window;
    unsigned extension_count;
    std::vector<const char*> extensions;
    std::vector<const char*> layers;

    vk::Instance instance;
    VkSurfaceKHR c_surface;
    vk::SurfaceKHR surface;
    VkDevice device;
    
#pragma region SDL

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
#pragma endregion

#pragma region Vulkan

    void initVulkan() {
        if (enableValidationLayers && !checkValidationLayerSupport())
            throw std::runtime_error("Validation layers not available!");
        createInstance();
        pickPhysicalDevice();
        createLogicalDevice();
    }

    void createInstance() {
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

        if (enableValidationLayers) {
            instInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            instInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            instInfo.enabledLayerCount = 0;
        }

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

    void pickPhysicalDevice() {
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) 
            throw std::runtime_error("Failed to find GPU with Vulkan support!");
        
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
            throw std::runtime_error("Failed to find suitable GPU!");
    }

    void createLogicalDevice() {

    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
                indices.graphicsFamily = i;

            if (indices.isComplete())
                break;
            i++;
        }
        return indices;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        return indices.isComplete();
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        bool layerFound;
        for (const char* layerName : validationLayers) {
            layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
                return false;
        }
        return true;
    }

#pragma endregion
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
#pragma endregion
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
