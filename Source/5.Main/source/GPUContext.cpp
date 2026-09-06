#include "stdafx.h"
#include "GPUContext.h"
#include "VulkanSDL3Context.h"
#include "VulkanTextureManager.h"
#include "BatchRenderer.h"
#include "ZzzBMD.h"
#include "New_RenderBMD.h"
#include "EmbeddedShaders.h"
#include "ZzzTexture.h"
#include "./Utilities/Log/ErrorReport.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <array>
#include <cstring>

GPUContext& GPUContext::Instance()
{
    static GPUContext instance;
    return instance;
}

GPUContext::GPUContext()
{
}

GPUContext::~GPUContext()
{
    Shutdown();
}

bool GPUContext::Init(SDL_Window* window, int width, int height)
{
    if (m_initialized) {
        return true;
    }

    m_width = width;
    m_height = height;

    VulkanSDL3Context& vkCtx = VulkanSDL3Context::Instance();
    if (!vkCtx.IsInitialized()) {
        if (!vkCtx.Init("MU Online - SDL3 Vulkan", width, height, false)) {
            std::cerr << "[GPUContext] Failed to initialize VulkanSDL3Context!" << std::endl;
            return false;
        }
    }

    m_device = vkCtx.GetDevice();
    m_physicalDevice = vkCtx.GetPhysicalDevice();
    m_surface = vkCtx.GetSurface();
    m_graphicsQueue = vkCtx.GetGraphicsQueue();
    m_presentQueue = vkCtx.GetPresentQueue();
    m_window = vkCtx.GetWindow();

    if (!CreateCommandPoolAndBuffers()) {
        g_ErrorReport.Write("[GPUContext] Failed: CreateCommandPoolAndBuffers\r\n");
        return false;
    }
    if (!CreateDescriptorSetLayouts()) {
        g_ErrorReport.Write("[GPUContext] Failed: CreateDescriptorSetLayouts\r\n");
        return false;
    }
    if (!CreateDescriptorPool()) {
        g_ErrorReport.Write("[GPUContext] Failed: CreateDescriptorPool\r\n");
        return false;
    }

    // Initialize Texture Manager with Vulkan objects FIRST
    VulkanTextureManager::Instance().Init(
        m_device,
        m_physicalDevice,
        m_commandPool,
        m_graphicsQueue,
        m_descriptorPool,
        m_textureDescriptorLayout
    );

    if (!CreateSwapchain(width, height)) {
        g_ErrorReport.Write("[GPUContext] Failed: CreateSwapchain\r\n");
        return false;
    }
    if (!CreateDepthResources()) {
        g_ErrorReport.Write("[GPUContext] Failed: CreateDepthResources\r\n");
        return false;
    }
    if (!CreateRenderPass()) {
        g_ErrorReport.Write("[GPUContext] Failed: CreateRenderPass\r\n");
        return false;
    }
    if (!CreateFramebuffers()) {
        g_ErrorReport.Write("[GPUContext] Failed: CreateFramebuffers\r\n");
        return false;
    }
    if (!CreateSyncObjects()) {
        g_ErrorReport.Write("[GPUContext] Failed: CreateSyncObjects\r\n");
        return false;
    }
    if (!CreateDynamicBuffers()) {
        g_ErrorReport.Write("[GPUContext] Failed: CreateDynamicBuffers\r\n");
        return false;
    }
    if (!CreateUnitQuads()) {
        g_ErrorReport.Write("[GPUContext] Failed: CreateUnitQuads\r\n");
        return false;
    }
    if (!CreatePipelines()) {
        g_ErrorReport.Write("[GPUContext] Failed: CreatePipelines\r\n");
        return false;
    }

    m_initialized = true;
    std::cout << "[GPUContext] Native Vulkan rendering engine initialized successfully (" << width << "x" << height << ")." << std::endl;
    return true;
}

void GPUContext::Shutdown()
{
    if (!m_initialized || m_device == VK_NULL_HANDLE) {
        return;
    }

    VulkanTextureManager::Instance().FlushUploadQueue();
    vkDeviceWaitIdle(m_device);

    // Destroy Pipelines
    if (m_terrainPipelineOpaque != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_terrainPipelineOpaque, nullptr);
        m_terrainPipelineOpaque = VK_NULL_HANDLE;
    }
    if (m_terrainPipelineBlend != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_terrainPipelineBlend, nullptr);
        m_terrainPipelineBlend = VK_NULL_HANDLE;
    }
    if (m_terrainPipelineAdd != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_terrainPipelineAdd, nullptr);
        m_terrainPipelineAdd = VK_NULL_HANDLE;
    }
    if (m_terrainPipelineDark != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_terrainPipelineDark, nullptr);
        m_terrainPipelineDark = VK_NULL_HANDLE;
    }
    if (m_terrainPipelineBlendNoDepth != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_terrainPipelineBlendNoDepth, nullptr);
        m_terrainPipelineBlendNoDepth = VK_NULL_HANDLE;
    }
    if (m_terrainPipelineAddNoDepth != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_terrainPipelineAddNoDepth, nullptr);
        m_terrainPipelineAddNoDepth = VK_NULL_HANDLE;
    }
    if (m_terrainPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_terrainPipelineLayout, nullptr);
        m_terrainPipelineLayout = VK_NULL_HANDLE;
    }

    if (m_meshPipelineOpaque != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_meshPipelineOpaque, nullptr);
        m_meshPipelineOpaque = VK_NULL_HANDLE;
    }
    if (m_meshPipelineAlphaBlend != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_meshPipelineAlphaBlend, nullptr);
        m_meshPipelineAlphaBlend = VK_NULL_HANDLE;
    }
    if (m_meshPipelineAdditive != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_meshPipelineAdditive, nullptr);
        m_meshPipelineAdditive = VK_NULL_HANDLE;
    }
    if (m_meshPipelineAlphaTest != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_meshPipelineAlphaTest, nullptr);
        m_meshPipelineAlphaTest = VK_NULL_HANDLE;
    }
    if (m_meshPipelineDark != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_meshPipelineDark, nullptr);
        m_meshPipelineDark = VK_NULL_HANDLE;
    }
    if (m_meshPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_meshPipelineLayout, nullptr);
        m_meshPipelineLayout = VK_NULL_HANDLE;
    }

    if (m_shadowPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_shadowPipeline, nullptr);
        m_shadowPipeline = VK_NULL_HANDLE;
    }
    if (m_shadowPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_shadowPipelineLayout, nullptr);
        m_shadowPipelineLayout = VK_NULL_HANDLE;
    }

    if (m_spritePipelineAlpha != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_spritePipelineAlpha, nullptr);
        m_spritePipelineAlpha = VK_NULL_HANDLE;
    }
    if (m_spritePipelineAdd != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_spritePipelineAdd, nullptr);
        m_spritePipelineAdd = VK_NULL_HANDLE;
    }
    if (m_spritePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_spritePipelineLayout, nullptr);
        m_spritePipelineLayout = VK_NULL_HANDLE;
    }

    if (m_uiPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_uiPipeline, nullptr);
        m_uiPipeline = VK_NULL_HANDLE;
    }
    if (m_uiPipelineAdd != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_uiPipelineAdd, nullptr);
        m_uiPipelineAdd = VK_NULL_HANDLE;
    }
    if (m_uiPipelineNone != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_uiPipelineNone, nullptr);
        m_uiPipelineNone = VK_NULL_HANDLE;
    }
    if (m_uiPipelineDark != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_uiPipelineDark, nullptr);
        m_uiPipelineDark = VK_NULL_HANDLE;
    }
    if (m_uiPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_uiPipelineLayout, nullptr);
        m_uiPipelineLayout = VK_NULL_HANDLE;
    }

    if (m_clothPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_clothPipeline, nullptr);
        m_clothPipeline = VK_NULL_HANDLE;
    }
    if (m_clothPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_clothPipelineLayout, nullptr);
        m_clothPipelineLayout = VK_NULL_HANDLE;
    }

    CleanupWaterCompute();
    CleanupClothCompute();

    DestroyUnitQuads();
    DestroyDynamicBuffers();

    VulkanTextureManager::Instance().Shutdown();

    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }
    if (m_storageDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_storageDescriptorLayout, nullptr);
        m_storageDescriptorLayout = VK_NULL_HANDLE;
    }
    if (m_uniformDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_uniformDescriptorLayout, nullptr);
        m_uniformDescriptorLayout = VK_NULL_HANDLE;
    }
    if (m_textureDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_textureDescriptorLayout, nullptr);
        m_textureDescriptorLayout = VK_NULL_HANDLE;
    }
    if (m_fragUniformDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_fragUniformDescriptorLayout, nullptr);
        m_fragUniformDescriptorLayout = VK_NULL_HANDLE;
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (m_imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        }
        if (m_renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        }
        if (m_inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
        }
    }

    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }

    CleanupSwapchain();
    DestroyDepthResources();

    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        m_renderPass = VK_NULL_HANDLE;
    }

    if (m_screenshotBuffer != VK_NULL_HANDLE) {
        if (m_screenshotMapped) {
            vkUnmapMemory(m_device, m_screenshotMemory);
            m_screenshotMapped = nullptr;
        }
        vkDestroyBuffer(m_device, m_screenshotBuffer, nullptr);
        vkFreeMemory(m_device, m_screenshotMemory, nullptr);
        m_screenshotBuffer = VK_NULL_HANDLE;
        m_screenshotMemory = VK_NULL_HANDLE;
        m_screenshotBufferSize = 0;
    }

    m_initialized = false;
    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_surface = VK_NULL_HANDLE;
    m_swapchain = VK_NULL_HANDLE;
}

void GPUContext::WaitIdle()
{
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }
}

std::vector<char> GPUContext::ReadSPVFile(const std::string& filename)
{
    auto embedded = EmbeddedShaders::GetEmbeddedSPV(filename);
    if (!embedded.empty()) {
        return embedded;
    }

    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::ifstream fileAlt("Shaders/" + filename, std::ios::ate | std::ios::binary);
        if (!fileAlt.is_open()) {
            fileAlt.open("Data/" + filename, std::ios::ate | std::ios::binary);
            if (!fileAlt.is_open()) {
                return {};
            }
        }
        size_t fileSize = (size_t)fileAlt.tellg();
        std::vector<char> buffer(fileSize);
        fileAlt.seekg(0);
        fileAlt.read(buffer.data(), fileSize);
        return buffer;
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

VkShaderModule GPUContext::CreateShaderModule(const std::vector<char>& code)
{
    if (code.empty()) {
        return VK_NULL_HANDLE;
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

bool GPUContext::CreateSwapchain(int width, int height)
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkResult capRes = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);
    if (capRes != VK_SUCCESS) {
        g_ErrorReport.Write("[GPUContext] vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed: %d\r\n", (int)capRes);
        return false;
    }

    g_ErrorReport.Write("[GPUContext] Surface caps: minImage=%u maxImage=%u curExtent=%ux%u minExtent=%ux%u maxExtent=%ux%u compositeAlpha=0x%X\r\n",
        capabilities.minImageCount, capabilities.maxImageCount,
        capabilities.currentExtent.width, capabilities.currentExtent.height,
        capabilities.minImageExtent.width, capabilities.minImageExtent.height,
        capabilities.maxImageExtent.width, capabilities.maxImageExtent.height,
        capabilities.supportedCompositeAlpha);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    if (formatCount == 0) {
        g_ErrorReport.Write("[GPUContext] No surface formats available\r\n");
        return false;
    }
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }
    m_swapchainImageFormat = surfaceFormat.format;
    g_ErrorReport.Write("[GPUContext] Selected format=%d colorSpace=%d\r\n", (int)surfaceFormat.format, (int)surfaceFormat.colorSpace);

    VkExtent2D extent;
    if (capabilities.currentExtent.width != 0xFFFFFFFF && capabilities.currentExtent.width > 0 && capabilities.currentExtent.height > 0) {
        extent = capabilities.currentExtent;
    } else {
        extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    if (extent.width == 0 || extent.height == 0) {
        g_ErrorReport.Write("[GPUContext] Swapchain extent is 0x0, window may not be visible yet\r\n");
        extent.width = (width > 0) ? static_cast<uint32_t>(width) : 1280;
        extent.height = (height > 0) ? static_cast<uint32_t>(height) : 720;
        extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    m_swapchainExtent = extent;

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    if (presentModeCount > 0) {
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, presentModes.data());
    }

    VkPresentModeKHR bestPresentMode = VK_PRESENT_MODE_FIFO_KHR; // Fallback
    for (const auto& mode : presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            bestPresentMode = mode;
            break;
        }
        if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestPresentMode = mode;
        }
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
        createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    const QueueFamilyIndices& queueIndices = VulkanSDL3Context::Instance().GetQueueIndices();
    uint32_t queueFamilyIndices[] = { queueIndices.graphicsFamily.value(), queueIndices.presentFamily.value() };
    if (queueIndices.graphicsFamily != queueIndices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = capabilities.currentTransform;

    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (!(capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)) {
        if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
            compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        } else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
            compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
        } else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
            compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        }
    }
    createInfo.compositeAlpha = compositeAlpha;

    createInfo.presentMode = bestPresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult swapchainRes = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain);
    if (swapchainRes != VK_SUCCESS) {
        g_ErrorReport.Write("[GPUContext] vkCreateSwapchainKHR failed with error %d\r\n", swapchainRes);
        return false;
    }

    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());

    m_swapchainImageViews.resize(imageCount);
    for (size_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_swapchainImageFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

void GPUContext::CleanupSwapchain()
{
    for (auto framebuffer : m_swapchainFramebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
    }
    m_swapchainFramebuffers.clear();

    for (auto imageView : m_swapchainImageViews) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device, imageView, nullptr);
        }
    }
    m_swapchainImageViews.clear();

    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

bool GPUContext::RecreateSwapchain()
{
    if (m_width == 0 || m_height == 0) {
        return false;
    }

    VkSurfaceCapabilitiesKHR capabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities) == VK_SUCCESS) {
        if (capabilities.currentExtent.width == 0 || capabilities.currentExtent.height == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return false;
        }
    }

    vkDeviceWaitIdle(m_device);

    CleanupSwapchain();
    DestroyDepthResources();

    if (!CreateSwapchain(m_width, m_height)) return false;
    if (!CreateDepthResources()) return false;
    if (!CreateFramebuffers()) return false;

    return true;
}

bool GPUContext::CreateDepthResources()
{
    DestroyDepthResources();

    std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    m_depthFormat = VK_FORMAT_D32_SFLOAT;
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);
        if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            m_depthFormat = format;
            break;
        }
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_swapchainExtent.width;
    imageInfo.extent.height = m_swapchainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = m_depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &m_depthImage) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(m_device, m_depthImage, &memReq);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    uint32_t memTypeIndex = 0;
    bool foundDeviceLocal = false;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memReq.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            memTypeIndex = i;
            foundDeviceLocal = true;
            break;
        }
    }
    if (!foundDeviceLocal) {
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (memReq.memoryTypeBits & (1 << i)) {
                memTypeIndex = i;
                break;
            }
        }
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memTypeIndex;

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_depthImageMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindImageMemory(m_device, m_depthImage, m_depthImageMemory, 0);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = m_depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    return (vkCreateImageView(m_device, &viewInfo, nullptr, &m_depthImageView) == VK_SUCCESS);
}

