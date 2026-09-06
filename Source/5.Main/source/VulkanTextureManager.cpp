#include "stdafx.h"
#include "VulkanTextureManager.h"
#include <iostream>
#include <cstring>

VulkanTextureManager& VulkanTextureManager::Instance()
{
    static VulkanTextureManager instance;
    return instance;
}

VulkanTextureManager::VulkanTextureManager()
{
}

VulkanTextureManager::~VulkanTextureManager()
{
    Shutdown();
}

bool VulkanTextureManager::Init(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, VkDescriptorPool descriptorPool, VkDescriptorSetLayout textureLayout)
{
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_descriptorPool = descriptorPool;
    m_textureLayout = textureLayout;
    m_fastDescriptorCache.assign(FAST_DESCRIPTOR_CACHE_SIZE, VK_NULL_HANDLE);
    m_initialized = true;

    // Create 1x1 default white texture with a reserved ID that cannot conflict with game IDs
    uint32_t whitePixel = 0xFFFFFFFF;
    m_defaultWhiteTextureId = 0x7FFFFFFF;
    CreateTextureWithId(m_defaultWhiteTextureId, 1, 1, 4, &whitePixel, true, false);

    // Create 1024x1024 RGBA dynamic font atlas texture
    std::vector<uint32_t> emptyAtlas(FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT, 0x00000000);
    CreateTextureWithId(BITMAP_FONT_DYNAMIC_ATLAS, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 4, emptyAtlas.data(), true, true);

    auto it = m_textures.find(BITMAP_FONT_DYNAMIC_ATLAS);
    if (it != m_textures.end()) {
        m_fontAtlasTexture = it->second;
    }

    // Create 4MB host-visible staging buffer for dynamic font atlas uploads
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT * 4;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_fontAtlasStagingBuffer) == VK_SUCCESS) {
        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(m_device, m_fontAtlasStagingBuffer, &memReq);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_fontAtlasStagingMemory) == VK_SUCCESS) {
            vkBindBufferMemory(m_device, m_fontAtlasStagingBuffer, m_fontAtlasStagingMemory, 0);
            vkMapMemory(m_device, m_fontAtlasStagingMemory, 0, bufferInfo.size, 0, &m_fontAtlasStagingMapped);
            if (m_fontAtlasStagingMapped) {
                memset(m_fontAtlasStagingMapped, 0, bufferInfo.size);
            }
        }
    }

    m_fontSlotCount = 0;
    m_fontAtlasDirty = false;

    return true;
}

void VulkanTextureManager::Shutdown()
{
    if (!m_initialized || m_device == VK_NULL_HANDLE) {
        return;
    }

    FlushUploadQueue();

    if (m_fontAtlasStagingMapped) {
        vkUnmapMemory(m_device, m_fontAtlasStagingMemory);
        m_fontAtlasStagingMapped = nullptr;
    }
    if (m_fontAtlasStagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_fontAtlasStagingBuffer, nullptr);
        m_fontAtlasStagingBuffer = VK_NULL_HANDLE;
    }
    if (m_fontAtlasStagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_fontAtlasStagingMemory, nullptr);
        m_fontAtlasStagingMemory = VK_NULL_HANDLE;
    }

    for (auto& [id, tex] : m_textures) {
        if (tex.descriptorSet != VK_NULL_HANDLE && m_descriptorPool != VK_NULL_HANDLE) {
            vkFreeDescriptorSets(m_device, m_descriptorPool, 1, &tex.descriptorSet);
            tex.descriptorSet = VK_NULL_HANDLE;
        }
        if (tex.sampler != VK_NULL_HANDLE) {
            vkDestroySampler(m_device, tex.sampler, nullptr);
        }
        if (tex.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device, tex.imageView, nullptr);
        }
        if (tex.image != VK_NULL_HANDLE) {
            vkDestroyImage(m_device, tex.image, nullptr);
        }
        if (tex.memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, tex.memory, nullptr);
        }
    }
    m_textures.clear();
    m_fastDescriptorCache.clear();
    m_initialized = false;
}

