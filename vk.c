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

#include "vkutil.h"
#include "util.h"
#include "os.h"
#include "vk.h"


unsigned long renderThread( void* vkp ){
  plvkInstance* vk = (plvkInstance*)vkp;
  while( vk->valid ){
    waitSemaphore( vk->rendering );
    draw();
    releaseSemaphore( vk->rendering );
  }
  return 0;
}


void plvkPrintInitInfo( void ){
  plvkInstance* vk = state.vk;
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
}

void plvkPrintGPUs( void ){
  plvkInstance* vk = state.vk;
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

void plvkEnd( plvkInstance* vk ){
  plvkStopRendering( vk );

  destroyAttachables( vk );
  // Destroy units.
  {
    plvkUnit* p = vk->unit;
    while( p ){
      plvkUnit* q = p->next;
      destroyUnit( p );
      p = q;
    }
  }
    
#ifdef DEBUG
  vk->vkDestroyDebugUtilsMessengerEXT( vk->instance, vk->vkdbg, NULL );
#endif
  destroyUBOs( vk );
  destroyPool( vk );
  destroyInstance( vk );
  memfree( vk );
}


plvkInstance* plvkInit( s32 whichGPU, u32 debugLevel ){
  plvkInstance* vk = createInstance( whichGPU, debugLevel );
  createPool( vk );
  createUBOs( vk ); 

  return vk;
}
void updateGPUstate( plvkInstance* vk, f32 time ){
  vk->UBOcpumem.time = time;
  void* data;
  vkMapMemory( vk->device, vk->UBOs[ vk->currentImage ]->memory, 0,
	       sizeof( gpuState ), 0, &data );
  memcpy( data, &vk->UBOcpumem, sizeof( gpuState ) );
  vkUnmapMemory( vk->device, vk->UBOs[ vk->currentImage ]->memory );
}
void draw( void ){
  static u64 firstDrawTime = 0;
  if( !firstDrawTime )
    firstDrawTime = tickCount();
  plvkInstance* vk = state.vk;
  {
    plvkUnit* t = vk->unit;
    while( t ){
      tickUnit( t );
      t = t->next;
    }
  }
  static u64 lasttime = 0;
  static u64 frameCount = 0;


  updateGPUstate( vk, 0.1 * (f32)( tickCount() - firstDrawTime )
		  / (f32)tickFrequency() );
      
  ++vk->currentImage;
  vk->currentImage %= 2;
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
}
bool plvkeventLoop( plvkInstance* vk ){
  bool quit = 0;
  plvkUnit* t = vk->unit;
  while( t ){
    if( t->display ){
      if( !weventLoop( t->display->gui ) )
	quit = 1;
    }
    t = t->next;
  }
  return !quit;
}

 
void plvkShow( plvkUnit* u ){
  if( u->display )
    guiShow( u->display->gui );
}
void plvkHide( plvkUnit* u ){
  if( u->display )
    guiHide( u->display->gui );
}


plvkUnit* plvkCreateUnit( plvkInstance* vk, u32 width, u32 height,
			  VkFormat format, u8 fragmentSize,
			  const char* fragName, const char* vertName,
			  bool displayed, const char* title, int x, int y,
			  plvkAttachable** attachments, u64 numAttachments,
			  u64 drawSize, const u8* pixels ){  
  plvkUnit* top = vk->unit;
  
  vk->unit = createUnit( vk, width, height, (VkFormat)format, fragmentSize,
			 fragName, vertName, displayed, title, x, y,
			 attachments, numAttachments, drawSize, pixels );
  if( !displayed ){
    new( att, plvkAttachable );
    att->type = PLVK_ATTACH_UNIT;
    att->unit = vk->unit;
    att->next = vk->attachables;
    vk->attachables = att;
  }
  vk->unit->next = top;
  return vk->unit;
}
plvkAttachable* plvkAddTexture( plvkInstance* vk, const char* name ){
  new( ret, plvkAttachable );
  ret->type = PLVK_ATTACH_TEXTURE;
  ret->texture = loadTexturePPM( vk, name );
  ret->next = vk->attachables;
  vk->attachables = ret;
  return ret;
}
void plvkStartRendering( plvkInstance* vk ){
  vk->valid = 1;
  vk->rendering = makeSemaphore();

  vk->renderThread = thread( renderThread, vk );
}
void plvkStopRendering( plvkInstance* vk ){
  vk->valid = 0;
  WaitForSingleObject( vk->renderThread, INFINITE );
  vkDeviceWaitIdle( vk->device );
}
void plvkPauseRendering( plvkInstance* vk ){
  waitSemaphore( vk->rendering );
}
void plvkResumeRendering( plvkInstance* vk ){
  releaseSemaphore( vk->rendering );
}
void plvkTickRendering( plvkInstance* vk ){
  waitSemaphore( vk->rendering );
  releaseSemaphore( vk->rendering );
}
plvkAttachable* plvkGetAttachable( plvkInstance* vk, u32 n ){
  if( !vk->attachables )
    return vk->attachables;
  plvkAttachable* ret = vk->attachables;
  u32 c = n;
  while( c-- && ret->next )
    ret = ret->next;
  return ret;
}