void GPUContext::DestroyDepthResources()
{
    if (m_depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_depthImageView, nullptr);
        m_depthImageView = VK_NULL_HANDLE;
    }
    if (m_depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_depthImage, nullptr);
        m_depthImage = VK_NULL_HANDLE;
    }
    if (m_depthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_depthImageMemory, nullptr);
        m_depthImageMemory = VK_NULL_HANDLE;
    }
}

bool GPUContext::CreateRenderPass()
{
    // Color Attachment
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth Attachment
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = m_depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependencies[2]{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

    return (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) == VK_SUCCESS);
}

bool GPUContext::CreateFramebuffers()
{
    m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

    for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            m_swapchainImageViews[i],
            m_depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS) {
            return false;
        }
    }
    return true;
}

bool GPUContext::CreateCommandPoolAndBuffers()
{
    VulkanSDL3Context& vkCtx = VulkanSDL3Context::Instance();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = vkCtx.GetQueueIndices().graphicsFamily.value();

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        return false;
    }

    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    return (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) == VK_SUCCESS);
}

bool GPUContext::CreateSyncObjects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            return false;
        }
    }
    return true;
}

bool GPUContext::CreateDescriptorSetLayouts()
{
    // Set 0: SSBO (Binding 0: InstanceData, Binding 1: BoneMatrices)
    std::array<VkDescriptorSetLayoutBinding, 2> ssboBindings{};
    ssboBindings[0].binding = 0;
    ssboBindings[0].descriptorCount = 1;
    ssboBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    ssboBindings[1].binding = 1;
    ssboBindings[1].descriptorCount = 1;
    ssboBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo ssboLayoutInfo{};
    ssboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ssboLayoutInfo.bindingCount = static_cast<uint32_t>(ssboBindings.size());
    ssboLayoutInfo.pBindings = ssboBindings.data();

    if (vkCreateDescriptorSetLayout(m_device, &ssboLayoutInfo, nullptr, &m_storageDescriptorLayout) != VK_SUCCESS) {
        return false;
    }

    // Set 1: VertUniforms (Binding 0)
    VkDescriptorSetLayoutBinding vertUboBinding{};
    vertUboBinding.binding = 0;
    vertUboBinding.descriptorCount = 1;
    vertUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    vertUboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo uboLayoutInfo{};
    uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uboLayoutInfo.bindingCount = 1;
    uboLayoutInfo.pBindings = &vertUboBinding;

    if (vkCreateDescriptorSetLayout(m_device, &uboLayoutInfo, nullptr, &m_uniformDescriptorLayout) != VK_SUCCESS) {
        return false;
    }

    // Set 2: Texture Sampler (Binding 0)
    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo samplerLayoutInfo{};
    samplerLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    samplerLayoutInfo.bindingCount = 1;
    samplerLayoutInfo.pBindings = &samplerBinding;

    if (vkCreateDescriptorSetLayout(m_device, &samplerLayoutInfo, nullptr, &m_textureDescriptorLayout) != VK_SUCCESS) {
        return false;
    }

    // Set 3: FragUniforms (Binding 0)
    VkDescriptorSetLayoutBinding fragUboBinding{};
    fragUboBinding.binding = 0;
    fragUboBinding.descriptorCount = 1;
    fragUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    fragUboBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo fragUboLayoutInfo{};
    fragUboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    fragUboLayoutInfo.bindingCount = 1;
    fragUboLayoutInfo.pBindings = &fragUboBinding;

    return (vkCreateDescriptorSetLayout(m_device, &fragUboLayoutInfo, nullptr, &m_fragUniformDescriptorLayout) == VK_SUCCESS);
}

bool GPUContext::CreateDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 4> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1000;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[1].descriptorCount = 2000;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 1000;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[3].descriptorCount = 8000;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 10000;

    return (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) == VK_SUCCESS);
}

static bool CreateBufferHelper(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, void** mappedData = nullptr)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, buffer, &memReq);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    uint32_t memTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memReq.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            memTypeIndex = i;
            break;
        }
    }
    if (memTypeIndex == UINT32_MAX) {
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (memReq.memoryTypeBits & (1 << i)) {
                memTypeIndex = i;
                break;
            }
        }
        if (memTypeIndex == UINT32_MAX) {
            vkDestroyBuffer(device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
            return false;
        }
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memTypeIndex;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        return false;
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);

    if (mappedData) {
        vkMapMemory(device, bufferMemory, 0, size, 0, mappedData);
    }
    return true;
}

uint32_t GPUContext::AllocateUBOData(FrameResource& res, const void* data, uint32_t size)
{
    if (!data || size == 0 || !res.uboMapped) return 0;
    uint32_t alignedOffset = (res.uboOffset + 255u) & ~255u;
    if (alignedOffset + size > res.uboCapacity) {
        return 0;
    }
    memcpy(static_cast<char*>(res.uboMapped) + alignedOffset, data, size);
    res.uboOffset = alignedOffset + size;
    return alignedOffset;
}

uint32_t GPUContext::AllocateIndirectData(FrameResource& res, const void* data, uint32_t size)
{
    if (!data || size == 0 || !res.indirectMapped) return UINT32_MAX;
    uint32_t alignedOffset = (res.indirectOffset + 3u) & ~3u;
    if (alignedOffset + size > res.indirectCapacity) {
        return UINT32_MAX;
    }
    memcpy(static_cast<char*>(res.indirectMapped) + alignedOffset, data, size);
    res.indirectOffset = alignedOffset + size;
    return alignedOffset;
}

bool GPUContext::CreateDynamicBuffers()
{
    VkMemoryPropertyFlags hostProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        FrameResource& res = m_frameResources[i];

        // 1. Dynamic Vertex Buffer (16 MB)
        if (!CreateBufferHelper(m_device, m_physicalDevice, res.vertexCapacity, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, hostProps, res.vertexBuffer, res.vertexMemory, &res.vertexMapped)) return false;

        // 2. Dynamic Index Buffer (4 MB)
        if (!CreateBufferHelper(m_device, m_physicalDevice, res.indexCapacity, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, hostProps, res.indexBuffer, res.indexMemory, &res.indexMapped)) return false;

        // 3. Dynamic Instance SSBO (8 MB)
        if (!CreateBufferHelper(m_device, m_physicalDevice, res.instanceSSBOCapacity, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, hostProps, res.instanceSSBO, res.instanceSSBOMemory, &res.instanceSSBOMapped)) return false;

        // 4. Dynamic Bone SSBO (8 MB)
        if (!CreateBufferHelper(m_device, m_physicalDevice, res.boneSSBOCapacity, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, hostProps, res.boneSSBO, res.boneSSBOMemory, &res.boneSSBOMapped)) return false;

        // 5. Dynamic UBO Buffer (4 MB)
        if (!CreateBufferHelper(m_device, m_physicalDevice, res.uboCapacity, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, hostProps, res.uboBuffer, res.uboMemory, &res.uboMapped)) return false;

        // 6. Dynamic Indirect Buffer (2 MB)
        if (!CreateBufferHelper(m_device, m_physicalDevice, res.indirectCapacity, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, hostProps, res.indirectBuffer, res.indirectMemory, &res.indirectMapped)) return false;

        // Allocate Descriptor Sets for this frame
        // Set 0 (SSBO)
        VkDescriptorSetAllocateInfo dsAlloc0{};
        dsAlloc0.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAlloc0.descriptorPool = m_descriptorPool;
        dsAlloc0.descriptorSetCount = 1;
        dsAlloc0.pSetLayouts = &m_storageDescriptorLayout;
        vkAllocateDescriptorSets(m_device, &dsAlloc0, &res.storageDescriptorSet);

        VkDescriptorBufferInfo ssboInfo0{};
        ssboInfo0.buffer = res.instanceSSBO;
        ssboInfo0.offset = 0;
        ssboInfo0.range = res.instanceSSBOCapacity;

        VkDescriptorBufferInfo ssboInfo1{};
        ssboInfo1.buffer = res.boneSSBO;
        ssboInfo1.offset = 0;
        ssboInfo1.range = res.boneSSBOCapacity;

        std::array<VkWriteDescriptorSet, 2> writes0{};
        writes0[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes0[0].dstSet = res.storageDescriptorSet;
        writes0[0].dstBinding = 0;
        writes0[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes0[0].descriptorCount = 1;
        writes0[0].pBufferInfo = &ssboInfo0;

        writes0[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes0[1].dstSet = res.storageDescriptorSet;
        writes0[1].dstBinding = 1;
        writes0[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes0[1].descriptorCount = 1;
        writes0[1].pBufferInfo = &ssboInfo1;

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes0.size()), writes0.data(), 0, nullptr);

        // Set 1 (VertUniforms - Dynamic UBO)
        VkDescriptorSetAllocateInfo dsAlloc1{};
        dsAlloc1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAlloc1.descriptorPool = m_descriptorPool;
        dsAlloc1.descriptorSetCount = 1;
        dsAlloc1.pSetLayouts = &m_uniformDescriptorLayout;
        vkAllocateDescriptorSets(m_device, &dsAlloc1, &res.uniformDescriptorSet);

        VkDescriptorBufferInfo uboInfo1{};
        uboInfo1.buffer = res.uboBuffer;
        uboInfo1.offset = 0;
        uboInfo1.range = 2048; // Must cover TerrainVertUBO (1168 bytes)

        VkWriteDescriptorSet write1{};
        write1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write1.dstSet = res.uniformDescriptorSet;
        write1.dstBinding = 0;
        write1.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write1.descriptorCount = 1;
        write1.pBufferInfo = &uboInfo1;

        vkUpdateDescriptorSets(m_device, 1, &write1, 0, nullptr);

        // Set 3 (FragUniforms - Dynamic UBO)
        VkDescriptorSetAllocateInfo dsAlloc3{};
        dsAlloc3.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAlloc3.descriptorPool = m_descriptorPool;
        dsAlloc3.descriptorSetCount = 1;
        dsAlloc3.pSetLayouts = &m_fragUniformDescriptorLayout;
        vkAllocateDescriptorSets(m_device, &dsAlloc3, &res.fragUniformDescriptorSet);

        VkDescriptorBufferInfo uboInfo3{};
        uboInfo3.buffer = res.uboBuffer;
        uboInfo3.offset = 0;
        uboInfo3.range = 256;

        VkWriteDescriptorSet write3{};
        write3.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write3.dstSet = res.fragUniformDescriptorSet;
        write3.dstBinding = 0;
        write3.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write3.descriptorCount = 1;
        write3.pBufferInfo = &uboInfo3;

        vkUpdateDescriptorSets(m_device, 1, &write3, 0, nullptr);
    }
    return true;
}

void GPUContext::DestroyDynamicBuffers()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        FrameResource& res = m_frameResources[i];

        if (res.vertexMapped) { vkUnmapMemory(m_device, res.vertexMemory); res.vertexMapped = nullptr; }
        if (res.vertexBuffer) { vkDestroyBuffer(m_device, res.vertexBuffer, nullptr); res.vertexBuffer = VK_NULL_HANDLE; }
        if (res.vertexMemory) { vkFreeMemory(m_device, res.vertexMemory, nullptr); res.vertexMemory = VK_NULL_HANDLE; }

        if (res.indexMapped) { vkUnmapMemory(m_device, res.indexMemory); res.indexMapped = nullptr; }
        if (res.indexBuffer) { vkDestroyBuffer(m_device, res.indexBuffer, nullptr); res.indexBuffer = VK_NULL_HANDLE; }
        if (res.indexMemory) { vkFreeMemory(m_device, res.indexMemory, nullptr); res.indexMemory = VK_NULL_HANDLE; }

        if (res.instanceSSBOMapped) { vkUnmapMemory(m_device, res.instanceSSBOMemory); res.instanceSSBOMapped = nullptr; }
        if (res.instanceSSBO) { vkDestroyBuffer(m_device, res.instanceSSBO, nullptr); res.instanceSSBO = VK_NULL_HANDLE; }
        if (res.instanceSSBOMemory) { vkFreeMemory(m_device, res.instanceSSBOMemory, nullptr); res.instanceSSBOMemory = VK_NULL_HANDLE; }

        if (res.boneSSBOMapped) { vkUnmapMemory(m_device, res.boneSSBOMemory); res.boneSSBOMapped = nullptr; }
        if (res.boneSSBO) { vkDestroyBuffer(m_device, res.boneSSBO, nullptr); res.boneSSBO = VK_NULL_HANDLE; }
        if (res.boneSSBOMemory) { vkFreeMemory(m_device, res.boneSSBOMemory, nullptr); res.boneSSBOMemory = VK_NULL_HANDLE; }

        if (res.uboMapped) { vkUnmapMemory(m_device, res.uboMemory); res.uboMapped = nullptr; }
        if (res.uboBuffer) { vkDestroyBuffer(m_device, res.uboBuffer, nullptr); res.uboBuffer = VK_NULL_HANDLE; }
        if (res.uboMemory) { vkFreeMemory(m_device, res.uboMemory, nullptr); res.uboMemory = VK_NULL_HANDLE; }

        if (res.indirectMapped) { vkUnmapMemory(m_device, res.indirectMemory); res.indirectMapped = nullptr; }
        if (res.indirectBuffer) { vkDestroyBuffer(m_device, res.indirectBuffer, nullptr); res.indirectBuffer = VK_NULL_HANDLE; }
        if (res.indirectMemory) { vkFreeMemory(m_device, res.indirectMemory, nullptr); res.indirectMemory = VK_NULL_HANDLE; }
    }
}

