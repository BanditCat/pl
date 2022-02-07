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
// Vulkan interface.                                                          //
////////////////////////////////////////////////////////////////////////////////

#define VK_USE_PLATFORM_WIN32_KHR
#include "inc\vulkan\vulkan.h"
#include "pl.h"
#include "os.h"
#include "vk.h"
#include "util.h"
#include "gui.h"

// Requirements
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
const char* requiredDeviceExtensions[] = { "VK_KHR_swapchain" };
const u32 numRequiredExtensions =
  sizeof( requiredExtensions ) / sizeof( char* );
const u32 numRequiredLayers = sizeof( requiredLayers ) / sizeof( char* );
const u32 numRequiredDeviceExtensions =
  sizeof( requiredDeviceExtensions ) / sizeof( char* );


// Instance wide state.
typedef struct {
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
  
  // Function pointers.
#define FPDEFINE( x ) PFN_##x x
#ifdef DEBUG
  FPDEFINE( vkCreateDebugUtilsMessengerEXT );
  FPDEFINE( vkDestroyDebugUtilsMessengerEXT );
  VkDebugUtilsMessengerEXT vkdbg;
#endif
} plvkState;


// Function pointer helper function.
#define FPGET( x ) vk->x = (PFN_##x) vkGetInstanceProcAddr( vk->instance, #x );
void getFuncPointers( plvkState* vk ){
  (void)vk;
#ifdef DEBUG   
  FPGET( vkCreateDebugUtilsMessengerEXT );
  FPGET( vkDestroyDebugUtilsMessengerEXT );
#endif
}

// This function creates shader modules.
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

// This function chooses the render size.
VkExtent2D getExtent( const VkSurfaceCapabilitiesKHR* caps, const guiInfo* g ) {
  if( caps->currentExtent.width != UINT32_MAX ){
    return caps->currentExtent;
  } else {
    VkExtent2D  wh;
    wh.width = g->clientWidth;
    wh.height = g->clientHeight;
    if( wh.width > caps->maxImageExtent.width )
      wh.width = caps->maxImageExtent.width;
    if( wh.width < caps->minImageExtent.width )
      wh.width = caps->minImageExtent.width;
    if( wh.height > caps->maxImageExtent.height )
      wh.height = caps->maxImageExtent.height;
    if( wh.height < caps->minImageExtent.height )
      wh.height = caps->minImageExtent.height;
    return wh;
  }
}
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
#endif

// Debug layer createinfo helper.
#ifdef DEBUG
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

