#pragma once

#include "se_device.hpp"
#include "se_swap_chain.hpp"
#include "se_window.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace se
{
  class SERenderer
  {
  public:
    SERenderer(SEWindow &window, SEDevice &device);
    ~SERenderer();

    SERenderer(const SERenderer &) = delete;
    SERenderer &operator=(const SERenderer &) = delete;

    VkRenderPass getSwapChainRenderPass() const { return seSwapChain->getRenderPass(); }
    float getAspectRatio() const { return seSwapChain->extentAspectRatio(); }
    bool isFrameInProgress() const { return isFrameStarted; }

    VkCommandBuffer getCurrentCommandBuffer() const
    {
      assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
      return commandBuffers[currentFrameIndex];
    }

    int getFrameIndex() const
    {
      assert(isFrameStarted && "Cannot get frame index when frame not in progress");
      return currentFrameIndex;
    }

    VkCommandBuffer beginFrame();
    void endFrame();
    void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

  private:
    void createCommandBuffers();
    void freeCommandBuffers();
    void recreateSwapChain();

    SEWindow &seWindow;
    SEDevice &seDevice;
    std::unique_ptr<SESwapChain> seSwapChain;
    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t currentImageIndex;
    int currentFrameIndex{0};
    bool isFrameStarted{false};
  };
}