bool GPUContext::CreateUnitQuads()
{
    DestroyUnitQuads();

    VkMemoryPropertyFlags hostProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    // 1. Sprite Quad VBO (16 bytes stride: float2 pos, float2 uv)
    struct SpriteQuadVertex { float pos[2]; float uv[2]; };
    SpriteQuadVertex spriteVerts[4] = {
        { {-0.5f, -0.5f}, {0.0f, 0.0f} },
        { { 0.5f, -0.5f}, {1.0f, 0.0f} },
        { { 0.5f,  0.5f}, {1.0f, 1.0f} },
        { {-0.5f,  0.5f}, {0.0f, 1.0f} }
    };
    uint32_t quadIndices[6] = { 0, 1, 2, 0, 2, 3 };

    void* mapped = nullptr;
    if (!CreateBufferHelper(m_device, m_physicalDevice, sizeof(spriteVerts), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, hostProps, m_spriteQuadVBO, m_spriteQuadVBOMemory, &mapped)) return false;
    memcpy(mapped, spriteVerts, sizeof(spriteVerts));
    vkUnmapMemory(m_device, m_spriteQuadVBOMemory);

    if (!CreateBufferHelper(m_device, m_physicalDevice, sizeof(quadIndices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, hostProps, m_spriteQuadIBO, m_spriteQuadIBOMemory, &mapped)) return false;
    memcpy(mapped, quadIndices, sizeof(quadIndices));
    vkUnmapMemory(m_device, m_spriteQuadIBOMemory);

    // 2. Image Quad VBO (8 bytes stride: float2 aPos01)
    struct ImageQuadVertex { float pos[2]; };
    ImageQuadVertex imageVerts[4] = {
        { {0.0f, 0.0f} },
        { {0.0f, 1.0f} },
        { {1.0f, 1.0f} },
        { {1.0f, 0.0f} }
    };

    if (!CreateBufferHelper(m_device, m_physicalDevice, sizeof(imageVerts), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, hostProps, m_imageQuadVBO, m_imageQuadVBOMemory, &mapped)) return false;
    memcpy(mapped, imageVerts, sizeof(imageVerts));
    vkUnmapMemory(m_device, m_imageQuadVBOMemory);

    if (!CreateBufferHelper(m_device, m_physicalDevice, sizeof(quadIndices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, hostProps, m_imageQuadIBO, m_imageQuadIBOMemory, &mapped)) return false;
    memcpy(mapped, quadIndices, sizeof(quadIndices));
    vkUnmapMemory(m_device, m_imageQuadIBOMemory);

    return true;
}

void GPUContext::DestroyUnitQuads()
{
    if (m_spriteQuadVBO) { vkDestroyBuffer(m_device, m_spriteQuadVBO, nullptr); m_spriteQuadVBO = VK_NULL_HANDLE; }
    if (m_spriteQuadVBOMemory) { vkFreeMemory(m_device, m_spriteQuadVBOMemory, nullptr); m_spriteQuadVBOMemory = VK_NULL_HANDLE; }
    if (m_spriteQuadIBO) { vkDestroyBuffer(m_device, m_spriteQuadIBO, nullptr); m_spriteQuadIBO = VK_NULL_HANDLE; }
    if (m_spriteQuadIBOMemory) { vkFreeMemory(m_device, m_spriteQuadIBOMemory, nullptr); m_spriteQuadIBOMemory = VK_NULL_HANDLE; }

    if (m_imageQuadVBO) { vkDestroyBuffer(m_device, m_imageQuadVBO, nullptr); m_imageQuadVBO = VK_NULL_HANDLE; }
    if (m_imageQuadVBOMemory) { vkFreeMemory(m_device, m_imageQuadVBOMemory, nullptr); m_imageQuadVBOMemory = VK_NULL_HANDLE; }
    if (m_imageQuadIBO) { vkDestroyBuffer(m_device, m_imageQuadIBO, nullptr); m_imageQuadIBO = VK_NULL_HANDLE; }
    if (m_imageQuadIBOMemory) { vkFreeMemory(m_device, m_imageQuadIBOMemory, nullptr); m_imageQuadIBOMemory = VK_NULL_HANDLE; }
}

enum BlendMode { BLEND_NONE, BLEND_ALPHA, BLEND_ADD, BLEND_DARK };

static VkPipeline CreatePipelineHelper(
    VkDevice device,
    VkRenderPass renderPass,
    VkPipelineLayout layout,
    VkShaderModule vertShader,
    VkShaderModule fragShader,
    const VkVertexInputBindingDescription* bindingDescs,
    uint32_t bindingCount,
    const VkVertexInputAttributeDescription* attrDescs,
    uint32_t attrCount,
    BlendMode blendMode,
    bool depthTest,
    bool depthWrite,
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT,
    bool is2D = false,
    const VkSpecializationInfo* fragSpecInfo = nullptr)
{
    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShader;
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShader;
    shaderStages[1].pName = "main";
    shaderStages[1].pSpecializationInfo = fragSpecInfo;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = bindingCount;
    vertexInputInfo.pVertexBindingDescriptions = bindingDescs;
    vertexInputInfo.vertexAttributeDescriptionCount = attrCount;
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = cullMode;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    if (blendMode == BLEND_ALPHA) {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    } else if (blendMode == BLEND_ADD) {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    } else if (blendMode == BLEND_DARK) {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    } else {
        colorBlendAttachment.blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
    return pipeline;
}

bool GPUContext::CreatePipelines()
{
    // =========================================================================
    // 1. Terrain Pipelines
    // =========================================================================
    {
        std::array<VkDescriptorSetLayout, 3> setLayouts = {
            m_storageDescriptorLayout, // Set 0
            m_uniformDescriptorLayout, // Set 1
            m_textureDescriptorLayout  // Set 2
        };
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();
        vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_terrainPipelineLayout);

        auto vertCode = ReadSPVFile("terrain.vert.spv");
        auto fragCode = ReadSPVFile("terrain.frag.spv");
        VkShaderModule vertModule = CreateShaderModule(vertCode);
        VkShaderModule fragModule = CreateShaderModule(fragCode);

        // Vertex layout: float3 pos, float2 uv, uint color (RGBA8) -> 24 bytes
        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(float) * 5 + sizeof(uint32_t);
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 3> attrDescs{};
        attrDescs[0].binding = 0; attrDescs[0].location = 0; attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT; attrDescs[0].offset = 0;
        attrDescs[1].binding = 0; attrDescs[1].location = 1; attrDescs[1].format = VK_FORMAT_R32G32_SFLOAT; attrDescs[1].offset = sizeof(float) * 3;
        attrDescs[2].binding = 0; attrDescs[2].location = 2; attrDescs[2].format = VK_FORMAT_R8G8B8A8_UNORM; attrDescs[2].offset = sizeof(float) * 5;

        VkSpecializationMapEntry specEntry{};
        specEntry.constantID = 0;
        specEntry.offset = 0;
        specEntry.size = sizeof(VkBool32);

        VkBool32 lightTrue = VK_TRUE;
        VkSpecializationInfo specInfoWithLight{};
        specInfoWithLight.mapEntryCount = 1;
        specInfoWithLight.pMapEntries = &specEntry;
        specInfoWithLight.dataSize = sizeof(VkBool32);
        specInfoWithLight.pData = &lightTrue;

        VkBool32 lightFalse = VK_FALSE;
        VkSpecializationInfo specInfoNoLight{};
        specInfoNoLight.mapEntryCount = 1;
        specInfoNoLight.pMapEntries = &specEntry;
        specInfoNoLight.dataSize = sizeof(VkBool32);
        specInfoNoLight.pData = &lightFalse;

        m_terrainPipelineOpaque = CreatePipelineHelper(m_device, m_renderPass, m_terrainPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_NONE, true, true, VK_CULL_MODE_NONE, false, &specInfoWithLight);
        m_terrainPipelineBlend = CreatePipelineHelper(m_device, m_renderPass, m_terrainPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_ALPHA, true, false, VK_CULL_MODE_NONE, false, &specInfoWithLight);
        m_terrainPipelineAdd = CreatePipelineHelper(m_device, m_renderPass, m_terrainPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_ADD, true, false, VK_CULL_MODE_NONE, false, &specInfoNoLight);
        m_terrainPipelineDark = CreatePipelineHelper(m_device, m_renderPass, m_terrainPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_DARK, true, false, VK_CULL_MODE_NONE, false, &specInfoNoLight);
        m_terrainPipelineBlendNoDepth = CreatePipelineHelper(m_device, m_renderPass, m_terrainPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_ALPHA, false, false, VK_CULL_MODE_NONE, false, &specInfoWithLight);
        m_terrainPipelineAddNoDepth = CreatePipelineHelper(m_device, m_renderPass, m_terrainPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_ADD, false, false, VK_CULL_MODE_NONE, false, &specInfoNoLight);

        if (vertModule) vkDestroyShaderModule(m_device, vertModule, nullptr);
        if (fragModule) vkDestroyShaderModule(m_device, fragModule, nullptr);
    }

    // =========================================================================
    // 2. 3D Model / BMD Mesh Pipelines
    // =========================================================================
    {
        std::array<VkDescriptorSetLayout, 4> setLayouts = {
            m_storageDescriptorLayout,     // Set 0: SSBO (Instance + Bone)
            m_uniformDescriptorLayout,     // Set 1: VertUniforms
            m_textureDescriptorLayout,     // Set 2: Texture Sampler
            m_fragUniformDescriptorLayout  // Set 3: FragUniforms
        };
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();
        vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_meshPipelineLayout);

        auto vertCode = ReadSPVFile("general.vert.spv");
        auto fragCode = ReadSPVFile("general.frag.spv");
        VkShaderModule vertModule = CreateShaderModule(vertCode);
        VkShaderModule fragModule = CreateShaderModule(fragCode);

        // VertexBMD: pos (float3), normal (float3), texCoord (float2), bonePacked (uint) -> 36 bytes
        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = 36;
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 4> attrDescs{};
        attrDescs[0].binding = 0; attrDescs[0].location = 0; attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT; attrDescs[0].offset = 0;
        attrDescs[1].binding = 0; attrDescs[1].location = 1; attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT; attrDescs[1].offset = 12;
        attrDescs[2].binding = 0; attrDescs[2].location = 2; attrDescs[2].format = VK_FORMAT_R32G32_SFLOAT; attrDescs[2].offset = 24;
        attrDescs[3].binding = 0; attrDescs[3].location = 3; attrDescs[3].format = VK_FORMAT_R32_UINT; attrDescs[3].offset = 32;

        m_meshPipelineOpaque = CreatePipelineHelper(m_device, m_renderPass, m_meshPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_NONE, true, true, VK_CULL_MODE_NONE);
        m_meshPipelineAlphaBlend = CreatePipelineHelper(m_device, m_renderPass, m_meshPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_ALPHA, true, false, VK_CULL_MODE_NONE);
        m_meshPipelineAdditive = CreatePipelineHelper(m_device, m_renderPass, m_meshPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_ADD, true, false, VK_CULL_MODE_NONE);
        m_meshPipelineAlphaTest = CreatePipelineHelper(m_device, m_renderPass, m_meshPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_NONE, true, true, VK_CULL_MODE_NONE);
        m_meshPipelineDark = CreatePipelineHelper(m_device, m_renderPass, m_meshPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_DARK, true, false, VK_CULL_MODE_NONE);

        if (vertModule) vkDestroyShaderModule(m_device, vertModule, nullptr);
        if (fragModule) vkDestroyShaderModule(m_device, fragModule, nullptr);
    }

    // =========================================================================
    // 3. Shadow Pipeline
    // =========================================================================
    {
        std::array<VkDescriptorSetLayout, 2> setLayouts = {
            m_storageDescriptorLayout, // Set 0
            m_uniformDescriptorLayout  // Set 1
        };
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();
        vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_shadowPipelineLayout);

        auto vertCode = ReadSPVFile("shadow.vert.spv");
        auto fragCode = ReadSPVFile("shadow.frag.spv");
        VkShaderModule vertModule = CreateShaderModule(vertCode);
        VkShaderModule fragModule = CreateShaderModule(fragCode);

        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = 36;
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 4> attrDescs{};
        attrDescs[0].binding = 0; attrDescs[0].location = 0; attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT; attrDescs[0].offset = 0;
        attrDescs[1].binding = 0; attrDescs[1].location = 1; attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT; attrDescs[1].offset = 12;
        attrDescs[2].binding = 0; attrDescs[2].location = 2; attrDescs[2].format = VK_FORMAT_R32G32_SFLOAT; attrDescs[2].offset = 24;
        attrDescs[3].binding = 0; attrDescs[3].location = 3; attrDescs[3].format = VK_FORMAT_R32_UINT; attrDescs[3].offset = 32;

        m_shadowPipeline = CreatePipelineHelper(m_device, m_renderPass, m_shadowPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_ALPHA, true, false, VK_CULL_MODE_NONE);

        if (vertModule) vkDestroyShaderModule(m_device, vertModule, nullptr);
        if (fragModule) vkDestroyShaderModule(m_device, fragModule, nullptr);
    }

    // =========================================================================
    // 4. Sprite & Particle Pipelines
    // =========================================================================
    {
        std::array<VkDescriptorSetLayout, 3> setLayouts = {
            m_storageDescriptorLayout, // Set 0
            m_uniformDescriptorLayout, // Set 1
            m_textureDescriptorLayout  // Set 2
        };
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();
        vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_spritePipelineLayout);

        auto vertCode = ReadSPVFile("sprite.vert.spv");
        auto fragCode = ReadSPVFile("sprite.frag.spv");
        VkShaderModule vertModule = CreateShaderModule(vertCode);
        VkShaderModule fragModule = CreateShaderModule(fragCode);

        // Quad corners: float2 pos, float2 uv -> 16 bytes
        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = 16;
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 2> attrDescs{};
        attrDescs[0].binding = 0; attrDescs[0].location = 0; attrDescs[0].format = VK_FORMAT_R32G32_SFLOAT; attrDescs[0].offset = 0;
        attrDescs[1].binding = 0; attrDescs[1].location = 1; attrDescs[1].format = VK_FORMAT_R32G32_SFLOAT; attrDescs[1].offset = 8;

        m_spritePipelineAlpha = CreatePipelineHelper(m_device, m_renderPass, m_spritePipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_ALPHA, true, false, VK_CULL_MODE_NONE);
        m_spritePipelineAdd = CreatePipelineHelper(m_device, m_renderPass, m_spritePipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_ADD, true, false, VK_CULL_MODE_NONE);

        if (vertModule) vkDestroyShaderModule(m_device, vertModule, nullptr);
        if (fragModule) vkDestroyShaderModule(m_device, fragModule, nullptr);
    }

    // =========================================================================
    // 5. 2D UI / Image Pipeline
    // =========================================================================
    {
        std::array<VkDescriptorSetLayout, 3> setLayouts = {
            m_storageDescriptorLayout, // Set 0
            m_uniformDescriptorLayout, // Set 1
            m_textureDescriptorLayout  // Set 2
        };
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();
        vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_uiPipelineLayout);

        auto vertCode = ReadSPVFile("image.vert.spv");
        auto fragCode = ReadSPVFile("image.frag.spv");
        VkShaderModule vertModule = CreateShaderModule(vertCode);
        VkShaderModule fragModule = CreateShaderModule(fragCode);

        // aPos01: float2 -> 8 bytes
        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = 8;
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attrDesc{};
        attrDesc.binding = 0; attrDesc.location = 0; attrDesc.format = VK_FORMAT_R32G32_SFLOAT; attrDesc.offset = 0;

        m_uiPipeline = CreatePipelineHelper(m_device, m_renderPass, m_uiPipelineLayout, vertModule, fragModule, &bindingDesc, 1, &attrDesc, 1, BLEND_ALPHA, false, false, VK_CULL_MODE_NONE, true);
        m_uiPipelineAdd = CreatePipelineHelper(m_device, m_renderPass, m_uiPipelineLayout, vertModule, fragModule, &bindingDesc, 1, &attrDesc, 1, BLEND_ADD, false, false, VK_CULL_MODE_NONE, true);
        m_uiPipelineNone = CreatePipelineHelper(m_device, m_renderPass, m_uiPipelineLayout, vertModule, fragModule, &bindingDesc, 1, &attrDesc, 1, BLEND_NONE, false, false, VK_CULL_MODE_NONE, true);
        m_uiPipelineDark = CreatePipelineHelper(m_device, m_renderPass, m_uiPipelineLayout, vertModule, fragModule, &bindingDesc, 1, &attrDesc, 1, BLEND_DARK, false, false, VK_CULL_MODE_NONE, true);

        if (vertModule) vkDestroyShaderModule(m_device, vertModule, nullptr);
        if (fragModule) vkDestroyShaderModule(m_device, fragModule, nullptr);
    }

    // =========================================================================
    // 6. Cloth Pipeline
    // =========================================================================
    {
        std::array<VkDescriptorSetLayout, 2> setLayouts = {
            m_uniformDescriptorLayout, // Set 1
            m_textureDescriptorLayout  // Set 2
        };
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();
        vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_clothPipelineLayout);

        auto vertCode = ReadSPVFile("cloth.vert.spv");
        auto fragCode = ReadSPVFile("cloth.frag.spv");
        VkShaderModule vertModule = CreateShaderModule(vertCode);
        VkShaderModule fragModule = CreateShaderModule(fragCode);

        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = 20;
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 2> attrDescs{};
        attrDescs[0].binding = 0; attrDescs[0].location = 0; attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT; attrDescs[0].offset = 0;
        attrDescs[1].binding = 0; attrDescs[1].location = 1; attrDescs[1].format = VK_FORMAT_R32G32_SFLOAT; attrDescs[1].offset = 12;

        m_clothPipeline = CreatePipelineHelper(m_device, m_renderPass, m_clothPipelineLayout, vertModule, fragModule, &bindingDesc, 1, attrDescs.data(), (uint32_t)attrDescs.size(), BLEND_ALPHA, true, true, VK_CULL_MODE_NONE);

        if (vertModule) vkDestroyShaderModule(m_device, vertModule, nullptr);
        if (fragModule) vkDestroyShaderModule(m_device, fragModule, nullptr);
    }

    return true;
}



