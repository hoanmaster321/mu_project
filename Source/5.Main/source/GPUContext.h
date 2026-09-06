#pragma once

#include "volk.h"
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <utility>
#include <cstdint>
#include <glm/glm.hpp>
#include "GPURenderState.h"
#include "BatchRenderer.h"

// Forward declaration
struct _Mesh_t;
typedef struct _Mesh_t Mesh_t;
class BMD;

class GPUContext
{
public:
    // ------------------------------------------------------------------------
    // Batch & Command Structures
    // ------------------------------------------------------------------------
    struct TerrainDrawCmd
    {
        uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
        int32_t vertexOffset = 0;
    };

    struct TerrainMergedBatch
    {
        int batchType = 0;
        uint32_t renderFlags = 0;
        int textureIndex = 0;
        std::vector<TerrainDrawCmd> cmds;
    };

    using GPUDynamicPointLight = ::GPUDynamicPointLight;
    using TerrainVertUBO = ::TerrainVertUBO;

    struct SpriteBatch
    {
        int texture = 0;
        int blendType = 0;
        float scale = 1.0f;
        uint32_t firstVertex = 0;
        uint32_t vertexCount = 0;
        bool tile = false;
    };

    struct SpriteVertUBO
    {
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
        glm::vec2 screenSize;
        float scale = 1.0f;
        uint32_t instanceBase = 0;
    };

    struct alignas(16) GPUSpriteInstance
    {
        glm::vec4 position;    // xyz = world/screen pos, w = unused
        glm::vec2 size;        // half-extents
        float rotation = 0.0f; // degrees
        float _pad0 = 0.0f;
        glm::vec4 light;       // rgb + custom tint in a
        glm::vec4 uv;          // xy = uv origin, zw = uv size
        float alpha = 1.0f;
        float depth = 0.0f;
        glm::vec2 _pad1{ 0.0f };
    };

    struct MeshDrawGroup
    {
        Mesh_t* mesh = nullptr;
        BMD* model = nullptr;
        int modelIndex = 0;
        int meshIndex = 0;
        int textureIndex = 0;
        uint32_t renderFlags = 0;
        int blendType = 0;
        bool depthWrite = true;
        bool noCull = false;
        uint32_t firstIndex = 0;
        int32_t vertexOffset = 0;
        uint32_t instanceCount = 0;
        uint32_t firstInstance = 0;
    };

    struct ShadowDrawGroup
    {
        Mesh_t* mesh = nullptr;
        BMD* model = nullptr;
        int modelIndex = 0;
        int meshIndex = 0;
        uint32_t firstIndex = 0;
        int32_t vertexOffset = 0;
        uint32_t instanceCount = 0;
        uint32_t firstInstance = 0;
    };

    struct GPUInstanceData
    {
        glm::vec4 modelMatrix{ 0.0f };
        glm::vec4 lightColor{ 1.0f };
        int32_t   renderMode = 0;
        int32_t   finalRenderMode = 0;
        int32_t   sourceIndex = 0;
        int32_t   chromeMode = 0;
        glm::vec2 blendUV{ 0.0f };
        float     newScale = 0.0f;
        float     alpha = 1.0f;
        float     bodyScale = 1.0f;
        float     boneScale = 1.0f;
        int32_t   boneOffset = 0;
        float     customIntensity = 1.0f;
        glm::vec3 bodyLightColor{ 1.0f };
        uint32_t  flags = 0;
    };
    static_assert(sizeof(GPUInstanceData) == 96, "GPUInstanceData must match the std430 InstanceData layout in general.vert.glsl");

    struct GeneralVertUBO
    {
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
        float worldTime = 0.0f;
        float _pad0a = 0, _pad0b = 0, _pad0c = 0;
        glm::vec3 shadowAngle{ 0.0f };
        float _pad1 = 0;
        glm::vec4 lightPosition{ 0.0f, 0.0f, 0.0f, 1.0f };
        uint32_t instanceBase = 0;
        uint32_t vertexBase = 0;
        float _pad3b = 0, _pad3c = 0;
    };

