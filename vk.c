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
    plvkDraw();
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
  if( vk->useTensorCores ){
    printl( "\nTensor core formats:" );
    const char* types[] = {
      "COMPONENT_TYPE_FLOAT16\n",
      "COMPONENT_TYPE_FLOAT32\n",
      "COMPONENT_TYPE_FLOAT64\n",
      "COMPONENT_TYPE_SINT8\n",
      "COMPONENT_TYPE_SINT16\n",
      "COMPONENT_TYPE_SINT32\n",
      "COMPONENT_TYPE_SINT64\n",
      "COMPONENT_TYPE_UINT8\n",
      "COMPONENT_TYPE_UINT16\n",
      "COMPONENT_TYPE_UINT32\n",
      "COMPONENT_TYPE_UINT64\n"
    };
    for( u64 i = 0; i < vk->tensorPropertyCount; i++ ){
      print( "Type #" ); printInt( i ); printl( ": {" );
      print( "  MSize: " ); printInt( vk->tensorProperties[ i ].MSize ); endl();
      print( "  NSize: " ); printInt( vk->tensorProperties[ i ].NSize ); endl();
      print( "  KSize: " ); printInt( vk->tensorProperties[ i ].KSize ); endl();
      print( "  AType: " ); print( types[ vk->tensorProperties[ i ].AType ] ); 
      print( "  BType: " ); print( types[ vk->tensorProperties[ i ].BType ] ); 
      print( "  CType: " ); print( types[ vk->tensorProperties[ i ].CType ] ); 
      print( "  DType: " ); print( types[ vk->tensorProperties[ i ].DType ] );
      printl( "}" );
    }
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


plvkInstance* plvkInit( s32 whichGPU, u32 debugLevel, bool useTensorCores ){
  marc;
  plvkInstance* vk = createInstance( whichGPU, debugLevel, useTensorCores );
  createPool( vk );
  createUBOs( vk ); 

  return vk;
}
void updateGPUstate( plvkInstance* vk, f32 time, f32 delta ){
  vk->UBOcpumem.time = time;
  vk->UBOcpumem.deltaTime = delta;
  updateInputBuffers( state.osstate->devices, &( vk->UBOcpumem.input ) );
  void* data;
  vkMapMemory( vk->device, vk->UBOs[ vk->currentImage ]->memory, 0,
	       sizeof( gpuState ), 0, &data );
  memcpy( data, &vk->UBOcpumem, sizeof( gpuState ) );
  vkUnmapMemory( vk->device, vk->UBOs[ vk->currentImage ]->memory );
}
void plvkDraw( void ){
  static u64 firstDrawTime = 0;
  if( !firstDrawTime )
    firstDrawTime = tickCount();
  plvkInstance* vk = state.vk;
  {
    plvkUnit* t = vk->unit;
    while( t ){
      for( u32 i = 0; i < t->tickCount; ++i )
	tickUnit( t );
      t = t->next;
    }
  }
  {
    plvkUniformBufferCallback* t = vk->ucallbacks;
    while( t ){
      void* m = mapUniformBuffer( vk, t->buf );
      t->func( m, t->data );
      unmapUniformBuffer( vk, t->buf );
      t = t->next;
    }
  }
  static u64 lastTime = 0;
  static u64 frameCount = 0;

  u64 elapsed = 0;
  static u64 lastFrameTime = 0;
  u64 now = tickCount();
  elapsed = (f64)( now - lastFrameTime ) / tickFrequency();
  
  updateGPUstate( vk, (f32)( now - firstDrawTime ) / (f32)tickFrequency(),
		  (f32)( now - lastTime ) / (f32)tickFrequency() );
      
  ++vk->currentImage;
  vk->currentImage %= 2;
  if( state.fps ){
    if( elapsed > 1.0 ){
      print( "FPS: " );
      printFloat( (f64)frameCount / elapsed );
      endl();
      lastFrameTime = now;
      frameCount = 0;
    }
  }
  lastTime = now;
  ++frameCount;
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
			  u64 drawSize, const void* pixels, u32 tickCount,
			  u32* specializations, u64 numSpecializations ){  
  plvkUnit* top = vk->unit;
  
  vk->unit = createUnit( vk, width, height, (VkFormat)format, fragmentSize,
			 fragName, vertName, displayed, title, x, y,
			 attachments, numAttachments, drawSize, pixels,
			 tickCount, specializations, numSpecializations );
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
plvkAttachable* plvkAddBuffer( plvkInstance* vk, void* data, u64 size ){
  new( ret, plvkAttachable );
  ret->type = PLVK_ATTACH_BUFFER;
  ret->buffer = createComputeBuffer( vk, data, size );
  ret->next = vk->attachables;
  vk->attachables = ret;
  return ret;
}
plvkAttachable* plvkAddUniformBuffer( plvkInstance* vk, u64 size,
				      void (*uniform)( void*out, void* in ),
				      void* indata ){
  new( ret, plvkAttachable );
  ret->type = PLVK_ATTACH_BUFFER;
  ret->buffer = createUniformBuffer( vk, size, uniform, indata );
  ret->next = vk->attachables;
  vk->attachables = ret;

  new( cb, plvkUniformBufferCallback );
  cb->func = uniform;
  cb->data = indata;
  cb->next = vk->ucallbacks;
  cb->buf = ret->buffer;
  vk->ucallbacks = cb;
  
  return ret;
}
void* plvkCopyComputeBuffer( plvkUnit* u ){
  return copyComputeBuffer( u );
}
void plvkTickUnit( plvkUnit* u ){
  tickUnit( u );
}
void plvkGetUnitSize( plvkUnit* u, u64* w, u64* h ){
  getUnitSize( u, w, h );
}
