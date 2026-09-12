#pragma once

#include "volk.h"
#include <unordered_map>
#include <vector>
#include <cstdint>

struct VulkanTexture
{
    uint32_t id = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 4;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

class VulkanTextureManager
{
public:
    static VulkanTextureManager& Instance();

    bool Init(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, VkDescriptorPool descriptorPool, VkDescriptorSetLayout textureLayout);
    void Shutdown();

    uint32_t CreateTexture(uint32_t width, uint32_t height, uint32_t components, const void* pixels, bool linearFilter = true, bool clamp = false);
    uint32_t CreateTextureWithId(uint32_t textureId, uint32_t width, uint32_t height, uint32_t components, const void* pixels, bool linearFilter = true, bool clamp = false);
    void DestroyTexture(uint32_t textureId);
    VulkanTexture* GetTexture(uint32_t textureId);
    VkDescriptorSet GetDescriptorSet(uint32_t textureId);
    bool HasTexture(uint32_t textureId) const;

    uint32_t GetDefaultWhiteTexture() const { return m_defaultWhiteTextureId; }
    VkDescriptorSet GetDefaultDescriptorSet();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void BeginBatch();
    void EndBatch();
    void FlushUploadQueue();
    bool HasPendingUploads() const { return m_uploadCmd != VK_NULL_HANDLE || !m_pendingStaging.empty(); }

    // Dynamic Font Atlas
    struct FontSlotUV {
        float u0, v0, uWidth, vHeight;
    };
    FontSlotUV UploadFontSlot(const void* srcBuffer, uint32_t srcPitchWidth, uint32_t copyWidth, uint32_t copyHeight);
    FontSlotUV UploadFontSlotSpecific(uint32_t slot, const void* srcBuffer, uint32_t srcPitchWidth, uint32_t copyWidth, uint32_t copyHeight);
    void ResetFontAtlas();
    bool IsFontAtlasDirty() const { return m_fontAtlasDirty; }
    void FlushFontAtlas(VkCommandBuffer cmd);

private:
    VulkanTextureManager();
    ~VulkanTextureManager();

    VulkanTextureManager(const VulkanTextureManager&) = delete;
    VulkanTextureManager& operator=(const VulkanTextureManager&) = delete;

    bool CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    bool CreateImageView(VkImage image, VkFormat format, VkImageView& imageView);
    bool CreateSampler(bool linear, bool clamp, VkSampler& sampler);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
    VkCommandBuffer EnsureUploadCommandBuffer();

    struct PendingStagingResource {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_textureLayout = VK_NULL_HANDLE;

    VkCommandBuffer m_uploadCmd = VK_NULL_HANDLE;
    VkFence m_uploadFence = VK_NULL_HANDLE;
    std::vector<PendingStagingResource> m_pendingStaging;
    bool m_batchRecording = false;

    std::unordered_map<uint32_t, VulkanTexture> m_textures;
    static constexpr uint32_t FAST_DESCRIPTOR_CACHE_SIZE = 65536;
    std::vector<VkDescriptorSet> m_fastDescriptorCache;
    VulkanTexture m_defaultWhiteTexture{};
    uint32_t m_nextTextureId = 1;
    uint32_t m_defaultWhiteTextureId = 0;
    bool m_initialized = false;

    // Dynamic Font Atlas members
    VulkanTexture m_fontAtlasTexture{};
    VkBuffer m_fontAtlasStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_fontAtlasStagingMemory = VK_NULL_HANDLE;
    void* m_fontAtlasStagingMapped = nullptr;
    uint32_t m_fontSlotCount = 0;
    uint32_t m_dynamicSlotCount = 0;
    uint32_t m_fontAtlasMaxDirtyRow = 0;
    bool m_fontAtlasDirty = false;
};

#define BITMAP_FONT_DYNAMIC_ATLAS 60000
#define FONT_ATLAS_WIDTH  2048
#define FONT_ATLAS_HEIGHT 4096
#define FONT_SLOT_WIDTH   256
#define FONT_SLOT_HEIGHT  32
#define FONT_ATLAS_COLS   (FONT_ATLAS_WIDTH / FONT_SLOT_WIDTH)   // 8
#define FONT_ATLAS_ROWS   (FONT_ATLAS_HEIGHT / FONT_SLOT_HEIGHT) // 128
#define MAX_FONT_SLOTS    (FONT_ATLAS_COLS * FONT_ATLAS_ROWS)     // 1024
#define CACHE_FONT_SLOTS  896
#define DYNAMIC_SLOT_START 896
#define DYNAMIC_FONT_SLOTS (MAX_FONT_SLOTS - DYNAMIC_SLOT_START)  // 128

