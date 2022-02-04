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

#include "inc\vulkan\vulkan.h"
#include "pl.h"
#include "os.h"
#include "vk.h"
#include "util.h"

// Requirements
#ifdef DEBUG
const char* requiredExtensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
#else
const char* requiredExtensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
#endif
#ifdef DEBUG
const char* requiredLayers[] = { "VK_LAYER_KHRONOS_validation" };
#else
const char* requiredLayers[] = {};
#endif
const u32 numRequiredExtensions = sizeof( requiredExtensions ) / sizeof( char* );
const u32 numRequiredLayers = sizeof( requiredLayers ) / sizeof( char* );


// Instance wide state.
typedef struct {
  VkInstance instance;
  u32 numGpus;
  VkPhysicalDevice* gpus;
  VkPhysicalDeviceProperties* gpuProperties;
  u32 numExtensions;
  VkExtensionProperties* extensions;
  u32 numLayers;
  VkLayerProperties* layers;

  // Function pointers.
#define FPDEFINE( x ) PFN_##x x
#ifdef DEBUG
  FPDEFINE( vkCreateDebugUtilsMessengerEXT );
  FPDEFINE( vkDestroyDebugUtilsMessengerEXT );
  VkDebugUtilsMessengerEXT vkdbg;
#endif
} plvkState;


// Function pointer helper function.
#define FPGET( x )   vk->x = (PFN_##x) vkGetInstanceProcAddr( vk->instance, #x );
void getFuncPointers( plvkState* vk ){
  (void)vk;
#ifdef DEBUG   
  FPGET( vkCreateDebugUtilsMessengerEXT );
  FPGET( vkDestroyDebugUtilsMessengerEXT );
#endif
}

// Validation layer callback.
#ifdef DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL plvkDebugcb(
					   VkDebugUtilsMessageSeverityFlagBitsEXT severity,
					   VkDebugUtilsMessageTypeFlagsEXT type,
					   const VkDebugUtilsMessengerCallbackDataEXT* callback,
					   void* user ) {
  (void)user;
  print( "validation " );
  printInt(type );
  print( ", " );
  printInt( severity );
  print( ": " );
  printl( callback->pMessage );

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
    //VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  ci->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
  ci->pfnUserCallback = plvkDebugcb;
  ci->pUserData = NULL;
}
#endif

plvkStatep plvkInit( void ){
  new( vk, plvkState );
#ifdef DEBUG
  printl( "Initializing vulkan..." );
#endif

  // Get extensions.
  vkEnumerateInstanceExtensionProperties( NULL, &vk->numExtensions, NULL );
  vk->extensions = newae( VkExtensionProperties, vk->numExtensions );
  vkEnumerateInstanceExtensionProperties( NULL, &vk->numExtensions, vk->extensions );
  for( u32 i = 0; i < numRequiredExtensions; ++i ){
    bool has = 0;
    for( u32 j = 0; j < vk->numExtensions; j++ ){
      if( !strcomp( requiredExtensions[ i ], vk->extensions[ j ].extensionName ) )
	has = 1;
    }
    if( !has )
      die( "Required extension not found!" );
  }
#ifdef DEBUG
  printl( "Extensions:" );
  for( u32 i = 0; i < vk->numExtensions; ++i ){
    printInt( i ); print( ": " ); printl( vk->extensions[ i ].extensionName );
  }
#endif

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
#ifdef DEBUG
  printl( "Layers:" );
  for( u32 i = 0; i < vk->numLayers; ++i ){
    print( vk->layers[ i ].layerName ); print( ": " ); printl( vk->layers[ i ].description );
  }
#endif  

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
#ifdef DEBUG
  {
    VkDebugUtilsMessengerCreateInfoEXT ci;
    setupDebugCreateinfo( &ci );
    create.pNext = &ci;
  }
#else
  create.pNext = NULL;
#endif
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

  // Enumerate GPUs.
  if( VK_SUCCESS != vkEnumeratePhysicalDevices( vk->instance, &vk->numGpus, NULL ) )
    die( "Failed to count gpus." );
  if( !vk->numGpus )
    die( "No GPUs found :(" );
  vk->gpus = newae( VkPhysicalDevice, vk->numGpus );
  if( VK_SUCCESS != vkEnumeratePhysicalDevices( vk->instance, &vk->numGpus, vk->gpus ) )
    die( "Failed to enumerate gpus." );
  vk->gpuProperties = newae( VkPhysicalDeviceProperties, vk->numGpus );
  for( u32 i = 0; i < vk->numGpus; ++i )
    vkGetPhysicalDeviceProperties( vk->gpus[ i ], vk->gpuProperties + i );
#ifdef DEBUG
  printl( "GPUs:" );
  for( u32 i = 0; i < vk->numGpus; ++i ){
    printInt( i ); print( ": " ); printl( vk->gpuProperties[ i ].deviceName );
  }
#endif


  return vk;
}

void plvkEnd( plvkStatep vkp ){
  plvkState* vk = vkp;
#ifdef DEBUG
  vk->vkDestroyDebugUtilsMessengerEXT( vk->instance, vk->vkdbg, NULL );
#endif

  if( vk->gpus )
    memfree( vk->gpus );
  if( vk->gpuProperties )
    memfree( vk->gpuProperties );
  if( vk->extensions )
    memfree( vk->extensions );
  if( vk->layers )
    memfree( vk->layers );
  vkDestroyInstance( vk->instance, NULL );
  memfree( vk );
}