    struct GeneralFragUBO
    {
        float batchTexture = 1.0f;
        float brightness = 1.0f;
        float alphaTestThreshold = 0.01f;
        float padding = 0.0f;
    };

    struct ShadowVertUBO
    {
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
        uint32_t instanceBase = 0;
        float _pad0a = 0, _pad0b = 0, _pad0c = 0;
    };

    struct ImageVertUBO
    {
        glm::vec2 screenSize;
        uint32_t instanceBase = 0;
        float _pad = 0.0f;
    };

    // ------------------------------------------------------------------------
    // Metrics
    // ------------------------------------------------------------------------
    uint32_t m_frameMeshBatchCalls = 0;
    uint32_t m_frameImagesMerged = 0;

    static GPUContext& Instance();

    bool Init(SDL_Window* window, int width, int height);
    void Shutdown();
    void WaitIdle();

    // Frame Lifecycle
    float m_clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    void SetClearColor(float r, float g, float b, float a = 1.0f)
    {
        m_clearColor[0] = r;
        m_clearColor[1] = g;
        m_clearColor[2] = b;
        m_clearColor[3] = a;
    }
    bool BeginFrame();
    void EndFrame();
    bool Present();

    void OnResize(int width, int height);

    // Native Draw Calls
    void DrawTerrainMerged(const void* vertices, uint32_t vertByteSize, const void* indices, uint32_t indexByteSize, const std::vector<TerrainMergedBatch>& batches, const TerrainVertUBO& ubo);
    void DrawTerrainMergedPreallocated(uint32_t vertOffset, uint32_t vertByteSize, uint32_t idxOffset, uint32_t idxByteSize, const std::vector<TerrainMergedBatch>& batches, const TerrainVertUBO& ubo);
    TerrainVertex_t* AllocateTerrainVertexBuffer(uint32_t vertexCount, uint32_t& outVertOffset);
    uint32_t* AllocateTerrainIndexBuffer(uint32_t indexCount, uint32_t& outIdxOffset);

    void DrawSprites(const void* instances, uint32_t instByteSize, const std::vector<SpriteBatch>& batches, const SpriteVertUBO& ubo);
    void DrawSpritesPreallocated(uint32_t instOffset, uint32_t instByteSize, uint32_t firstInstIndex, const std::vector<SpriteBatch>& batches, const SpriteVertUBO& ubo);
    GPUSpriteInstance* AllocateSpriteInstanceBuffer(uint32_t count, uint32_t& outFirstInstance, uint32_t& outInstanceSSBOOffset);
    void DrawMeshesAndShadows(const void* instances, uint32_t instByteSize, const std::vector<MeshDrawGroup>& meshGroups, const std::vector<ShadowDrawGroup>& shadowGroups, const GeneralVertUBO& vertUBO, const GeneralFragUBO& fragUBO, const ShadowVertUBO& shadowUBO);
    void DrawInstancedMesh(VkBuffer vbo, VkBuffer ibo, uint32_t indexCount, uint32_t instanceCount, uint32_t textureId, int blendType, bool depthTest, bool depthWrite, const void* uboData, uint32_t uboSize, const void* instancesData, uint32_t instancesByteSize, const void* boneData, uint32_t boneSize);
    void DrawInstancedMeshPreallocatedBones(VkBuffer vbo, VkBuffer ibo, uint32_t indexCount, uint32_t instanceCount, uint32_t textureId, int blendType, bool depthTest, bool depthWrite, const void* uboData, uint32_t uboSize, const void* instancesData, uint32_t instancesByteSize, uint32_t boneSSBOOffset, uint32_t boneBaseRows, uint32_t boneByteSize);
    void DrawInstancedMeshPreallocated(VkBuffer vbo, VkBuffer ibo, uint32_t indexCount, uint32_t instanceCount, uint32_t textureId, int blendType, bool depthTest, bool depthWrite, const void* uboData, uint32_t uboSize, uint32_t instanceSSBOStart, uint32_t baseInstanceIndex, uint32_t boneSSBOOffset, uint32_t boneByteSize);
    GPUInstanceData* AllocateInstanceBuffer(uint32_t instanceCount, uint32_t& outBaseInstanceIndex, uint32_t& outInstanceSSBOStart);

