#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_gpu.h>
#include "volk.h"
#include <vector>
#include <string>
#include <optional>

#ifndef SDL_PROP_GPU_DEVICE_CREATE_D3D12_ALLOW_FEWER_RESOURCE_SLOTS_BOOLEAN
#define SDL_PROP_GPU_DEVICE_CREATE_D3D12_ALLOW_FEWER_RESOURCE_SLOTS_BOOLEAN "SDL.gpu.device.create.d3d12.allowfewerresourceslots"
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class VulkanSDL3Context {
public:
    static VulkanSDL3Context& Instance();

    bool Init(const char* title, int width, int height, bool enableValidation = true);
    bool InitD3D12Device();
    void Shutdown();

    // Getters
    SDL_Window* GetWindow() const { return m_window; }
    SDL_GPUDevice* GetGPUDevice() const { return m_gpuDevice; }
    VkInstance GetInstance() const { return m_instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
    VkDevice GetDevice() const { return m_device; }
    VkSurfaceKHR GetSurface() const { return m_surface; }
    VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue GetPresentQueue() const { return m_presentQueue; }
    const QueueFamilyIndices& GetQueueIndices() const { return m_queueIndices; }

    bool IsInitialized() const { return m_initialized; }
    bool SupportsMultiDrawIndirect() const { return m_supportsMultiDrawIndirect; }
    bool SupportsDrawIndirectFirstInstance() const { return m_supportsDrawIndirectFirstInstance; }

private:
    VulkanSDL3Context();
    ~VulkanSDL3Context();

    VulkanSDL3Context(const VulkanSDL3Context&) = delete;
    VulkanSDL3Context& operator=(const VulkanSDL3Context&) = delete;

    bool InitSDL(const char* title, int width, int height);
    bool CreateGPUDevice();
    bool CreateInstance(bool enableValidation);
    bool CreateSurface();
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    bool CheckValidationLayerSupport();

    bool m_initialized = false;
    bool m_validationEnabled = false;
    bool m_supportsMultiDrawIndirect = false;
    bool m_supportsDrawIndirectFirstInstance = false;

    SDL_Window* m_window = nullptr;
    SDL_GPUDevice* m_gpuDevice = nullptr;
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    QueueFamilyIndices m_queueIndices;

    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> m_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
};
