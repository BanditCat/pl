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



// This function creates UBOs.
void createUBOLayout( plvkState* vk ){
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
						 &vk->bufferLayout ) ) 
    die( "Descriptor layout creation failed." );
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
VkExtent2D getExtent( const VkSurfaceCapabilitiesKHR* caps ) {
  plvkState* vk = state.vk;
  const guiInfo* g = vk->gui;
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
void createSwap( plvkState* vk ){
  vkDeviceWaitIdle( vk->device );
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vk->gpu, vk->surface,
					     &vk->surfaceCapabilities );
  vk->extent = getExtent( &vk->surfaceCapabilities );
  if( !vk->extent.width || !vk->extent.height )
    vk->rendering = 0;
  else{
    vk->rendering = 1;
  
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

    vkDestroyShaderModule( vk->device, displayFragmentShader, NULL );
    vkDestroyShaderModule( vk->device, displayVertexShader, NULL );
  }
}
void destroySwap( plvkStatep vkp ){
  plvkState* vk = vkp;
  if( vk->rendering ){
    vkDeviceWaitIdle( vk->device );
    for( u32 i = 0; i < vk->numImages; ++i ){
      vkDestroyFramebuffer( vk->device, vk->framebuffers[ i ], NULL );
      vkDestroyImageView( vk->device, vk->imageViews[ i ], NULL );
    }
    vkDestroyDescriptorPool( vk->device, vk->descriptorPool, NULL );
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
  if( vk->UBOmems )
    memfree( vk->UBOmems );
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
  if( vk->device )
    vkDestroyDevice( vk->device, NULL);
  m;
#ifdef DEBUG
  vk->vkDestroyDebugUtilsMessengerEXT( vk->instance, vk->vkdbg, NULL );
#endif
  if( vk->instance )
    vkDestroyInstance( vk->instance, NULL );
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
  m;
  if( vk->gui )
    wend( vk->gui );
  memfree( vk );
  m;
}



void plvkInit( s32 whichGPU, void* vgui, u32 debugLevel ){
  new( vk, plvkState );
  vk->gui = (guiInfo*)vgui;
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
  createSwap( vk );
  
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
  
}
void updateGPUstate( plvkState* vk, f32 time ){
  vk->UBOcpumem.time = time;
  void* data;
  vkMapMemory( vk->device, vk->UBOmems[ vk->currentImage ], 0,
	       sizeof( gpuState ), 0, &data );
  memcpy( data, &vk->UBOcpumem, sizeof( gpuState ) );
  vkUnmapMemory( vk->device, vk->UBOmems[ vk->currentImage ] );
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
    createSwap( vk );
  }
}
