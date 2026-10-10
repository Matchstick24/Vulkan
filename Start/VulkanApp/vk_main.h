#ifndef VK_MAIN_H
#define VK_MAIN_H

// INCLUDE //

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <map>
#include <set>
#include <optional>
#include <limits>
#include <algorithm>
#include <fstream>

// APP VARIABLES //

constexpr int Window_Width = 800;
constexpr int Window_Height = 600;

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

constexpr char App_Name[] = "Vulkan App";

constexpr char Vert_Spv_Dir[] = "Shaders/vert.spv";
constexpr char Frag_Spv_Dir[] = "Shaders/frag.spv";

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

// APPLICATION //

class Vulkan_App
{
    public:

        void run() 
        {
            init_Vulkan();
            main_Loop();
            cleanup();
        }

    private:

        // Variables //

        VkInstance       vk_instance;
        VkPhysicalDevice vk_physicalDevice = VK_NULL_HANDLE;
        VkDevice         vk_device;

        VkSwapchainKHR           vk_swapChain; 
        std::vector<VkImage>     vk_swapChainImages;
        VkFormat                 vk_swapChainImageFormat;
        VkExtent2D               vk_swapChainExtent;
          
        std::vector<VkImageView>   vk_swapChainImageViews;
        std::vector<VkFramebuffer> vk_swapChainFramebuffers;

        VkQueue          vk_graphicsQueue;
        VkQueue          vk_presentQueue;
        
        VkPipeline       vk_graphicsPipeline;
        VkRenderPass     vk_renderPass;
        VkPipelineLayout vk_pipelineLayout;

        VkCommandPool   vk_commandPool;
        std::vector<VkCommandBuffer> vk_commandBuffers;

        std::vector<VkSemaphore> vk_imageAvailableSemaphores;
        std::vector<VkSemaphore> vk_renderFinishedSemaphores;
        std::vector<VkFence    > vk_inFlightFences;

        bool vk_framebufferResized = false;

        VkSurfaceKHR     vk_surface;
        GLFWwindow*         window;

        uint32_t currentFrame = 0;

