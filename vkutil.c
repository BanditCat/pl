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
plvkState* createInstance( void ){
  new( vk, plvkState );
  state.vk = vk;


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
  return vk;
}
