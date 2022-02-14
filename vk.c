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
#include "vkutil.h"
#include "util.h"
#include "os.h"
#include "vk.h"



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

void rebuild( plvkState* vk ){
  m;
  vkDeviceWaitIdle( vk->device );

  vk->swap = createSwap( vk, 1, state.frameCount );
  if( vk->swap ){
    m;
    u32 fsize, vsize;
    const char* frag = loadBuiltin( "frag", &fsize );
    const char* vert = loadBuiltin( "vert", &vsize );
    vk->pipe = createPipeline( vk, frag, fsize, vert, vsize );
    m;
    // Framebuffers
    if( !vk->framebuffers )
      vk->framebuffers = newae( VkFramebuffer, vk->swap->numImages );
    m;
    for( u32 i = 0; i < vk->swap->numImages; ++i ){
      VkImageView attachments[ 1 ] = { vk->swap->imageViews[ i ] };
      VkFramebufferCreateInfo fbci = {};
      fbci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      fbci.renderPass = vk->pipe->renderPass;
      fbci.attachmentCount = 1;
      fbci.pAttachments = attachments;
      fbci.width = vk->extent.width;
      fbci.height = vk->extent.height;
      fbci.layers = 1;
      if( VK_SUCCESS != vkCreateFramebuffer( vk->device, &fbci,
					     NULL, &vk->framebuffers[ i ] ) )
	die( "Framebuffer creation failed." );
    }

    if( !vk->commandBuffers )
      vk->commandBuffers = newae( VkCommandBuffer, vk->swap->numImages );

    VkCommandBufferAllocateInfo cbai = {};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = vk->pool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = vk->swap->numImages;
    if( VK_SUCCESS != vkAllocateCommandBuffers( vk->device, &cbai,
						vk->commandBuffers ) )
      die( "Command buffer creation failed." );
    m;
    createUBOs( vk );
    createDescriptorPool( vk );
    createDescriptorSets( vk );
    
    // Command buffers and render passes.
    for( u32 i = 0; i < vk->swap->numImages; i++ ){
      VkCommandBufferBeginInfo cbbi = {};
      cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      if( VK_SUCCESS != vkBeginCommandBuffer( vk->commandBuffers[ i ], &cbbi ) )
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

      vkCmdBeginRenderPass( vk->commandBuffers[ i ], &rpbi,
			    VK_SUBPASS_CONTENTS_INLINE );
      vkCmdBindPipeline( vk->commandBuffers[ i ],
			 VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipe->pipeline );
      vkCmdBindDescriptorSets( vk->commandBuffers[ i ],
			       VK_PIPELINE_BIND_POINT_GRAPHICS,
			       vk->pipe->pipelineLayout, 0, 1,
			       &vk->descriptorSets[ i ], 0, NULL );
      vkCmdDraw( vk->commandBuffers[ i ], 3, 1, 0, 0 );
      vkCmdEndRenderPass( vk->commandBuffers[ i ] );
      if( VK_SUCCESS != vkEndCommandBuffer( vk->commandBuffers[ i ] ) )
	die( "Command buffer recording failed." );
    }

  }
}

void unbuild( plvkStatep vkp ){
  m;
  plvkState* vk = vkp;
  if( vk->swap ){
    m;
    vkDeviceWaitIdle( vk->device );
    m;
    for( u32 i = 0; i < vk->swap->numImages; ++i ){
      vkDestroyFramebuffer( vk->device, vk->framebuffers[ i ], NULL );
    }
    m;
    destroyDescriptorPool( vk );
    vkFreeCommandBuffers( vk->device, vk->pool, vk->swap->numImages,
			  vk->commandBuffers );
    destroyPipeline( vk, vk->pipe );
    m;
    destroyUBOs( vk );
    destroySwap( vk, vk->swap );
    m;
  }
}
void plvkEnd( plvkStatep vkp ){
  m;
  plvkState* vk = vkp;
  vkDeviceWaitIdle( vk->device );
  m;
  for( u32 i = 0; i < vk->swap->numImages; ++i ){
    vkDestroySemaphore( vk->device, vk->imageAvailables[ i ], NULL );
    vkDestroySemaphore( vk->device, vk->renderCompletes[ i ], NULL );
    vkDestroyFence( vk->device, vk->fences[ i ], NULL );
  }
  m;
  unbuild( vkp );
  if( vk->bufferLayout )
    destroyUBOLayout( vk, vk->bufferLayout );
  m;
  if( vk->pool )
    vkDestroyCommandPool( vk->device, vk->pool, NULL );
  if( vk->UBOs )
    memfree( vk->UBOs );
  if( vk->imageAvailables )
    memfree( vk->imageAvailables );
  if( vk->renderCompletes )
    memfree( vk->renderCompletes );
  if( vk->fences )
    memfree( vk->fences );
  if( vk->fenceSyncs )
    memfree( vk->fenceSyncs );
  if( vk->surface )
    vkDestroySurfaceKHR( vk->instance, vk->surface, NULL );
  m;
#ifdef DEBUG
  vk->vkDestroyDebugUtilsMessengerEXT( vk->instance, vk->vkdbg, NULL );
#endif
  m;
  // Free memory.
  if( vk->descriptorSets )
    memfree( vk->descriptorSets );
  if( vk->commandBuffers )
    memfree( vk->commandBuffers );
  m;
  if( vk->framebuffers )
    memfree( vk->framebuffers );
  destroyDevice( vk );
  if( vk->surfaceFormats )
    memfree( vk->surfaceFormats );
  if( vk->surfacePresentations )
    memfree( vk->surfacePresentations );
  m;
  if( vk->gui )
    wend( vk->gui );
  memfree( vk );
  m;
}