uint32_t VulkanTextureManager::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    if (m_physicalDevice == VK_NULL_HANDLE) {
        return 0;
    }
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    // Fallback 1: If HOST_VISIBLE is requested, ensure we find a HOST_VISIBLE memory type
    if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
                return i;
            }
        }
    }

    // Fallback 2: Any matching memory type bit
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i)) {
            return i;
        }
    }
    return 0;
}

bool VulkanTextureManager::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        vkDestroyImage(m_device, image, nullptr);
        image = VK_NULL_HANDLE;
        return false;
    }

    vkBindImageMemory(m_device, image, imageMemory, 0);
    return true;
}

bool VulkanTextureManager::CreateImageView(VkImage image, VkFormat format, VkImageView& imageView)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    return (vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) == VK_SUCCESS);
}

bool VulkanTextureManager::CreateSampler(bool linear, bool clamp, VkSampler& sampler)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    samplerInfo.minFilter = linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    samplerInfo.addressModeU = clamp ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = clamp ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = clamp ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    return (vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler) == VK_SUCCESS);
}

VkCommandBuffer VulkanTextureManager::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void VulkanTextureManager::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    if (commandBuffer == VK_NULL_HANDLE) return;
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

VkCommandBuffer VulkanTextureManager::EnsureUploadCommandBuffer()
{
    if (m_uploadCmd != VK_NULL_HANDLE) {
        return m_uploadCmd;
    }

    if (m_device == VK_NULL_HANDLE || m_commandPool == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(m_device, &allocInfo, &m_uploadCmd) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(m_uploadCmd, &beginInfo) != VK_SUCCESS) {
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_uploadCmd);
        m_uploadCmd = VK_NULL_HANDLE;
        return VK_NULL_HANDLE;
    }

    return m_uploadCmd;
}

void VulkanTextureManager::BeginBatch()
{
    m_batchRecording = true;
    EnsureUploadCommandBuffer();
}

void VulkanTextureManager::EndBatch()
{
    m_batchRecording = false;
    FlushUploadQueue();
}

void VulkanTextureManager::FlushUploadQueue()
{
    if (m_device == VK_NULL_HANDLE || !m_initialized || m_commandPool == VK_NULL_HANDLE || m_graphicsQueue == VK_NULL_HANDLE) {
        return;
    }

    if (m_uploadCmd == VK_NULL_HANDLE && m_pendingStaging.empty()) {
        return;
    }

    if (m_uploadCmd != VK_NULL_HANDLE) {
        vkEndCommandBuffer(m_uploadCmd);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_uploadCmd;

        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_graphicsQueue);

        vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_uploadCmd);
        m_uploadCmd = VK_NULL_HANDLE;
    }

    for (const auto& item : m_pendingStaging) {
        if (item.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, item.buffer, nullptr);
        }
        if (item.memory != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, item.memory, nullptr);
        }
    }
    m_pendingStaging.clear();
}

void VulkanTextureManager::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    EndSingleTimeCommands(commandBuffer);
}

void VulkanTextureManager::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    EndSingleTimeCommands(commandBuffer);
}

