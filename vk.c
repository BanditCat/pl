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
  getExtent( vk );
  
  VkPresentModeKHR pm = VK_PRESENT_MODE_FIFO_KHR;
  VkSwapchainCreateInfoKHR scci = {};
  scci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  scci.surface = vk->surface;
  scci.minImageCount = state.frameCount;
  scci.imageFormat = vk->theSurfaceFormat.format;
  scci.imageColorSpace = vk->theSurfaceFormat.colorSpace;
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
  if( !vk->numImages )
    vkGetSwapchainImagesKHR( vk->device, vk->swap, &vk->numImages, NULL );
  if( !vk->images )
    vk->images = newae( VkImage, vk->numImages );
  vkGetSwapchainImagesKHR( vk->device, vk->swap, &vk->numImages, vk->images );
  // Image views.
  if( !vk->imageViews )
    vk->imageViews = newae( VkImageView, vk->numImages );
  for( u32 i = 0; i < vk->numImages; ++i ){
    VkImageViewCreateInfo ivci = {};
    ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.image = vk->images[ i ];
    ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ivci.format = vk->theSurfaceFormat.format;
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
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &vk->bufferLayout;
    if( VK_SUCCESS !=
	vkCreatePipelineLayout( vk->device, &plci,
				NULL, &vk->pipelineLayout ) )
      die( "Pipeline creation failed." );

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = vk->theSurfaceFormat.format;
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
  }
    
  // Framebuffers
  if( !vk->framebuffers )
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

  if( !vk->commandBuffers )
    vk->commandBuffers = newae( VkCommandBuffer, vk->numImages );

  VkCommandBufferAllocateInfo cbai = {};
  cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbai.commandPool = vk->pool;
  cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cbai.commandBufferCount = vk->numImages;
  if( VK_SUCCESS != vkAllocateCommandBuffers( vk->device, &cbai,
					      vk->commandBuffers ) )
    die( "Command buffer creation failed." );

  createUBOs( vk );
  createDescriptorPool( vk );
  createDescriptorSets( vk );
    
  // Command buffers and render passes.
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

    vkCmdBeginRenderPass( vk->commandBuffers[ i ], &rpbi,
			  VK_SUBPASS_CONTENTS_INLINE );
    vkCmdBindPipeline( vk->commandBuffers[ i ],
		       VK_PIPELINE_BIND_POINT_GRAPHICS, vk->pipeline );
    vkCmdBindDescriptorSets( vk->commandBuffers[ i ],
			     VK_PIPELINE_BIND_POINT_GRAPHICS,
			     vk->pipelineLayout, 0, 1,
			     &vk->descriptorSets[ i ], 0, NULL );
    vkCmdDraw( vk->commandBuffers[ i ], 3, 1, 0, 0 );
    vkCmdEndRenderPass( vk->commandBuffers[ i ] );
    if( VK_SUCCESS != vkEndCommandBuffer( vk->commandBuffers[ i ] ) )
      die( "Command buffer recording failed." );
  }

  destroyModule( vk, displayFragmentShader );
  destroyModule( vk, displayVertexShader );
}

void destroySwap( plvkStatep vkp ){
  plvkState* vk = vkp;
  if( vk->rendering ){
    vkDeviceWaitIdle( vk->device );
    for( u32 i = 0; i < vk->numImages; ++i ){
      vkDestroyFramebuffer( vk->device, vk->framebuffers[ i ], NULL );
      vkDestroyImageView( vk->device, vk->imageViews[ i ], NULL );
    }
    destroyDescriptorPool( vk );
    vkFreeCommandBuffers( vk->device, vk->pool, vk->numImages,
			  vk->commandBuffers );
    if( vk->pipeline )
      vkDestroyPipeline( vk->device, vk->pipeline, NULL );
    if( vk->pipelineLayout )
      vkDestroyPipelineLayout( vk->device, vk->pipelineLayout, NULL );
    if( vk->renderPass )
      vkDestroyRenderPass( vk->device, vk->renderPass, NULL );
    if( vk->swap )
      vkDestroySwapchainKHR( vk->device, vk->swap, NULL );
    destroyUBOs( vk );
  }
}
void plvkEnd( plvkStatep vkp ){
  m;
  plvkState* vk = vkp;
  vkDeviceWaitIdle( vk->device );
  m;
  for( u32 i = 0; i < vk->numImages; ++i ){
    vkDestroySemaphore( vk->device, vk->imageAvailables[ i ], NULL );
    vkDestroySemaphore( vk->device, vk->renderCompletes[ i ], NULL );
    vkDestroyFence( vk->device, vk->fences[ i ], NULL );
  }
  m;
  destroySwap( vkp );
  if( vk->bufferLayout )
    vkDestroyDescriptorSetLayout( vk->device, vk->bufferLayout, NULL );
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
  if( vk->imageViews )
    memfree( vk->imageViews );
  m;
  if( vk->images )
    memfree( vk->images );
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

  VkCommandPoolCreateInfo cpci = {};
  cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cpci.queueFamilyIndex = 0;
  if( VK_SUCCESS != vkCreateCommandPool( vk->device, &cpci, NULL,
					 &vk->pool ) )
    die( "Command pool creation failed." );


  createUBOLayout( vk );
  rebuild( vk );
  
  // Semaphores and fences.
  vk->imageAvailables = newae( VkSemaphore, vk->numImages );
  vk->renderCompletes = newae( VkSemaphore, vk->numImages );
  vk->fences = newae( VkFence, vk->numImages );
  vk->fenceSyncs = newae( VkFence, vk->numImages );
  for( u32 i = 0; i < vk->numImages; ++i )
    vk->fenceSyncs[ i ] = VK_NULL_HANDLE;
  VkSemaphoreCreateInfo sci = {};
  sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fci = {};
  fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  for( u32 i = 0; i < vk->numImages; ++i ){
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
  if( vk->rendering ){
    static u64 lasttime = 0;
    static u64 frameCount = 0;

    vkWaitForFences( vk->device, 1, &vk->fences[ vk->currentImage ], VK_TRUE,
		     UINT64_MAX );

    uint32_t index = 0;
    if( VK_ERROR_OUT_OF_DATE_KHR ==
	vkAcquireNextImageKHR( vk->device, vk->swap, 1000000000,
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
      VkSwapchainKHR swaps[] = { vk->swap };
      presentation.swapchainCount = 1;
      presentation.pSwapchains = swaps;
      presentation.pImageIndices = &index;
      if( VK_ERROR_OUT_OF_DATE_KHR ==
	  vkQueuePresentKHR( vk->queue, &presentation ) )
	recreate = 1;
      ++vk->currentImage;
      vk->currentImage %= vk->numImages;
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
    destroySwap( vk );
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
