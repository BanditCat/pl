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
  newa( tl, VkDescriptorSetLayout, vk->swap->numImages );
  for( u32 i = 0; i < vk->swap->numImages; ++i )
    tl[ i ] = vk->bufferLayout;
  VkDescriptorSetAllocateInfo dsai = {};
  dsai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsai.descriptorPool = vk->descriptorPool;
  dsai.descriptorSetCount = vk->swap->numImages;
  dsai.pSetLayouts = tl;

  
  if( !vk->descriptorSets )
    vk->descriptorSets = newae( VkDescriptorSet, vk->swap->numImages );

  if( VK_SUCCESS != vkAllocateDescriptorSets( vk->device, &dsai,
					      vk->descriptorSets ) )
    die( "Failed to allocate descriptor sets." );

  for( u32 i = 0; i < vk->swap->numImages; ++i ){
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = vk->UBOs[ i ]->buffer;
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


void destroyDescriptorSets( plvkState* vk ){
  memfree( vk->descriptorSets );
}


void createDescriptorPool( plvkState* vk ) {
  VkDescriptorPoolSize dps = {};
  dps.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  dps.descriptorCount = vk->swap->numImages;

  VkDescriptorPoolCreateInfo dpci = {};
  dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  dpci.poolSizeCount = 1;
  dpci.pPoolSizes = &dps;
  dpci.maxSets = vk->swap->numImages;
  if( VK_SUCCESS != vkCreateDescriptorPool( vk->device, &dpci, NULL,
					    &vk->descriptorPool ) )
    die( "Descriptor pool creation failed." );
}


void destroyDescriptorPool( plvkState* vk ){
  vkDestroyDescriptorPool( vk->device, vk->descriptorPool, NULL );
}


u32 findMemoryType( plvkState* vk, u32 filter, VkMemoryPropertyFlags props ){
  VkPhysicalDeviceMemoryProperties pdmp;
  vkGetPhysicalDeviceMemoryProperties( vk->gpu, &pdmp );
  for( u32 i = 0; i < pdmp.memoryTypeCount; ++i )
    if( filter & ( 1 << i ) &&
	( ( pdmp.memoryTypes[ i ].propertyFlags &
	    props ) == props ) ){
      return i;
    }
  die( "No suitable GPU memory buffer found." );
  return 0; // DNE
}

plvkBuffer* createBuffer( plvkState* vk, u64 size, VkBufferUsageFlags usage,
			  VkMemoryPropertyFlags props ){
  new( ret, plvkBuffer );
  VkBufferCreateInfo bci = {};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.size = size;
  bci.usage = usage;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if( VK_SUCCESS != vkCreateBuffer( vk->device, &bci,
				    NULL, &ret->buffer ) )
    die( "Buffer creation failed." );

  VkMemoryRequirements mr;
  vkGetBufferMemoryRequirements( vk->device, ret->buffer, &mr );

  VkMemoryAllocateInfo mai = {};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = mr.size;
  mai.memoryTypeIndex = findMemoryType( vk, mr.memoryTypeBits, props );

  if( VK_SUCCESS != vkAllocateMemory( vk->device, &mai, NULL,
				      &ret->memory ) )
    die( "GPU memory allocation failed." );

  vkBindBufferMemory( vk->device, ret->buffer, ret->memory, 0 );
  ret->size = size;
  return ret;
}


void destroyBuffer( plvkState* vk, plvkBuffer* p ){
  vkDestroyBuffer( vk->device, p->buffer, NULL );
  mark;
  vkFreeMemory( vk->device, p->memory, NULL );
  memfree( p );
}


void createUBOs( plvkState* vk ){
  VkDeviceSize size = sizeof( gpuState );
  vk->UBOs = newae( plvkBuffer*, vk->swap->numImages );
  for( size_t i = 0; i < vk->swap->numImages; i++ ){
    vk->UBOs[ i ] = createBuffer( vk, size,
				  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
  }
}


void destroyUBOs( plvkState* vk ){
  for( u32 i = 0; i < vk->swap->numImages; ++i )
    destroyBuffer( vk, vk->UBOs[ i ] );
  memfree( vk->UBOs );
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


void destroyDevice( plvkState* vk ){
  if( vk->device )
    vkDestroyDevice( vk->device, NULL);
  if( vk->instance )
    vkDestroyInstance( vk->instance, NULL );

  if( vk->extensions )
    memfree( vk->extensions );
  if( vk->layers )
    memfree( vk->layers );
  if( vk->gpus )
    memfree( vk->gpus );
  if( vk->gpuProperties )
    memfree( vk->gpuProperties );
  if( vk->deviceExtensions )
    memfree( vk->deviceExtensions );
  if( vk->queueFamilies )
    memfree( vk->queueFamilies );
}


VkExtent2D getExtent( plvkState* vk ){
  VkExtent2D wh = {};
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vk->gpu, vk->surface->surface,
					     &vk->surface->surfaceCapabilities );
  const guiInfo* g = vk->gui;
  if( vk->surface->surfaceCapabilities.currentExtent.width != UINT32_MAX ){
    wh = vk->surface->surfaceCapabilities.currentExtent;
  } else {
    wh.width = g->clientWidth;
    wh.height = g->clientHeight;
    if( wh.width > vk->surface->surfaceCapabilities.maxImageExtent.width )
      wh.width = vk->surface->surfaceCapabilities.maxImageExtent.width;
    if( wh.width < vk->surface->surfaceCapabilities.minImageExtent.width )
      wh.width = vk->surface->surfaceCapabilities.minImageExtent.width;
    if( wh.height > vk->surface->surfaceCapabilities.maxImageExtent.height )
      wh.height = vk->surface->surfaceCapabilities.maxImageExtent.height;
    if( wh.height < vk->surface->surfaceCapabilities.minImageExtent.height )
      wh.height = vk->surface->surfaceCapabilities.minImageExtent.height;
  }

  return wh;
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


void destroyModule( plvkState* vk, VkShaderModule sm ){
  vkDestroyShaderModule( vk->device, sm, NULL );
}


VkDescriptorSetLayout createUBOLayout( plvkState* vk ){
  VkDescriptorSetLayout ret;
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
						 &ret ) ) 
    die( "Descriptor layout creation failed." );
  return ret;
}


void destroyUBOLayout( plvkState* vk, VkDescriptorSetLayout dsl ){
  vkDestroyDescriptorSetLayout( vk->device, dsl, NULL );
}


plvkSwapchain* createSwap( plvkState* vk, bool vsync, u32 minFrames ){
  VkExtent2D wh = getExtent( vk );
  if( wh.width && wh.height ){
    vk->extent = wh;
  
    new( ret, plvkSwapchain );
    VkPresentModeKHR pm = vsync ? VK_PRESENT_MODE_FIFO_KHR :
      VK_PRESENT_MODE_IMMEDIATE_KHR ;
    VkSwapchainCreateInfoKHR scci = {};
    scci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    scci.surface = vk->surface->surface;
    scci.minImageCount = minFrames;
    scci.imageFormat = vk->surface->theSurfaceFormat.format;
    scci.imageColorSpace = vk->surface->theSurfaceFormat.colorSpace;
    scci.imageExtent = vk->extent;
    scci.imageArrayLayers = 1;
    scci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    scci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    scci.queueFamilyIndexCount = 0;
    scci.pQueueFamilyIndices = NULL;
    scci.preTransform = vk->surface->surfaceCapabilities.currentTransform;
    scci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    scci.presentMode = pm;
    scci.clipped = VK_TRUE;
    scci.oldSwapchain = VK_NULL_HANDLE;
    if( VK_SUCCESS !=
	vkCreateSwapchainKHR( vk->device, &scci, NULL, &ret->swap ) )
      die( "Swapchain creation failed." );
    vkGetSwapchainImagesKHR( vk->device, ret->swap, &ret->numImages, NULL );
    ret->images = newae( VkImage, ret->numImages );
    vkGetSwapchainImagesKHR( vk->device, ret->swap, &ret->numImages,
			     ret->images );
    // Image views.
    ret->imageViews = newae( VkImageView, ret->numImages );
    for( u32 i = 0; i < ret->numImages; ++i ){
      VkImageViewCreateInfo ivci = {};
      ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      ivci.image = ret->images[ i ];
      ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
      ivci.format = vk->surface->theSurfaceFormat.format;
      ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      ivci.subresourceRange.baseMipLevel = 0;
      ivci.subresourceRange.levelCount = 1;
      ivci.subresourceRange.baseArrayLayer = 0;
      ivci.subresourceRange.layerCount = 1;
      if( VK_SUCCESS != vkCreateImageView( vk->device, &ivci, NULL,
					   &ret->imageViews[ i ] ) )
	die( "Failed to create image view." );
    }
    return ret;
  } else
    return NULL;
}


void destroySwap( plvkState* vk, plvkSwapchain* swap ){
  for( u32 i = 0; i < swap->numImages; ++i )
    vkDestroyImageView( vk->device, swap->imageViews[ i ], NULL );
  vkDestroySwapchainKHR( vk->device, swap->swap, NULL );
  memfree( swap->images );
  memfree( swap->imageViews );
  memfree( swap );
}


plvkPipeline* createPipeline( plvkState* vk, const char* frag, u32 fsize,
			      const char* vert, u32 vsize ){
  VkShaderModule displayVertexShader;
  VkShaderModule displayFragmentShader;
  displayFragmentShader = createModule( vk->device, frag, fsize );
  displayVertexShader = createModule( vk->device, vert, vsize );

  new( ret, plvkPipeline );
  VkPipelineShaderStageCreateInfo pssci[ 2 ];
  pssci[ 0 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pssci[ 0 ].pNext = NULL;
  pssci[ 0 ].flags = 0;
  pssci[ 0 ].stage = VK_SHADER_STAGE_VERTEX_BIT;
  pssci[ 0 ].module = displayVertexShader;
  pssci[ 0 ].pName = "main";
  pssci[ 0 ].pSpecializationInfo = NULL;
  pssci[ 1 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pssci[ 1 ].pNext = NULL;
  pssci[ 1 ].flags = 0;
  pssci[ 1 ].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  pssci[ 1 ].module = displayFragmentShader;
  pssci[ 1 ].pName = "main";
  pssci[ 1 ].pSpecializationInfo = NULL;


  VkPipelineVertexInputStateCreateInfo pvici = {};
  pvici.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkPipelineInputAssemblyStateCreateInfo piasci = {};
  piasci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  piasci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  piasci.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) vk->extent.width;
  viewport.height = (float) vk->extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.extent = vk->extent;

  VkPipelineViewportStateCreateInfo pvsci = {};
  pvsci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  pvsci.viewportCount = 1;
  pvsci.pViewports = &viewport;
  pvsci.scissorCount = 1;
  pvsci.pScissors = &scissor;
    
  VkPipelineRasterizationStateCreateInfo prsci = {};
  prsci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  prsci.depthClampEnable = VK_FALSE;
  prsci.rasterizerDiscardEnable = VK_FALSE;
  prsci.polygonMode = VK_POLYGON_MODE_FILL;
  prsci.lineWidth = 1.0f;
  prsci.cullMode = VK_CULL_MODE_BACK_BIT;
  prsci.frontFace = VK_FRONT_FACE_CLOCKWISE;
  prsci.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo pmsci = {};
  pmsci.sType =
    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  pmsci.sampleShadingEnable = VK_FALSE;
  pmsci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  pmsci.minSampleShading = 1.0f;
  pmsci.pSampleMask = NULL;
  pmsci.alphaToCoverageEnable = VK_FALSE;
  pmsci.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState cba = {};
  cba.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  cba.blendEnable = VK_FALSE;
    

  VkPipelineColorBlendStateCreateInfo pcbsci = {};
  pcbsci.sType =
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  pcbsci.logicOpEnable = VK_FALSE;
  pcbsci.attachmentCount = 1;
  pcbsci.pAttachments = &cba;

  VkPipelineLayoutCreateInfo plci = {};
  plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  plci.setLayoutCount = 1;
  plci.pSetLayouts = &vk->bufferLayout;
  if( VK_SUCCESS !=
      vkCreatePipelineLayout( vk->device, &plci,
			      NULL, &ret->pipelineLayout ) )
    die( "Pipeline creation failed." );

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = vk->surface->theSurfaceFormat.format;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  // Render pass creation.
  VkRenderPassCreateInfo rpci = {};
  rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rpci.attachmentCount = 1;
  rpci.pAttachments = &colorAttachment;
  rpci.subpassCount = 1;
  rpci.pSubpasses = &subpass;
  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  rpci.dependencyCount = 1;
  rpci.pDependencies = &dependency;
  if( VK_SUCCESS != vkCreateRenderPass( vk->device, &rpci,
					NULL, &ret->renderPass ) ) 
    die( "Render pass creation failed." );

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = pssci;
  pipelineInfo.pVertexInputState = &pvici;
  pipelineInfo.pInputAssemblyState = &piasci;
  pipelineInfo.pViewportState = &pvsci;
  pipelineInfo.pRasterizationState = &prsci;
  pipelineInfo.pMultisampleState = &pmsci;
  pipelineInfo.pColorBlendState = &pcbsci;
  pipelineInfo.layout = ret->pipelineLayout;
  pipelineInfo.renderPass = ret->renderPass;
  pipelineInfo.subpass = 0;


  if( VK_SUCCESS != vkCreateGraphicsPipelines( vk->device, VK_NULL_HANDLE,
					       1, &pipelineInfo, NULL,
					       &ret->pipeline ) )
    die( "Pipeline creation failed." );

  destroyModule( vk, displayFragmentShader );
  destroyModule( vk, displayVertexShader );
  return ret;
}


void destroyPipeline( plvkState* vk, plvkPipeline* p ){
  vkDestroyPipeline( vk->device, p->pipeline, NULL );
  vkDestroyPipelineLayout( vk->device, p->pipelineLayout, NULL );
  vkDestroyRenderPass( vk->device, p->renderPass, NULL );
  memfree( p );
}


VkFramebuffer* createFramebuffers( plvkState* vk, plvkPipeline* pipe,
				   plvkSwapchain* swap ){
  newa( ret, VkFramebuffer, vk->swap->numImages );
  for( u32 i = 0; i < swap->numImages; ++i ){
    VkImageView attachments[ 1 ] = { swap->imageViews[ i ] };
    VkFramebufferCreateInfo fbci = {};
    fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbci.renderPass = pipe->renderPass;
    fbci.attachmentCount = 1;
    fbci.pAttachments = attachments;
    fbci.width = vk->extent.width;
    fbci.height = vk->extent.height;
    fbci.layers = 1;
    if( VK_SUCCESS != vkCreateFramebuffer( vk->device, &fbci,
					   NULL, &ret[ i ] ) )
      die( "Framebuffer creation failed." );
  }

  return ret;
}


void destroyFramebuffers( plvkState* vk, VkFramebuffer* fbs ){
  for( u32 i = 0; i < vk->swap->numImages; ++i )
    vkDestroyFramebuffer( vk->device, fbs[ i ], NULL );
  memfree( fbs );
}


VkCommandBuffer* createCommandBuffers( plvkState* vk ){
  newa( ret, VkCommandBuffer, vk->swap->numImages );

  VkCommandBufferAllocateInfo cbai = {};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.commandPool = vk->pool;
  cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cbai.commandBufferCount = vk->swap->numImages;
  if( VK_SUCCESS != vkAllocateCommandBuffers( vk->device, &cbai,
					      ret ) )
    die( "Command buffer creation failed." );
    
  // Command buffers and render passes.
  for( u32 i = 0; i < vk->swap->numImages; i++ ){
    VkCommandBufferBeginInfo cbbi = {};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if( VK_SUCCESS != vkBeginCommandBuffer( ret[ i ], &cbbi ) )
      die( "Command buffer begining failed." );

    VkRenderPassBeginInfo rpbi = {};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = vk->pipe->renderPass;
    rpbi.framebuffer = vk->framebuffers[ i ];
    rpbi.renderArea.extent = vk->extent;

    VkClearValue cc = {};
    cc.color.float32[ 3 ] = 1.0f;
    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &cc;

    vkCmdBeginRenderPass( ret[ i ], &rpbi,
			  VK_SUBPASS_CONTENTS_INLINE );
    vkCmdBindPipeline( ret[ i ],
		       VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipe->pipeline );
    vkCmdBindDescriptorSets( ret[ i ],
			     VK_PIPELINE_BIND_POINT_GRAPHICS,
			     vk->pipe->pipelineLayout, 0, 1,
			     &vk->descriptorSets[ i ], 0, NULL );
    vkCmdDraw( ret[ i ], 3, 1, 0, 0 );
    vkCmdEndRenderPass( ret[ i ] );
    if( VK_SUCCESS != vkEndCommandBuffer( ret[ i ] ) )
      die( "Command buffer recording failed." );
  }
  return ret;
}


void destroyCommandBuffers( plvkState* vk, VkCommandBuffer* cbs ){
  vkFreeCommandBuffers( vk->device, vk->pool, vk->swap->numImages,
			cbs );
  memfree( cbs );
}


void createPoolAndFences( plvkState* vk ){
  // Command pool.
  VkCommandPoolCreateInfo cpci = {};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.queueFamilyIndex = 0;
  if( VK_SUCCESS != vkCreateCommandPool( vk->device, &cpci, NULL,
					 &vk->pool ) )
    die( "Command pool creation failed." );
  // Semaphores and fences.
  vk->imageAvailables = newae( VkSemaphore, vk->swap->numImages );
  vk->renderCompletes = newae( VkSemaphore, vk->swap->numImages );
  vk->fences = newae( VkFence, vk->swap->numImages );
  vk->fenceSyncs = newae( VkFence, vk->swap->numImages );
  for( u32 i = 0; i < vk->swap->numImages; ++i )
    vk->fenceSyncs[ i ] = VK_NULL_HANDLE;
  VkSemaphoreCreateInfo sci = {};
  sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fci = {};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  for( u32 i = 0; i < vk->swap->numImages; ++i ){
    if( VK_SUCCESS != vkCreateSemaphore( vk->device, &sci, NULL,
					 &vk->imageAvailables[ i ] ) ||
	VK_SUCCESS != vkCreateSemaphore( vk->device, &sci, NULL,
					 &vk->renderCompletes[ i ] ) )
      die( "Semaphore creation failed." );
    if( VK_SUCCESS != vkCreateFence( vk->device, &fci, NULL,
				     &vk->fences[ i ] ) )
      die( "Fence creation failed." );
  }
}


void destroyPoolAndFences( plvkState* vk, u32 numImages ){
  for( u32 i = 0; i < numImages; ++i ){
    vkDestroySemaphore( vk->device, vk->imageAvailables[ i ], NULL );
    vkDestroySemaphore( vk->device, vk->renderCompletes[ i ], NULL );
    vkDestroyFence( vk->device, vk->fences[ i ], NULL );
  }
  vkDestroyCommandPool( vk->device, vk->pool, NULL );
  memfree( vk->imageAvailables );
  memfree( vk->renderCompletes );
  memfree( vk->fences );
  memfree( vk->fenceSyncs );
}


plvkSurface* createSurface( plvkState* vk ){
  new( ret, plvkSurface ); 
  VkWin32SurfaceCreateInfoKHR srfci = {};
  srfci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  srfci.hwnd = vk->gui->handle;
  srfci.hinstance = GetModuleHandle( NULL ); 
  if( VK_SUCCESS != vkCreateWin32SurfaceKHR( vk->instance, &srfci, NULL,
					     &ret->surface ) )
    die( "Win32 surface creation failed." );
  VkBool32 supported = 0;
  vkGetPhysicalDeviceSurfaceSupportKHR( vk->gpu, 0, ret->surface, &supported );
  if( VK_FALSE == supported )
    die( "Surface not supported." );

  // Get device surface capabilities.
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vk->gpu, ret->surface,
					     &ret->surfaceCapabilities );
  vkGetPhysicalDeviceSurfaceFormatsKHR( vk->gpu,
					ret->surface,
					&ret->numSurfaceFormats, NULL );
  ret->surfaceFormats = newae( VkSurfaceFormatKHR, ret->numSurfaceFormats );
  vkGetPhysicalDeviceSurfaceFormatsKHR( vk->gpu, ret->surface,
					&ret->numSurfaceFormats,
					ret->surfaceFormats );
  vkGetPhysicalDeviceSurfacePresentModesKHR( vk->gpu, ret->surface,
					     &ret->numSurfacePresentations,
					     NULL );
  ret->surfacePresentations = newae( VkPresentModeKHR,
				     ret->numSurfacePresentations );
  vkGetPhysicalDeviceSurfacePresentModesKHR( vk->gpu, ret->surface,
					     &ret->numSurfacePresentations,
					     ret->surfacePresentations );
  if( ret->surfaceCapabilities.maxImageCount < 2 &&
      ret->surfaceCapabilities.minImageCount < 2 )
    die( "Double buffering not supported." );
  if( state.frameCount > ret->surfaceCapabilities.maxImageCount )
    die( "The requested number of frames is not supported." );
  ret->theSurfaceFormat = ret->surfaceFormats[ 0 ];
  return ret;
}


void destroySurface( plvkState* vk, plvkSurface* surf ){
  vkDestroySurfaceKHR( vk->instance, surf->surface, NULL );
  memfree( surf->surfaceFormats );
  memfree( surf->surfacePresentations );
  memfree( surf );
}


void createImage( plvkState* vk, plvkTexture* tex );
void copyBufferToImage( plvkState* vk, plvkBuffer* buf, plvkTexture* tex );
void transitionImageLayout( plvkState* vk, plvkTexture* tex,
			    VkImageLayout oldLayout, VkImageLayout newLayout );
plvkTexture* createTextureImage( plvkState* vk, u32 width, u32 height,
				 u8 channels, VkFormat format ){
  new( ret, plvkTexture );
  u8 pixels[] = {
    0, 0, 0, 255,
    255, 0, 0, 255,
    255, 255, 0, 255,
    0, 0, 255, 255,
    255, 0, 255, 255,
    123, 255, 255, 255
  };
  ret->width = width;
  ret->height = height;
  ret->channels = channels;
  ret->format = format;
  ret->size = ret->width * ret->height * ret->channels;
  ret->tiling = VK_IMAGE_TILING_OPTIMAL;
  ret->usage =  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  plvkBuffer* buf = createBuffer( vk, ret->size,
				  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

  void* data;
  vkMapMemory( vk->device, buf->memory, 0, buf->size, 0, &data );
  memcpy( data, pixels, buf->size );
  vkUnmapMemory( vk->device, buf->memory );
  createImage( vk, ret );
  /* texWidth, texHeight,, */
  /* VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | */
  /* VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, */
  /* textureImage, textureImageMemory); */
  transitionImageLayout( vk, ret,
			 VK_IMAGE_LAYOUT_UNDEFINED,
			 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
  copyBufferToImage( vk, buf, ret );
  transitionImageLayout( vk, ret, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
  destroyBuffer( vk, buf );
  mark;
  return ret;
}


VkCommandBuffer beginSingleTimeCommands( plvkState* vk ) {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = vk->pool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers( vk->device, &allocInfo, &commandBuffer );

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer( commandBuffer, &beginInfo );

  return commandBuffer;
}


void endSingleTimeCommands( plvkState* vk, VkCommandBuffer commandBuffer ) {
  vkEndCommandBuffer( commandBuffer );

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit( vk->queue, 1, &submitInfo, VK_NULL_HANDLE );
  vkQueueWaitIdle( vk->queue );

  vkFreeCommandBuffers( vk->device, vk->pool, 1, &commandBuffer );
}


void createImage( plvkState* vk, plvkTexture* tex ){
  /* void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) { */
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = tex->width;
  imageInfo.extent.height = tex->height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = tex->format;
  imageInfo.tiling = tex->tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = tex->usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if( VK_SUCCESS != vkCreateImage( vk->device, &imageInfo, NULL,
				   &tex->image ) )
    die( "Failed to create vulkan image." );
  VkMemoryRequirements mreqs;
  vkGetImageMemoryRequirements( vk->device, tex->image, &mreqs );
  VkMemoryAllocateInfo mai = {};
  mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  mai.allocationSize = mreqs.size;
  mai.memoryTypeIndex = findMemoryType( vk, mreqs.memoryTypeBits,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
  if( VK_SUCCESS !=
      vkAllocateMemory( vk->device, &mai, NULL, &tex->imageMem  ) )
    die( "Failed to allocate memory for texture." );
  vkBindImageMemory( vk->device, tex->image, tex->imageMem, 0 );
}
void transitionImageLayout( plvkState* vk, plvkTexture* tex,
			    VkImageLayout oldLayout, VkImageLayout newLayout ){
  VkCommandBuffer commandBuffer = beginSingleTimeCommands( vk );
  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = tex->image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  VkPipelineStageFlags sourceStage = 0;
  VkPipelineStageFlags destinationStage = 0;
  if( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ){
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
	      newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ){
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else
    die( "Layout transition failed." );
  vkCmdPipelineBarrier( commandBuffer, sourceStage, destinationStage, 0,0, NULL,
			0, NULL, 1, &barrier );
  endSingleTimeCommands( vk, commandBuffer );
}
void copyBufferToImage( plvkState* vk, plvkBuffer* buf, plvkTexture* tex ){
  VkCommandBuffer commandBuffer = beginSingleTimeCommands( vk );
  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageExtent.width = tex->width;
  region.imageExtent.height = tex->height,
  region.imageExtent.depth = 1;
  vkCmdCopyBufferToImage( commandBuffer, buf->buffer, tex->image,
			  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );
  mark;
  endSingleTimeCommands( vk, commandBuffer );
}
    

void createTextures( plvkState* vk ){
  vk->tex = createTextureImage( vk, 3, 2, 4, VK_FORMAT_R8G8B8A8_SRGB ); 
}


void destroyTexture( plvkState* vk, plvkTexture* tex ){
  vkDestroyImage( vk->device, tex->image, NULL );
  mark;
  vkFreeMemory( vk->device, tex->imageMem, NULL );
  memfree( tex );
}


void destroyTextures( plvkState* vk ){
  destroyTexture( vk, vk->tex );
}


