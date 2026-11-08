#pragma once

#include "engine/renderer/se_frame_context.hpp" // FrameContext is stored and returned by value.

#include <vulkan/vulkan.h>

#include <cassert>
#include <memory>
#include <optional>
#include <vector>

namespace se
{
    class SEDevice;
    class SEWindow;
    class SEOffscreenRenderer;
    class SESwapChain;
  class SERenderer
  {
  public:
    SERenderer(SEWindow &window, SEDevice &device);
    ~SERenderer();

    SERenderer(const SERenderer &) = delete;
    SERenderer &operator=(const SERenderer &) = delete;
    SERenderer(SERenderer&&) = delete;
    SERenderer& operator=(SERenderer&&) = delete;

    VkRenderPass getSwapChainRenderPass() const;
    uint32_t getSwapChainImageCount() const;
    float getAspectRatio() const;
    bool isFrameInProgress() const { return isFrameStarted; }

    VkCommandBuffer getCurrentCommandBuffer() const
    {
      assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
      return activeFrame.commandBuffer;
    }

    uint32_t getFrameIndex() const
    {
      assert(isFrameStarted && "Cannot get frame index when frame not in progress");
      return activeFrame.frameIndex;
    }

    SEOffscreenRenderer* getOffscreenRenderer() { return offscreenRenderer.get(); }

    std::optional<FrameContext> beginFrame();
    void endFrame(const FrameContext& frame);
    void beginSwapChainRenderPass(const FrameContext& frame);
    void endSwapChainRenderPass(const FrameContext& frame);

  private:
    void createCommandBuffers();
    void freeCommandBuffers();
    void recreateSwapChain();

    SEWindow &seWindow;
    SEDevice &seDevice;
    std::unique_ptr<SESwapChain> seSwapChain;
    std::vector<VkCommandBuffer> commandBuffers;
    std::unique_ptr<SEOffscreenRenderer> offscreenRenderer;

    FrameContext activeFrame{};
    bool isFrameStarted{ false };
  };
}
