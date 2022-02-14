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

#include "vkutil.h"
#include "util.h"
#include "os.h"

// Validation layer callback.
#ifdef DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL plvkDebugcb
( VkDebugUtilsMessageSeverityFlagBitsEXT severity,
  VkDebugUtilsMessageTypeFlagsEXT type,
  const VkDebugUtilsMessengerCallbackDataEXT* callback,
  void* user ) {
  (void)user;
  if( (u32)severity >= ((plvkState*)state.vk)->debugLevel ){
    print( "validation " );
    printInt( type );
    print( ", " );
    printInt( severity );
    print( ": " );
    printl( callback->pMessage );
  }

  return VK_FALSE;
}
// Debug layer createinfo helper.
void setupDebugCreateinfo( VkDebugUtilsMessengerCreateInfoEXT* ci ){
  ci->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  ci->pNext = NULL;
  ci->flags = 0;
  ci->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  ci->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
  ci->pfnUserCallback = plvkDebugcb;
  ci->pUserData = NULL;
}
#endif


// Requirements
const char* requiredDeviceExtensions[] = { "VK_KHR_swapchain" };
const u32 numRequiredDeviceExtensions =
  sizeof( requiredDeviceExtensions ) / sizeof( char* );
#ifdef DEBUG
const char* requiredExtensions[] =
  { "VK_KHR_surface", "VK_KHR_win32_surface",
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
#else
const char* requiredExtensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
#endif
#ifdef DEBUG
const char* requiredLayers[] = { "VK_LAYER_KHRONOS_validation" };
#else
const char* requiredLayers[] = {};
#endif
const u32 numRequiredExtensions =
  sizeof( requiredExtensions ) / sizeof( char* );
const u32 numRequiredLayers = sizeof( requiredLayers ) / sizeof( char* );


// GPU scoring function
u64 scoreGPU( VkPhysicalDeviceProperties* gpu ){
  u64 score = 0;
  if( gpu->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
    score += 1000000;
  score += gpu->limits.maxImageDimension2D;
  return score;
}


// Function pointer helper function.
#define FPGET( x ) vk->x = (PFN_##x) vkGetInstanceProcAddr( vk->instance, #x );
void getFuncPointers( plvkState* vk ){
  (void)vk;
#ifdef DEBUG   
  FPGET( vkCreateDebugUtilsMessengerEXT );
  FPGET( vkDestroyDebugUtilsMessengerEXT );
#endif
}


void createDescriptorSets( plvkState* vk ){
  newa( tl, VkDescriptorSetLayout, vk->numImages );
  for( u32 i = 0; i < vk->numImages; ++i )
    tl[ i ] = vk->bufferLayout;
  VkDescriptorSetAllocateInfo dsai = {};
  dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsai.descriptorPool = vk->descriptorPool;
  dsai.descriptorSetCount = vk->numImages;
  dsai.pSetLayouts = tl;

  
  if( !vk->descriptorSets )
    vk->descriptorSets = newae( VkDescriptorSet, vk->numImages );

  if( VK_SUCCESS != vkAllocateDescriptorSets( vk->device, &dsai,
					      vk->descriptorSets ) )
    die( "Failed to allocate descriptor sets." );

  for( u32 i = 0; i < vk->numImages; ++i ){
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = vk->UBOs[ i ];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof( gpuState );

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = vk->descriptorSets[ i ];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets( vk->device, 1, &descriptorWrite, 0, NULL );
  }
  memfree( tl );
}


void createDescriptorPool( plvkState* vk ) {
  VkDescriptorPoolSize dps = {};
  dps.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  dps.descriptorCount = vk->numImages;

  VkDescriptorPoolCreateInfo dpci = {};
  dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  dpci.poolSizeCount = 1;
  dpci.pPoolSizes = &dps;
  dpci.maxSets = vk->numImages;
  if( VK_SUCCESS != vkCreateDescriptorPool( vk->device, &dpci, NULL,
					    &vk->descriptorPool ) )
    die( "Descriptor pool creation failed." );
}


void createBuffer( plvkState* vk, VkDeviceSize size, VkBufferUsageFlags usage,
		   VkMemoryPropertyFlags properties, VkBuffer* buffer,
		   VkDeviceMemory* bufferMemory ){
  VkBufferCreateInfo bci = {};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.size = size;
  bci.usage = usage;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if( VK_SUCCESS != vkCreateBuffer( vk->device, &bci,
				    NULL, buffer ) )
    die( "Buffer creation failed." );

  VkMemoryRequirements mr;
  vkGetBufferMemoryRequirements( vk->device, *buffer, &mr );

  VkMemoryAllocateInfo mai = {};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = mr.size;

  VkPhysicalDeviceMemoryProperties pdmp;
  vkGetPhysicalDeviceMemoryProperties( vk->gpu, &pdmp );
  bool found = 0;
  u32 memtype;
  for( u32 i = 0; i < pdmp.memoryTypeCount; ++i )
    if( mr.memoryTypeBits & ( 1 << i ) &&
	( ( pdmp.memoryTypes[i].propertyFlags &
	    properties ) == properties ) ){
      memtype = i;
      found = 1;
    }
  if( !found )
    die( "No suitable GPU memory buffer found." );

  mai.memoryTypeIndex = memtype;

  if( VK_SUCCESS != vkAllocateMemory( vk->device, &mai, NULL,
				      bufferMemory ) )
    die( "GPU memory allocation failed." );

  vkBindBufferMemory( vk->device, *buffer, *bufferMemory, 0 );
}


void createUBOs( plvkState* vk ){
  VkDeviceSize size = sizeof( gpuState );
  if( !vk->UBOs )
    vk->UBOs = newae( VkBuffer, vk->numImages );
  if( !vk->UBOmems )
    vk->UBOmems = newae( VkDeviceMemory, vk->numImages );
  for( size_t i = 0; i < vk->numImages; i++ )
    createBuffer( vk, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		  &vk->UBOs[ i ], &vk->UBOmems[ i ] );
}


void destroyUBOs( plvkState* vk ){
  for( u32 i = 0; i < vk->numImages; ++i ){
    vkDestroyBuffer( vk->device, vk->UBOs[ i ], NULL );
    vkFreeMemory( vk->device, vk->UBOmems[ i ], NULL );
  }
}
plvkState* createDevice(  s32 whichGPU, u32 debugLevel,
			  char* title, int x, int y, int width, int height ){
  new( vk, plvkState );
  state.vk = vk;
  vk->gui = wsetup( title, x, y, width, height );
  vk->debugLevel = debugLevel;

  // Get extensions.
  vkEnumerateInstanceExtensionProperties( NULL, &vk->numExtensions, NULL );
  vk->extensions = newae( VkExtensionProperties, vk->numExtensions );
  vkEnumerateInstanceExtensionProperties( NULL, &vk->numExtensions,
					  vk->extensions );
  for( u32 i = 0; i < numRequiredExtensions; ++i ){
    bool has = 0;
    for( u32 j = 0; j < vk->numExtensions; j++ ){
      if( !strcomp( requiredExtensions[ i ],
		    vk->extensions[ j ].extensionName ) )
	has = 1;
    }
    if( !has )
      die( "Required instance extension not found!" );
  }

  // Get layers.
  vkEnumerateInstanceLayerProperties( &vk->numLayers, NULL );
  vk->layers = newae( VkLayerProperties, vk->numLayers );
  vkEnumerateInstanceLayerProperties( &vk->numLayers, vk->layers );
  for( u32 i = 0; i < numRequiredLayers; ++i ){
    bool has = 0;
    for( u32 j = 0; j < vk->numLayers; j++ ){
      if( !strcomp( requiredLayers[ i ], vk->layers[ j ].layerName ) )
	has = 1;
    }
    if( !has )
      die( "Required layer not found!" );
  }

  // Create vulkan instance.
  VkApplicationInfo prog;
  prog.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  prog.pApplicationName = TARGET;
  prog.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  prog.pEngineName = TARGET;
  prog.pNext = NULL;
  prog.engineVersion = VK_MAKE_VERSION(0, 0, 1);
  prog.apiVersion = VK_API_VERSION_1_2;
  VkInstanceCreateInfo create;
  create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create.flags = 0;
  create.pApplicationInfo = &prog;
  create.enabledLayerCount = numRequiredLayers;
  create.ppEnabledLayerNames = requiredLayers;
  create.enabledExtensionCount = numRequiredExtensions;
  create.ppEnabledExtensionNames = requiredExtensions;
  create.pNext = NULL;

  if( VK_SUCCESS != vkCreateInstance( &create, NULL, &vk->instance ) )
    die( "Failed to create vulkan instance." );

  // Get function pointers.
  getFuncPointers( vk );

  // Set up debugging.
#ifdef DEBUG
  VkDebugUtilsMessengerCreateInfoEXT ci;
  setupDebugCreateinfo( &ci );
  vk->vkCreateDebugUtilsMessengerEXT( vk->instance, &ci, NULL, &vk->vkdbg );
#endif


  // Enumerate GPUs and pick one.
  if( VK_SUCCESS != vkEnumeratePhysicalDevices( vk->instance,
						&vk->numGPUs, NULL ) )
    die( "Failed to count gpus." );
  if( !vk->numGPUs )
    die( "No GPUs found :(" );
  vk->gpus = newae( VkPhysicalDevice, vk->numGPUs );
  if( VK_SUCCESS != vkEnumeratePhysicalDevices( vk->instance, &vk->numGPUs,
						vk->gpus ) )
    die( "Failed to enumerate gpus." );
  vk->gpuProperties = newae( VkPhysicalDeviceProperties, vk->numGPUs );
  for( u32 i = 0; i < vk->numGPUs; ++i )
    vkGetPhysicalDeviceProperties( vk->gpus[ i ], vk->gpuProperties + i );
  u32 best = 0;
  u32 bestScore = 0;
  for( u32 i = 0; i < vk->numGPUs; ++i ){
    u64 score = scoreGPU( vk->gpuProperties + i );
    if( score > bestScore ){
      best = i;
      bestScore = score;
    }
  }
  if( whichGPU != -1 ){
    if( (u32)whichGPU >= vk->numGPUs )
      die( "Nonexistent gpu selected." );
    vk->gpuIndex = whichGPU;
  } else{
    vk->gpuIndex = best;
  }
  // Get selected gpu information.
  vk->gpu = vk->gpus[ vk->gpuIndex ];
  vk->selectedGpuProperties = vk->gpuProperties + vk->gpuIndex;
  vkGetPhysicalDeviceFeatures( vk->gpu, &vk->selectedGpuFeatures );
  vkEnumerateDeviceExtensionProperties( vk->gpu, NULL,
					&vk->numDeviceExtensions, NULL );
  // Check device extensions.
  vk->deviceExtensions = newae( VkExtensionProperties,
				vk->numDeviceExtensions );
  vkEnumerateDeviceExtensionProperties( vk->gpu, NULL,
					&vk->numDeviceExtensions,
					vk->deviceExtensions );
  for( u32 i = 0; i < numRequiredDeviceExtensions; ++i ){
    bool has = 0;
    for( u32 j = 0; j < vk->numDeviceExtensions; j++ ){
      if( !strcomp( requiredDeviceExtensions[ i ],
		    vk->deviceExtensions[ j ].extensionName ) )
	has = 1;
    }
    if( !has )
      die( "Required device extension not found!" );
  }

  // Get queue families
  vk->numQueues = 0;
  vkGetPhysicalDeviceQueueFamilyProperties( vk->gpu, &vk->numQueues, NULL );
  vk->queueFamilies = newae( VkQueueFamilyProperties, vk->numQueues );
  vkGetPhysicalDeviceQueueFamilyProperties( vk->gpu, &vk->numQueues,
					    vk->queueFamilies );
  bool found = 0;
  for( u32 i = 0; i < vk->numQueues; ++i ){
    if( !found &&
	( vk->queueFamilies[ i ].queueFlags & VK_QUEUE_GRAPHICS_BIT ) ){
      found = 1;
      vk->queueFamily = i;
    }
  }
  if( !found )
    die( "No vulkan queue family found that has VK_QUEUE_GRAPHICS_BIT." );

  // Create logical device and get queue handle for it.
  VkDeviceQueueCreateInfo qci = {};
  qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  qci.queueFamilyIndex = vk->queueFamily;
  qci.queueCount = 1;
  float qpriority = 1.0f;
  qci.pQueuePriorities = &qpriority;
  VkPhysicalDeviceFeatures deviceFeatures = {};
  VkDeviceCreateInfo dci = {};
  dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  dci.pQueueCreateInfos = &qci;
  dci.queueCreateInfoCount = 1;
  dci.pEnabledFeatures = &deviceFeatures;
  dci.enabledExtensionCount = numRequiredDeviceExtensions;
  dci.ppEnabledExtensionNames = requiredDeviceExtensions;
  
  vk->device = NULL;
  if( VK_SUCCESS != vkCreateDevice( vk->gpu, &dci, NULL, &vk->device ) )
    die( "Device creation failed." );
  vkGetDeviceQueue( vk->device, vk->queueFamily, 0, &vk->queue );

  return vk;
}


void getExtent( plvkState* vk ){
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vk->gpu, vk->surface,
					     &vk->surfaceCapabilities );
  const guiInfo* g = vk->gui;
  if( vk->surfaceCapabilities.currentExtent.width != UINT32_MAX ){
    vk->extent = vk->surfaceCapabilities.currentExtent;
  } else {
    VkExtent2D  wh;
    wh.width = g->clientWidth;
    wh.height = g->clientHeight;
    if( wh.width > vk->surfaceCapabilities.maxImageExtent.width )
      wh.width = vk->surfaceCapabilities.maxImageExtent.width;
    if( wh.width < vk->surfaceCapabilities.minImageExtent.width )
      wh.width = vk->surfaceCapabilities.minImageExtent.width;
    if( wh.height > vk->surfaceCapabilities.maxImageExtent.height )
      wh.height = vk->surfaceCapabilities.maxImageExtent.height;
    if( wh.height < vk->surfaceCapabilities.minImageExtent.height )
      wh.height = vk->surfaceCapabilities.minImageExtent.height;
    vk->extent = wh;
  }
  

  if( !vk->extent.width || !vk->extent.height )
    vk->rendering = 0;
  else
    vk->rendering = 1;
}


VkShaderModule createModule( VkDevice vkd, const char* data, u32 size ){
  VkShaderModuleCreateInfo ci = {};
  ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  ci.codeSize = size;
  ci.pCode = (const u32*)data;
  VkShaderModule ret;
  if( VK_SUCCESS != vkCreateShaderModule( vkd, &ci, NULL, &ret ) )
    die( "Module creation failed." );
  return ret;
}


void createUBOLayout( plvkState* vk ){
  VkDescriptorSetLayoutBinding dslb = {};
  dslb.binding = 0;
  dslb.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  dslb.descriptorCount = 1;
  dslb.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutCreateInfo dslci = {};
  dslci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  dslci.bindingCount = 1;
  dslci.pBindings = &dslb;
  if( VK_SUCCESS != vkCreateDescriptorSetLayout( vk->device, &dslci, NULL,
						 &vk->bufferLayout ) ) 
    die( "Descriptor layout creation failed." );
}