    struct ImageBatchRun {
        ImageBatchKey key{};
        uint32_t count = 0;
        ImageBatchRun() = default;
        ImageBatchRun(const ImageBatchKey& k, uint32_t c) : key(k), count(c) {}
    };
    void DrawImages(const std::vector<std::pair<ImageBatchKey, std::vector<GPUImageInstance>>>& batches);
    void DrawImagesPreallocated(uint32_t baseInstance, uint32_t instanceSSBOOffset, const std::vector<ImageBatchRun>& batchRuns);
    GPUImageInstance* AllocateImageInstanceBuffer(uint32_t count, uint32_t& outBaseInstance, uint32_t& outInstanceSSBOOffset);
    void ClearDepthBuffer();

    // Mesh Static Buffers (Vulkan)
    bool CreateMeshBuffers(const void* vData, uint32_t vSize, const void* iData, uint32_t iSize, VkBuffer& outVBO, VkDeviceMemory& outVBOMem, VkBuffer& outIBO, VkDeviceMemory& outIBOMem);
    void DestroyMeshBuffers(VkBuffer& vbo, VkDeviceMemory& vboMem, VkBuffer& ibo, VkDeviceMemory& iboMem);

    // Bone Staging Buffer (for Skeletal Skinning)
    void* AllocateBoneBuffer(uint32_t byteSize, uint32_t& outBoneBaseRows, uint32_t& outBoneSSBOOffset);
    void AppendBoneRows(const void* boneRows, uint32_t byteSize);
    void ClearBoneRows();

    void FlushClothDraws();
    bool IsRenderTargetPushed() const { return m_renderTargetPushed; }
    void PushRenderTarget() { m_renderTargetPushed = true; }
    void PopRenderTarget() { m_renderTargetPushed = false; }

    // Compute Shaders & Water Simulation
    struct WaterComputeParams {
        int32_t gridSize = 256;
        int32_t heroX = 0;
        int32_t heroY = 0;
        float   worldTime = 0.0f;
        int32_t runBaseWave = 1;
    };
    bool InitWaterCompute();
    void DispatchWaterWave(int waterPage, int heroX, int heroY, float worldTime, bool runBaseWave);
    int* GetWaterHeightBuffer(int page = 0);
    void CleanupWaterCompute();

    // Compute Shaders & Cloth Simulation
    struct GPUClothParticle {
        glm::vec4 pos;     // xyz = pos, w = invMass
        glm::vec4 vel;     // xyz = vel, w = state
        glm::vec4 force;   // xyz = force, w = oneTimeMoveCount
        glm::vec4 oneTime; // xyz = oneTimeMove, w = pad
    };
    struct GPUClothLink {
        uint32_t v0;
        uint32_t v1;
        float distMin;
        float distMax;
        uint32_t style;
        uint32_t pad0, pad1, pad2;
    };
    struct GPUClothSphereCol {
        glm::vec4 centerRadius; // xyz = center, w = radius
    };
    struct ClothComputeParams {
        uint32_t numParticles = 0;
        uint32_t numLinks = 0;
        uint32_t numSpheres = 0;
        uint32_t subSteps = 5;
        float fTime = 0.005f;
        float windX = 0.0f;
        float windY = 0.0f;
        float gravity = -9.8f;
        uint32_t dwType = 0;
    };
    bool InitClothCompute();
    void DispatchClothSimulation(GPUClothParticle* particles, uint32_t numParticles,
                                 const GPUClothLink* links, uint32_t numLinks,
                                 const GPUClothSphereCol* spheres, uint32_t numSpheres,
                                 float fTime, float windX, float windY, float gravity, uint32_t dwType);
    void CleanupClothCompute();

    // State management
    void SetRenderState(const GPURenderState& state);
    void SetViewport(float x, float y, float width, float height);
    void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

    // Dynamic Point Lighting
    void AddDynamicLight(float x, float y, float z, const float* lightColor, float range);
    void ClearDynamicLights();
    const std::vector<GPUDynamicPointLight>& GetDynamicLights() const { return m_dynamicLights; }

