#pragma once

// INCLUDE //

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "camera.h"
#include "math.h"
#include "mesh_import.h"

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtx/quaternion.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <chrono>

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

//#define STB_IMAGE_IMPLEMENTATION // errors for some reason
#include "stb_image.h"

// APP VARIABLES //

constexpr int Window_Width = 800;
constexpr int Window_Height = 800;

constexpr int MAX_FRAMES_IN_FLIGHT = 3;

constexpr char App_Name[] = "Vulkan App";

constexpr char TEXTURE_FILE[] = "Resources/Textures/Test.jpg";
constexpr char IMPORT_FILE[] = "Resources/Models/Monkey.obj";
constexpr char Vert_Spv_Dir[] = "Resources/Shaders/vert.spv";
constexpr char Frag_Spv_Dir[] = "Resources/Shaders/frag.spv";

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

static inline void vk_check(VkResult result) 
{
	if (result != VK_SUCCESS) 
    {
		std::cerr << "Vulkan call returned an error (" << result << ")\n";
		exit(EXIT_FAILURE);
	}
}

static inline void vk_check(bool result) 
{
	if (!result) 
    {
		std::cerr << "Call returned an error\n";
		exit(EXIT_FAILURE);
	}
}

// APPLICATION //

class Vulkan_App
{
    public:

        // STRUCTS //

        struct Vertex 
        {
            glm::vec3 pos;
            glm::vec3 color;

            static std::array<VkVertexInputAttributeDescription, 2> get_attribute_descriptions() 
            {
                std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

                attributeDescriptions[0].binding = 0;
                attributeDescriptions[0].location = 0;
                attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[0].offset = offsetof(Vertex, pos);

                attributeDescriptions[1].binding = 0;
                attributeDescriptions[1].location = 1;
                attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[1].offset = offsetof(Vertex, color);

                return attributeDescriptions;
            }

            static VkVertexInputBindingDescription get_binding_description() 
            {
                VkVertexInputBindingDescription bindingDescription{};

                bindingDescription.binding = 0;
                bindingDescription.stride = sizeof(Vertex);
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                return bindingDescription;
            }
        };

        struct UniformBufferObject 
        {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 proj;
        };

        // VARIABLES //

        Camera camera = {};

        // const std::vector<Vertex> vertices = 
        // {
        //     {{-0.5f, 0.0, -0.5f}, {1.0f, 0.0f, 0.0f}},
        //     {{0.5f, 0.0, -0.5f}, {0.0f, 1.0f, 0.0f}},
        //     {{0.5f, 0.0, 0.5f}, {0.0f, 0.0f, 1.0f}},
        //     {{-0.5f, 0.0, 0.5f}, {1.0f, 1.0f, 1.0f}}
        // };

        // const std::vector<uint16_t> indices = 
        // {
        //     0, 1, 2, 2, 3, 0
        // };

        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;

        // FUNCTIONS //

        void run() 
        {
            init_vulkan();
            main_loop();
            clean_up();
        }

    private:

        // VARIABLES //

        VkBuffer vertex_buffer;
        VkDeviceMemory vertex_buffer_memory;
        
        VkBuffer index_buffer;
        VkDeviceMemory index_buffer_memory;

        std::vector<VkBuffer> uniform_buffers;
        std::vector<VkDeviceMemory> uniform_buffers_memory;
        std::vector<void*> uniform_buffers_mapped;
        
        VkDescriptorPool descriptor_pool;
        std::vector<VkDescriptorSet> descriptor_sets;
        
        VkImageView texture_image_view;
        VkImage texture_image;
        VkDeviceMemory texture_image_memory;

        GLFWwindow* window;
        VkInstance  instance;
        VkDevice device;

        VkImage depth_image;
        VkDeviceMemory depth_image_memory;
        VkImageView depth_image_view;

        VkSurfaceKHR surface;
        VkPipeline graphics_pipeline;
        VkRenderPass render_pass;
        VkDescriptorSetLayout descriptor_set_layout;
        VkPipelineLayout pipeline_layout;

