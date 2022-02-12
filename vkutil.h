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

  VkSwapchainKHR swap;
  u32 numImages;
  VkImage* images;
  VkImageView* imageViews;
  VkFramebuffer* framebuffers; 

  VkExtent2D extent;
  VkPipelineLayout pipelineLayout;
  VkRenderPass renderPass;
  VkPipeline pipeline;

  VkCommandPool pool;
  VkCommandBuffer* commandBuffers;

  VkSemaphore* imageAvailables;
  VkSemaphore* renderCompletes;
  VkFence* fences;
  VkFence* fenceSyncs;
  u32 currentImage;

  bool rendering;

  VkDescriptorSetLayout bufferLayout;
  VkBuffer* UBOs;
  VkDeviceMemory* UBOmems;
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

plvkState* createInstance( void );
void createDescriptorPool( plvkState* vk );
void createDescriptorSets( plvkState* vk );
void getFuncPointers( plvkState* vk );
void createBuffer( plvkState* vk, VkDeviceSize size, VkBufferUsageFlags usage,
		   VkMemoryPropertyFlags properties, VkBuffer* buffer,
		   VkDeviceMemory* bufferMemory );
void destroyUBOs( plvkState* vk );
void createUBOs( plvkState* vk );
void getExtent( plvkState* vk );
