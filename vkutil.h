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

#define VK_USE_PLATFORM_WIN32_KHR
#include "inc\vulkan\vulkan.h"
#include "pl.h"
#include "gui.h"

typedef struct gpuState{
  f32 time;
} gpuState;

typedef struct plvkUnit plvkUnit;

// Win32/vulkan surface
typedef struct plvkSurface{
  VkSurfaceKHR surface;
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  u32 numSurfaceFormats;
  VkSurfaceFormatKHR* surfaceFormats;
  VkSurfaceFormatKHR theSurfaceFormat;
  u32 numSurfacePresentations;
  VkPresentModeKHR* surfacePresentations;
} plvkSurface;
// Pipeline and associated state.
typedef struct plvkPipeline{
  VkPipelineLayout pipelineLayout;
  VkRenderPass renderPass;
  VkPipeline pipeline;
} plvkPipeline;
// Swapchain and associated state.
typedef struct plvkSwapchain{
  VkSwapchainKHR swap;
  u32 numImages;
  VkImage* images;
  VkImageView* imageViews;
} plvkSwapchain;
// A GPU buffer.
typedef struct plvkBuffer{
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkMemoryPropertyFlags props;
  VkBufferUsageFlags usage;
  VkDeviceSize size;
} plvkBuffer;
// Textures.
typedef struct plvkTexture{
  u32 size;
  u32 width, height;
  u8 channels;
  VkImageUsageFlagBits usage;
  VkImageTiling tiling;
  VkFormat format;    
  VkImage image;
  VkDeviceMemory imageMem;
  u64 deviceSize;
  VkImageView view;
  VkSampler sampler;
} plvkTexture;
// Instance wide state.
typedef struct plvkIntance{
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
  plvkSurface* surface;
  
  u32 gpuIndex;
  VkPhysicalDevice gpu;
  VkPhysicalDeviceProperties* selectedGpuProperties;
  VkPhysicalDeviceFeatures selectedGpuFeatures;
  u32 numDeviceExtensions;
  VkExtensionProperties* deviceExtensions;
  u32 queueFamily;
  VkDevice device;
  u32 debugLevel;

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

  plvkTexture* tex;
  
  VkDescriptorSetLayout layout;
  plvkBuffer** UBOs;
  gpuState UBOcpumem;
  VkDescriptorPool descriptorPool;
  VkDescriptorSet* descriptorSets;

  // A tree of units.
  plvkUnit* unit;
  
  // Function pointers.
#define FPDEFINE( x ) PFN_##x x
#ifdef DEBUG
  FPDEFINE( vkCreateDebugUtilsMessengerEXT );
  FPDEFINE( vkDestroyDebugUtilsMessengerEXT );
  VkDebugUtilsMessengerEXT vkdbg;
#endif
} plvkInstance;
#define PLVKINSTANCE_DEFINED 1


typedef struct plvkUnitDisplay{
  guiInfo* gui;
  plvkSwapchain* swap;
  plvkSurface* surface;
  VkSemaphore imageAvailables[ 2 ];
  VkSemaphore renderCompletes[ 2 ];
} plvkUnitDisplay;


// This is the fundamental unit of GPU computation. This can be used for graphics
// or standalone.
typedef struct plvkUnit{
  // The logical device and instance that this unit will be run on. These
  // must remain valid throughout the lifetime of the plvkUnit.
  plvkInstance* instance;

  VkExtent2D size;

  VkFramebuffer framebuffers[ 2 ]; 

  plvkPipeline* pipe;

  VkCommandPool pool;
  VkCommandBuffer commandBuffers[ 2 ];

  VkFence* fences;

  plvkTexture* textures[ 2 ];
  
  VkDescriptorSetLayout layout;

  VkDescriptorPool descriptorPool;
  VkDescriptorSet descriptorSets[ 2 ];

  // This is 0 or 1. Write to ping, read from pong, then write to pong,
  // read from ping.
  bool pingpong;

  // Null if the unit isn't displayed.
  plvkUnitDisplay* display;

  // The next unit.
  plvkUnit* next;

  VkFormat format;
} plvkUnit;

// ubofunc may be NULL. title, x and y are ignored if displayed is false.
// title may be NULL if displayed == 0. format is ignored for displayed units,
// the format will be whatever the surfae supports.
plvkUnit* createUnit( plvkInstance* vk, u32 width, u32 height,
		      VkFormat format, u8 components,
		      const char* fragName, const char* vertName,
		      bool displayed, const char* title, int x, int y );

void destroyUnit( plvkUnit* u );
void tickUnit( plvkUnit* u );
void waitUnit( plvkUnit* u );
// The data returned must be memfree'd by the caller.  This is an expensive
// operation.
const u8* copyUnit( plvkUnit* u );


plvkInstance* createInstance(  s32 whichGPU, u32 debugLevel,
			  char* title, int x, int y, int width, int height );
void destroyInstance( plvkInstance* vk );
void createDescriptorPool( plvkInstance* vk );
void destroyDescriptorPool( plvkInstance* vk );
// Descriptor sets are cleaned up along with the descriptor pool.
void createDescriptorSets( plvkInstance* vk );
void destroyDescriptorSets( plvkInstance* vk );
void getFuncPointers( plvkInstance* vk );
plvkBuffer* createBuffer( plvkInstance* vk, u64 size, VkBufferUsageFlags usage,
			  VkMemoryPropertyFlags props );
void destroyBuffer( plvkInstance* vk, plvkBuffer* p );
void createUBOs( plvkInstance* vk );
void destroyUBOs( plvkInstance* vk );
VkExtent2D getExtent( plvkInstance* vk );
VkShaderModule createModule( VkDevice vkd, const char* data, u32 size );
void destroyModule( plvkInstance* vk, VkShaderModule sm );
VkDescriptorSetLayout createLayout( plvkInstance* vk );
void destroyLayout( plvkInstance* vk );
u64 scoreGPU( VkPhysicalDeviceProperties* gpu );
plvkSwapchain* createSwap( plvkInstance* vk, bool vsync, u32 minFrames );
void destroySwap( plvkInstance* vk, plvkSwapchain* swap );
plvkPipeline* createPipeline( plvkInstance* vk, const char* frag, u32 fsize,
			      const char* vert, u32 vsize );
void destroyPipeline( plvkInstance* vk, plvkPipeline* p );
VkFramebuffer* createFramebuffers( plvkInstance* vk, plvkPipeline* p,
				   plvkSwapchain* swap );
void destroyFramebuffers( plvkInstance* vk, VkFramebuffer* fbs );
VkCommandBuffer* createCommandBuffers( plvkInstance* vk );
void destroyCommandBuffers( plvkInstance* vk, VkCommandBuffer* cbs );
// Command pool, semaphores, and fences.
void createPoolAndFences( plvkInstance* vk );
void destroyPoolAndFences( plvkInstance* vk, u32 numImages );
plvkSurface* createSurface( plvkInstance* vk );
void destroySurface( plvkInstance* vk, plvkSurface* surf );
void createTextures( plvkInstance* vk );
void destroyTextures( plvkInstance* vk );  
VkImageView createView( plvkInstance* vk, VkImage img, VkFormat format );
VkSampler createSampler( plvkInstance* vk );
