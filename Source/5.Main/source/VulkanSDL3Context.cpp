#include "stdafx.h"
#include "VulkanSDL3Context.h"
#include "./Utilities/Log/ErrorReport.h"
#include <iostream>
#include <set>
#include <cstring>
#include <vector>

VulkanSDL3Context& VulkanSDL3Context::Instance()
{
    static VulkanSDL3Context instance;
    return instance;
}

VulkanSDL3Context::VulkanSDL3Context()
{
}

VulkanSDL3Context::~VulkanSDL3Context()
{
    Shutdown();
}

bool VulkanSDL3Context::Init(const char* title, int width, int height, bool enableValidation)
{
    if (m_initialized) {
        return true;
    }

    m_validationEnabled = enableValidation;

    if (!InitSDL(title, width, height)) {
        g_ErrorReport.Write("[VulkanSDL3] InitSDL failed\r\n");
        return false;
    }

    PFN_vkGetInstanceProcAddr getProc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
    if (!getProc) {
        g_ErrorReport.Write("[VulkanSDL3] SDL_Vulkan_GetVkGetInstanceProcAddr returned NULL: %s\r\n", SDL_GetError() ? SDL_GetError() : "unknown");
        std::cerr << "[VulkanSDL3Context] SDL_Vulkan_GetVkGetInstanceProcAddr returned NULL!" << std::endl;
        return false;
    }

    volkInitializeCustom(getProc);

    if (!CreateInstance(enableValidation)) {
        g_ErrorReport.Write("[VulkanSDL3] CreateInstance failed\r\n");
        return false;
    }

    volkLoadInstance(m_instance);

    if (!CreateSurface()) {
        g_ErrorReport.Write("[VulkanSDL3] CreateSurface failed\r\n");
        return false;
    }

    if (!PickPhysicalDevice()) {
        g_ErrorReport.Write("[VulkanSDL3] PickPhysicalDevice failed - no suitable GPU found\r\n");
        return false;
    }

    if (!CreateLogicalDevice()) {
        g_ErrorReport.Write("[VulkanSDL3] CreateLogicalDevice failed\r\n");
        return false;
    }

    volkLoadDevice(m_device);

    CreateGPUDevice();

    m_initialized = true;
    g_ErrorReport.Write("[VulkanSDL3] SDL3 + Vulkan context successfully initialized\r\n");
    std::cout << "[VulkanSDL3Context] SDL3 + Vulkan context successfully initialized." << std::endl;
    return true;
}

void VulkanSDL3Context::Shutdown()
{
    if (m_gpuDevice != nullptr) {
        SDL_DestroyGPUDevice(m_gpuDevice);
        m_gpuDevice = nullptr;
    }

    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }

    if (m_surface != VK_NULL_HANDLE && m_instance != VK_NULL_HANDLE) {
        SDL_Vulkan_DestroySurface(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }

    if (m_window != nullptr) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    m_initialized = false;
}

bool VulkanSDL3Context::InitSDL(const char* title, int width, int height)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "[VulkanSDL3Context] SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    extern HWND g_hWnd;
    HWND hWnd = g_hWnd;
    if (hWnd != NULL) {
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, hWnd);
        SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
        m_window = SDL_CreateWindowWithProperties(props);
        SDL_DestroyProperties(props);
    }

    if (!m_window) {
        m_window = SDL_CreateWindow(
            title ? title : "Main Client - Vulkan SDL3",
            width > 0 ? width : 1280,
            height > 0 ? height : 720,
            SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN
        );
    }

    if (!m_window) {
        std::cerr << "[VulkanSDL3Context] SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

bool VulkanSDL3Context::CreateGPUDevice()
{
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXBC_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_D3D12_ALLOW_FEWER_RESOURCE_SLOTS_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, m_validationEnabled);

    m_gpuDevice = SDL_CreateGPUDeviceWithProperties(props);
    SDL_DestroyProperties(props);

    if (m_gpuDevice != nullptr) {
        const char* driver = SDL_GetGPUDeviceDriver(m_gpuDevice);
        std::cout << "[VulkanSDL3Context] SDL_GPUDevice initialized with D3D12/Vulkan fewer resource slots fallback (Driver: "
                  << (driver ? driver : "unknown") << ")." << std::endl;
        return true;
    }
    return false;
}

