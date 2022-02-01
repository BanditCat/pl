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

typedef struct {
  VkInstance instance;
  u32 numGpus;
  VkPhysicalDevice* gpus;
  VkPhysicalDeviceProperties* gpuProperties;
} plvkState;

plvkStatep plvkInit( void ){
  new( vk, plvkState );
  plvkStatep ret = vk;
#ifdef DEBUG
  printl( "Initializing vulkan..." );
#endif
  VkApplicationInfo prog;
  prog.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  prog.pApplicationName = TARGET;
  prog.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  prog.pEngineName = TARGET;
  prog.engineVersion = VK_MAKE_VERSION(0, 0, 1);
  prog.apiVersion = VK_API_VERSION_1_0;
  VkInstanceCreateInfo create;
  create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create.pNext = NULL;
  create.flags = 0;
  create.pApplicationInfo = &prog;
  create.enabledLayerCount = 0;
  const char* layerNames[ 1 ] = { "" };
  create.ppEnabledLayerNames = layerNames;
  create.enabledExtensionCount = 0;
  const char* extNames[ 1 ] = { "" };
  create.ppEnabledExtensionNames = extNames;
  if( VK_SUCCESS != vkCreateInstance( &create, NULL, &vk->instance ) )
    die( "Failed to create vulkan instance." );
  if( VK_SUCCESS != vkEnumeratePhysicalDevices( vk->instance, &vk->numGpus, NULL ) )
    die( "Failed to count gpus." );
  vk->gpus = newae( VkPhysicalDevice, vk->numGpus );
  if( VK_SUCCESS != vkEnumeratePhysicalDevices( vk->instance, &vk->numGpus, vk->gpus ) )
    die( "Failed to enumerate gpus." );
  vk->gpuProperties = newae( VkPhysicalDeviceProperties, vk->numGpus );
#ifdef DEBUG
  printl( "GPUs:" );
#endif
  for( u32 i = 0; i < vk->numGpus; ++i ){
    vkGetPhysicalDeviceProperties( vk->gpus[ i ], vk->gpuProperties + i );
#ifdef DEBUG
    printInt( i ); print( ": " ); printl( vk->gpuProperties[ i ].deviceName );
#endif
      }
  return ret;
}

void plvkEnd( plvkStatep vkp ){
  plvkState* vk = vkp;
  if( vk->gpus )
    memfree( vk->gpus );
  if( vk->gpuProperties )
    memfree( vk->gpuProperties );
  vkDestroyInstance( vk->instance, NULL );
  memfree( vk );
}