plvkStatep plvkInit( s32 whichGPU, u32 debugLevel, char* title, int x, int y,
		     int width, int height ){
  m;
  // Set up validation layer callback.
  plvkState* vk = createDevice( whichGPU, debugLevel, title, x, y,
				width, height );
  m;



  // Create surface
  VkWin32SurfaceCreateInfoKHR srfci = {};
  srfci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  srfci.hwnd = vk->gui->handle;
  srfci.hinstance = GetModuleHandle( NULL ); 
  if( VK_SUCCESS != vkCreateWin32SurfaceKHR( vk->instance, &srfci, NULL,
					     &vk->surface ) )
    die( "Win32 surface creation failed." );
  VkBool32 supported = 0;
  vkGetPhysicalDeviceSurfaceSupportKHR( vk->gpu, 0, vk->surface, &supported );
  if( VK_FALSE == supported )
    die( "Surface not supported." );

  m;
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
  if( vk->surfaceCapabilities.maxImageCount < 2 &&
      vk->surfaceCapabilities.minImageCount < 2 )
    die( "Double buffering not supported." );
  if( state.frameCount > vk->surfaceCapabilities.maxImageCount )
    die( "The requested number of frames is not supported." );
  vk->theSurfaceFormat = vk->surfaceFormats[ 0 ];

  // Command pool.
  m;
  VkCommandPoolCreateInfo cpci = {};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.queueFamilyIndex = 0;
  if( VK_SUCCESS != vkCreateCommandPool( vk->device, &cpci, NULL,
					 &vk->pool ) )
    die( "Command pool creation failed." );


  vk->bufferLayout = createUBOLayout( vk );
  m;
  rebuild( vk );
  m;
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

  m;
  return vk;
}
void updateGPUstate( plvkState* vk, f32 time ){
  vk->UBOcpumem.time = time;
  void* data;
  vkMapMemory( vk->device, vk->UBOs[ vk->currentImage ].memory, 0,
	       sizeof( gpuState ), 0, &data );
  memcpy( data, &vk->UBOcpumem, sizeof( gpuState ) );
  vkUnmapMemory( vk->device, vk->UBOs[ vk->currentImage ].memory );
}
void draw( void ){
  static u64 firstDrawTime = 0;
  if( !firstDrawTime )
    firstDrawTime = tickCount();
  plvkState* vk = state.vk;
  bool recreate = 0;
  if( vk->swap ){
    static u64 lasttime = 0;
    static u64 frameCount = 0;

    vkWaitForFences( vk->device, 1, &vk->fences[ vk->currentImage ], VK_TRUE,
		     UINT64_MAX );

    uint32_t index = 0;
    if( VK_ERROR_OUT_OF_DATE_KHR ==
	vkAcquireNextImageKHR( vk->device, vk->swap->swap, 1000000000,
			       vk->imageAvailables[ vk->currentImage ],
			       VK_NULL_HANDLE, &index ) )
      recreate = 1;
    else{
      if( vk->fenceSyncs[ index ] != VK_NULL_HANDLE )
	vkWaitForFences( vk->device, 1, &vk->fenceSyncs[ index ], VK_TRUE,
			 UINT64_MAX );
      vk->fenceSyncs[ index ] = vk->fences[ vk->currentImage ];

      updateGPUstate( vk, (f32)( tickCount() - firstDrawTime )
		      / (f32)tickFrequency() );
      
      VkSubmitInfo submitInfo = {};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

      VkSemaphore semaphores[] = { vk->imageAvailables[ vk->currentImage ] };
      VkPipelineStageFlags stages[] =
	{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
      submitInfo.waitSemaphoreCount = 1;
      submitInfo.pWaitSemaphores = semaphores;
      submitInfo.pWaitDstStageMask = stages;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &vk->commandBuffers[ index ];
      VkSemaphore finishedSemaphores[] = { vk->renderCompletes[ vk->currentImage ] };
      submitInfo.signalSemaphoreCount = 1;
      submitInfo.pSignalSemaphores = finishedSemaphores;
      vkResetFences( vk->device, 1, &vk->fences[ vk->currentImage ] );
      if( VK_SUCCESS != vkQueueSubmit( vk->queue, 1, &submitInfo,
				       vk->fences[ vk->currentImage ] ) )
	die( "Queue submition failed." );

      VkPresentInfoKHR presentation = {};
      presentation.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

      presentation.waitSemaphoreCount = 1;
      presentation.pWaitSemaphores = finishedSemaphores;
      VkSwapchainKHR swaps[] = { vk->swap->swap };
      presentation.swapchainCount = 1;
      presentation.pSwapchains = swaps;
      presentation.pImageIndices = &index;
      if( VK_ERROR_OUT_OF_DATE_KHR ==
	  vkQueuePresentKHR( vk->queue, &presentation ) )
	recreate = 1;
      ++vk->currentImage;
      vk->currentImage %= vk->swap->numImages;
    }
    if( state.fps ){
      if( 0 == lasttime  )
	lasttime = tickCount();
      else{
	u64 now = tickCount();
	f64 elapsed = (f64)( now - lasttime ) / tickFrequency();
	if( elapsed > 1.0 ){
	  print( "FPS: " );
	  printFloat( (f64)frameCount / elapsed );
	  endl();
	  lasttime = now;
	  frameCount = 0;
	}
      }
      ++frameCount;
    }
  }else
    recreate = 1;
  if( recreate ){
    unbuild( vk );
    rebuild( vk );
  }
}
bool plvkeventLoop( plvkStatep p ){
  plvkState* vk = p;
  return weventLoop( vk->gui );
}

void plvkShow( plvkStatep p ){
  plvkState* vk = p;
  guiShow( vk->gui );
}
void plvkHide( plvkStatep p ){
  plvkState* vk = p;
  guiHide( vk->gui );
}