// GPU scoring function
u64 scoreGPU( VkPhysicalDeviceProperties* gpu ){
  u64 score = 0;
  if( gpu->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
    score += 1000000;
  score += gpu->limits.maxImageDimension2D;
  return score;
}


#ifdef DEBUG
void plvkPrintInitInfo( void ){
  plvkState* vk = state.vk;
  printl( "\nInstance extensions:" );
  for( u32 i = 0; i < vk->numExtensions; ++i ){
    printInt( i ); print( ": " ); printl( vk->extensions[ i ].extensionName );
  }
  printl( "\nLayers:" );
  for( u32 i = 0; i < vk->numLayers; ++i ){
    print( vk->layers[ i ].layerName ); print( ": " );
    printl( vk->layers[ i ].description );
  }
  printl( "\nQueue families:" );
  for( u32 i = 0; i < vk->numQueues; ++i ){
    printInt( i );  print( ": " );
    printInt( vk->queueFamilies[ i ].queueFlags ); printl( "" );
  }
  print( "Using queue family " ); printInt( vk->queueFamily ); printl( "." );
  printl( "\nDevice extensions:" );
  for( u32 i = 0; i < vk->numDeviceExtensions; ++i ){
    printInt( i ); print( ": " );
    printl( vk->deviceExtensions[ i ].extensionName );
  }
  printl( "\nSurface formats:" );
  for( u32 i = 0; i < vk->numSurfaceFormats; ++i ){
    printInt( i ); printl( ": " );
    print( "       format: " );
    printInt( vk->surfaceFormats[ i ].format ); endl();
    print( "  color space: " );
    printInt( vk->surfaceFormats[ i ].colorSpace ); endl();
  }
  printl( "\nSurface presentations:" );
  for( u32 i = 0; i < vk->numSurfacePresentations; ++i ){
    printInt( i ); print( ": " );
    printInt( vk->surfacePresentations[ i ] ); endl();
  }

}
#endif  

void plvkPrintGPUs( void ){
  plvkState* vk = state.vk;
  printl( "GPUs:" );
  u32 best = 0;
  u32 bestScore = 0;
  for( u32 i = 0; i < vk->numGPUs; ++i ){
    u64 score = scoreGPU( vk->gpuProperties + i );
    if( score > bestScore ){
      best = i;
      bestScore = score;
    }
    print( "GPU " );
    printInt( i );
    print( ": " );
    print( vk->gpuProperties[ i ].deviceName );
    print( " (score " );
    printInt( score );
    printl( ")" );
  }
  print( "Using GPU " ); printInt( vk->gpuIndex ); print( ": " );
  print( vk->selectedGpuProperties->deviceName ); printl( " (this can be changed with the -gpu=x command line option)" );
}

void plvkEnd( plvkStatep vkp ){
  plvkState* vk = vkp;
#ifdef DEBUG
  vk->vkDestroyDebugUtilsMessengerEXT( vk->instance, vk->vkdbg, NULL );
#endif
  vkDestroyCommandPool( vk->device, vk->pool, NULL );
  vkDestroyPipeline( vk->device, vk->pipeline, NULL );
  vkDestroyRenderPass( vk->device, vk->renderPass, NULL );
  vkDestroyPipelineLayout( vk->device, vk->pipelineLayout, NULL );
  for( u32 i = 0; i < vk->numImages; ++i ){
    vkDestroyFramebuffer( vk->device, vk->framebuffers[ i ], NULL );
    vkDestroyImageView( vk->device, vk->imageViews[ i ], NULL );
  }
  vkDestroySwapchainKHR( vk->device, vk->swap, NULL );
  vkDestroySurfaceKHR( vk->instance, vk->surface, NULL );
  vkDestroyDevice( vk->device, NULL);
  vkDestroyInstance( vk->instance, NULL );
  if( vk->commandBuffers )
    memfree( vk->commandBuffers );
  if( vk->framebuffers )
    memfree( vk->framebuffers );
  if( vk->imageViews )
    memfree( vk->imageViews );
  if( vk->images )
    memfree( vk->images );
  if( vk->gpus )
    memfree( vk->gpus );
  if( vk->gpuProperties )
    memfree( vk->gpuProperties );
  if( vk->extensions )
    memfree( vk->extensions );
  if( vk->deviceExtensions )
    memfree( vk->deviceExtensions );
  if( vk->layers )
    memfree( vk->layers );
  if( vk->queueFamilies )
    memfree( vk->queueFamilies );
  if( vk->surfaceFormats )
    memfree( vk->surfaceFormats );
  if( vk->surfacePresentations )
    memfree( vk->surfacePresentations );
  memfree( vk );
}



void plvkInit( u32 whichGPU, void* vgui, u32 debugLevel ){
  guiInfo* gui = (guiInfo*)vgui;
  new( vk, plvkState );
  state.vk = vk;
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
  // Set up validation layer callback.
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
  if( whichGPU != (u32)-1 ){
    if( whichGPU >= vk->numGPUs )
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


  // Create surface
  VkWin32SurfaceCreateInfoKHR sci = {};
  sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  sci.hwnd = gui->handle;
  sci.hinstance = GetModuleHandle( NULL ); 
  if( VK_SUCCESS != vkCreateWin32SurfaceKHR( vk->instance, &sci, NULL,
					     &vk->surface ) )
    die( "Win32 surface creation failed." );
  VkBool32 supported = 0;
  vkGetPhysicalDeviceSurfaceSupportKHR( vk->gpu, 0, vk->surface, &supported );
  if( VK_FALSE == supported )
    die( "Surface not supported." );


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

  // Get device surface capabilities.
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vk->gpu, vk->surface,
					     &vk->surfaceCapabilities );
  vkGetPhysicalDeviceSurfaceFormatsKHR( vk->gpu,
					vk->surface,
					&vk->numSurfaceFormats, NULL );
  vk->surfaceFormats = newae( VkSurfaceFormatKHR, vk->numSurfaceFormats );
  vkGetPhysicalDeviceSurfaceFormatsKHR( vk->gpu, vk->surface,
					&vk->numSurfaceFormats,
					vk->surfaceFormats );
  vkGetPhysicalDeviceSurfacePresentModesKHR( vk->gpu, vk->surface,
					     &vk->numSurfacePresentations,
					     NULL );
  vk->surfacePresentations = newae( VkPresentModeKHR,
				    vk->numSurfacePresentations );
  vkGetPhysicalDeviceSurfacePresentModesKHR( vk->gpu, vk->surface,
					     &vk->numSurfacePresentations,
					     vk->surfacePresentations );
  // Swapchain. Just pick the easy ones. VK_PRESENT_MODE_FIFO_KHR is
  // guarenteed to be supported by spec.
  // Require double buffering.
  if( vk->surfaceCapabilities.maxImageCount < 2 &&
      vk->surfaceCapabilities.minImageCount < 2 )
    die( "Double buffering not supported." );
  VkSurfaceFormatKHR sf = vk->surfaceFormats[ 0 ];
  VkPresentModeKHR pm = VK_PRESENT_MODE_FIFO_KHR;
  vk->extent = getExtent( &vk->surfaceCapabilities, gui );
  VkSwapchainCreateInfoKHR scci = {};
  scci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  scci.surface = vk->surface;
  scci.minImageCount = 2;
  scci.imageFormat = sf.format;
  scci.imageColorSpace = sf.colorSpace;
  scci.imageExtent = vk->extent;
  scci.imageArrayLayers = 1;
  scci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  scci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  scci.queueFamilyIndexCount = 0;
  scci.pQueueFamilyIndices = NULL;
  scci.preTransform = vk->surfaceCapabilities.currentTransform;
  scci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  scci.presentMode = pm;
  scci.clipped = VK_TRUE;
  scci.oldSwapchain = VK_NULL_HANDLE;
  if( VK_SUCCESS != vkCreateSwapchainKHR( vk->device, &scci, NULL, &vk->swap ) )
    die( "Swapchain creation failed." );
  vkGetSwapchainImagesKHR( vk->device, vk->swap, &vk->numImages, NULL );
  vk->images = newae( VkImage, vk->numImages );
  vkGetSwapchainImagesKHR( vk->device, vk->swap, &vk->numImages, vk->images );
  // Image views.
  vk->imageViews = newae( VkImageView, vk->numImages );
  for( u32 i = 0; i < vk->numImages; ++i ){
    VkImageViewCreateInfo ivci = {};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = vk->images[ i ];
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = sf.format;
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
					 &vk->imageViews[ i ] ) )
      die( "Failed to create image view." );
  }

  // Shaders
  VkShaderModule displayVertexShader;
  VkShaderModule displayFragmentShader;
  {
    u32 ssize;
    const char* shade = loadBuiltin( "frag", &ssize );
    displayFragmentShader = createModule( vk->device, shade, ssize );
    shade = loadBuiltin( "vert", &ssize );
    displayVertexShader = createModule( vk->device, shade, ssize );

  }

  // Pipeline
  {
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
    if( VK_SUCCESS !=
	vkCreatePipelineLayout( vk->device, &plci,
				NULL, &vk->pipelineLayout ) )
      die( "Pipeline creation failed." );

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = sf.format;
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
    
    VkRenderPassCreateInfo rpci = {};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 1;
    rpci.pAttachments = &colorAttachment;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;

    if( VK_SUCCESS != vkCreateRenderPass( vk->device, &rpci,
					  NULL, &vk->renderPass ) ) 
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
    pipelineInfo.layout = vk->pipelineLayout;
    pipelineInfo.renderPass = vk->renderPass;
    pipelineInfo.subpass = 0;


    if( VK_SUCCESS != vkCreateGraphicsPipelines( vk->device, VK_NULL_HANDLE,
						 1, &pipelineInfo, NULL,
						 &vk->pipeline ) )
      die( "Pipeline creation failed." );

    // Framebuffers
    vk->framebuffers = newae( VkFramebuffer, vk->numImages );
    for( u32 i = 0; i < vk->numImages; ++i ){
      VkImageView attachments[ 1 ] = { vk->imageViews[ i ] };
      VkFramebufferCreateInfo fbci = {};
      fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      fbci.renderPass = vk->renderPass;
      fbci.attachmentCount = 1;
      fbci.pAttachments = attachments;
      fbci.width = vk->extent.width;
      fbci.height = vk->extent.height;
      fbci.layers = 1;
      if( VK_SUCCESS != vkCreateFramebuffer( vk->device, &fbci,
					     NULL, &vk->framebuffers[ i ] ) )
	die( "Framebuffer creation failed." );
    }

    // Command pool.
    {
      VkCommandPoolCreateInfo cpci = {};
      cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
      cpci.queueFamilyIndex = 0;
      if( VK_SUCCESS != vkCreateCommandPool( vk->device, &cpci, NULL,
					     &vk->pool ) )
	die( "Command pool creation failed." );

      vk->commandBuffers = newae( VkCommandBuffer, vk->numImages );

      VkCommandBufferAllocateInfo cbai = {};
      cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      cbai.commandPool = vk->pool;
      cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      cbai.commandBufferCount = vk->numImages;
      if( VK_SUCCESS != vkAllocateCommandBuffers( vk->device, &cbai,
						  vk->commandBuffers ) )
	die( "Command buffer creation failed." );
    }

    // Render passes.
    for( u32 i = 0; i < vk->numImages; i++ ){
      VkCommandBufferBeginInfo cbbi = {};
      cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      if( VK_SUCCESS != vkBeginCommandBuffer( vk->commandBuffers[ i ], &cbbi ) )
	die( "Command buffer begining failed." );

      VkRenderPassBeginInfo rpbi = {};
      rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      rpbi.renderPass = vk->renderPass;
      rpbi.framebuffer = vk->framebuffers[ i ];
      rpbi.renderArea.extent = vk->extent;

      VkClearValue cc = {};
      cc.color.float32[ 3 ] = 1.0f;
      rpbi.clearValueCount = 1;
      rpbi.pClearValues = &cc;
    }
    
    // Cleanup
    vkDestroyShaderModule( vk->device, displayFragmentShader, NULL );
    vkDestroyShaderModule( vk->device, displayVertexShader, NULL );
  }
}