bool GPUContext::BeginFrame()
{
    if (!m_initialized) return false;

    if (m_frameActive) {
        EndFrame();
    }

    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        return false;
    }

    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

    // Reset frame dynamic buffer allocations
    FrameResource& res = m_frameResources[m_currentFrame];
    res.vertexOffset = 0;
    res.indexOffset = 0;
    res.instanceSSBOOffset = 0;
    res.boneSSBOOffset = 0;
    res.uboOffset = 0;
    res.indirectOffset = 0;
    m_dynamicLights.clear();

    if (VulkanTextureManager::Instance().HasPendingUploads()) {
        VulkanTextureManager::Instance().FlushUploadQueue();
    }

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    m_frameActive = true;
    m_renderPassActive = false;
    m_drawCallsThisFrame = 0;
    VulkanTextureManager::Instance().ResetFontAtlas();
    m_deferredCommands.clear();
    m_deferredTerrainDraws.clear();
    m_deferredMeshDraws.clear();
    m_deferredSpriteDraws.clear();
    m_deferredImageDraws.clear();
    if (m_deferredCommands.capacity() < 512) m_deferredCommands.reserve(512);
    if (m_deferredTerrainDraws.capacity() < 64) m_deferredTerrainDraws.reserve(64);
    if (m_deferredMeshDraws.capacity() < 256) m_deferredMeshDraws.reserve(256);
    if (m_deferredSpriteDraws.capacity() < 256) m_deferredSpriteDraws.reserve(256);
    if (m_deferredImageDraws.capacity() < 256) m_deferredImageDraws.reserve(256);
    return true;
}

void GPUContext::EndFrame()
{
    if (!m_frameActive) return;

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    if (m_renderPassActive) {
        vkCmdEndRenderPass(cmd);
        m_renderPassActive = false;
    }

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        std::cerr << "[GPUContext] Failed to record command buffer!" << std::endl;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
        std::cerr << "[GPUContext] Failed to submit draw command buffer!" << std::endl;
    }

    m_frameActive = false;
}

bool GPUContext::Present()
{
    if (!m_initialized || m_swapchain == VK_NULL_HANDLE) {
        return false;
    }

    if (!m_frameActive) {
        if (!BeginFrame()) {
            return false;
        }
    }

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = m_renderPass;
    rpInfo.framebuffer = m_swapchainFramebuffers[m_imageIndex];
    rpInfo.renderArea.offset = { 0, 0 };
    rpInfo.renderArea.extent = m_swapchainExtent;
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{ m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3] }};
    clearValues[1].depthStencil = { 1.0f, 0 };
    rpInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    rpInfo.pClearValues = clearValues.data();

    if (VulkanTextureManager::Instance().IsFontAtlasDirty()) {
        VulkanTextureManager::Instance().FlushFontAtlas(cmd);
    }

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    m_renderPassActive = true;

    // Flush all pending batches from BatchRenderer
    CBatchRenderer::Instance().FlushAllBatches();

    // Replay all deferred Vulkan commands in exact chronological order
    ReplayDeferredCommands();

    if (m_renderPassActive) {
        vkCmdEndRenderPass(cmd);
        m_renderPassActive = false;
    }

    bool captureThisFrame = m_screenshotRequested;
    if (captureThisFrame) {
        CaptureScreenshotInternal(cmd);
    }

    EndFrame();

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { m_swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &m_imageIndex;

    VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapchain();
    }

    if (captureThisFrame) {
        vkQueueWaitIdle(m_graphicsQueue);
        SaveScreenshotToFile();
        m_screenshotRequested = false;
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return (result == VK_SUCCESS);
}

void GPUContext::RequestScreenshot(const std::string& filename)
{
    m_screenshotRequested = true;
    m_screenshotFilename = filename;
}

void GPUContext::CaptureScreenshotInternal(VkCommandBuffer cmd)
{
    uint32_t width = m_swapchainExtent.width;
    uint32_t height = m_swapchainExtent.height;
    uint32_t requiredSize = width * height * 4;

    if (m_screenshotBuffer == VK_NULL_HANDLE || m_screenshotBufferSize < requiredSize)
    {
        if (m_screenshotBuffer != VK_NULL_HANDLE) {
            if (m_screenshotMapped) {
                vkUnmapMemory(m_device, m_screenshotMemory);
                m_screenshotMapped = nullptr;
            }
            vkDestroyBuffer(m_device, m_screenshotBuffer, nullptr);
            vkFreeMemory(m_device, m_screenshotMemory, nullptr);
            m_screenshotBuffer = VK_NULL_HANDLE;
            m_screenshotMemory = VK_NULL_HANDLE;
        }

        m_screenshotBufferSize = requiredSize;
        if (!CreateBufferHelper(m_device, m_physicalDevice, requiredSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_screenshotBuffer, m_screenshotMemory, &m_screenshotMapped))
        {
            g_ErrorReport.Write("[GPUContext] Failed to create screenshot staging buffer!\r\n");
            return;
        }
    }

    // Transition swapchain image from PRESENT_SRC_KHR to TRANSFER_SRC_OPTIMAL
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_swapchainImages[m_imageIndex];
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    // Copy swapchain image to staging buffer
    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageOffset = { 0, 0, 0 };
    copyRegion.imageExtent = { width, height, 1 };

    vkCmdCopyImageToBuffer(
        cmd,
        m_swapchainImages[m_imageIndex],
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        m_screenshotBuffer,
        1,
        &copyRegion
    );

    // Transition swapchain image back to PRESENT_SRC_KHR
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

void GPUContext::SaveScreenshotToFile()
{
    if (!m_screenshotMapped || m_screenshotFilename.empty())
        return;

    int width = static_cast<int>(m_swapchainExtent.width);
    int height = static_cast<int>(m_swapchainExtent.height);
    if (width <= 0 || height <= 0)
        return;

    // Ensure destination directory exists
    std::string path = m_screenshotFilename;
    size_t slashPos = path.find_last_of("\\/");
    if (slashPos != std::string::npos) {
        std::string dir = path.substr(0, slashPos);
        CreateDirectoryA(dir.c_str(), NULL);
    }

    std::vector<unsigned char> rgbBuffer(width * height * 3);
    const uint8_t* src = static_cast<const uint8_t*>(m_screenshotMapped);

    // WriteJpeg expects rows from bottom to top (due to OpenGL legacy scanline invert):
    // while (cinfo.next_scanline < cinfo.image_height) {
    //     row_pointer[0] = &Buffer[(Height - 1 - cinfo.next_scanline) * row_stride];
    // }
    // Flipping Y here ensures the output image is upright and properly oriented.
    for (int y = 0; y < height; ++y) {
        int dstY = height - 1 - y;
        for (int x = 0; x < width; ++x) {
            int srcIdx = (y * width + x) * 4;
            int dstIdx = (dstY * width + x) * 3;
            if (m_swapchainImageFormat == VK_FORMAT_B8G8R8A8_UNORM || m_swapchainImageFormat == VK_FORMAT_B8G8R8A8_SRGB) {
                rgbBuffer[dstIdx + 0] = src[srcIdx + 2]; // R
                rgbBuffer[dstIdx + 1] = src[srcIdx + 1]; // G
                rgbBuffer[dstIdx + 2] = src[srcIdx + 0]; // B
            } else {
                rgbBuffer[dstIdx + 0] = src[srcIdx + 0]; // R
                rgbBuffer[dstIdx + 1] = src[srcIdx + 1]; // G
                rgbBuffer[dstIdx + 2] = src[srcIdx + 2]; // B
            }
        }
    }

    WriteJpeg(const_cast<char*>(m_screenshotFilename.c_str()), width, height, rgbBuffer.data(), 100);
}

void GPUContext::OnResize(int width, int height)
{
    if (width > 0 && height > 0) {
        m_width = width;
        m_height = height;
        RecreateSwapchain();
    }
}

void GPUContext::SetViewport(float x, float y, float width, float height)
{
    if (!m_frameActive) return;
    VkViewport vp{ x, y, width, height, 0.0f, 1.0f };
    vkCmdSetViewport(m_commandBuffers[m_currentFrame], 0, 1, &vp);
}

void GPUContext::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    if (!m_frameActive) return;
    VkRect2D sc{ {x, y}, {width, height} };
    vkCmdSetScissor(m_commandBuffers[m_currentFrame], 0, 1, &sc);
}

void GPUContext::SetRenderState(const GPURenderState& state)
{
}

TerrainVertex_t* GPUContext::AllocateTerrainVertexBuffer(uint32_t vertexCount, uint32_t& outVertOffset)
{
    if (!m_frameActive || vertexCount == 0) return nullptr;
    FrameResource& res = m_frameResources[m_currentFrame];

    uint32_t vertByteSize = vertexCount * sizeof(TerrainVertex_t);
    if (res.vertexOffset + vertByteSize > res.vertexCapacity) return nullptr;

    outVertOffset = res.vertexOffset;
    res.vertexOffset += vertByteSize;

    return reinterpret_cast<TerrainVertex_t*>(static_cast<char*>(res.vertexMapped) + outVertOffset);
}

uint32_t* GPUContext::AllocateTerrainIndexBuffer(uint32_t indexCount, uint32_t& outIdxOffset)
{
    if (!m_frameActive || indexCount == 0) return nullptr;
    FrameResource& res = m_frameResources[m_currentFrame];

    uint32_t idxByteSize = indexCount * sizeof(uint32_t);
    if (res.indexOffset + idxByteSize > res.indexCapacity) return nullptr;

    outIdxOffset = res.indexOffset;
    res.indexOffset += idxByteSize;

    return reinterpret_cast<uint32_t*>(static_cast<char*>(res.indexMapped) + outIdxOffset);
}

void GPUContext::DrawTerrainMergedPreallocated(uint32_t vertOffset, uint32_t vertByteSize, uint32_t idxOffset, uint32_t idxByteSize, const std::vector<TerrainMergedBatch>& batches, const TerrainVertUBO& ubo)
{
    if (!m_frameActive || vertByteSize == 0 || idxByteSize == 0 || batches.empty()) return;

    DeferredTerrainDraw draw = {};
    draw.vertOffset = vertOffset;
    draw.vertByteSize = vertByteSize;
    draw.idxOffset = idxOffset;
    draw.idxByteSize = idxByteSize;
    draw.batches = batches;
    draw.ubo = ubo;
    m_deferredTerrainDraws.push_back(std::move(draw));

    DeferredCommand dcmd{};
    dcmd.type = DeferredCmdType::Terrain;
    dcmd.index = static_cast<uint32_t>(m_deferredTerrainDraws.size() - 1);
    m_deferredCommands.push_back(dcmd);
}

void GPUContext::DrawTerrainMerged(const void* vertices, uint32_t vertByteSize, const void* indices, uint32_t indexByteSize, const std::vector<TerrainMergedBatch>& batches, const TerrainVertUBO& ubo)
{
    if (!m_frameActive || !vertices || vertByteSize == 0 || !indices || indexByteSize == 0 || batches.empty()) return;

    uint32_t vertCount = vertByteSize / sizeof(TerrainVertex_t);
    uint32_t idxCount = indexByteSize / sizeof(uint32_t);

    uint32_t vertOffset = 0;
    uint32_t idxOffset = 0;
    TerrainVertex_t* dstVerts = AllocateTerrainVertexBuffer(vertCount, vertOffset);
    uint32_t* dstIndices = AllocateTerrainIndexBuffer(idxCount, idxOffset);
    if (!dstVerts || !dstIndices) return;

    memcpy(dstVerts, vertices, vertByteSize);
    memcpy(dstIndices, indices, indexByteSize);

    DrawTerrainMergedPreallocated(vertOffset, vertByteSize, idxOffset, indexByteSize, batches, ubo);
}