uint32_t VulkanTextureManager::CreateTexture(uint32_t width, uint32_t height, uint32_t components, const void* pixels, bool linearFilter, bool clamp)
{
    if (width == 0 || height == 0 || !pixels || m_device == VK_NULL_HANDLE || !m_initialized) {
        return 0;
    }

    // Convert pixels to RGBA8 if needed
    VkDeviceSize imageSize = width * height * 4;
    std::vector<uint8_t> rgbaBuffer;
    const void* uploadPixels = pixels;

    if (components == 1) {
        rgbaBuffer.resize(width * height * 4);
        const uint8_t* src = static_cast<const uint8_t*>(pixels);
        uint8_t* dst = rgbaBuffer.data();
        for (uint32_t i = 0; i < width * height; ++i) {
            dst[i * 4 + 0] = 255;
            dst[i * 4 + 1] = 255;
            dst[i * 4 + 2] = 255;
            dst[i * 4 + 3] = src[i];
        }
        uploadPixels = rgbaBuffer.data();
    } else if (components == 3) {
        rgbaBuffer.resize(width * height * 4);
        const uint8_t* src = static_cast<const uint8_t*>(pixels);
        uint8_t* dst = rgbaBuffer.data();
        for (uint32_t i = 0; i < width * height; ++i) {
            dst[i * 4 + 0] = src[i * 3 + 0];
            dst[i * 4 + 1] = src[i * 3 + 1];
            dst[i * 4 + 2] = src[i * 3 + 2];
            dst[i * 4 + 3] = 255;
        }
        uploadPixels = rgbaBuffer.data();
    }

    // 1. Create Staging Buffer
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        return 0;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        return 0;
    }

    vkBindBufferMemory(m_device, stagingBuffer, stagingBufferMemory, 0);

    void* data = nullptr;
    VkResult mapRes = vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    if (mapRes != VK_SUCCESS || data == nullptr) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return 0;
    }
    memcpy(data, uploadPixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device, stagingBufferMemory);

    // 2. Create Image & allocate device memory
    VulkanTexture tex{};
    tex.width = width;
    tex.height = height;
    tex.channels = 4;

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    if (!CreateImage(width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.image, tex.memory)) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return 0;
    }

    // 3. Record into active upload command buffer
    VkCommandBuffer cmd = EnsureUploadCommandBuffer();
    if (cmd == VK_NULL_HANDLE) {
        vkDestroyImage(m_device, tex.image, nullptr);
        vkFreeMemory(m_device, tex.memory, nullptr);
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return 0;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = tex.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    m_pendingStaging.push_back({ stagingBuffer, stagingBufferMemory });

    // 4. Create Image View & Sampler
    CreateImageView(tex.image, format, tex.imageView);
    CreateSampler(linearFilter, clamp, tex.sampler);

    // 5. Allocate Descriptor Set
    if (m_descriptorPool != VK_NULL_HANDLE && m_textureLayout != VK_NULL_HANDLE) {
        VkDescriptorSetAllocateInfo setAllocInfo{};
        setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setAllocInfo.descriptorPool = m_descriptorPool;
        setAllocInfo.descriptorSetCount = 1;
        setAllocInfo.pSetLayouts = &m_textureLayout;

        if (vkAllocateDescriptorSets(m_device, &setAllocInfo, &tex.descriptorSet) == VK_SUCCESS) {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = tex.imageView;
            imageInfo.sampler = tex.sampler;

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = tex.descriptorSet;
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    if (!m_batchRecording) {
        FlushUploadQueue();
    }

    uint32_t assignedId = m_nextTextureId++;
    tex.id = assignedId;
    m_textures[assignedId] = tex;
    if (assignedId < FAST_DESCRIPTOR_CACHE_SIZE) {
        m_fastDescriptorCache[assignedId] = tex.descriptorSet;
    }

    return assignedId;
}

uint32_t VulkanTextureManager::CreateTextureWithId(uint32_t textureId, uint32_t width, uint32_t height, uint32_t components, const void* pixels, bool linearFilter, bool clamp)
{
    if (width == 0 || height == 0 || !pixels || m_device == VK_NULL_HANDLE || !m_initialized) {
        return 0;
    }

    if (HasTexture(textureId)) {
        DestroyTexture(textureId);
    }

    // Convert pixels to RGBA8 if needed
    VkDeviceSize imageSize = width * height * 4;
    std::vector<uint8_t> rgbaBuffer;
    const void* uploadPixels = pixels;

    if (components == 1) {
        rgbaBuffer.resize(width * height * 4);
        const uint8_t* src = static_cast<const uint8_t*>(pixels);
        uint8_t* dst = rgbaBuffer.data();
        for (uint32_t i = 0; i < width * height; ++i) {
            dst[i * 4 + 0] = 255;
            dst[i * 4 + 1] = 255;
            dst[i * 4 + 2] = 255;
            dst[i * 4 + 3] = src[i];
        }
        uploadPixels = rgbaBuffer.data();
    } else if (components == 3) {
        rgbaBuffer.resize(width * height * 4);
        const uint8_t* src = static_cast<const uint8_t*>(pixels);
        uint8_t* dst = rgbaBuffer.data();
        for (uint32_t i = 0; i < width * height; ++i) {
            dst[i * 4 + 0] = src[i * 3 + 0];
            dst[i * 4 + 1] = src[i * 3 + 1];
            dst[i * 4 + 2] = src[i * 3 + 2];
            dst[i * 4 + 3] = 255;
        }
        uploadPixels = rgbaBuffer.data();
    }

    // 1. Create Staging Buffer
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        return 0;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        return 0;
    }

    vkBindBufferMemory(m_device, stagingBuffer, stagingBufferMemory, 0);

    void* data = nullptr;
    VkResult mapRes = vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    if (mapRes != VK_SUCCESS || data == nullptr) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return 0;
    }
    memcpy(data, uploadPixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device, stagingBufferMemory);

    // 2. Create Image & allocate device memory
    VulkanTexture tex{};
    tex.width = width;
    tex.height = height;
    tex.channels = 4;

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    if (!CreateImage(width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex.image, tex.memory)) {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return 0;
    }

    // 3. Record into active upload command buffer
    VkCommandBuffer cmd = EnsureUploadCommandBuffer();
    if (cmd == VK_NULL_HANDLE) {
        vkDestroyImage(m_device, tex.image, nullptr);
        vkFreeMemory(m_device, tex.memory, nullptr);
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
        return 0;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = tex.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, tex.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    m_pendingStaging.push_back({ stagingBuffer, stagingBufferMemory });

    // 4. Create Image View & Sampler
    CreateImageView(tex.image, format, tex.imageView);
    CreateSampler(linearFilter, clamp, tex.sampler);

    // 5. Allocate Descriptor Set
    if (m_descriptorPool != VK_NULL_HANDLE && m_textureLayout != VK_NULL_HANDLE) {
        VkDescriptorSetAllocateInfo setAllocInfo{};
        setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setAllocInfo.descriptorPool = m_descriptorPool;
        setAllocInfo.descriptorSetCount = 1;
        setAllocInfo.pSetLayouts = &m_textureLayout;

        if (vkAllocateDescriptorSets(m_device, &setAllocInfo, &tex.descriptorSet) == VK_SUCCESS) {
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = tex.imageView;
            imageInfo.sampler = tex.sampler;

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = tex.descriptorSet;
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    if (!m_batchRecording) {
        FlushUploadQueue();
    }

    tex.id = textureId;
    m_textures[textureId] = tex;
    if (textureId < FAST_DESCRIPTOR_CACHE_SIZE) {
        m_fastDescriptorCache[textureId] = tex.descriptorSet;
    }
    return textureId;
}

bool VulkanTextureManager::HasTexture(uint32_t textureId) const
{
    return m_textures.find(textureId) != m_textures.end();
}

void VulkanTextureManager::DestroyTexture(uint32_t textureId)
{
    auto it = m_textures.find(textureId);
    if (it == m_textures.end()) {
        return;
    }

    if (textureId < FAST_DESCRIPTOR_CACHE_SIZE) {
        m_fastDescriptorCache[textureId] = VK_NULL_HANDLE;
    }

    if (!m_pendingStaging.empty() || m_uploadCmd != VK_NULL_HANDLE) {
        FlushUploadQueue();
    }

    VulkanTexture& tex = it->second;
    if (tex.descriptorSet != VK_NULL_HANDLE && m_descriptorPool != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(m_device, m_descriptorPool, 1, &tex.descriptorSet);
        tex.descriptorSet = VK_NULL_HANDLE;
    }
    if (tex.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, tex.sampler, nullptr);
    }
    if (tex.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, tex.imageView, nullptr);
    }
    if (tex.image != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, tex.image, nullptr);
    }
    if (tex.memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, tex.memory, nullptr);
    }

    m_textures.erase(it);
}

