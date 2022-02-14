////////////////////////////////////////////////////////////////////////////////
//     Copyright (c) Jon DuBois 2022. This file is part of pseudoluminal.     //
//                                                                            //
// This program is free software: you can redistribute it and/or modify       //
// it under the terms of the GNU Affero General Public License as published   //
// by the Free Software Foundation, either version 3 of the License, or       //
// (at your option) any later version.                                        //
//                                                                            //
// This program is distributed in the hope that it will be useful,            //
// but WITHOUT ANY WARRANTY; without even the implied warranty of             //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              //
// GNU Affero General Public License for more details.                        //
//                                                                            //
// You should have received a copy of the GNU Affero General Public License   //
// along with this program.  If not, see <https://www.gnu.org/licenses/>.     //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
// Vulkan internal interface.                                                 //
////////////////////////////////////////////////////////////////////////////////

#include "inc\vulkan\vulkan.h"
#include "pl.h"
#include "gui.h"

typedef struct {
  f32 time;
} gpuState;


// Pipeline and associated state.
typedef struct{
  VkPipelineLayout pipelineLayout;
  VkRenderPass renderPass;
  VkPipeline pipeline;
} plvkPipeline;
// Swapchain and associated state.
typedef struct{
  VkSwapchainKHR swap;
  u32 numImages;
  VkImage* images;
  VkImageView* imageViews;
} plvkSwapchain;
// A GPU buffer.
typedef struct{
  VkBuffer buffer;
  VkDeviceMemory memory;
} plvkBuffer;
// Instance wide state.
typedef struct {
  VkInstance instance;
  guiInfo* gui;
  u32 numGPUs;
  VkPhysicalDevice* gpus;
  VkPhysicalDeviceProperties* gpuProperties;
  u32 numExtensions;
  VkExtensionProperties* extensions;
  u32 numLayers;
  VkLayerProperties* layers;
  u32 numQueues;
  VkQueue queue;
  VkQueueFamilyProperties* queueFamilies;
  VkSurfaceKHR surface;

  u32 gpuIndex;
  VkPhysicalDevice gpu;
  VkPhysicalDeviceProperties* selectedGpuProperties;
  VkPhysicalDeviceFeatures selectedGpuFeatures;
  u32 numDeviceExtensions;
  VkExtensionProperties* deviceExtensions;
  u32 queueFamily;
  VkDevice device;
  u32 debugLevel;

  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  u32 numSurfaceFormats;
  VkSurfaceFormatKHR* surfaceFormats;
  VkSurfaceFormatKHR theSurfaceFormat;
  u32 numSurfacePresentations;
  VkPresentModeKHR* surfacePresentations;

  // swap WILL BE NULL IF NOT RENDERING!!!
  plvkSwapchain* swap;
  VkFramebuffer* framebuffers; 

  VkExtent2D extent;
  plvkPipeline* pipe;

  VkCommandPool pool;
  VkCommandBuffer* commandBuffers;

  VkSemaphore* imageAvailables;
  VkSemaphore* renderCompletes;
  VkFence* fences;
  VkFence* fenceSyncs;
  u32 currentImage;

  VkDescriptorSetLayout bufferLayout;
  plvkBuffer* UBOs;
  gpuState UBOcpumem;
  VkDescriptorPool descriptorPool;
  VkDescriptorSet* descriptorSets;
  
  // Function pointers.
#define FPDEFINE( x ) PFN_##x x
#ifdef DEBUG
  FPDEFINE( vkCreateDebugUtilsMessengerEXT );
  FPDEFINE( vkDestroyDebugUtilsMessengerEXT );
  VkDebugUtilsMessengerEXT vkdbg;
#endif
} plvkState;


plvkState* createDevice(  s32 whichGPU, u32 debugLevel,
			  char* title, int x, int y, int width, int height );
void destroyDevice( plvkState* vk );
void createDescriptorPool( plvkState* vk );
void destroyDescriptorPool( plvkState* vk );
// Descriptor sets are cleaned up along with the descriptor pool.
void createDescriptorSets( plvkState* vk );
void getFuncPointers( plvkState* vk );
void createBuffer( plvkState* vk, VkDeviceSize size, VkBufferUsageFlags usage,
		   plvkBuffer* buffer );
void destroyBuffer( plvkState* vk, plvkBuffer* p );
void createUBOs( plvkState* vk );
void destroyUBOs( plvkState* vk );
VkExtent2D getExtent( plvkState* vk );
VkShaderModule createModule( VkDevice vkd, const char* data, u32 size );
void destroyModule( plvkState* vk, VkShaderModule sm );
VkDescriptorSetLayout createUBOLayout( plvkState* vk );
void destroyUBOLayout( plvkState* vk, VkDescriptorSetLayout dsl );
u64 scoreGPU( VkPhysicalDeviceProperties* gpu );
// Creates a swapchain with the specified properties. 
plvkSwapchain* createSwap( plvkState* vk, bool vsync, u32 minFrames );
void destroySwap( plvkState* vk, plvkSwapchain* swap );
plvkPipeline* createPipeline( plvkState* vk, const char* frag, u32 fsize,
			      const char* vert, u32 vsize );
void destroyPipeline( plvkState* vk, plvkPipeline* p );