void GPUContext::ReplaySingleTerrainDraw(const DeferredTerrainDraw& draw)
{
    FrameResource& res = m_frameResources[m_currentFrame];
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    // Upload UBO with dynamic offset
    uint32_t uboOffset = AllocateUBOData(res, &draw.ubo, sizeof(TerrainVertUBO));

    // Bind Buffers
    VkDeviceSize vOffset = draw.vertOffset;
    vkCmdBindVertexBuffers(cmd, 0, 1, &res.vertexBuffer, &vOffset);
    vkCmdBindIndexBuffer(cmd, res.indexBuffer, draw.idxOffset, VK_INDEX_TYPE_UINT32);

    // Bind Set 0 (SSBO) and Set 1 (VertUniforms UBO)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_terrainPipelineLayout, 0, 1, &res.storageDescriptorSet, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_terrainPipelineLayout, 1, 1, &res.uniformDescriptorSet, 1, &uboOffset);

    VkPipeline lastPipeline = VK_NULL_HANDLE;
    int lastTexId = -1;

    for (const auto& batch : draw.batches) {
        VkPipeline pipeline = m_terrainPipelineOpaque;
        const bool noDepth = (batch.renderFlags & 1) != 0 || (batch.batchType == 6) || (batch.batchType == 7);
        if (batch.batchType == 1 || batch.batchType == 4 || batch.batchType == 7) {
            pipeline = (noDepth && m_terrainPipelineAddNoDepth != VK_NULL_HANDLE) ? m_terrainPipelineAddNoDepth : m_terrainPipelineAdd;
        } else if (batch.batchType == 5) {
            pipeline = (m_terrainPipelineDark != VK_NULL_HANDLE) ? m_terrainPipelineDark : ((noDepth && m_terrainPipelineBlendNoDepth != VK_NULL_HANDLE) ? m_terrainPipelineBlendNoDepth : m_terrainPipelineBlend);
        } else if (batch.batchType == 2 || batch.batchType == 3 || batch.batchType == 6) {
            pipeline = (noDepth && m_terrainPipelineBlendNoDepth != VK_NULL_HANDLE) ? m_terrainPipelineBlendNoDepth : m_terrainPipelineBlend;
        }
        if (pipeline != lastPipeline && pipeline != VK_NULL_HANDLE) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            lastPipeline = pipeline;
        }

        if (batch.textureIndex != lastTexId) {
            VkDescriptorSet texSet = VulkanTextureManager::Instance().GetDescriptorSet(batch.textureIndex);
            if (texSet == VK_NULL_HANDLE) texSet = VulkanTextureManager::Instance().GetDefaultDescriptorSet();
            if (texSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_terrainPipelineLayout, 2, 1, &texSet, 0, nullptr);
            }
            lastTexId = batch.textureIndex;
        }

        // Issue draw commands (Indirect when supported, fallback to direct merged)
        if (!batch.cmds.empty()) {
#if CBMu_ENABLE_VK_INDIRECT_DRAW
            bool useIndirect = VulkanSDL3Context::Instance().SupportsMultiDrawIndirect() && (res.indirectMapped != nullptr);
            if (useIndirect) {
                static thread_local std::vector<VkDrawIndexedIndirectCommand> s_indirectTerrainCmds;
                s_indirectTerrainCmds.clear();
                s_indirectTerrainCmds.reserve(batch.cmds.size());

                uint32_t curFirstIndex = batch.cmds[0].firstIndex;
                uint32_t curIndexCount = batch.cmds[0].indexCount;
                int32_t curVertexOffset = batch.cmds[0].vertexOffset;

                for (size_t i = 1; i < batch.cmds.size(); ++i) {
                    const auto& nextCmd = batch.cmds[i];
                    if (nextCmd.vertexOffset == curVertexOffset && (curFirstIndex + curIndexCount == nextCmd.firstIndex)) {
                        curIndexCount += nextCmd.indexCount;
                    } else {
                        if (curIndexCount > 0) {
                            VkDrawIndexedIndirectCommand icmd{};
                            icmd.indexCount = curIndexCount;
                            icmd.instanceCount = 1;
                            icmd.firstIndex = curFirstIndex;
                            icmd.vertexOffset = curVertexOffset;
                            icmd.firstInstance = 0;
                            s_indirectTerrainCmds.push_back(icmd);
                        }
                        curFirstIndex = nextCmd.firstIndex;
                        curIndexCount = nextCmd.indexCount;
                        curVertexOffset = nextCmd.vertexOffset;
                    }
                }
                if (curIndexCount > 0) {
                    VkDrawIndexedIndirectCommand icmd{};
                    icmd.indexCount = curIndexCount;
                    icmd.instanceCount = 1;
                    icmd.firstIndex = curFirstIndex;
                    icmd.vertexOffset = curVertexOffset;
                    icmd.firstInstance = 0;
                    s_indirectTerrainCmds.push_back(icmd);
                }

                if (!s_indirectTerrainCmds.empty()) {
                    uint32_t byteSize = static_cast<uint32_t>(s_indirectTerrainCmds.size() * sizeof(VkDrawIndexedIndirectCommand));
                    uint32_t cmdOffset = AllocateIndirectData(res, s_indirectTerrainCmds.data(), byteSize);
                    if (cmdOffset != UINT32_MAX) {
                        vkCmdDrawIndexedIndirect(cmd, res.indirectBuffer, cmdOffset, static_cast<uint32_t>(s_indirectTerrainCmds.size()), sizeof(VkDrawIndexedIndirectCommand));
                        m_drawCallsThisFrame++;
                    } else {
                        for (const auto& icmd : s_indirectTerrainCmds) {
                            vkCmdDrawIndexed(cmd, icmd.indexCount, icmd.instanceCount, icmd.firstIndex, icmd.vertexOffset, icmd.firstInstance);
                            m_drawCallsThisFrame++;
                        }
                    }
                }
            } else
#endif
            {
                uint32_t curFirstIndex = batch.cmds[0].firstIndex;
                uint32_t curIndexCount = batch.cmds[0].indexCount;
                int32_t curVertexOffset = batch.cmds[0].vertexOffset;

                for (size_t i = 1; i < batch.cmds.size(); ++i) {
                    const auto& nextCmd = batch.cmds[i];
                    if (nextCmd.vertexOffset == curVertexOffset && (curFirstIndex + curIndexCount == nextCmd.firstIndex)) {
                        curIndexCount += nextCmd.indexCount;
                    } else {
                        if (curIndexCount > 0) {
                            vkCmdDrawIndexed(cmd, curIndexCount, 1, curFirstIndex, curVertexOffset, 0);
                            m_drawCallsThisFrame++;
                        }
                        curFirstIndex = nextCmd.firstIndex;
                        curIndexCount = nextCmd.indexCount;
                        curVertexOffset = nextCmd.vertexOffset;
                    }
                }
                if (curIndexCount > 0) {
                    vkCmdDrawIndexed(cmd, curIndexCount, 1, curFirstIndex, curVertexOffset, 0);
                    m_drawCallsThisFrame++;
                }
            }
        }
    }
}

GPUContext::GPUSpriteInstance* GPUContext::AllocateSpriteInstanceBuffer(uint32_t count, uint32_t& outFirstInstance, uint32_t& outInstanceSSBOOffset)
{
    if (!m_frameActive || count == 0) return nullptr;

    FrameResource& res = m_frameResources[m_currentFrame];

    if (res.instanceSSBOOffset % sizeof(GPUSpriteInstance) != 0) {
        res.instanceSSBOOffset += sizeof(GPUSpriteInstance) - (res.instanceSSBOOffset % sizeof(GPUSpriteInstance));
    }

    uint32_t byteSize = count * sizeof(GPUSpriteInstance);
    if (res.instanceSSBOOffset + byteSize > res.instanceSSBOCapacity) return nullptr;

    outInstanceSSBOOffset = res.instanceSSBOOffset;
    outFirstInstance = static_cast<uint32_t>(outInstanceSSBOOffset / sizeof(GPUSpriteInstance));
    res.instanceSSBOOffset += byteSize;

    return reinterpret_cast<GPUSpriteInstance*>(static_cast<char*>(res.instanceSSBOMapped) + outInstanceSSBOOffset);
}

void GPUContext::DrawSpritesPreallocated(uint32_t instOffset, uint32_t instByteSize, uint32_t firstInstIndex, const std::vector<SpriteBatch>& batches, const SpriteVertUBO& ubo)
{
    if (!m_frameActive || batches.empty() || instByteSize == 0) return;

    DeferredSpriteDraw draw = {};
    draw.instanceSSBOOffset = instOffset;
    draw.instanceByteSize = instByteSize;
    draw.firstInstance = firstInstIndex;
    draw.batches = batches;
    draw.ubo = ubo;
    draw.ubo.instanceBase = 0; // Using firstInstance in vkCmdDrawIndexed
    m_deferredSpriteDraws.push_back(std::move(draw));

    DeferredCommand dcmd{};
    dcmd.type = DeferredCmdType::Sprite;
    dcmd.index = static_cast<uint32_t>(m_deferredSpriteDraws.size() - 1);
    m_deferredCommands.push_back(dcmd);
}

void GPUContext::DrawSprites(const void* instances, uint32_t instByteSize, const std::vector<SpriteBatch>& batches, const SpriteVertUBO& ubo)
{
    if (!m_frameActive || !instances || instByteSize == 0 || batches.empty()) return;

    uint32_t count = instByteSize / sizeof(GPUSpriteInstance);
    if (count == 0) return;

    uint32_t firstInstIndex = 0;
    uint32_t instOffset = 0;
    GPUSpriteInstance* dst = AllocateSpriteInstanceBuffer(count, firstInstIndex, instOffset);
    if (!dst) return;

    memcpy(dst, instances, instByteSize);
    DrawSpritesPreallocated(instOffset, instByteSize, firstInstIndex, batches, ubo);
}

void GPUContext::ReplaySingleSpriteDraw(const DeferredSpriteDraw& draw)
{
    FrameResource& res = m_frameResources[m_currentFrame];
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    // Bind Quad VBO/IBO
    VkDeviceSize vOffset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_spriteQuadVBO, &vOffset);
    vkCmdBindIndexBuffer(cmd, m_spriteQuadIBO, 0, VK_INDEX_TYPE_UINT32);

    // Upload this draw's VertUniforms UBO with dynamic offset
    uint32_t uboOffset = AllocateUBOData(res, &draw.ubo, sizeof(SpriteVertUBO));

    // Bind Set 0 (SSBO) and Set 1 (VertUniforms dynamic)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_spritePipelineLayout, 0, 1, &res.storageDescriptorSet, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_spritePipelineLayout, 1, 1, &res.uniformDescriptorSet, 1, &uboOffset);

    VkPipeline lastPipeline = VK_NULL_HANDLE;
    int lastTexId = -1;

    for (const auto& batch : draw.batches) {
        VkPipeline pipeline = (batch.blendType == 2) ? m_spritePipelineAdd : m_spritePipelineAlpha;
        if (pipeline != lastPipeline && pipeline != VK_NULL_HANDLE) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            lastPipeline = pipeline;
        }

        if (batch.texture != lastTexId) {
            VkDescriptorSet texSet = VulkanTextureManager::Instance().GetDescriptorSet(batch.texture);
            if (texSet == VK_NULL_HANDLE) texSet = VulkanTextureManager::Instance().GetDefaultDescriptorSet();
            if (texSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_spritePipelineLayout, 2, 1, &texSet, 0, nullptr);
            }
            lastTexId = batch.texture;
        }

        if (batch.vertexCount > 0) {
            vkCmdDrawIndexed(cmd, 6, batch.vertexCount, 0, 0, draw.firstInstance + batch.firstVertex);
            m_drawCallsThisFrame++;
        }
    }
}

void GPUContext::DrawMeshesAndShadows(const void* instances, uint32_t instByteSize, const std::vector<MeshDrawGroup>& meshGroups, const std::vector<ShadowDrawGroup>& shadowGroups, const GeneralVertUBO& vertUBO, const GeneralFragUBO& fragUBO, const ShadowVertUBO& shadowUBO)
{
    if (!m_frameActive) return;

    FrameResource& res = m_frameResources[m_currentFrame];
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    if (instances && instByteSize > 0 && res.instanceSSBOOffset + instByteSize <= res.instanceSSBOCapacity) {
        memcpy(static_cast<char*>(res.instanceSSBOMapped) + res.instanceSSBOOffset, instances, instByteSize);
        res.instanceSSBOOffset += instByteSize;
    }

    // Upload VertUBO and FragUBO with dynamic offsets
    uint32_t vertOffset = AllocateUBOData(res, &vertUBO, sizeof(GeneralVertUBO));
    uint32_t fragOffset = AllocateUBOData(res, &fragUBO, sizeof(GeneralFragUBO));

    // 1. Draw 3D Meshes
    if (!meshGroups.empty()) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipelineLayout, 0, 1, &res.storageDescriptorSet, 0, nullptr);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipelineLayout, 1, 1, &res.uniformDescriptorSet, 1, &vertOffset);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipelineLayout, 3, 1, &res.fragUniformDescriptorSet, 1, &fragOffset);

        for (const auto& mg : meshGroups) {
            if (mg.instanceCount == 0) continue;

            VkPipeline pipeline = m_meshPipelineOpaque;
            if (mg.blendType == 1) pipeline = m_meshPipelineAlphaBlend;
            else if (mg.blendType == 2 || mg.blendType == 7) pipeline = m_meshPipelineAdditive;
            else if (mg.blendType == 3) pipeline = (m_meshPipelineDark != VK_NULL_HANDLE) ? m_meshPipelineDark : m_meshPipelineAlphaBlend;
            else if (!mg.depthWrite) pipeline = m_meshPipelineAlphaBlend;

            if (pipeline != VK_NULL_HANDLE) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            }

            VkDescriptorSet texSet = VulkanTextureManager::Instance().GetDescriptorSet(mg.textureIndex);
            if (texSet == VK_NULL_HANDLE) texSet = VulkanTextureManager::Instance().GetDefaultDescriptorSet();
            if (texSet != VK_NULL_HANDLE) {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipelineLayout, 2, 1, &texSet, 0, nullptr);
            }

            // Draw instance
            vkCmdDraw(cmd, 3, mg.instanceCount, 0, mg.firstInstance);
        }
    }

    // 2. Draw Shadows
    if (!shadowGroups.empty()) {
        uint32_t shadowOffset = AllocateUBOData(res, &shadowUBO, sizeof(ShadowVertUBO));

        if (m_shadowPipeline != VK_NULL_HANDLE) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowPipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowPipelineLayout, 0, 1, &res.storageDescriptorSet, 0, nullptr);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowPipelineLayout, 1, 1, &res.uniformDescriptorSet, 1, &shadowOffset);

            for (const auto& sg : shadowGroups) {
                if (sg.instanceCount > 0) {
                    vkCmdDraw(cmd, 3, sg.instanceCount, 0, sg.firstInstance);
                }
            }
        }
    }

    m_frameMeshBatchCalls += static_cast<uint32_t>(meshGroups.size() + shadowGroups.size());
}

GPUImageInstance* GPUContext::AllocateImageInstanceBuffer(uint32_t count, uint32_t& outBaseInstance, uint32_t& outInstanceSSBOOffset)
{
    if (!m_frameActive || count == 0) return nullptr;

    FrameResource& res = m_frameResources[m_currentFrame];

    res.instanceSSBOOffset = (res.instanceSSBOOffset + 63u) & ~63u;

    uint32_t totalByteSize = count * sizeof(GPUImageInstance);
    if (res.instanceSSBOOffset + totalByteSize > res.instanceSSBOCapacity) return nullptr;

    outInstanceSSBOOffset = res.instanceSSBOOffset;
    outBaseInstance = outInstanceSSBOOffset / sizeof(GPUImageInstance);
    res.instanceSSBOOffset += totalByteSize;

    return reinterpret_cast<GPUImageInstance*>(static_cast<char*>(res.instanceSSBOMapped) + outInstanceSSBOOffset);
}