bool VulkanSDL3Context::InitD3D12Device()
{
    if (m_gpuDevice != nullptr) {
        SDL_DestroyGPUDevice(m_gpuDevice);
        m_gpuDevice = nullptr;
    }

    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            g_ErrorReport.Write("[VulkanSDL3Context] SDL_Init(SDL_INIT_VIDEO) failed for D3D12: %s\r\n", SDL_GetError());
            return false;
        }
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "direct3d12");
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXIL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_DXBC_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_D3D12_ALLOW_FEWER_RESOURCE_SLOTS_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_PREFERLOWPOWER_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, false);

    m_gpuDevice = SDL_CreateGPUDeviceWithProperties(props);
    SDL_DestroyProperties(props);

    if (m_gpuDevice != nullptr) {
        const char* driver = SDL_GetGPUDeviceDriver(m_gpuDevice);
        g_ErrorReport.Write("[VulkanSDL3Context] SDL_GPUDevice Direct3D 12 initialized (Driver: %s)\r\n", driver ? driver : "unknown");
        return true;
    }
    return false;
}

bool VulkanSDL3Context::CheckValidationLayerSupport()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    if (layerCount == 0) {
        return false;
    }

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : m_validationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }
    return true;
}

bool VulkanSDL3Context::CreateInstance(bool enableValidation)
{
    if (enableValidation && !CheckValidationLayerSupport()) {
        std::cerr << "[VulkanSDL3Context] Validation layers requested, but not available! Disabling." << std::endl;
        enableValidation = false;
        m_validationEnabled = false;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Main Client";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Custom Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    Uint32 sdlExtCount = 0;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtCount);
    if (!sdlExtensions) {
        std::cerr << "[VulkanSDL3Context] SDL_Vulkan_GetInstanceExtensions failed: " << SDL_GetError() << std::endl;
        return false;
    }

    std::vector<const char*> extensions;
    if (sdlExtensions && sdlExtCount > 0) {
        for (Uint32 i = 0; i < sdlExtCount; ++i) extensions.push_back(sdlExtensions[i]);
    }
    bool hasSurface = false;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    bool hasWin32Surface = false;
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    bool hasAndroidSurface = false;
#endif
    for (const char* ext : extensions) {
        if (strcmp(ext, VK_KHR_SURFACE_EXTENSION_NAME) == 0) hasSurface = true;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        if (strcmp(ext, VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) hasWin32Surface = true;
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        if (strcmp(ext, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0) hasAndroidSurface = true;
#endif
    }
    if (!hasSurface) extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    if (!hasWin32Surface) extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    if (!hasAndroidSurface) extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidation) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        std::cerr << "[VulkanSDL3Context] vkCreateInstance failed with code: " << result << std::endl;
        return false;
    }

    return true;
}

bool VulkanSDL3Context::CreateSurface()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    extern HWND g_hWnd;
    HWND hWnd = g_hWnd;
    if (hWnd != NULL && m_instance != VK_NULL_HANDLE) {
        VkWin32SurfaceCreateInfoKHR win32Info{};
        win32Info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        win32Info.hwnd = hWnd;
        win32Info.hinstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
        VkResult res = vkCreateWin32SurfaceKHR(m_instance, &win32Info, nullptr, &m_surface);
        if (res == VK_SUCCESS) {
            g_ErrorReport.Write("[VulkanSDL3] Native Win32 Vulkan Surface created (hWnd=0x%08X)\r\n", (unsigned int)(uintptr_t)hWnd);
            std::cout << "[VulkanSDL3Context] Native Win32 Vulkan Surface successfully created." << std::endl;
            return true;
        }
        g_ErrorReport.Write("[VulkanSDL3] vkCreateWin32SurfaceKHR failed: %d, trying SDL fallback\r\n", (int)res);
    }
#endif

    if (m_window && SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &m_surface)) {
        g_ErrorReport.Write("[VulkanSDL3] SDL Vulkan Surface created via SDL_Vulkan_CreateSurface\r\n");
        return true;
    }

    g_ErrorReport.Write("[VulkanSDL3] Failed to create Vulkan surface: %s\r\n", SDL_GetError() ? SDL_GetError() : "unknown");
    std::cerr << "[VulkanSDL3Context] Failed to create Vulkan surface!" << std::endl;
    return false;
}