    // Screenshot
    void RequestScreenshot(const std::string& filename);

    // Getters
    VkDevice GetDevice() const { return m_device; }
    VkRenderPass GetRenderPass() const { return m_renderPass; }
    VkCommandBuffer GetCurrentCommandBuffer() const { return m_commandBuffers[m_currentFrame]; }
    uint32_t GetCurrentFrame() const { return m_currentFrame; }
    bool IsInitialized() const { return m_initialized; }
    bool IsFrameActive() const { return m_frameActive; }
    uint32_t GetDrawCallsThisFrame() const { return m_drawCallsThisFrame; }

private:
    GPUContext();
    ~GPUContext();

    GPUContext(const GPUContext&) = delete;
    GPUContext& operator=(const GPUContext&) = delete;

    bool CreateSwapchain(int width, int height);
    void CleanupSwapchain();
    bool RecreateSwapchain();

    bool CreateRenderPass();
    bool CreateDepthResources();
    void DestroyDepthResources();
    bool CreateFramebuffers();
    bool CreateCommandPoolAndBuffers();
    bool CreateSyncObjects();
    bool CreateDescriptorSetLayouts();
    bool CreateDescriptorPool();
    bool CreateDynamicBuffers();
    void DestroyDynamicBuffers();
    bool CreateUnitQuads();
    void DestroyUnitQuads();
    bool CreatePipelines();

    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    std::vector<char> ReadSPVFile(const std::string& filename);

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    struct FrameResource;
    uint32_t AllocateUBOData(FrameResource& res, const void* data, uint32_t size);
    uint32_t AllocateIndirectData(FrameResource& res, const void* data, uint32_t size);

    struct FrameResource
    {
        // Dynamic Vertex Buffer
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
        void* vertexMapped = nullptr;
        uint32_t vertexOffset = 0;
        uint32_t vertexCapacity = 16 * 1024 * 1024; // 16 MB

        // Dynamic Index Buffer
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexMemory = VK_NULL_HANDLE;
        void* indexMapped = nullptr;
        uint32_t indexOffset = 0;
        uint32_t indexCapacity = 4 * 1024 * 1024; // 4 MB

        // Dynamic Instance SSBO (Set 0 Binding 0)
        VkBuffer instanceSSBO = VK_NULL_HANDLE;
        VkDeviceMemory instanceSSBOMemory = VK_NULL_HANDLE;
        void* instanceSSBOMapped = nullptr;
        uint32_t instanceSSBOOffset = 0;
        uint32_t instanceSSBOCapacity = 8 * 1024 * 1024; // 8 MB

        // Dynamic Bone SSBO (Set 0 Binding 1)
        VkBuffer boneSSBO = VK_NULL_HANDLE;
        VkDeviceMemory boneSSBOMemory = VK_NULL_HANDLE;
        void* boneSSBOMapped = nullptr;
        uint32_t boneSSBOOffset = 0;
        uint32_t boneSSBOCapacity = 8 * 1024 * 1024; // 8 MB

        // Dynamic Indirect Draw Command Buffer
        VkBuffer indirectBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indirectMemory = VK_NULL_HANDLE;
        void* indirectMapped = nullptr;
        uint32_t indirectOffset = 0;
        uint32_t indirectCapacity = 2 * 1024 * 1024; // 2 MB (~100,000 commands)

        // Dynamic Uniform Buffer (Set 1 Binding 0 VertUBO, Set 3 Binding 0 FragUBO)
        VkBuffer uboBuffer = VK_NULL_HANDLE;
        VkDeviceMemory uboMemory = VK_NULL_HANDLE;
        void* uboMapped = nullptr;
        uint32_t uboOffset = 0;
        uint32_t uboCapacity = 4 * 1024 * 1024; // 4 MB

        // Descriptor Sets for this frame
        VkDescriptorSet storageDescriptorSet = VK_NULL_HANDLE;   // Set 0 (Instance SSBO + Bone SSBO)
        VkDescriptorSet uniformDescriptorSet = VK_NULL_HANDLE;   // Set 1 (VertUniforms UBO)
        VkDescriptorSet fragUniformDescriptorSet = VK_NULL_HANDLE; // Set 3 (FragUniforms UBO)
    };