        VkSwapchainKHR swapchain;
        std::vector<VkImage> swapchain_images;
        VkFormat swapchain_image_format;
        VkExtent2D swapchain_extent;

        std::vector<VkImageView> swapchain_image_views;
        std::vector<VkFramebuffer> swapchain_framebuffers;

        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        
        VkQueue graphics_queue;
        VkQueue present_queue;

        VkCommandPool command_pool;
        std::vector<VkCommandBuffer> command_buffers;

        std::vector<VkSemaphore> image_available_semaphores;
        std::vector<VkSemaphore> render_finished_semaphores;
        std::vector<VkFence> in_flight_fences;

        bool framebuffer_resized = false;
        uint32_t current_frame = 0;
        
        const std::vector<const char*> device_extensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        
        // STRUCTS AND FUNCTIONS //
        
        struct QueueFamilyIndices 
        {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;

            bool isComplete() 
            {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
        };
        
        struct SwapChainSupportDetails 
        {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR>   presentModes;
        };
        
        VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
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

        VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes) 
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

        VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) 
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
        
        SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device) 
        {
            SwapChainSupportDetails details;
            
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
            
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
            
            if (formatCount != 0) 
            {
                details.formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
            }
            
            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
            
            if (presentModeCount != 0) 
            {
                details.presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
            }
            
            return details;
        }
        
        QueueFamilyIndices find_queue_families(VkPhysicalDevice device) 
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
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

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

        /* DEPTH 

        VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) 
        {
            for (VkFormat format : candidates) 
            {
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

                if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) 
                {
                    return format;
                } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) 
                {
                    return format;
                }
            }
            vk_check(false); // no format supported 
        }

        VkFormat find_depth_format() 
        {
            return find_supported_format
            (
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );
        }

        bool has_stencil_component(VkFormat format) 
        {
            return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
        }

        */

        // FUNCTIONS //

        void init_vulkan()
        {
            // GLFW //

            glfwInit();

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 

            window = glfwCreateWindow(Window_Width, Window_Height, App_Name, nullptr, nullptr);
            glfwSetWindowUserPointer(window, this);
            glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

            camera.Set_Window(window);
            camera.Set_Projection(glm::perspective(glm::radians(70.0f), swapchain_extent.width / (float) swapchain_extent.height, 0.1f, 1000.0f));

            // VULKAN //

            // VMA -- vulkan memory allocator needs to be done -- //

            create_instance();
            
            vk_check(glfwCreateWindowSurface(instance, window, nullptr, &surface));

            // ADD DEBUGGER - MAYBE //

            pick_physical_device();
            create_logical_device();
            create_swapchain();
            create_image_views();
            create_render_pass();
            create_descriptor_set_layout();
            create_graphics_pipeline();
            create_framebuffers();
            create_command_pool();
            /* DEPTH 
                create_depth_resources();
            */
            create_texture_image();
            create_texture_image_view();
            create_texture_sampler();
            create_vertex_buffer();
            create_index_buffer();
            create_uniform_buffers();
            create_descriptor_pool();
            create_descriptor_sets();
            create_command_buffers();
            create_sync_objects();
        }

        void main_loop()
        {
            while (!glfwWindowShouldClose(window))
            {
                glfwPollEvents();
                draw_frame();
            }

            vkDeviceWaitIdle(device);
        }

        void clean_up()
        {
            cleanup_swapchain();

            vkDestroyImageView(device, texture_image_view, nullptr);
                    
            vkDestroyImage(device, texture_image, nullptr);
            vkFreeMemory(device, texture_image_memory, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
            {
                vkDestroyBuffer(device, uniform_buffers[i], nullptr);
                vkFreeMemory(device, uniform_buffers_memory[i], nullptr);
            }

            vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
            vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
                    
            vkDestroyBuffer(device, index_buffer, nullptr);
            vkFreeMemory(device, index_buffer_memory, nullptr);

            vkDestroyBuffer(device, vertex_buffer, nullptr);
            vkFreeMemory(device, vertex_buffer_memory, nullptr);

            vkDestroyPipeline(device, graphics_pipeline, nullptr);
            vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
            vkDestroyRenderPass(device, render_pass, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
            {
                vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
                vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
                vkDestroyFence(device, in_flight_fences[i], nullptr);
            }

            vkDestroyCommandPool(device, command_pool, nullptr);

            vkDestroySurfaceKHR(instance, surface, nullptr);
            vkDestroyInstance  (instance, nullptr);

            glfwDestroyWindow(window);
            glfwTerminate();
        }

        // rendering // 

        void draw_frame()
        {
            vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &imageIndex);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) 
            {
                framebuffer_resized = false;
                recreate_swapchain();
            } else 
            {
                vk_check(result == VK_SUCCESS);
            }

            vkResetFences(device, 1, &in_flight_fences[current_frame]);
            
            vkResetCommandBuffer(command_buffers[current_frame], 0);
            record_command_buffer(command_buffers[current_frame], imageIndex);

            update_uniform_buffer(current_frame);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = {image_available_semaphores[current_frame]};
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &command_buffers[current_frame];

            VkSemaphore signalSemaphores[] = {render_finished_semaphores[current_frame]};
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;
            
            if (vkQueueSubmit(graphics_queue, 1, &submitInfo, in_flight_fences[current_frame]) != VK_SUCCESS) 
            {
                throw std::runtime_error("failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = {swapchain};
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.pResults = nullptr; // Optional

            result = vkQueuePresentKHR(present_queue, &presentInfo);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) 
            {
                recreate_swapchain();
            } else 
            {
                vk_check(result == VK_SUCCESS);
            }

            current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
        }

        void recreate_swapchain()
        {
            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            
            while (width == 0 || height == 0) 
            {
                glfwGetFramebufferSize(window, &width, &height);
                glfwWaitEvents();
            }

            vkDeviceWaitIdle(device);
            cleanup_swapchain();

            create_swapchain();
            create_image_views();
            create_framebuffers();
        }

        void record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) 
        {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0; // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            vk_check(vkBeginCommandBuffer(commandBuffer, &beginInfo));
          
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass  = render_pass;
            renderPassInfo.framebuffer = swapchain_framebuffers[imageIndex];

            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapchain_extent;

            VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
                    
                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width  = static_cast<float>(swapchain_extent.width);
                viewport.height = static_cast<float>(swapchain_extent.height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.offset = {0, 0};
                scissor.extent = swapchain_extent;
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
                            
                VkBuffer vertexBuffers[] = {vertex_buffer};
                VkDeviceSize offsets[] = {0};
                
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[current_frame], 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(commandBuffer);
            vk_check(vkEndCommandBuffer(commandBuffer));
        }

        void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) 
        {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create buffer!");
            }

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

            vk_check(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));
            vkBindBufferMemory(device, buffer, bufferMemory, 0);
        }

        VkCommandBuffer begin_single_time_commands() 
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = command_pool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);
            return commandBuffer;
        }

        void end_single_time_commands(VkCommandBuffer command_buffer) 
        {
            vkEndCommandBuffer(command_buffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &command_buffer;

            vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphics_queue);

            vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
        }

        void copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) 
        {
            VkCommandBuffer commandBuffer = begin_single_time_commands();

            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

            end_single_time_commands(commandBuffer);
        }

        void create_vertex_buffer()
        {
            std::vector<Mesh_Import::Vertex> vertices_intermediate;
            Mesh_Import::SetMeshData(IMPORT_FILE, vertices_intermediate, indices);

            for (auto v : vertices_intermediate)
            {
                Vertex vertice = {.pos = v.pos, .color = v.color};
                vertices.push_back(vertice);
            }

            VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
            
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
                memcpy(data, vertices.data(), (size_t) bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);

            create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_buffer_memory);
            copy_buffer(stagingBuffer, vertex_buffer, bufferSize);

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }

        void create_index_buffer()
        {
            VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, indices.data(), (size_t) bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);

            create_buffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_buffer_memory);
            copy_buffer(stagingBuffer, index_buffer, bufferSize);

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);

        }

        void create_descriptor_set_layout()
        {
            VkDescriptorSetLayoutBinding uboLayoutBinding{};
            uboLayoutBinding.binding = 0;
            uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboLayoutBinding.descriptorCount = 1;
            uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = 1;
            layoutInfo.pBindings = &uboLayoutBinding;

            vk_check(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptor_set_layout));
        }

        void create_uniform_buffers()
        {
            VkDeviceSize bufferSize = sizeof(UniformBufferObject);

            uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
            uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
            uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
            {
                create_buffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffers[i], uniform_buffers_memory[i]);
                vkMapMemory(device, uniform_buffers_memory[i], 0, bufferSize, 0, &uniform_buffers_mapped[i]);
            }
        }

        void update_uniform_buffer(uint32_t currentImage)
        {
            static auto startTime = std::chrono::high_resolution_clock::now();
            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

            //glm::vec3 v = Math::GetForwardVector(camera.Transform);
            glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.25f, 0.5f, 1.0f));
            //glm::vec3 mv = Math::GetForwardVector(model);

            // std::printf("-------------------------------- \n");
            // std::printf("CAMERA: %f, %f, %f \n", v.x, v.y, v.z);
            // std::printf("MODEL: %f, %f, %f \n", mv.x, mv.y, mv.z);
            // std::printf("%f", time);
            // std::printf("-------------------------------- \n");

            camera.Step(0.01f);
        
            UniformBufferObject ubo{};
            ubo.model = model;
            ubo.view = camera.View;
            ubo.proj = glm::perspective(glm::radians(70.0f), swapchain_extent.width / (float) swapchain_extent.height, 0.1f, 1000.0f);
            ubo.proj[1][1] *= -1; // invert y because vulkan is different //

            memcpy(uniform_buffers_mapped[currentImage], &ubo, sizeof(ubo));
        }

        void create_descriptor_pool()
        {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

            vk_check(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptor_pool));
        }

        void create_descriptor_sets()
        {
            std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout);
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptor_pool;
            allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            allocInfo.pSetLayouts = layouts.data();

            descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);

            vk_check(vkAllocateDescriptorSets(device, &allocInfo, descriptor_sets.data()));

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
            {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = uniform_buffers[i];
                bufferInfo.offset = 0;
                bufferInfo.range = sizeof(UniformBufferObject);

                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = descriptor_sets[i];
                descriptorWrite.dstBinding = 0;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrite.descriptorCount = 1;            
                descriptorWrite.pBufferInfo = &bufferInfo;
                descriptorWrite.pImageInfo = nullptr; // Optional
                descriptorWrite.pTexelBufferView = nullptr; // Optional

                vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
            }
        }

        void create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = static_cast<uint32_t>(width);
            imageInfo.extent.height = static_cast<uint32_t>(height);
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.flags = 0; // Optional

            vk_check(vkCreateImage(device, &imageInfo, nullptr, &texture_image));

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device, texture_image, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            vk_check(vkAllocateMemory(device, &allocInfo, nullptr, &texture_image_memory));
            vkBindImageMemory(device, texture_image, texture_image_memory, 0);
        }

        void transition_image_layout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) 
        {
            VkCommandBuffer commandBuffer = begin_single_time_commands();
            
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
            barrier.srcAccessMask = 0; // TODO
            barrier.dstAccessMask = 0; // TODO

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;
            
            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
            {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            } else 
            {
                vk_check(false); // unsupported layout transition
            }
            
            vkCmdPipelineBarrier(
                commandBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            end_single_time_commands(commandBuffer);
        }

        void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) 
        {
            VkCommandBuffer commandBuffer = begin_single_time_commands();

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = {0, 0, 0};
            region.imageExtent = {
                width,
                height,
                1
            };

            vkCmdCopyBufferToImage(
                commandBuffer,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );

            end_single_time_commands(commandBuffer);
        }
        
        VkImageView create_image_view(VkImage image, VkFormat format) 
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

            VkImageView imageView;
            vk_check(vkCreateImageView(device, &viewInfo, nullptr, &imageView));
            return imageView;
        }

        void create_texture_image()
        {
            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = stbi_load(TEXTURE_FILE, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            VkDeviceSize imageSize = texWidth * texHeight * 4;

            if (!pixels) 
            {
                throw std::runtime_error("failed to load texture image!");
            }

            VkBuffer staging_buffer;
            VkDeviceMemory staging_buffer_memory;
            
            create_buffer(
                imageSize, 
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                staging_buffer, 
                staging_buffer_memory
            );

            void* data;
            vkMapMemory(device, staging_buffer_memory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(device, staging_buffer_memory);

            stbi_image_free(pixels);
            create_image(
                texWidth, 
                texHeight, 
                VK_FORMAT_R8G8B8A8_SRGB, 
                VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                texture_image, 
                texture_image_memory
            );

            transition_image_layout(
                texture_image, 
                VK_FORMAT_R8G8B8A8_SRGB, 
                VK_IMAGE_LAYOUT_UNDEFINED, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            );

            copy_buffer_to_image(
                staging_buffer, 
                texture_image,
                static_cast<uint32_t>(texWidth), 
                static_cast<uint32_t>(texHeight)
            );

            transition_image_layout(
                texture_image, 
                VK_FORMAT_R8G8B8A8_SRGB, 
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
            
            vkDestroyBuffer(device, staging_buffer, nullptr);
            vkFreeMemory(device, staging_buffer_memory, nullptr);
        }

        void create_texture_image_view()
        {
            texture_image_view = create_image_view(texture_image, VK_FORMAT_R8G8B8A8_SRGB);
        }

        void create_texture_sampler()
        {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }

        /* DEPTH 
        
        void create_depth_resources()
        {
            VkFormat depth_format = find_depth_format();
        }
        
        */

        // vulkan functions //

        uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) // finds a suitable memory type for a buffer or image based on the requirements and properties specified //
        {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
            {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
                {
                    return i;
                }
            }

            vk_check(false); // No suitable memory type found //
            return EXIT_FAILURE; // This line will never be reached, but is required to avoid compiler warnings //
        }

        void create_instance() // creates a Vulkan instance, which is the connection between the application and the Vulkan library //
        {
            vk_check((enableValidationLayers && check_validation_layer_support()) || !enableValidationLayers);

            VkApplicationInfo appInfo
            {
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName = App_Name,
                .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                .pEngineName = "No Engine",
                .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                .apiVersion = VK_API_VERSION_1_3,
            };

            VkInstanceCreateInfo createInfo
            {
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pApplicationInfo = &appInfo,
            };

            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            createInfo.enabledExtensionCount = glfwExtensionCount;
            createInfo.ppEnabledExtensionNames = glfwExtensions;
            createInfo.enabledLayerCount = 0;

            vk_check(vkCreateInstance(&createInfo, nullptr, &instance));

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

        void pick_physical_device() // selects a physical device (GPU) that supports Vulkan and meets the application's requirements //
        {
            uint32_t device_count = 0;
            vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

            vk_check(device_count != 0); // check if there are any Vulkan-capable devices //
            
            std::vector<VkPhysicalDevice> devices(device_count);
            vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

            for (const auto& device : devices) 
            {
                if (is_device_suitable(device)) 
                {
                    physical_device = device;
                    break;
                }
            }

            vk_check(physical_device != VK_NULL_HANDLE); // check if a suitable device was found //
        }

        void create_logical_device() // creates a logical device, which is an abstraction of the physical device and allows the application to interact with it //
        {
            QueueFamilyIndices indices = find_queue_families(physical_device);

            std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
            std::set<uint32_t> unique_queue_families = {indices.graphicsFamily.value(), indices.presentFamily.value()};

            float queue_priority = 1.0f;

            for (uint32_t queueFamily : unique_queue_families) // ran twice if familys are different, once if they are the same //
            {
                VkDeviceQueueCreateInfo queue_create_info{};
                queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_create_info.queueFamilyIndex = queueFamily;
                queue_create_info.queueCount = 1;
                queue_create_info.pQueuePriorities = &queue_priority;
                queue_create_infos.push_back(queue_create_info);
            }

            VkPhysicalDeviceFeatures device_features{};
            VkDeviceCreateInfo create_info{};

            create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

            create_info.enabledExtensionCount   = static_cast<uint32_t>(device_extensions.size());
            create_info.ppEnabledExtensionNames = device_extensions.data();

            create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
            create_info.pQueueCreateInfos    = queue_create_infos.data();
        
            create_info.pEnabledFeatures = &device_features;

            if (enableValidationLayers) // This is not really needed, but says to include it? //
            {
                create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                create_info.ppEnabledLayerNames = validationLayers.data();
            } else 
            {
                create_info.enabledLayerCount = 0;
            }

            vk_check(vkCreateDevice(physical_device, &create_info, nullptr, &device));

            vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphics_queue);
            vkGetDeviceQueue(device, indices.presentFamily.value(),  0, &present_queue); 
        }

        void create_swapchain() // creates a swapchain, which is a series of images that are presented to the screen in a specific order //
        {
            SwapChainSupportDetails swapChainSupport = query_swapchain_support(physical_device);

            VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats     );
            VkPresentModeKHR   presentMode   = choose_swap_present_mode  (swapChainSupport.presentModes);
            VkExtent2D         extent        = choose_swap_extent       (swapChainSupport.capabilities);

            uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

            if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
            {
                imageCount = swapChainSupport.capabilities.maxImageCount;
            }
            
            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = surface;

            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = find_queue_families(physical_device);
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

            vk_check(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain));

            vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
            swapchain_images.resize(imageCount);
            vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchain_images.data());

            swapchain_image_format = surfaceFormat.format;
            swapchain_extent = extent;
        }

        void create_image_views() // creates image views for the swapchain images, which are used to access the images in the swapchain //
        {
            swapchain_image_views.resize(swapchain_images.size());

            for (size_t i = 0; i < swapchain_images.size(); i++) 
            {
                swapchain_image_views[i] = create_image_view(swapchain_images[i], swapchain_image_format);
            }
        }

        void create_render_pass() // creates a render pass, which defines how the rendering will be done and what attachments will be used //
        {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = swapchain_image_format;
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

            vk_check(vkCreateRenderPass(device, &renderPassInfo, nullptr, &render_pass));
        }

        void create_graphics_pipeline() // creates a graphics pipeline, which defines how the rendering will be done and what shaders will be used //
        {
            auto Shader_vert_spv = read_file(Vert_Spv_Dir);
            auto Shader_frag_spv = read_file(Frag_Spv_Dir);
            
            VkShaderModule vertShaderModule = create_shader_module(Shader_vert_spv); // Changing the name format does not mean anything //
            VkShaderModule fragShaderModule = create_shader_module(Shader_frag_spv);

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

            auto bindingDescription = Vertex::get_binding_description();
            auto attributeDescriptions = Vertex::get_attribute_descriptions();
           
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
            vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
            vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
            viewport.width    = (float) swapchain_extent.width;
            viewport.height   = (float) swapchain_extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{}; // Filter like //
            scissor.offset = {0, 0};
            scissor.extent = swapchain_extent;

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
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // VK_CULL_MODE_NONE
            rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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
            pipelineLayoutInfo.setLayoutCount = 1; 
            pipelineLayoutInfo.pSetLayouts = &descriptor_set_layout;
            pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
            pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

            vk_check(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipeline_layout));

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

            pipelineInfo.layout = pipeline_layout;

            pipelineInfo.renderPass = render_pass;
            pipelineInfo.subpass = 0;

            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
            pipelineInfo.basePipelineIndex = -1; // Optional

            vk_check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphics_pipeline));

            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
        }

        void create_framebuffers() // creates framebuffers for the swapchain images, which are used to render the images to the screen //
        {
            const size_t size = swapchain_image_views.size();
            swapchain_framebuffers.resize(size);

            for (size_t i = 0; i < size; i++) 
            {
                VkImageView attachments[] = 
                {
                    swapchain_image_views[i]
                };

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = render_pass;
                framebufferInfo.attachmentCount = 1;
                framebufferInfo.pAttachments = attachments;
                framebufferInfo.width = swapchain_extent.width;
                framebufferInfo.height = swapchain_extent.height;
                framebufferInfo.layers = 1;

                vk_check(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchain_framebuffers[i]));
            }
        }

        void create_command_pool() // creates a command pool, which is used to allocate command buffers for recording rendering commands //
        {
            QueueFamilyIndices queueFamilyIndices = find_queue_families(physical_device);

            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

            vk_check(vkCreateCommandPool(device, &poolInfo, nullptr, &command_pool));
        }

        void create_command_buffers() // creates command buffers, which are used to record rendering commands that will be submitted to the GPU //
        {
            command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = command_pool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = (uint32_t) MAX_FRAMES_IN_FLIGHT;

            vk_check(vkAllocateCommandBuffers(device, &allocInfo, command_buffers.data()));
        }

        void create_sync_objects() // creates synchronization objects, which are used to synchronize the rendering commands and the presentation of the images to the screen //
        {
            image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
            render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
            in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
            {
                vk_check(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &image_available_semaphores[i]));
                vk_check(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &render_finished_semaphores[i]));
                vk_check(vkCreateFence(device, &fenceInfo, nullptr, &in_flight_fences[i]));
            }
        }
    
        // destroy functions //

        void cleanup_swapchain()
        {
            for (auto framebuffer : swapchain_framebuffers) 
            {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
            }

            for (auto image_view : swapchain_image_views) 
            {
                vkDestroyImageView(device, image_view, nullptr);
            }

            vkDestroySwapchainKHR(device, swapchain, nullptr);
        }

        // HELPERS //

        VkShaderModule create_shader_module(const std::vector<char>& code) 
        {
            VkShaderModuleCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // from u8 to u32 pointer //

            VkShaderModule shaderModule;
            vk_check(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));
            return shaderModule;
        }

        static std::vector<char> read_file(const std::string& file_name) 
        {
            std::ifstream file(file_name, std::ios::ate | std::ios::binary);
            vk_check(file.is_open());

            size_t file_size = (size_t)file.tellg();
            std::vector<char> buffer(file_size);

            file.seekg(0);
            file.read(buffer.data(), file_size);
            file.close();

            return buffer;
        }

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height) 
        {
            auto app = reinterpret_cast<Vulkan_App*>(glfwGetWindowUserPointer(window));
            app->framebuffer_resized = true;
        }

        bool checkdevice_extensionsupport(VkPhysicalDevice device)
        {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string> requiredExtensions(device_extensions.begin(), device_extensions.end());

            for (const auto& extension : availableExtensions)
            {
                requiredExtensions.erase(extension.extensionName);
            }

            return requiredExtensions.empty();
        }

        bool is_device_suitable(VkPhysicalDevice device)
        {
            QueueFamilyIndices indices = find_queue_families(device);

            bool extensionsSupported = checkdevice_extensionsupport(device);

            bool swapChainAdequate = false;
            if (extensionsSupported) 
            {
                SwapChainSupportDetails swapChainSupport = query_swapchain_support(device);
                swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
            }

            return indices.isComplete() && extensionsSupported && swapChainAdequate;
        }

        bool check_validation_layer_support() 
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