        const std::vector<const char*> deviceExtensions = // I suppose could make it constexpr but it is simpler this way //
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        struct SwapChainSupportDetails 
        {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR>   presentModes;
        };

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) 
        {
            SwapChainSupportDetails details;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vk_surface, &details.capabilities);

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, vk_surface, &formatCount, nullptr);

            if (formatCount != 0) 
            {
                details.formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, vk_surface, &formatCount, details.formats.data());
            }

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, vk_surface, &presentModeCount, nullptr);

            if (presentModeCount != 0) 
            {
                details.presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, vk_surface, &presentModeCount, details.presentModes.data());
            }

            return details;
        }

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
        {
            for (const auto& availableFormat : availableFormats) 
            {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
                {
                    return availableFormat;
                }
            }

            return availableFormats[0]; // The first that works //
        }

        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
        {
            for (const auto& availablePresentMode : availablePresentModes) 
            {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) // No latency and no tearing mode(not the best for low power divices) //
                {
                    return availablePresentMode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR;

            /* Explanation from vulkan tutorial :

            VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front
            of the queue when the display is refreshed and the program inserts rendered
            images at the back of the queue. If the queue is full then the program has to wait.
            This is most similar to vertical sync as found in modern games.
            The moment that the display is refreshed is known as "vertical blank".
            
            */
        }

        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) 
        {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
            {
                return capabilities.currentExtent;
            } else 
            {
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);

                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };

                actualExtent.width  = std::clamp(actualExtent.width , capabilities.minImageExtent.width , capabilities.maxImageExtent.width );
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                return actualExtent;
            }
        }

        struct QueueFamilyIndices 
        {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;

            bool isComplete() 
            {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
        };

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) 
        {
            QueueFamilyIndices indices;

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto& queueFamily : queueFamilies) 
            {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    indices.graphicsFamily = i;
                }

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vk_surface, &presentSupport);

                if (presentSupport) 
                {
                    indices.presentFamily = i;
                }

                if (indices.isComplete()) 
                {
                    break;
                }

                i++;
            }

            return indices;
        }

        // Main Vulkan Functions //

        void init_Vulkan() 
        {
            // GLFW //

            glfwInit();

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 

            window = glfwCreateWindow(Window_Width, Window_Height, App_Name, nullptr, nullptr);
            glfwSetWindowUserPointer(window, this);
            glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

            // VULKAN //

            createInstance();
            createSurface();
            // ADD DEBUGGER //
            pickPhysicalDevice();
            createLogicalDevice();
            createSwapChain();
            createImageViews();
            createRenderPass();
            createGraphicsPipeline();
            createFramebuffers();
            createCommandPool();
            createCommandBuffers();
            createSyncObjects();
        }

        void main_Loop() 
        {
            while (!glfwWindowShouldClose(window))
            {
                glfwPollEvents();
                drawFrame();
            }

            vkDeviceWaitIdle(vk_device);
        }

        void cleanup() 
        {
            cleanupSwapChain();

            vkDestroyPipeline(vk_device, vk_graphicsPipeline, nullptr);
            vkDestroyPipelineLayout(vk_device, vk_pipelineLayout, nullptr);
            vkDestroyRenderPass(vk_device, vk_renderPass, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
            {
                vkDestroySemaphore(vk_device, vk_renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(vk_device, vk_imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(vk_device, vk_inFlightFences[i], nullptr);
            }

            vkDestroyCommandPool(vk_device, vk_commandPool, nullptr);

            vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
            vkDestroyInstance  (vk_instance, nullptr);

            glfwDestroyWindow(window);
            glfwTerminate();
        }
        
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height) 
        {
            auto app = reinterpret_cast<Vulkan_App*>(glfwGetWindowUserPointer(window));
            app->vk_framebufferResized = true;
        }

        // Main Graphics //

        void drawFrame()
        {
            vkWaitForFences(vk_device, 1, &vk_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(vk_device, vk_swapChain, UINT64_MAX, vk_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vk_framebufferResized) 
            {
                vk_framebufferResized = false;
                recreateSwapChain();
            } else if (result != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to acquire swap chain image!");
            }

            vkResetFences(vk_device, 1, &vk_inFlightFences[currentFrame]);
            
            vkResetCommandBuffer(vk_commandBuffers[currentFrame], 0);
            recordCommandBuffer(vk_commandBuffers[currentFrame], imageIndex);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = {vk_imageAvailableSemaphores[currentFrame]};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &vk_commandBuffers[currentFrame];

            VkSemaphore signalSemaphores[] = {vk_renderFinishedSemaphores[currentFrame]};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;
            
            if (vkQueueSubmit(vk_graphicsQueue, 1, &submitInfo, vk_inFlightFences[currentFrame]) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = {vk_swapChain};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.pResults = nullptr; // Optional

            result = vkQueuePresentKHR(vk_presentQueue, &presentInfo);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
            {
                recreateSwapChain();
            } else if (result != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to present swap chain image!");
            }

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        void cleanupSwapChain()
        {
            for (auto framebuffer : vk_swapChainFramebuffers) 
            {
                vkDestroyFramebuffer(vk_device, framebuffer, nullptr);
            }

            for (auto imageView : vk_swapChainImageViews) 
            {
                vkDestroyImageView(vk_device, imageView, nullptr);
            }

            vkDestroySwapchainKHR(vk_device, vk_swapChain, nullptr);
        }

        void recreateSwapChain() 
        {
            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            
            while (width == 0 || height == 0) 
            {
                glfwGetFramebufferSize(window, &width, &height);
                glfwWaitEvents();
            }

            vkDeviceWaitIdle(vk_device);
            cleanupSwapChain();

            createSwapChain();
            createImageViews();
            createFramebuffers();
        }

        void createSyncObjects()
        {
            vk_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            vk_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            vk_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
            {
                if (vkCreateSemaphore(vk_device, &semaphoreInfo, nullptr, &vk_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                    vkCreateSemaphore(vk_device, &semaphoreInfo, nullptr, &vk_renderFinishedSemaphores[i]) != VK_SUCCESS ||
                    vkCreateFence(vk_device, &fenceInfo, nullptr, &vk_inFlightFences[i]) != VK_SUCCESS) {

                    throw std::runtime_error("failed to create synchronization objects for a frame!");
                }
            }
        }

        // Graphics Pipeline Related //

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) 
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0; // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to begin recording command buffer!");
            }
          
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass  = vk_renderPass;
            renderPassInfo.framebuffer = vk_swapChainFramebuffers[imageIndex];

            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = vk_swapChainExtent;

            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_graphicsPipeline);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width  = static_cast<float>(vk_swapChainExtent.width);
            viewport.height = static_cast<float>(vk_swapChainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = vk_swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
            vkCmdEndRenderPass(commandBuffer);

            if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

        void createCommandBuffers()
        {
            vk_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = vk_commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;

            if (vkAllocateCommandBuffers(vk_device, &allocInfo, vk_commandBuffers.data()) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate command buffers!");
            }
        }

        void createCommandPool()
        {
            QueueFamilyIndices queueFamilyIndices = findQueueFamilies(vk_physicalDevice);

            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

            if (vkCreateCommandPool(vk_device, &poolInfo, nullptr, &vk_commandPool) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to create command pool!");
            }
        }

        void createFramebuffers()
        {
            vk_swapChainFramebuffers.resize(vk_swapChainImageViews.size());

            for (size_t i = 0; i < vk_swapChainImageViews.size(); i++) 
            {
                VkImageView attachments[] = {
                    vk_swapChainImageViews[i]
                };

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = vk_renderPass;
                framebufferInfo.attachmentCount = 1;
                framebufferInfo.pAttachments = attachments;
                framebufferInfo.width = vk_swapChainExtent.width;
                framebufferInfo.height = vk_swapChainExtent.height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(vk_device, &framebufferInfo, nullptr, &vk_swapChainFramebuffers[i]) != VK_SUCCESS) 
                {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }
        }

        void createRenderPass()
        {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = vk_swapChainImageFormat;
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

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = 1;
            renderPassInfo.pAttachments = &colorAttachment;
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            if (vkCreateRenderPass(vk_device, &renderPassInfo, nullptr, &vk_renderPass) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to create render pass!");
            }
        }

        void createGraphicsPipeline() 
        {
            auto Shader_vert_spv = readFile(Vert_Spv_Dir);
            auto Shader_frag_spv = readFile(Frag_Spv_Dir);
            
            VkShaderModule vertShaderModule = createShaderModule(Shader_vert_spv); // Changing the name format does not mean anything //
            VkShaderModule fragShaderModule = createShaderModule(Shader_frag_spv);

            VkPipelineShaderStageCreateInfo vertShaderStageInfo{};

            vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertShaderStageInfo.module = vertShaderModule;
            vertShaderStageInfo.pName = "main"; // Name of then entry point //

            VkPipelineShaderStageCreateInfo fragShaderStageInfo{};

            fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragShaderStageInfo.module = fragShaderModule;
            fragShaderStageInfo.pName = "main";

            VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 0;
            vertexInputInfo.pVertexBindingDescriptions = nullptr;   // Optional
            vertexInputInfo.vertexAttributeDescriptionCount = 0;
            vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

            std::vector<VkDynamicState> dynamicStates = 
            {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            };

            VkPipelineDynamicStateCreateInfo dynamicState{};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicState.pDynamicStates = dynamicStates.data();

            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // explanation (FROM VK TUTORIAL) :: triangle from every 3 vertices without reuse //
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            VkViewport viewport{}; // Transformation like , where as the scissors decides what gets drawn //
            viewport.x        = 0.0f;
            viewport.y        = 0.0f;
            viewport.width    = (float) vk_swapChainExtent.width;
            viewport.height   = (float) vk_swapChainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{}; // Filter like //
            scissor.offset = {0, 0};
            scissor.extent = vk_swapChainExtent;

            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // FILL, LINE, POINT //
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

            rasterizer.depthBiasEnable = VK_FALSE;
            rasterizer.depthBiasConstantFactor = 0.0f; // Optional
            rasterizer.depthBiasClamp = 0.0f; // Optional
            rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

            VkPipelineMultisampleStateCreateInfo multisampling{}; // I think it needs GPU feature // 
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = VK_LOGIC_OP_COPY;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;
            colorBlending.blendConstants[0] = 0.0f;
            colorBlending.blendConstants[1] = 0.0f;
            colorBlending.blendConstants[2] = 0.0f;
            colorBlending.blendConstants[3] = 0.0f;

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 0; // Optional
            pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
            pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
            pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

            if (vkCreatePipelineLayout(vk_device, &pipelineLayoutInfo, nullptr, &vk_pipelineLayout) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to create pipeline layout!");
            }

            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount = 2;
            pipelineInfo.pStages = shaderStages;

            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = nullptr; // Optional
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDynamicState = &dynamicState;

            pipelineInfo.layout = vk_pipelineLayout;

            pipelineInfo.renderPass = vk_renderPass;
            pipelineInfo.subpass = 0;

            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
            pipelineInfo.basePipelineIndex = -1; // Optional

            if (vkCreateGraphicsPipelines(vk_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vk_graphicsPipeline) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to create graphics pipeline!");
            }

            vkDestroyShaderModule(vk_device, fragShaderModule, nullptr);
            vkDestroyShaderModule(vk_device, vertShaderModule, nullptr);
        }

        VkShaderModule createShaderModule(const std::vector<char>& code) 
        {
            VkShaderModuleCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // from u8 to u32 pointer //

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(vk_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to create shader module!");
            }

            return shaderModule;
        }

        static std::vector<char> readFile(const std::string& file_name) 
        {
            std::ifstream file(file_name, std::ios::ate | std::ios::binary);

            if (!file.is_open()) 
            {
                throw std::runtime_error("failed to open file!");
            }

            size_t file_size = (size_t)file.tellg();
            std::vector<char> buffer(file_size);

            file.seekg(0);
            file.read(buffer.data(), file_size);
            file.close();

            return buffer;
        }

        // More Vulkan Functions //

        void createImageViews() 
        {
            vk_swapChainImageViews.resize(vk_swapChainImages.size());

            for (size_t i = 0; i < vk_swapChainImages.size(); i++) 
            {
                VkImageViewCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = vk_swapChainImages[i];
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = vk_swapChainImageFormat;

                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(vk_device, &createInfo, nullptr, &vk_swapChainImageViews[i]) != VK_SUCCESS) 
                {
                    throw std::runtime_error("failed to create image views!");
                }
            }
        }
        
        void createSwapChain() 
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(vk_physicalDevice);

            VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats     );
            VkPresentModeKHR   presentMode   = chooseSwapPresentMode  (swapChainSupport.presentModes);
            VkExtent2D         extent        = chooseSwapExtent       (swapChainSupport.capabilities);

            uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

            //                     ???                          //
            if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
            {
                imageCount = swapChainSupport.capabilities.maxImageCount;
            }

            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = vk_surface;

            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = findQueueFamilies(vk_physicalDevice);
            uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

            if (indices.graphicsFamily != indices.presentFamily) 
            {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            } else 
            {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0; // Optional
                createInfo.pQueueFamilyIndices = nullptr; // Optional
            }

            createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

            createInfo.presentMode = presentMode;
            createInfo.clipped     = VK_TRUE;

            createInfo.oldSwapchain = VK_NULL_HANDLE; // So the window wont resize //

            if (vkCreateSwapchainKHR(vk_device, &createInfo, nullptr, &vk_swapChain) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create swap chain!");
            }

            vkGetSwapchainImagesKHR(vk_device, vk_swapChain, &imageCount, nullptr);
            vk_swapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(vk_device, vk_swapChain, &imageCount, vk_swapChainImages.data());

            vk_swapChainImageFormat = surfaceFormat.format;
            vk_swapChainExtent      = extent;
        }

        void createLogicalDevice()
        {
            QueueFamilyIndices indices = findQueueFamilies(vk_physicalDevice);

            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

            float queuePriority = 1.0f;
            for (uint32_t queueFamily : uniqueQueueFamilies) 
            {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            VkPhysicalDeviceFeatures deviceFeatures{};
            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

            createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
            createInfo.ppEnabledExtensionNames = deviceExtensions.data();

            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.pQueueCreateInfos    = queueCreateInfos.data();
        
            createInfo.pEnabledFeatures = &deviceFeatures;

            if (enableValidationLayers) // This is not really needed, but says to include it? //
            {
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();
            } else 
            {
                createInfo.enabledLayerCount = 0;
            }

            if (vkCreateDevice(vk_physicalDevice, &createInfo, nullptr, &vk_device) != VK_SUCCESS) {
                throw std::runtime_error("failed to create logical device!");
            }

            vkGetDeviceQueue(vk_device, indices.graphicsFamily.value(), 0, &vk_graphicsQueue);
            vkGetDeviceQueue(vk_device, indices.presentFamily.value(),  0, &vk_presentQueue);
        }

        void createInstance()
        {
            if (enableValidationLayers && !checkValidationLayerSupport()) 
            {
                throw std::runtime_error("validation layers requested, but not available!");
            }

            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = App_Name;
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "No Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_0;

            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;

            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            createInfo.enabledExtensionCount = glfwExtensionCount;
            createInfo.ppEnabledExtensionNames = glfwExtensions;
            createInfo.enabledLayerCount = 0;

            if (vkCreateInstance(&createInfo, nullptr, &vk_instance) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create instance!");
            }

            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> extensions(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

            std::cout << "available extensions:\n";
            for (const auto& extension : extensions) 
            {
                std::cout << '\t' << extension.extensionName << '\n';
            }

            if (enableValidationLayers) 
            {
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();
            } else 
            {
                createInfo.enabledLayerCount = 0;
            }
        }

        void createSurface() 
        {
            if (glfwCreateWindowSurface(vk_instance, window, nullptr, &vk_surface) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to create window surface!");
            }
        }

        void pickPhysicalDevice() 
        {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(vk_instance, &deviceCount, nullptr);

            if (deviceCount == 0) 
            {
                throw std::runtime_error("failed to find GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(vk_instance, &deviceCount, devices.data());

            for (const auto& device : devices) 
            {
                if (isDeviceSuitable(device))
                {
                    vk_physicalDevice = device;
                    break;
                }
            }

            if (vk_physicalDevice == VK_NULL_HANDLE) 
            {
                throw std::runtime_error("failed to find a suitable GPU!");
            }
        }

        bool isDeviceSuitable(VkPhysicalDevice device)
        {
            QueueFamilyIndices indices = findQueueFamilies(device);

            bool extensionsSupported = checkDeviceExtensionSupport(device);

            bool swapChainAdequate = false;
            if (extensionsSupported) 
            {
                SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
                swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
            }

            return indices.isComplete() && extensionsSupported && swapChainAdequate;
        }

        bool checkDeviceExtensionSupport(VkPhysicalDevice device)
        {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

            for (const auto& extension : availableExtensions)
            {
                requiredExtensions.erase(extension.extensionName);
            }

            return requiredExtensions.empty();
        }

        bool checkValidationLayerSupport() 
        {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            for (const char* layerName : validationLayers) 
            {
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
};

#endif