    SDL_Window* m_window = nullptr;
    int m_width = 1280;
    int m_height = 720;
    bool m_initialized = false;
    bool m_renderTargetPushed = false;

    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;

    // Swapchain
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D m_swapchainExtent = { 1280, 720 };
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    std::vector<VkFramebuffer> m_swapchainFramebuffers;

    // Depth buffer
    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;
    VkFormat m_depthFormat = VK_FORMAT_D24_UNORM_S8_UINT;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;

    // Sync objects
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

    // Descriptors layouts & pool
    VkDescriptorSetLayout m_storageDescriptorLayout = VK_NULL_HANDLE;     // Set 0: SSBO (Instance + Bone)
    VkDescriptorSetLayout m_uniformDescriptorLayout = VK_NULL_HANDLE;     // Set 1: VertUniforms UBO
    VkDescriptorSetLayout m_textureDescriptorLayout = VK_NULL_HANDLE;     // Set 2: Combined Image Sampler
    VkDescriptorSetLayout m_fragUniformDescriptorLayout = VK_NULL_HANDLE; // Set 3: FragUniforms UBO
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    // Frame dynamic resources
    FrameResource m_frameResources[MAX_FRAMES_IN_FLIGHT];

    // Static Unit Quads (for Sprites and UI Images)
    VkBuffer m_spriteQuadVBO = VK_NULL_HANDLE;
    VkDeviceMemory m_spriteQuadVBOMemory = VK_NULL_HANDLE;
    VkBuffer m_spriteQuadIBO = VK_NULL_HANDLE;
    VkDeviceMemory m_spriteQuadIBOMemory = VK_NULL_HANDLE;

    VkBuffer m_imageQuadVBO = VK_NULL_HANDLE;
    VkDeviceMemory m_imageQuadVBOMemory = VK_NULL_HANDLE;
    VkBuffer m_imageQuadIBO = VK_NULL_HANDLE;
    VkDeviceMemory m_imageQuadIBOMemory = VK_NULL_HANDLE;

    // Pipelines
    VkPipelineLayout m_terrainPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_terrainPipelineOpaque = VK_NULL_HANDLE;
    VkPipeline m_terrainPipelineBlend = VK_NULL_HANDLE;
    VkPipeline m_terrainPipelineAdd = VK_NULL_HANDLE;
    VkPipeline m_terrainPipelineDark = VK_NULL_HANDLE;
    VkPipeline m_terrainPipelineBlendNoDepth = VK_NULL_HANDLE;
    VkPipeline m_terrainPipelineAddNoDepth = VK_NULL_HANDLE;

    VkPipelineLayout m_meshPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_meshPipelineOpaque = VK_NULL_HANDLE;
    VkPipeline m_meshPipelineAlphaBlend = VK_NULL_HANDLE;
    VkPipeline m_meshPipelineAdditive = VK_NULL_HANDLE;
    VkPipeline m_meshPipelineAlphaTest = VK_NULL_HANDLE;
    VkPipeline m_meshPipelineDark = VK_NULL_HANDLE;

    VkPipelineLayout m_shadowPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_shadowPipeline = VK_NULL_HANDLE;

    VkPipelineLayout m_spritePipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_spritePipelineAlpha = VK_NULL_HANDLE;
    VkPipeline m_spritePipelineAdd = VK_NULL_HANDLE;

    VkPipelineLayout m_uiPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_uiPipeline = VK_NULL_HANDLE;
    VkPipeline m_uiPipelineAdd = VK_NULL_HANDLE;
    VkPipeline m_uiPipelineNone = VK_NULL_HANDLE;
    VkPipeline m_uiPipelineDark = VK_NULL_HANDLE;

    VkPipelineLayout m_clothPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_clothPipeline = VK_NULL_HANDLE;