void GPUContext::DrawImagesPreallocated(uint32_t baseInstance, uint32_t instanceSSBOOffset, const std::vector<ImageBatchRun>& batchRuns)
{
    if (!m_frameActive || batchRuns.empty()) return;

    DeferredImageDraw draw{};
    draw.instanceSSBOOffset = instanceSSBOOffset;
    draw.baseInstance = baseInstance;
    draw.batches = batchRuns;
    m_deferredImageDraws.push_back(std::move(draw));

    DeferredCommand dcmd{};
    dcmd.type = DeferredCmdType::Image;
    dcmd.index = static_cast<uint32_t>(m_deferredImageDraws.size() - 1);
    m_deferredCommands.push_back(dcmd);
}

void GPUContext::DrawImages(const std::vector<std::pair<ImageBatchKey, std::vector<GPUImageInstance>>>& batches)
{
    if (!m_frameActive || batches.empty()) return;

    uint32_t totalInstances = 0;
    for (const auto& b : batches) totalInstances += static_cast<uint32_t>(b.second.size());
    if (totalInstances == 0) return;

    uint32_t baseInstance = 0;
    uint32_t instanceSSBOOffset = 0;
    GPUImageInstance* dst = AllocateImageInstanceBuffer(totalInstances, baseInstance, instanceSSBOOffset);
    if (!dst) return;

    std::vector<ImageBatchRun> batchRuns;
    batchRuns.reserve(batches.size());

    uint32_t written = 0;
    for (const auto& b : batches) {
        uint32_t bCount = static_cast<uint32_t>(b.second.size());
        if (bCount > 0) {
            memcpy(dst + written, b.second.data(), bCount * sizeof(GPUImageInstance));
            written += bCount;
            batchRuns.push_back({ b.first, bCount });
        }
    }

    DrawImagesPreallocated(baseInstance, instanceSSBOOffset, batchRuns);
}

void GPUContext::ReplaySingleImageDraw(const DeferredImageDraw& draw)
{
    FrameResource& res = m_frameResources[m_currentFrame];
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    // Bind Quad VBO/IBO
    VkDeviceSize vOffset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_imageQuadVBO, &vOffset);
    vkCmdBindIndexBuffer(cmd, m_imageQuadIBO, 0, VK_INDEX_TYPE_UINT32);
    // Upload ImageVertUBO with dynamic offset
    ImageVertUBO imgUBO{};
    imgUBO.screenSize = glm::vec2(static_cast<float>(m_swapchainExtent.width), static_cast<float>(m_swapchainExtent.height));
    imgUBO.instanceBase = draw.baseInstance;
    uint32_t imgUboOffset = AllocateUBOData(res, &imgUBO, sizeof(ImageVertUBO));

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_uiPipelineLayout, 0, 1, &res.storageDescriptorSet, 0, nullptr);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_uiPipelineLayout, 1, 1, &res.uniformDescriptorSet, 1, &imgUboOffset);

    VkPipeline currentBoundPipeline = VK_NULL_HANDLE;
    VkDescriptorSet lastBoundTexSet = VK_NULL_HANDLE;
    VkRect2D lastScissor = { { -1, -1 }, { 0, 0 } };
    uint32_t runningInstance = 0;
    for (const auto& b : draw.batches) {
        uint32_t count = b.count;
        if (count == 0) continue;

        VkPipeline targetPipeline = m_uiPipeline;
        int blendMode = (b.key.RenderFlags & RENDER_ALPHA_BLEND_MASK) >> RENDER_ALPHA_BLEND_SHIFT;
        if (blendMode == 2) {
            targetPipeline = (m_uiPipelineAdd != VK_NULL_HANDLE) ? m_uiPipelineAdd : m_uiPipeline;
        } else if (blendMode == 3) {
            targetPipeline = (m_uiPipelineDark != VK_NULL_HANDLE) ? m_uiPipelineDark : m_uiPipeline;
        } else if (blendMode == 0) {
            targetPipeline = (m_uiPipelineNone != VK_NULL_HANDLE) ? m_uiPipelineNone : m_uiPipeline;
        }

        if (targetPipeline != currentBoundPipeline && targetPipeline != VK_NULL_HANDLE) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, targetPipeline);
            currentBoundPipeline = targetPipeline;
        }

        VkRect2D sc;
        if (b.key.scissorEnabled && b.key.scissorW > 0 && b.key.scissorH > 0) {
            sc = { {b.key.scissorX, b.key.scissorY}, {static_cast<uint32_t>(b.key.scissorW), static_cast<uint32_t>(b.key.scissorH)} };
        } else {
            sc = { {0, 0}, m_swapchainExtent };
        }

        if (sc.offset.x != lastScissor.offset.x || sc.offset.y != lastScissor.offset.y ||
            sc.extent.width != lastScissor.extent.width || sc.extent.height != lastScissor.extent.height) {
            vkCmdSetScissor(cmd, 0, 1, &sc);
            lastScissor = sc;
        }

        VkDescriptorSet texSet = VulkanTextureManager::Instance().GetDescriptorSet(b.key.Texture);
        if (texSet == VK_NULL_HANDLE) texSet = VulkanTextureManager::Instance().GetDefaultDescriptorSet();
        if (texSet != VK_NULL_HANDLE && texSet != lastBoundTexSet) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_uiPipelineLayout, 2, 1, &texSet, 0, nullptr);
            lastBoundTexSet = texSet;
        }

        vkCmdDrawIndexed(cmd, 6, count, 0, 0, runningInstance);
        runningInstance += count;
        m_drawCallsThisFrame++;
    }

    // Reset default scissor only if needed
    if (lastScissor.offset.x != 0 || lastScissor.offset.y != 0 ||
        lastScissor.extent.width != m_swapchainExtent.width || lastScissor.extent.height != m_swapchainExtent.height) {
        VkRect2D fullScissor{ {0, 0}, m_swapchainExtent };
        vkCmdSetScissor(cmd, 0, 1, &fullScissor);
    }
}

void GPUContext::AppendBoneRows(const void* boneRows, uint32_t byteSize)
{
    if (!boneRows || byteSize == 0) return;
    FrameResource& res = m_frameResources[m_currentFrame];
    if (res.boneSSBOOffset + byteSize <= res.boneSSBOCapacity) {
        memcpy(static_cast<char*>(res.boneSSBOMapped) + res.boneSSBOOffset, boneRows, byteSize);
        res.boneSSBOOffset += byteSize;
    }
}

void GPUContext::ClearBoneRows()
{
    m_frameResources[m_currentFrame].boneSSBOOffset = 0;
}

void GPUContext::FlushClothDraws()
{
}

bool GPUContext::CreateMeshBuffers(const void* vData, uint32_t vSize, const void* iData, uint32_t iSize,
    VkBuffer& outVBO, VkDeviceMemory& outVBOMem, VkBuffer& outIBO, VkDeviceMemory& outIBOMem)
{
    if (m_device == VK_NULL_HANDLE || !vData || vSize == 0 || !iData || iSize == 0) return false;

    VkMemoryPropertyFlags hostProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    void* mapped = nullptr;

    // If graphics queue or command pool is not ready yet, use direct host memory
    if (m_commandPool == VK_NULL_HANDLE || m_graphicsQueue == VK_NULL_HANDLE) {
        if (!CreateBufferHelper(m_device, m_physicalDevice, vSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, hostProps, outVBO, outVBOMem, &mapped)) {
            return false;
        }
        memcpy(mapped, vData, vSize);
        vkUnmapMemory(m_device, outVBOMem);

        if (!CreateBufferHelper(m_device, m_physicalDevice, iSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, hostProps, outIBO, outIBOMem, &mapped)) {
            vkDestroyBuffer(m_device, outVBO, nullptr);
            vkFreeMemory(m_device, outVBOMem, nullptr);
            outVBO = VK_NULL_HANDLE; outVBOMem = VK_NULL_HANDLE;
            return false;
        }
        memcpy(mapped, iData, iSize);
        vkUnmapMemory(m_device, outIBOMem);
        return true;
    }

    VkMemoryPropertyFlags deviceProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    // 1. Create Staging Buffers
    VkBuffer vboStaging = VK_NULL_HANDLE;
    VkDeviceMemory vboStagingMem = VK_NULL_HANDLE;

    if (!CreateBufferHelper(m_device, m_physicalDevice, vSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, hostProps, vboStaging, vboStagingMem, &mapped)) {
        return false;
    }
    memcpy(mapped, vData, vSize);
    vkUnmapMemory(m_device, vboStagingMem);

    VkBuffer iboStaging = VK_NULL_HANDLE;
    VkDeviceMemory iboStagingMem = VK_NULL_HANDLE;
    if (!CreateBufferHelper(m_device, m_physicalDevice, iSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, hostProps, iboStaging, iboStagingMem, &mapped)) {
        vkDestroyBuffer(m_device, vboStaging, nullptr);
        vkFreeMemory(m_device, vboStagingMem, nullptr);
        return false;
    }
    memcpy(mapped, iData, iSize);
    vkUnmapMemory(m_device, iboStagingMem);

    // 2. Create Destination DEVICE_LOCAL Buffers
    if (!CreateBufferHelper(m_device, m_physicalDevice, vSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, deviceProps, outVBO, outVBOMem)) {
        vkDestroyBuffer(m_device, vboStaging, nullptr);
        vkFreeMemory(m_device, vboStagingMem, nullptr);
        vkDestroyBuffer(m_device, iboStaging, nullptr);
        vkFreeMemory(m_device, iboStagingMem, nullptr);
        return false;
    }

    if (!CreateBufferHelper(m_device, m_physicalDevice, iSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, deviceProps, outIBO, outIBOMem)) {
        vkDestroyBuffer(m_device, outVBO, nullptr);
        vkFreeMemory(m_device, outVBOMem, nullptr);
        outVBO = VK_NULL_HANDLE; outVBOMem = VK_NULL_HANDLE;
        vkDestroyBuffer(m_device, vboStaging, nullptr);
        vkFreeMemory(m_device, vboStagingMem, nullptr);
        vkDestroyBuffer(m_device, iboStaging, nullptr);
        vkFreeMemory(m_device, iboStagingMem, nullptr);
        return false;
    }

    // 3. Record copy commands into single-time command buffer
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandPool = m_commandPool;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &cmd) == VK_SUCCESS) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkBufferCopy vCopyRegion{ 0, 0, vSize };
        vkCmdCopyBuffer(cmd, vboStaging, outVBO, 1, &vCopyRegion);

        VkBufferCopy iCopyRegion{ 0, 0, iSize };
        vkCmdCopyBuffer(cmd, iboStaging, outIBO, 1, &iCopyRegion);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_graphicsQueue);

        vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
    }

    // 4. Destroy staging buffers
    vkDestroyBuffer(m_device, vboStaging, nullptr);
    vkFreeMemory(m_device, vboStagingMem, nullptr);
    vkDestroyBuffer(m_device, iboStaging, nullptr);
    vkFreeMemory(m_device, iboStagingMem, nullptr);

    return true;
}

void GPUContext::DestroyMeshBuffers(VkBuffer& vbo, VkDeviceMemory& vboMem, VkBuffer& ibo, VkDeviceMemory& iboMem)
{
    if (!m_initialized || m_device == VK_NULL_HANDLE) {
        vbo = VK_NULL_HANDLE;
        vboMem = VK_NULL_HANDLE;
        ibo = VK_NULL_HANDLE;
        iboMem = VK_NULL_HANDLE;
        return;
    }
    if (vbo != VK_NULL_HANDLE || ibo != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }
    if (vbo != VK_NULL_HANDLE) { vkDestroyBuffer(m_device, vbo, nullptr); vbo = VK_NULL_HANDLE; }
    if (vboMem != VK_NULL_HANDLE) { vkFreeMemory(m_device, vboMem, nullptr); vboMem = VK_NULL_HANDLE; }
    if (ibo != VK_NULL_HANDLE) { vkDestroyBuffer(m_device, ibo, nullptr); ibo = VK_NULL_HANDLE; }
    if (iboMem != VK_NULL_HANDLE) { vkFreeMemory(m_device, iboMem, nullptr); iboMem = VK_NULL_HANDLE; }
}



void* GPUContext::AllocateBoneBuffer(uint32_t byteSize, uint32_t& outBoneBaseRows, uint32_t& outBoneSSBOOffset)
{
    if (!m_frameActive || byteSize == 0 || (byteSize % sizeof(glm::vec4)) != 0) return nullptr;
    FrameResource& res = m_frameResources[m_currentFrame];

    if (res.boneSSBOOffset % sizeof(glm::vec4) != 0) {
        res.boneSSBOOffset += sizeof(glm::vec4) - (res.boneSSBOOffset % sizeof(glm::vec4));
    }

    if (res.boneSSBOOffset + byteSize > res.boneSSBOCapacity) {
        return nullptr;
    }

    outBoneSSBOOffset = res.boneSSBOOffset;
    outBoneBaseRows = static_cast<uint32_t>(outBoneSSBOOffset / sizeof(glm::vec4));
    res.boneSSBOOffset += byteSize;

    return static_cast<char*>(res.boneSSBOMapped) + outBoneSSBOOffset;
}

GPUContext::GPUInstanceData* GPUContext::AllocateInstanceBuffer(uint32_t instanceCount, uint32_t& outBaseInstanceIndex, uint32_t& outInstanceSSBOStart)
{
    if (!m_frameActive || instanceCount == 0) return nullptr;
    FrameResource& res = m_frameResources[m_currentFrame];

    if (res.instanceSSBOOffset % sizeof(GPUInstanceData) != 0) {
        res.instanceSSBOOffset += sizeof(GPUInstanceData) - (res.instanceSSBOOffset % sizeof(GPUInstanceData));
    }

    uint32_t byteSize = instanceCount * sizeof(GPUInstanceData);
    if (res.instanceSSBOOffset + byteSize > res.instanceSSBOCapacity) return nullptr;

    outInstanceSSBOStart = res.instanceSSBOOffset;
    outBaseInstanceIndex = static_cast<uint32_t>(outInstanceSSBOStart / sizeof(GPUInstanceData));
    res.instanceSSBOOffset += byteSize;

    return reinterpret_cast<GPUInstanceData*>(static_cast<char*>(res.instanceSSBOMapped) + outInstanceSSBOStart);
}

