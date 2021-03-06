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
#include "input.h"

typedef struct gpuState{
  f32 time;
  f32 deltaTime;
  inputDeviceBuffers input;
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
// A GPU buffer.
typedef struct plvkBuffer{
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkMemoryPropertyFlags props;
  VkBufferUsageFlags usage;
  VkDeviceSize size;
  u32 type;
} plvkBuffer;
// Textures.
typedef struct plvkTexture{
  u32 size;
  u32 width, height;
  u8 fragmentSize;
  VkImageUsageFlagBits usage;
  VkImageTiling tiling;
  VkFormat format;    
  VkImage image;
  VkDeviceMemory imageMem;
  u64 deviceSize;
  VkImageView view;
  VkSampler sampler;
} plvkTexture;
// Swapchain and associated state.
typedef struct plvkSwapchain{
  VkSwapchainKHR swap;
  u32 numImages;
  VkImage* images;
  VkImageView* imageViews;
  plvkTexture* depth;
} plvkSwapchain;
typedef struct plvkAttachable{
  enum {
    PLVK_ATTACH_INVALID = 0,
    PLVK_ATTACH_TEXTURE = 1,
    PLVK_ATTACH_UNIT = 2,
    PLVK_ATTACH_BUFFER = 3
  } type;
  union {
    plvkTexture* texture;
    plvkUnit* unit;
    plvkBuffer* buffer;
  };
  struct plvkAttachable* next;
} plvkAttachable;
typedef struct plvkUniformBufferCallback{
  struct plvkUniformBufferCallback* next;
  plvkBuffer* buf;
  void* data;
  void (*func)( void*, void* );
} plvkUniformBufferCallback;

// Instance wide state.
typedef struct plvkInstance{
  bool useTensorCores;
  u32 tensorPropertyCount;
  VkCooperativeMatrixPropertiesNV* tensorProperties;
  
  VkInstance instance;
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
  //  plvkSurface* surface;
  
  u32 gpuIndex;
  VkPhysicalDevice gpu;
  VkPhysicalDeviceProperties* selectedGpuProperties;
  VkPhysicalDeviceFeatures selectedGpuFeatures;
  u32 numDeviceExtensions;
  VkExtensionProperties* deviceExtensions;
  u32 queueFamily;
  VkDevice device;
  u32 debugLevel;

  VkExtent2D extent;
  plvkPipeline* pipe;

  VkCommandPool pool;

  u32 currentImage;

  
  // A linked list of attachable items, which will be referenced during
  // unit creation. Adding a texture, unit, or compute unit puts a new element
  // of this list at the front, pushingdown all other elements.
  plvkAttachable* attachables;
  
  //  VkDescriptorSetLayout layout;
  plvkBuffer** UBOs;
  gpuState UBOcpumem;

  // A tree of units.
  plvkUnit* unit;

  // All callbacs are called once per draw.
  plvkUniformBufferCallback* ucallbacks;
  
  // Function pointers.
#define FPDEFINE( x ) PFN_##x x
#ifdef DEBUG
  FPDEFINE( vkCreateDebugUtilsMessengerEXT );
  FPDEFINE( vkDestroyDebugUtilsMessengerEXT );
  VkDebugUtilsMessengerEXT vkdbg;
#endif
  FPDEFINE( vkGetPhysicalDeviceCooperativeMatrixPropertiesNV );
  
  bool valid;
  HANDLE renderThread;
  HANDLE rendering;
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
  const char* fragName;
  const char* vertName;

  u64 numSpecializations;
  u32* specializations;
  
  u32 tickCount;
  
  u64 numAttachments;
  plvkAttachable** attachments;

  VkExtent2D size;

  VkFramebuffer framebuffers[ 2 ];
  plvkBuffer* buffers[ 2 ];

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

  // The size of the draw command.
  u64 drawSize;
} plvkUnit;

// title, x and y are ignored if displayed is false.
// title may be NULL if displayed == 0. format is ignored for displayed units,
// the format will be whatever the surface supports.
plvkUnit* createUnit( plvkInstance* vk, u32 width, u32 height,
		      VkFormat format, u8 fragmentSize,
		      const char* fragName, const char* vertName,
		      bool displayed, const char* title, int x, int y,
		      plvkAttachable** attachments, u64 numAttachments,
		      u64 drawSize, const void* pixels, u32 tickCount,
		      const u32* specializations, u64 numSpecializations );

void destroyUnit( plvkUnit* u );
void tickUnit( plvkUnit* u );
void waitUnit( plvkUnit* u );
// The data returned must be memfree'd by the caller.  This is an expensive
// operation.
const u8* copyUnit( plvkUnit* u );
plvkBuffer* createComputeBuffer( plvkInstance* vk, const void* data, u64 size );
// Returned array must be freed.
void* copyComputeBuffer( plvkUnit* u );


plvkInstance* createInstance(  s32 whichGPU, u32 debugLevel, bool useTensorCores );
void destroyInstance( plvkInstance* vk );
void createUBOs( plvkInstance* vk );
void destroyUBOs( plvkInstance* vk );

u64 scoreGPU( VkPhysicalDeviceProperties* gpu );

void createPool( plvkInstance* vk );
void destroyPool( plvkInstance* vk );

VkImageView createView( plvkInstance* vk, VkImage img, VkFormat format );
VkSampler createSampler( plvkInstance* vk );
plvkTexture* loadTexturePPM( plvkInstance* vk, const char* name );
void destroyAttachables( plvkInstance* vk );

plvkBuffer* createUniformBuffer( plvkInstance* vk, u64 size,
				 void (*uniform)( void*out, void* in ),
				 void* indata );
void* mapUniformBuffer( plvkInstance* vk, plvkBuffer* b );
void unmapUniformBuffer( plvkInstance* vk, plvkBuffer* b );
void getUnitSize( plvkUnit* u, u64* w, u64* h );