    // Water Compute Pipeline
    VkDescriptorSetLayout m_waterComputeDescriptorLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_waterComputePipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_waterComputePipeline = VK_NULL_HANDLE;
    VkDescriptorSet m_waterComputeDescriptorSets[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
    VkBuffer m_waterSSBO = VK_NULL_HANDLE;
    VkDeviceMemory m_waterSSBOMemory = VK_NULL_HANDLE;
    void* m_waterSSBOMapped = nullptr;
    bool m_waterComputeInitialized = false;

    // Cloth Compute Pipeline
    VkDescriptorSetLayout m_clothComputeDescriptorLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_clothComputePipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_clothComputePipeline = VK_NULL_HANDLE;
    VkDescriptorSet m_clothComputeDescriptorSet = VK_NULL_HANDLE;
    VkBuffer m_clothSSBO = VK_NULL_HANDLE;
    VkDeviceMemory m_clothSSBOMemory = VK_NULL_HANDLE;
    void* m_clothSSBOMapped = nullptr;
    bool m_clothComputeInitialized = false;

    // Dynamic Point Lighting
    std::vector<GPUDynamicPointLight> m_dynamicLights;

    uint32_t m_currentFrame = 0;
    uint32_t m_imageIndex = 0;
    bool m_frameActive = false;
    bool m_renderPassActive = false;
    uint32_t m_drawCallsThisFrame = 0;

    // Screenshot
    bool m_screenshotRequested = false;
    std::string m_screenshotFilename;
    VkBuffer m_screenshotBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_screenshotMemory = VK_NULL_HANDLE;
    void* m_screenshotMapped = nullptr;
    uint32_t m_screenshotBufferSize = 0;
    void CaptureScreenshotInternal(VkCommandBuffer cmd);
    void SaveScreenshotToFile();

    // Deferred mesh draw queue
    struct DeferredMeshDraw {
        VkBuffer vbo;
        VkBuffer ibo;
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t textureId;
        int      blendType;
        bool     depthTest;
        bool     depthWrite;
        uint32_t instanceSSBOOffset;  // byte offset into instanceSSBO
        uint32_t instanceByteSize;    // bytes written for instances
        uint32_t boneSSBOOffset;      // byte offset into boneSSBO
        uint32_t boneByteSize;        // bytes written for bones
        uint32_t firstInstance;       // gl_InstanceIndex start for this draw
        GeneralVertUBO vertUBO;
        GeneralFragUBO fragUBO;
    };
    std::vector<DeferredMeshDraw> m_deferredMeshDraws;
    std::vector<DeferredMeshDraw> m_opaqueMeshDraws;
    std::vector<DeferredMeshDraw> m_transparentMeshDraws;

    // Deferred terrain draw queue
    struct DeferredTerrainDraw {
        uint32_t vertOffset;
        uint32_t vertByteSize;
        uint32_t idxOffset;
        uint32_t idxByteSize;
        std::vector<TerrainMergedBatch> batches;
        TerrainVertUBO ubo;
    };
    std::vector<DeferredTerrainDraw> m_deferredTerrainDraws;

    // Deferred sprite / particle draw queue
    struct DeferredSpriteDraw {
        uint32_t instanceSSBOOffset;
        uint32_t instanceByteSize;
        uint32_t firstInstance;
        std::vector<SpriteBatch> batches;
        SpriteVertUBO ubo;
    };
    std::vector<DeferredSpriteDraw> m_deferredSpriteDraws;

    // Deferred 2D image / UI draw queue
    struct DeferredImageDraw {
        uint32_t instanceSSBOOffset;
        uint32_t baseInstance;
        std::vector<ImageBatchRun> batches;
    };
    std::vector<DeferredImageDraw> m_deferredImageDraws;

    enum class DeferredCmdType : uint8_t {
        Terrain,
        Mesh,
        Sprite,
        Image,
        ClearDepth
    };
    struct DeferredCommand {
        DeferredCmdType type;
        uint32_t index;
    };
    std::vector<DeferredCommand> m_deferredCommands;

    void ReplayDeferredCommands();
    void ReplaySingleTerrainDraw(const DeferredTerrainDraw& draw);
    void ReplaySingleSpriteDraw(const DeferredSpriteDraw& draw);
    void ReplaySingleImageDraw(const DeferredImageDraw& draw);
    void ReplayMeshRange(size_t cmdStart, size_t cmdEnd);
};