void GPUContext::DrawInstancedMeshPreallocated(VkBuffer vbo, VkBuffer ibo, uint32_t indexCount, uint32_t instanceCount,
    uint32_t textureId, int blendType, bool depthTest, bool depthWrite,
    const void* uboData, uint32_t uboSize,
    uint32_t instanceSSBOStart, uint32_t baseInstanceIndex,
    uint32_t boneSSBOOffset, uint32_t boneByteSize)
{
    if (!m_frameActive || vbo == VK_NULL_HANDLE || ibo == VK_NULL_HANDLE || indexCount == 0 || instanceCount == 0) return;
    if (!uboData || uboSize < sizeof(RenderUniformBlock)) return;

    const RenderUniformBlock& blk = *static_cast<const RenderUniformBlock*>(uboData);
    FrameResource& res = m_frameResources[m_currentFrame];

    // Build UBOs - instanceBase=0 because we use firstInstance in vkCmdDrawIndexed
    GeneralVertUBO vertUBO = {};
    memcpy(&vertUBO.viewMatrix, blk.uView, sizeof(float) * 16);
    memcpy(&vertUBO.projMatrix, blk.uProj, sizeof(float) * 16);
    // Invert row 1 (Y) for Vulkan NDC (OpenGL Y-up → Vulkan Y-down)
    vertUBO.projMatrix[0][1] = -vertUBO.projMatrix[0][1];
    vertUBO.projMatrix[1][1] = -vertUBO.projMatrix[1][1];
    vertUBO.projMatrix[2][1] = -vertUBO.projMatrix[2][1];
    vertUBO.projMatrix[3][1] = -vertUBO.projMatrix[3][1];
    // Remap depth row 2 (Z) from OpenGL [-1,1] to Vulkan [0,1]: (row 2 + row 3) * 0.5
    vertUBO.projMatrix[0][2] = (vertUBO.projMatrix[0][2] + vertUBO.projMatrix[0][3]) * 0.5f;
    vertUBO.projMatrix[1][2] = (vertUBO.projMatrix[1][2] + vertUBO.projMatrix[1][3]) * 0.5f;
    vertUBO.projMatrix[2][2] = (vertUBO.projMatrix[2][2] + vertUBO.projMatrix[2][3]) * 0.5f;
    vertUBO.projMatrix[3][2] = (vertUBO.projMatrix[3][2] + vertUBO.projMatrix[3][3]) * 0.5f;
    vertUBO.worldTime = static_cast<float>(fmod(static_cast<double>(WorldTime), 100000.0));
    vertUBO.shadowAngle = glm::vec3(0.f, 0.f, -45.f);
    vertUBO.lightPosition = glm::vec4(blk.u_lightPosition[0], blk.u_lightPosition[1], blk.u_lightPosition[2], blk.u_lightPosition[3]);
    vertUBO.instanceBase = 0;  // Using firstInstance instead
    vertUBO.vertexBase = 0;

    GeneralFragUBO fragUBO = {};
    fragUBO.batchTexture = 1.0f;
    fragUBO.brightness = 1.0f;
    fragUBO.alphaTestThreshold = (blendType == 2) ? 0.0f : 0.01f;

    // Defer the draw - it will be replayed after GL blit in Present()
    DeferredMeshDraw draw = {};
    draw.vbo = vbo;
    draw.ibo = ibo;
    draw.indexCount = indexCount;
    draw.instanceCount = instanceCount;
    draw.textureId = textureId;
    draw.blendType = blendType;
    draw.depthTest = depthTest;
    draw.depthWrite = depthWrite;
    draw.instanceSSBOOffset = instanceSSBOStart;
    draw.instanceByteSize = instanceCount * sizeof(GPUInstanceData);
    draw.boneSSBOOffset = boneSSBOOffset;
    draw.boneByteSize = boneByteSize;
    draw.firstInstance = baseInstanceIndex;
    draw.vertUBO = vertUBO;
    draw.fragUBO = fragUBO;
    m_deferredMeshDraws.push_back(std::move(draw));

    DeferredCommand dcmd{};
    dcmd.type = DeferredCmdType::Mesh;
    dcmd.index = static_cast<uint32_t>(m_deferredMeshDraws.size() - 1);
    m_deferredCommands.push_back(dcmd);
}

void GPUContext::DrawInstancedMeshPreallocatedBones(VkBuffer vbo, VkBuffer ibo, uint32_t indexCount, uint32_t instanceCount,
    uint32_t textureId, int blendType, bool depthTest, bool depthWrite,
    const void* uboData, uint32_t uboSize,
    const void* instancesData, uint32_t instancesByteSize,
    uint32_t boneSSBOOffset, uint32_t boneBaseRows, uint32_t boneByteSize)
{
    if (!instancesData || instanceCount == 0) return;

    uint32_t baseInstanceIndex = 0;
    uint32_t instanceSSBOStart = 0;
    GPUInstanceData* dstInstances = AllocateInstanceBuffer(instanceCount, baseInstanceIndex, instanceSSBOStart);
    if (!dstInstances) return;

    const GPUInstanceData* srcInstances = static_cast<const GPUInstanceData*>(instancesData);
    if (boneBaseRows == 0) {
        memcpy(dstInstances, srcInstances, instancesByteSize);
    } else {
        const int32_t offset = static_cast<int32_t>(boneBaseRows);
        for (uint32_t i = 0; i < instanceCount; ++i) {
            dstInstances[i] = srcInstances[i];
            dstInstances[i].boneOffset += offset;
        }
    }

    DrawInstancedMeshPreallocated(vbo, ibo, indexCount, instanceCount, textureId, blendType, depthTest, depthWrite,
        uboData, uboSize, instanceSSBOStart, baseInstanceIndex, boneSSBOOffset, boneByteSize);
}

void GPUContext::DrawInstancedMesh(VkBuffer vbo, VkBuffer ibo, uint32_t indexCount, uint32_t instanceCount,
    uint32_t textureId, int blendType, bool depthTest, bool depthWrite,
    const void* uboData, uint32_t uboSize,
    const void* instancesData, uint32_t instancesByteSize,
    const void* boneData, uint32_t boneByteSize)
{
    if (!boneData || boneByteSize == 0) return;

    uint32_t boneBaseRows = 0;
    uint32_t boneSSBOOffset = 0;
    void* dst = AllocateBoneBuffer(boneByteSize, boneBaseRows, boneSSBOOffset);
    if (!dst) return;
    memcpy(dst, boneData, boneByteSize);

    DrawInstancedMeshPreallocatedBones(vbo, ibo, indexCount, instanceCount, textureId, blendType, depthTest, depthWrite,
        uboData, uboSize, instancesData, instancesByteSize, boneSSBOOffset, boneBaseRows, boneByteSize);
}

void GPUContext::ClearDepthBuffer()
{
    if (!m_frameActive) return;
    DeferredCommand dcmd{};
    dcmd.type = DeferredCmdType::ClearDepth;
    dcmd.index = 0;
    m_deferredCommands.push_back(dcmd);
}

void GPUContext::ReplayMeshRange(size_t cmdStart, size_t cmdEnd)
{
    FrameResource& res = m_frameResources[m_currentFrame];
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    // Bind SSBO (Set 0) ONCE for all mesh draws in this replay pass
    if (res.storageDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipelineLayout, 0, 1, &res.storageDescriptorSet, 0, nullptr);
    }

    // State cache to minimize Vulkan command recording overhead
    VkPipeline lastPipeline = VK_NULL_HANDLE;
    VkBuffer lastVBO = VK_NULL_HANDLE;
    VkBuffer lastIBO = VK_NULL_HANDLE;
    uint32_t lastTexId = UINT32_MAX;
    GeneralVertUBO lastVertUBO{};
    GeneralFragUBO lastFragUBO{};
    bool hasLastVertUBO = false;
    bool hasLastFragUBO = false;

    static thread_local std::vector<VkDrawIndexedIndirectCommand> s_pendingMeshCmds;
    s_pendingMeshCmds.clear();

    auto flushMeshBatch = [&]() {
        if (s_pendingMeshCmds.empty()) return;
        if (s_pendingMeshCmds.size() == 1) {
            const auto& icmd = s_pendingMeshCmds[0];
            vkCmdDrawIndexed(cmd, icmd.indexCount, icmd.instanceCount, icmd.firstIndex, icmd.vertexOffset, icmd.firstInstance);
            m_drawCallsThisFrame++;
        } else {
#if CBMu_ENABLE_VK_INDIRECT_DRAW
            if (VulkanSDL3Context::Instance().SupportsMultiDrawIndirect() && res.indirectMapped != nullptr) {
                uint32_t byteSize = static_cast<uint32_t>(s_pendingMeshCmds.size() * sizeof(VkDrawIndexedIndirectCommand));
                uint32_t cmdOffset = AllocateIndirectData(res, s_pendingMeshCmds.data(), byteSize);
                if (cmdOffset != UINT32_MAX) {
                    vkCmdDrawIndexedIndirect(cmd, res.indirectBuffer, cmdOffset, static_cast<uint32_t>(s_pendingMeshCmds.size()), sizeof(VkDrawIndexedIndirectCommand));
                    m_drawCallsThisFrame++;
                } else {
                    for (const auto& icmd : s_pendingMeshCmds) {
                        vkCmdDrawIndexed(cmd, icmd.indexCount, icmd.instanceCount, icmd.firstIndex, icmd.vertexOffset, icmd.firstInstance);
                        m_drawCallsThisFrame++;
                    }
                }
            } else
#endif
            {
                for (const auto& icmd : s_pendingMeshCmds) {
                    vkCmdDrawIndexed(cmd, icmd.indexCount, icmd.instanceCount, icmd.firstIndex, icmd.vertexOffset, icmd.firstInstance);
                    m_drawCallsThisFrame++;
                }
            }
        }
        s_pendingMeshCmds.clear();
    };

    for (size_t i = cmdStart; i < cmdEnd; ++i) {
        uint32_t meshIdx = m_deferredCommands[i].index;
        if (meshIdx >= m_deferredMeshDraws.size()) continue;
        const auto& draw = m_deferredMeshDraws[meshIdx];
        if (draw.vbo == VK_NULL_HANDLE || draw.ibo == VK_NULL_HANDLE || draw.indexCount == 0 || draw.instanceCount == 0) continue;

        // Select pipeline (only rebind if changed)
        VkPipeline pipeline = m_meshPipelineOpaque;
        if (draw.blendType == 1) pipeline = m_meshPipelineAlphaBlend;
        else if (draw.blendType == 2 || draw.blendType == 7) pipeline = m_meshPipelineAdditive;
        else if (draw.blendType == 3) pipeline = (m_meshPipelineDark != VK_NULL_HANDLE) ? m_meshPipelineDark : m_meshPipelineAlphaBlend;
        else if (!draw.depthWrite) pipeline = m_meshPipelineAlphaBlend;

        bool vertChanged = !hasLastVertUBO || memcmp(&draw.vertUBO, &lastVertUBO, sizeof(GeneralVertUBO)) != 0;
        bool fragChanged = !hasLastFragUBO || memcmp(&draw.fragUBO, &lastFragUBO, sizeof(GeneralFragUBO)) != 0;
        bool stateChanged = (pipeline != lastPipeline) || (draw.textureId != lastTexId) ||
                            (draw.vbo != lastVBO) || (draw.ibo != lastIBO) ||
                            vertChanged || fragChanged;

        if (stateChanged) {
            flushMeshBatch();

            if (pipeline != lastPipeline && pipeline != VK_NULL_HANDLE) {
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                lastPipeline = pipeline;
            }

            if (vertChanged) {
                uint32_t lastVertOffset = AllocateUBOData(res, &draw.vertUBO, sizeof(GeneralVertUBO));
                memcpy(&lastVertUBO, &draw.vertUBO, sizeof(GeneralVertUBO));
                hasLastVertUBO = true;
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipelineLayout, 1, 1, &res.uniformDescriptorSet, 1, &lastVertOffset);
            }

            if (fragChanged) {
                uint32_t lastFragOffset = AllocateUBOData(res, &draw.fragUBO, sizeof(GeneralFragUBO));
                memcpy(&lastFragUBO, &draw.fragUBO, sizeof(GeneralFragUBO));
                hasLastFragUBO = true;
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipelineLayout, 3, 1, &res.fragUniformDescriptorSet, 1, &lastFragOffset);
            }

            if (draw.textureId != lastTexId) {
                VkDescriptorSet texSet = VulkanTextureManager::Instance().GetDescriptorSet(draw.textureId);
                if (texSet == VK_NULL_HANDLE) texSet = VulkanTextureManager::Instance().GetDefaultDescriptorSet();
                if (texSet != VK_NULL_HANDLE) {
                    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipelineLayout, 2, 1, &texSet, 0, nullptr);
                }
                lastTexId = draw.textureId;
            }

            if (draw.vbo != lastVBO) {
                VkDeviceSize vOffset = 0;
                vkCmdBindVertexBuffers(cmd, 0, 1, &draw.vbo, &vOffset);
                lastVBO = draw.vbo;
            }
            if (draw.ibo != lastIBO) {
                vkCmdBindIndexBuffer(cmd, draw.ibo, 0, VK_INDEX_TYPE_UINT32);
                lastIBO = draw.ibo;
            }
        }

        VkDrawIndexedIndirectCommand icmd{};
        icmd.indexCount = draw.indexCount;
        icmd.instanceCount = draw.instanceCount;
        icmd.firstIndex = 0;
        icmd.vertexOffset = 0;
        icmd.firstInstance = draw.firstInstance;
        s_pendingMeshCmds.push_back(icmd);
    }

    flushMeshBatch();
}

void GPUContext::ReplayDeferredCommands()
{
    if (m_deferredCommands.empty()) return;

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    VkViewport vp{ 0.0f, 0.0f, static_cast<float>(m_swapchainExtent.width), static_cast<float>(m_swapchainExtent.height), 0.0f, 1.0f };
    vkCmdSetViewport(cmd, 0, 1, &vp);
    VkRect2D sc{ {0, 0}, m_swapchainExtent };
    vkCmdSetScissor(cmd, 0, 1, &sc);

    for (size_t i = 0; i < m_deferredCommands.size(); ++i) {
        const auto& dcmd = m_deferredCommands[i];
        switch (dcmd.type) {
            case DeferredCmdType::Terrain:
                if (dcmd.index < m_deferredTerrainDraws.size()) {
                    ReplaySingleTerrainDraw(m_deferredTerrainDraws[dcmd.index]);
                }
                break;
            case DeferredCmdType::Mesh:
                if (dcmd.index < m_deferredMeshDraws.size()) {
                    size_t meshEnd = i + 1;
                    while (meshEnd < m_deferredCommands.size() && m_deferredCommands[meshEnd].type == DeferredCmdType::Mesh) {
                        meshEnd++;
                    }
                    ReplayMeshRange(i, meshEnd);
                    i = meshEnd - 1;
                }
                break;
            case DeferredCmdType::Sprite:
                if (dcmd.index < m_deferredSpriteDraws.size()) {
                    ReplaySingleSpriteDraw(m_deferredSpriteDraws[dcmd.index]);
                }
                break;
            case DeferredCmdType::Image:
                if (dcmd.index < m_deferredImageDraws.size()) {
                    ReplaySingleImageDraw(m_deferredImageDraws[dcmd.index]);
                }
                break;
            case DeferredCmdType::ClearDepth:
                {
                    VkClearAttachment clearAttachment{};
                    clearAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                    clearAttachment.clearValue.depthStencil = { 1.0f, 0 };
                    VkClearRect clearRect{};
                    clearRect.rect.offset = { 0, 0 };
                    clearRect.rect.extent = m_swapchainExtent;
                    clearRect.baseArrayLayer = 0;
                    clearRect.layerCount = 1;
                    vkCmdClearAttachments(cmd, 1, &clearAttachment, 1, &clearRect);
                }
                break;
        }
    }

    m_deferredCommands.clear();
    m_deferredTerrainDraws.clear();
    m_deferredMeshDraws.clear();
    m_deferredSpriteDraws.clear();
    m_deferredImageDraws.clear();
}