VulkanTexture* VulkanTextureManager::GetTexture(uint32_t textureId)
{
    if (!m_initialized) {
        return nullptr;
    }
    auto it = m_textures.find(textureId);
    if (it != m_textures.end()) {
        return &it->second;
    }
    if (m_defaultWhiteTextureId != 0) {
        auto defIt = m_textures.find(m_defaultWhiteTextureId);
        if (defIt != m_textures.end()) {
            return &defIt->second;
        }
    }
    return nullptr;
}

VkDescriptorSet VulkanTextureManager::GetDescriptorSet(uint32_t textureId)
{
    if (!m_initialized) {
        return VK_NULL_HANDLE;
    }
    if (textureId < FAST_DESCRIPTOR_CACHE_SIZE) {
        VkDescriptorSet ds = m_fastDescriptorCache[textureId];
        if (ds != VK_NULL_HANDLE) {
            return ds;
        }
    }
    VulkanTexture* tex = GetTexture(textureId);
    return tex ? tex->descriptorSet : VK_NULL_HANDLE;
}

VkDescriptorSet VulkanTextureManager::GetDefaultDescriptorSet()
{
    if (!m_initialized) {
        return VK_NULL_HANDLE;
    }
    return GetDescriptorSet(m_defaultWhiteTextureId);
}