QueueFamilyIndices VulkanSDL3Context::FindQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
        i++;
    }

    return indices;
}

bool VulkanSDL3Context::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        g_ErrorReport.Write("[VulkanSDL3] No GPUs with Vulkan support found\r\n");
        std::cerr << "[VulkanSDL3Context] Failed to find GPUs with Vulkan support!" << std::endl;
        return false;
    }

    g_ErrorReport.Write("[VulkanSDL3] Found %u Vulkan-capable GPU(s)\r\n", deviceCount);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
    QueueFamilyIndices bestIndices;
    int bestScore = -1;

    for (const auto& device : devices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        QueueFamilyIndices indices = FindQueueFamilies(device);

        g_ErrorReport.Write("[VulkanSDL3] GPU: %s (Type=%d) Graphics=%s Present=%s\r\n",
            properties.deviceName,
            (int)properties.deviceType,
            indices.graphicsFamily.has_value() ? "YES" : "NO",
            indices.presentFamily.has_value() ? "YES" : "NO");

        if (!indices.isComplete()) {
            g_ErrorReport.Write("[VulkanSDL3]   -> Skipped: incomplete queue families\r\n");
            continue;
        }

        int score = 1;
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        } else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            score += 500;
        } else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
            score += 200;
        } else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
            score += 100;
        }

        if (score > bestScore) {
            bestScore = score;
            bestDevice = device;
            bestIndices = indices;
        }
    }

    if (bestDevice != VK_NULL_HANDLE) {
        m_physicalDevice = bestDevice;
        m_queueIndices = bestIndices;
        VkPhysicalDeviceProperties prop;
        vkGetPhysicalDeviceProperties(m_physicalDevice, &prop);
        g_ErrorReport.Write("[VulkanSDL3] Selected GPU: %s (graphicsFamily=%u, presentFamily=%u)\r\n",
            prop.deviceName,
            bestIndices.graphicsFamily.value(),
            bestIndices.presentFamily.value());
        std::cout << "[VulkanSDL3Context] Selected GPU: " << prop.deviceName << " (Type: " << prop.deviceType << ")" << std::endl;
        return true;
    }

    g_ErrorReport.Write("[VulkanSDL3] No suitable GPU found after checking all devices\r\n");
    std::cerr << "[VulkanSDL3Context] Failed to find a suitable GPU!" << std::endl;
    return false;
}

bool VulkanSDL3Context::CreateLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        m_queueIndices.graphicsFamily.value(),
        m_queueIndices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures supportedFeatures{};
    vkGetPhysicalDeviceFeatures(m_physicalDevice, &supportedFeatures);

    VkPhysicalDeviceFeatures deviceFeatures{};
    if (supportedFeatures.multiDrawIndirect) {
        deviceFeatures.multiDrawIndirect = VK_TRUE;
        m_supportsMultiDrawIndirect = true;
    }
    if (supportedFeatures.drawIndirectFirstInstance) {
        deviceFeatures.drawIndirectFirstInstance = VK_TRUE;
        m_supportsDrawIndirectFirstInstance = true;
    }

    std::cout << "[VulkanSDL3Context] Hardware features: multiDrawIndirect=" 
              << (m_supportsMultiDrawIndirect ? "SUPPORTED" : "UNSUPPORTED")
              << ", drawIndirectFirstInstance=" 
              << (m_supportsDrawIndirectFirstInstance ? "SUPPORTED" : "UNSUPPORTED") << std::endl;

    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (m_validationEnabled) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        std::cerr << "[VulkanSDL3Context] vkCreateDevice failed with code: " << result << std::endl;
        return false;
    }

    vkGetDeviceQueue(m_device, m_queueIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_queueIndices.presentFamily.value(), 0, &m_presentQueue);

    return true;
}
