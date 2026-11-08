#include "engine/renderer/se_swap_chain.hpp"
#include "engine/vulkan/se_device.hpp"
#include "engine/vulkan/se_vk_result.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace se
{

  SESwapChain::SESwapChain(SEDevice &deviceRef, VkExtent2D extent)
      : device{deviceRef}, windowExtent{extent}
  {
    try
    {
      init();
    }
    catch (...)
    {
      destroyOwnedHandles();
      throw;
    }
  }

  SESwapChain::SESwapChain(
      SEDevice &deviceRef, VkExtent2D extent, std::shared_ptr<SESwapChain> previous)
      : device{deviceRef}, windowExtent{extent}, oldSwapChain{previous}
  {
    try
    {
      init();
      oldSwapChain = nullptr;
    }
    catch (...)
    {
      destroyOwnedHandles();
      throw;
    }
  }

  void SESwapChain::init()
  {
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDepthResources();
    createFramebuffers();
    createSyncObjects();
  }

  SESwapChain::~SESwapChain()
  {
    destroyOwnedHandles();
  }

  void SESwapChain::destroyOwnedHandles() noexcept
  {
    const VkDevice logicalDevice = device.device();
    for (VkSemaphore semaphore : renderFinishedSemaphores)
      if (semaphore != VK_NULL_HANDLE) vkDestroySemaphore(logicalDevice, semaphore, nullptr);
    renderFinishedSemaphores.clear();
    for (VkSemaphore semaphore : imageAvailableSemaphores)
      if (semaphore != VK_NULL_HANDLE) vkDestroySemaphore(logicalDevice, semaphore, nullptr);
    imageAvailableSemaphores.clear();
    for (VkFence fence : inFlightFences)
      if (fence != VK_NULL_HANDLE) vkDestroyFence(logicalDevice, fence, nullptr);
    inFlightFences.clear();
    imagesInFlight.clear();

    for (VkFramebuffer framebuffer : swapChainFramebuffers)
      if (framebuffer != VK_NULL_HANDLE) vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
    swapChainFramebuffers.clear();

    if (renderPass != VK_NULL_HANDLE)
    {
      vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
      renderPass = VK_NULL_HANDLE;
    }

    for (VkImageView view : depthImageViews)
      if (view != VK_NULL_HANDLE) vkDestroyImageView(logicalDevice, view, nullptr);
    depthImageViews.clear();
    for (VkImage image : depthImages)
      if (image != VK_NULL_HANDLE) vkDestroyImage(logicalDevice, image, nullptr);
    depthImages.clear();
    for (VkDeviceMemory memory : depthImageMemorys)
      if (memory != VK_NULL_HANDLE) vkFreeMemory(logicalDevice, memory, nullptr);
    depthImageMemorys.clear();

    for (VkImageView view : swapChainImageViews)
      if (view != VK_NULL_HANDLE) vkDestroyImageView(logicalDevice, view, nullptr);
    swapChainImageViews.clear();
    swapChainImages.clear();

    if (descriptorSetLayout != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);
      descriptorSetLayout = VK_NULL_HANDLE;
    }
    if (swapChain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
      swapChain = VK_NULL_HANDLE;
    }
  }

  VkResult SESwapChain::acquireNextImage(uint32_t *imageIndex)
  {
    checkVkResult(vkWaitForFences(
        device.device(),
        1,
        &inFlightFences[currentFrame],
        VK_TRUE,
        std::numeric_limits<uint64_t>::max()), "vkWaitForFences(frame)");

    VkResult result = vkAcquireNextImageKHR(
        device.device(),
        swapChain,
        std::numeric_limits<uint64_t>::max(),
        imageAvailableSemaphores[currentFrame], // must be a not signaled semaphore
        VK_NULL_HANDLE,
        imageIndex);

    return result;
  }

  VkResult SESwapChain::submitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex)
  {
    if (imagesInFlight[*imageIndex] != VK_NULL_HANDLE)
    {
      checkVkResult(vkWaitForFences(device.device(), 1, &imagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX),
          "vkWaitForFences(swapchain image)");
    }
    imagesInFlight[*imageIndex] = inFlightFences[currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[*imageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    checkVkResult(vkResetFences(device.device(), 1, &inFlightFences[currentFrame]), "vkResetFences(frame)");
    checkVkResult(vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]), "vkQueueSubmit");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = imageIndex;

    auto result = vkQueuePresentKHR(device.presentQueue(), &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
  }

  void SESwapChain::createSwapChain()
  {
    SwapChainSupportDetails swapChainSupport = device.getSwapChainSupport();

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount)
    {
      imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = device.surface();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily)
    {
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      createInfo.queueFamilyIndexCount = 0;     // Optional
      createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;

    checkVkResult(vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapChain), "vkCreateSwapchainKHR");

    // we only specified a minimum number of images in the swap chain, so the implementation is
    // allowed to create a swap chain with more. That's why we'll first query the final number of
    // images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
    // retrieve the handles.
    checkVkResult(vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, nullptr),
        "vkGetSwapchainImagesKHR(count)");
    swapChainImages.resize(imageCount);
    checkVkResult(vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, swapChainImages.data()),
        "vkGetSwapchainImagesKHR(data)");

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
  }

  void SESwapChain::createImageViews()
  {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
      VkImageViewCreateInfo viewInfo{};
      viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      viewInfo.image = swapChainImages[i];
      viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      viewInfo.format = swapChainImageFormat;
      viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      viewInfo.subresourceRange.baseMipLevel = 0;
      viewInfo.subresourceRange.levelCount = 1;
      viewInfo.subresourceRange.baseArrayLayer = 0;
      viewInfo.subresourceRange.layerCount = 1;

      checkVkResult(vkCreateImageView(device.device(), &viewInfo, nullptr, &swapChainImageViews[i]), "vkCreateImageView");
    }
  }

  void SESwapChain::createRenderPass()
  {
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
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

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = getSwapChainImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.dstSubpass = 0;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcAccessMask = 0;
    dependency.srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    checkVkResult(vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass), "vkCreateRenderPass");
  }
  
  void SESwapChain::createFramebuffers()
  {
    swapChainFramebuffers.resize(imageCount());
    for (size_t i = 0; i < imageCount(); i++)
    {
      std::array<VkImageView, 2> attachments = {swapChainImageViews[i], depthImageViews[i]};

      VkExtent2D swapChainExtent = getSwapChainExtent();
      VkFramebufferCreateInfo framebufferInfo = {};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = renderPass;
      framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
      framebufferInfo.pAttachments = attachments.data();
      framebufferInfo.width = swapChainExtent.width;
      framebufferInfo.height = swapChainExtent.height;
      framebufferInfo.layers = 1;

      checkVkResult(vkCreateFramebuffer(
              device.device(),
              &framebufferInfo,
              nullptr,
              &swapChainFramebuffers[i]), "vkCreateFramebuffer");
    }
  }

  void SESwapChain::createDepthResources()
  {
    VkFormat depthFormat = findDepthFormat();
    swapChainDepthFormat = depthFormat;
    VkExtent2D swapChainExtent = getSwapChainExtent();

    depthImages.resize(imageCount());
    depthImageMemorys.resize(imageCount());
    depthImageViews.resize(imageCount());

    for (int i = 0; i < depthImages.size(); i++)
    {
      VkImageCreateInfo imageInfo{};
      imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      imageInfo.imageType = VK_IMAGE_TYPE_2D;
      imageInfo.extent.width = swapChainExtent.width;
      imageInfo.extent.height = swapChainExtent.height;
      imageInfo.extent.depth = 1;
      imageInfo.mipLevels = 1;
      imageInfo.arrayLayers = 1;
      imageInfo.format = depthFormat;
      imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
      imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
      imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      imageInfo.flags = 0;

      device.createImageWithInfo(
          imageInfo,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
          depthImages[i],
          depthImageMemorys[i]);

      VkImageViewCreateInfo viewInfo{};
      viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      viewInfo.image = depthImages[i];
      viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      viewInfo.format = depthFormat;
      viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      viewInfo.subresourceRange.baseMipLevel = 0;
      viewInfo.subresourceRange.levelCount = 1;
      viewInfo.subresourceRange.baseArrayLayer = 0;
      viewInfo.subresourceRange.layerCount = 1;

      checkVkResult(vkCreateImageView(device.device(), &viewInfo, nullptr, &depthImageViews[i]), "vkCreateImageView");
    }
  }

  void SESwapChain::createSyncObjects()
  {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(imageCount());
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame)
    {
      checkVkResult(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[frame]),
          "vkCreateSemaphore(image available)");
      checkVkResult(vkCreateFence(device.device(), &fenceInfo, nullptr, &inFlightFences[frame]),
          "vkCreateFence(in-flight)");
    }

    for (size_t image = 0; image < renderFinishedSemaphores.size(); ++image)
    {
      checkVkResult(vkCreateSemaphore(device.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[image]), "vkCreateSemaphore");
    }
  }

  VkSurfaceFormatKHR SESwapChain::chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats)
  {
    for (const auto &availableFormat : availableFormats)
    {
      if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
          availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        return availableFormat;
      }
    }

    return availableFormats[0];
  }

  VkPresentModeKHR SESwapChain::chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes)
  {
    for (const auto &availablePresentMode : availablePresentModes)
    {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
      {
        // std::cout << "Present mode: Mailbox" << std::endl;
        return availablePresentMode;
      }
    }

    for (const auto &availablePresentMode : availablePresentModes)
    {
      if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
      {
        // std::cout << "Present mode: Immediate" << std::endl;
        return availablePresentMode;
      }
    }

    //std::cout << "Present mode: V-Sync" << std::endl;
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D SESwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
      return capabilities.currentExtent;
    }
    else
    {
      VkExtent2D actualExtent = windowExtent;
      actualExtent.width = std::max(
          capabilities.minImageExtent.width,
          std::min(capabilities.maxImageExtent.width, actualExtent.width));
      actualExtent.height = std::max(
          capabilities.minImageExtent.height,
          std::min(capabilities.maxImageExtent.height, actualExtent.height));

      return actualExtent;
    }
  }

  VkFormat SESwapChain::findDepthFormat()
  {
    return device.findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  }
}