VulkanTextureManager::FontSlotUV VulkanTextureManager::UploadFontSlot(const void* srcBuffer, uint32_t srcPitchWidth, uint32_t copyWidth, uint32_t copyHeight)
{
    FontSlotUV uv{ 0.0f, 0.0f, 0.0f, 0.0f };
    if (!m_fontAtlasStagingMapped || !srcBuffer || copyWidth == 0 || copyHeight == 0) return uv;

    uint32_t slot = m_fontSlotCount % MAX_FONT_SLOTS;
    m_fontSlotCount++;
    m_fontAtlasDirty = true;

    uint32_t col = slot % FONT_ATLAS_COLS;
    uint32_t row = slot / FONT_ATLAS_COLS;

    uint32_t dstX = col * FONT_SLOT_WIDTH;
    uint32_t dstY = row * FONT_SLOT_HEIGHT;

    uint8_t* dstBase = static_cast<uint8_t*>(m_fontAtlasStagingMapped);
    const uint8_t* srcBase = static_cast<const uint8_t*>(srcBuffer);

    uint32_t actualW = (copyWidth > FONT_SLOT_WIDTH) ? FONT_SLOT_WIDTH : copyWidth;
    uint32_t actualH = (copyHeight > FONT_SLOT_HEIGHT) ? FONT_SLOT_HEIGHT : copyHeight;

    for (uint32_t y = 0; y < actualH; ++y) {
        uint32_t dstOffset = ((dstY + y) * FONT_ATLAS_WIDTH + dstX) * 4;
        uint32_t srcOffset = (y * srcPitchWidth) * 4;
        memcpy(dstBase + dstOffset, srcBase + srcOffset, actualW * 4);
    }

    uv.u0 = static_cast<float>(dstX) / static_cast<float>(FONT_ATLAS_WIDTH);
    uv.v0 = static_cast<float>(dstY) / static_cast<float>(FONT_ATLAS_HEIGHT);
    uv.uWidth = static_cast<float>(copyWidth) / static_cast<float>(FONT_ATLAS_WIDTH);
    uv.vHeight = static_cast<float>(copyHeight) / static_cast<float>(FONT_ATLAS_HEIGHT);

    return uv;
}

void VulkanTextureManager::ResetFontAtlas()
{
    m_fontSlotCount = 0;
    m_fontAtlasDirty = false;
}

void VulkanTextureManager::FlushFontAtlas(VkCommandBuffer cmd)
{
    if (!m_fontAtlasDirty || m_fontAtlasTexture.image == VK_NULL_HANDLE || m_fontAtlasStagingBuffer == VK_NULL_HANDLE) {
        return;
    }

    uint32_t activeRows = (m_fontSlotCount + FONT_ATLAS_COLS - 1) / FONT_ATLAS_COLS;
    if (activeRows == 0) activeRows = 1;
    if (activeRows > FONT_ATLAS_ROWS) activeRows = FONT_ATLAS_ROWS;
    uint32_t copyHeight = activeRows * FONT_SLOT_HEIGHT;

    // 1. Transition image from SHADER_READ_ONLY_OPTIMAL to TRANSFER_DST_OPTIMAL
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_fontAtlasTexture.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    // 2. Copy staging buffer to atlas image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = FONT_ATLAS_WIDTH;
    region.bufferImageHeight = FONT_ATLAS_HEIGHT;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { FONT_ATLAS_WIDTH, copyHeight, 1 };

    vkCmdCopyBufferToImage(cmd, m_fontAtlasStagingBuffer, m_fontAtlasTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // 3. Transition image back to SHADER_READ_ONLY_OPTIMAL
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    m_fontAtlasDirty = false;
}

