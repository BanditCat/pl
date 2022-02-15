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
  vkDeviceWaitIdle( vk->device );

  vk->swap = createSwap( vk, 1, state.frameCount );
  if( vk->swap ){
    if( !vk->pool )
      createPoolAndFences( vk );
    u32 fsize, vsize;
    const char* frag = loadBuiltin( "frag", &fsize );
    const char* vert = loadBuiltin( "vert", &vsize );
    vk->pipe = createPipeline( vk, frag, fsize, vert, vsize );
    vk->framebuffers = createFramebuffers( vk, vk->pipe, vk->swap );

    createUBOs( vk );
    createDescriptorPool( vk );
    createDescriptorSets( vk );
    vk->commandBuffers = createCommandBuffers( vk );

  }
}

void unbuild( plvkStatep vkp ){
  plvkState* vk = vkp;
  if( vk->swap ){
    vkDeviceWaitIdle( vk->device );
    destroyFramebuffers( vk, vk->framebuffers );
    destroyDescriptorPool( vk );
    destroyPipeline( vk, vk->pipe );
    destroyCommandBuffers( vk, vk->commandBuffers );
    destroyUBOs( vk );
    destroySwap( vk, vk->swap );
  }
}
void plvkEnd( plvkStatep vkp ){
  plvkState* vk = vkp;
  vkDeviceWaitIdle( vk->device );
  {
    u32 ni = vk->swap->numImages;
    unbuild( vkp );
    destroyPoolAndFences( vk, ni );
  }
  destroyUBOLayout( vk, vk->bufferLayout );
  if( vk->surface )
    vkDestroySurfaceKHR( vk->instance, vk->surface, NULL );
#ifdef DEBUG
  vk->vkDestroyDebugUtilsMessengerEXT( vk->instance, vk->vkdbg, NULL );
#endif
  destroyDescriptorSets( vk );
  destroyDevice( vk );
  if( vk->surfaceFormats )
    memfree( vk->surfaceFormats );
  if( vk->surfacePresentations )
    memfree( vk->surfacePresentations );
  if( vk->gui )
    wend( vk->gui );
  memfree( vk );
}


plvkStatep plvkInit( s32 whichGPU, u32 debugLevel, char* title, int x, int y,
		     int width, int height ){
  plvkState* vk = createDevice( whichGPU, debugLevel, title, x, y,
				width, height );
  

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

  vk->bufferLayout = createUBOLayout( vk );
  rebuild( vk );
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