// ============================================================================
// Water Compute Pipeline & Simulation
// ============================================================================

bool GPUContext::InitWaterCompute()
{
    if (m_waterComputeInitialized) return true;
    if (m_device == VK_NULL_HANDLE) return false;

    // 1. Create SSBO for 4 pages of 256x256 integers (1 MB)
    VkDeviceSize waterBufferSize = 4 * 256 * 256 * sizeof(int32_t); // 1,048,576 bytes
    VkMemoryPropertyFlags hostProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (!CreateBufferHelper(m_device, m_physicalDevice, waterBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        hostProps, m_waterSSBO, m_waterSSBOMemory, &m_waterSSBOMapped)) {
        std::cerr << "[GPUContext] Failed to create water compute SSBO!" << std::endl;
        return false;
    }
    memset(m_waterSSBOMapped, 0, static_cast<size_t>(waterBufferSize));

    // 2. Create Descriptor Set Layout (Bindings 0, 1, 2, 3: Storage Buffers for Compute)
    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
    for (uint32_t i = 0; i < 4; i++) {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_waterComputeDescriptorLayout) != VK_SUCCESS) {
        CleanupWaterCompute();
        return false;
    }

    // 3. Create Pipeline Layout with Push Constants
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(WaterComputeParams);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_waterComputeDescriptorLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_waterComputePipelineLayout) != VK_SUCCESS) {
        CleanupWaterCompute();
        return false;
    }

    // 4. Create Compute Pipeline
    auto compCode = ReadSPVFile("water.comp.spv");
    if (compCode.empty()) {
        std::cerr << "[GPUContext] Failed to load water.comp.spv!" << std::endl;
        CleanupWaterCompute();
        return false;
    }
    VkShaderModule compModule = CreateShaderModule(compCode);
    if (!compModule) {
        CleanupWaterCompute();
        return false;
    }

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = compModule;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = m_waterComputePipelineLayout;

    if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_waterComputePipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(m_device, compModule, nullptr);
        std::cerr << "[GPUContext] Failed to create water compute pipeline!" << std::endl;
        CleanupWaterCompute();
        return false;
    }
    vkDestroyShaderModule(m_device, compModule, nullptr);

    // 5. Allocate 2 Descriptor Sets for ping-pong page flipping
    VkDeviceSize pageSize = 256 * 256 * sizeof(int32_t); // 256 KB
    for (int p = 0; p < 2; p++) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_waterComputeDescriptorLayout;
        if (vkAllocateDescriptorSets(m_device, &allocInfo, &m_waterComputeDescriptorSets[p]) != VK_SUCCESS) {
            CleanupWaterCompute();
            return false;
        }

        // When p == 0: oldPage = page 1, newPage = page 0
        // When p == 1: oldPage = page 0, newPage = page 1
        VkDescriptorBufferInfo bufferInfos[4]{};
        bufferInfos[0].buffer = m_waterSSBO;
        bufferInfos[0].offset = (p == 0 ? 1 : 0) * pageSize;
        bufferInfos[0].range = pageSize;

        bufferInfos[1].buffer = m_waterSSBO;
        bufferInfos[1].offset = (p == 0 ? 0 : 1) * pageSize;
        bufferInfos[1].range = pageSize;

        bufferInfos[2].buffer = m_waterSSBO;
        bufferInfos[2].offset = 2 * pageSize;
        bufferInfos[2].range = pageSize;

        bufferInfos[3].buffer = m_waterSSBO;
        bufferInfos[3].offset = 3 * pageSize;
        bufferInfos[3].range = pageSize;

        std::array<VkWriteDescriptorSet, 4> writes{};
        for (uint32_t b = 0; b < 4; b++) {
            writes[b].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[b].dstSet = m_waterComputeDescriptorSets[p];
            writes[b].dstBinding = b;
            writes[b].descriptorCount = 1;
            writes[b].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[b].pBufferInfo = &bufferInfos[b];
        }
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    m_waterComputeInitialized = true;
    std::cout << "[GPUContext] Water Compute Pipeline initialized successfully." << std::endl;
    return true;
}

void GPUContext::DispatchWaterWave(int waterPage, int heroX, int heroY, float worldTime, bool runBaseWave)
{
    if (!m_waterComputeInitialized) {
        if (!InitWaterCompute()) return;
    }
    if (m_renderPassActive) {
        return; // Vulkan spec: vkCmdDispatch cannot be recorded inside an active render pass
    }

    // Allocate dedicated single-use command buffer for synchronous execution,
    // guaranteeing fresh data for CPU CreateTerrain() with zero race conditions.
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(m_device, &allocInfo, &cmd) != VK_SUCCESS) {
        return;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Host-to-compute barrier: CPU writes (e.g. addSineWave) made visible to compute shader
    VkMemoryBarrier hostToCompute{};
    hostToCompute.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    hostToCompute.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    hostToCompute.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 1, &hostToCompute, 0, nullptr, 0, nullptr);

    int pageIdx = waterPage & 1;

    WaterComputeParams params{};
    params.gridSize = 256;
    params.heroX = heroX;
    params.heroY = heroY;
    params.worldTime = worldTime;
    params.runBaseWave = runBaseWave ? 1 : 0;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_waterComputePipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_waterComputePipelineLayout, 0, 1, &m_waterComputeDescriptorSets[pageIdx], 0, nullptr);
    vkCmdPushConstants(cmd, m_waterComputePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(WaterComputeParams), &params);

    // 16 x 16 workgroups of (16, 16) threads = 256 x 256 invocations
    vkCmdDispatch(cmd, 16, 16, 1);

    // Compute-to-host barrier: compute writes made visible to CPU host memory
    VkMemoryBarrier computeToHost{};
    computeToHost.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    computeToHost.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    computeToHost.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
        0, 1, &computeToHost, 0, nullptr, 0, nullptr);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
}

int* GPUContext::GetWaterHeightBuffer(int page)
{
    if (!m_waterComputeInitialized) {
        if (!InitWaterCompute()) return nullptr;
    }
    if (!m_waterSSBOMapped) return nullptr;
    if (page < 0) page = 0;
    if (page > 3) page = 3;
    return static_cast<int*>(m_waterSSBOMapped) + (page * 256 * 256);
}

void GPUContext::CleanupWaterCompute()
{
    if (m_waterComputeDescriptorSets[0] != VK_NULL_HANDLE || m_waterComputeDescriptorSets[1] != VK_NULL_HANDLE) {
        VkDescriptorSet sets[2] = { m_waterComputeDescriptorSets[0], m_waterComputeDescriptorSets[1] };
        uint32_t count = 0;
        if (sets[0] != VK_NULL_HANDLE) count++;
        if (sets[1] != VK_NULL_HANDLE) count++;
        if (count > 0 && m_descriptorPool != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(m_device, m_descriptorPool, count, sets);
        }
        m_waterComputeDescriptorSets[0] = VK_NULL_HANDLE;
        m_waterComputeDescriptorSets[1] = VK_NULL_HANDLE;
    }
    if (m_waterComputePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_waterComputePipeline, nullptr);
        m_waterComputePipeline = VK_NULL_HANDLE;
    }
    if (m_waterComputePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_waterComputePipelineLayout, nullptr);
        m_waterComputePipelineLayout = VK_NULL_HANDLE;
    }
    if (m_waterComputeDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_waterComputeDescriptorLayout, nullptr);
        m_waterComputeDescriptorLayout = VK_NULL_HANDLE;
    }
    if (m_waterSSBO != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_waterSSBO, nullptr);
        m_waterSSBO = VK_NULL_HANDLE;
    }
    if (m_waterSSBOMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_waterSSBOMemory, nullptr);
        m_waterSSBOMemory = VK_NULL_HANDLE;
    }
    m_waterSSBOMapped = nullptr;
    m_waterComputeInitialized = false;
}

void GPUContext::AddDynamicLight(float x, float y, float z, const float* lightColor, float range)
{
    if (range <= 0.0f || !lightColor) return;
    if (m_dynamicLights.size() >= 128) return;

    GPUDynamicPointLight light{};
    light.posRadius = glm::vec4(x, y, z, range * 100.0f); // TERRAIN_SCALE = 100.0f
    light.colorIntensity = glm::vec4(lightColor[0], lightColor[1], lightColor[2], 1.0f);
    m_dynamicLights.push_back(light);
}

void GPUContext::ClearDynamicLights()
{
    m_dynamicLights.clear();
}

bool GPUContext::InitClothCompute()
{
    if (m_clothComputeInitialized) return true;
    if (m_device == VK_NULL_HANDLE) return false;

    // 1. Create SSBO for particles, links, spheres (total 256 KB)
    VkDeviceSize ssboSize = 256 * 1024;
    if (!CreateBufferHelper(m_device, m_physicalDevice, ssboSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_clothSSBO, m_clothSSBOMemory, &m_clothSSBOMapped)) {
        std::cerr << "[GPUContext] Failed to allocate Cloth SSBO!" << std::endl;
        return false;
    }

    // 2. Create Descriptor Set Layout (Bindings 0, 1, 2)
    std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
    for (uint32_t i = 0; i < 3; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_clothComputeDescriptorLayout) != VK_SUCCESS) {
        return false;
    }

    // 3. Create Pipeline Layout with Push Constants
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ClothComputeParams);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_clothComputeDescriptorLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_clothComputePipelineLayout) != VK_SUCCESS) {
        return false;
    }

    // 4. Create Compute Pipeline
    auto compCode = ReadSPVFile("cloth.comp.spv");
    if (compCode.empty()) {
        std::cerr << "[GPUContext] Failed to load cloth.comp.spv!" << std::endl;
        return false;
    }
    VkShaderModule compModule = CreateShaderModule(compCode);
    if (!compModule) return false;

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = compModule;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.layout = m_clothComputePipelineLayout;

    if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_clothComputePipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(m_device, compModule, nullptr);
        std::cerr << "[GPUContext] Failed to create cloth compute pipeline!" << std::endl;
        return false;
    }
    vkDestroyShaderModule(m_device, compModule, nullptr);

    // 5. Allocate Descriptor Set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_clothComputeDescriptorLayout;
    if (vkAllocateDescriptorSets(m_device, &allocInfo, &m_clothComputeDescriptorSet) != VK_SUCCESS) {
        return false;
    }

    VkDescriptorBufferInfo bufferInfos[3]{};
    bufferInfos[0].buffer = m_clothSSBO;
    bufferInfos[0].offset = 0;
    bufferInfos[0].range = 64 * 1024;

    bufferInfos[1].buffer = m_clothSSBO;
    bufferInfos[1].offset = 64 * 1024;
    bufferInfos[1].range = 128 * 1024;

    bufferInfos[2].buffer = m_clothSSBO;
    bufferInfos[2].offset = 192 * 1024;
    bufferInfos[2].range = 64 * 1024;

    std::array<VkWriteDescriptorSet, 3> writes{};
    for (uint32_t b = 0; b < 3; b++) {
        writes[b].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[b].dstSet = m_clothComputeDescriptorSet;
        writes[b].dstBinding = b;
        writes[b].descriptorCount = 1;
        writes[b].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[b].pBufferInfo = &bufferInfos[b];
    }
    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    m_clothComputeInitialized = true;
    std::cout << "[GPUContext] Cloth Compute Pipeline initialized successfully." << std::endl;
    return true;
}

void GPUContext::DispatchClothSimulation(GPUClothParticle* particles, uint32_t numParticles,
                                         const GPUClothLink* links, uint32_t numLinks,
                                         const GPUClothSphereCol* spheres, uint32_t numSpheres,
                                         float fTime, float windX, float windY, float gravity, uint32_t dwType)
{
    if (!particles || numParticles == 0) return;
    if (!m_clothComputeInitialized) {
        if (!InitClothCompute()) return;
    }
    if (!m_frameActive) {
        if (!BeginFrame()) return;
    }

    char* mapped = static_cast<char*>(m_clothSSBOMapped);
    size_t particleBytes = (std::min)(static_cast<size_t>(numParticles * sizeof(GPUClothParticle)), static_cast<size_t>(64 * 1024));
    memcpy(mapped, particles, particleBytes);

    if (links && numLinks > 0) {
        size_t linkBytes = (std::min)(static_cast<size_t>(numLinks * sizeof(GPUClothLink)), static_cast<size_t>(128 * 1024));
        memcpy(mapped + 64 * 1024, links, linkBytes);
    }
    if (spheres && numSpheres > 0) {
        size_t sphereBytes = (std::min)(static_cast<size_t>(numSpheres * sizeof(GPUClothSphereCol)), static_cast<size_t>(64 * 1024));
        memcpy(mapped + 192 * 1024, spheres, sphereBytes);
    }

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    ClothComputeParams params{};
    params.numParticles = numParticles;
    params.numLinks = numLinks;
    params.numSpheres = numSpheres;
    params.subSteps = 1;
    params.fTime = fTime;
    params.windX = windX;
    params.windY = windY;
    params.gravity = gravity;
    params.dwType = dwType;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_clothComputePipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_clothComputePipelineLayout, 0, 1, &m_clothComputeDescriptorSet, 0, nullptr);
    vkCmdPushConstants(cmd, m_clothComputePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ClothComputeParams), &params);

    uint32_t groupCountX = (numParticles + 63) / 64;
    vkCmdDispatch(cmd, groupCountX, 1, 1);

    VkMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT,
        0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

    memcpy(particles, mapped, particleBytes);
}

void GPUContext::CleanupClothCompute()
{
    if (m_clothComputePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_clothComputePipeline, nullptr);
        m_clothComputePipeline = VK_NULL_HANDLE;
    }
    if (m_clothComputePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_clothComputePipelineLayout, nullptr);
        m_clothComputePipelineLayout = VK_NULL_HANDLE;
    }
    if (m_clothComputeDescriptorLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_clothComputeDescriptorLayout, nullptr);
        m_clothComputeDescriptorLayout = VK_NULL_HANDLE;
    }
    if (m_clothSSBO != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_clothSSBO, nullptr);
        m_clothSSBO = VK_NULL_HANDLE;
    }
    if (m_clothSSBOMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_clothSSBOMemory, nullptr);
        m_clothSSBOMemory = VK_NULL_HANDLE;
    }
    m_clothSSBOMapped = nullptr;
    m_clothComputeDescriptorSet = VK_NULL_HANDLE;
    m_clothComputeInitialized = false;
}

