#include <cstdlib>
#include <cmath>
#include <chrono>
#include <functional>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>

#define VOLK_IMPLEMENTATION
#include "volk.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

int main() {
  uint32_t samples = 16384;
  VkResult result = VK_SUCCESS;

  /* Create Instance
   * * * * * * * * * * * * * * * */
  result = volkInitialize();
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }
  VkApplicationInfo application_info {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = NULL,
    .pApplicationName = NULL,
    .applicationVersion = 0,
    .pEngineName = NULL,
    .engineVersion = 0,
    .apiVersion = VK_MAKE_VERSION(1, 1, 0)
  };
  const char *enabled_layer_names[] {
    "VK_LAYER_KHRONOS_validation"
  };
  VkInstanceCreateInfo instance_info {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .pApplicationInfo = &application_info,
    #ifdef NDEBUG
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = NULL,
    #else
    .enabledLayerCount = 1,
    .ppEnabledLayerNames = enabled_layer_names,
    #endif
    .enabledExtensionCount = 0,
    .ppEnabledExtensionNames = NULL
  };
  VkInstance instance;
  result = vkCreateInstance(&instance_info,
                            NULL,
                            &instance);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }
  volkLoadInstance(instance);

  /* Enumerate Physical Devices
   * * * * * * * * * * * * * * * */
  uint32_t physical_device_count;
  result = vkEnumeratePhysicalDevices(instance,
                                      &physical_device_count,
                                      NULL);
  if (physical_device_count == 0) {
    std::cout << "No device.\n";
    return VK_ERROR_UNKNOWN;
  }
  VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)
    malloc(sizeof(VkPhysicalDevice) * physical_device_count);
  result = vkEnumeratePhysicalDevices(instance,
                                      &physical_device_count,
                                      physical_devices);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }

  /* Select Physical Device
   * * * * * * * * * * * * * * * */
  VkPhysicalDeviceProperties physical_device_properties;
  for (uint32_t i = 0; i < physical_device_count; ++i) {
    vkGetPhysicalDeviceProperties(physical_devices[i],
                                  &physical_device_properties);
    printf("Physical Device %d: %s\n",
           i, physical_device_properties.deviceName);
  }
  uint32_t physical_device_index;
  printf("Please input the index of your desired physical device: ");
  scanf("%d", &physical_device_index);
  if (physical_device_index > physical_device_count - 1) {
    printf("Invalid physical device index.\n");
    return VK_ERROR_UNKNOWN;
  }
  VkPhysicalDevice physical_device = physical_devices[physical_device_index];
  vkGetPhysicalDeviceProperties(physical_device,
                                &physical_device_properties);

  /* Obtain Total Start
   * * * * * * * * * * * * * * * */
  auto total_start = std::chrono::steady_clock::now();

  /* Create Device
   * * * * * * * * * * * * * * * */
  float queue_priorities[] { 1.0f };
  VkDeviceQueueCreateInfo device_queue_create_infos[] {
    { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .queueFamilyIndex = 0,
      .queueCount = 1,
      .pQueuePriorities = queue_priorities }
  };
  VkDeviceCreateInfo device_create_info {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = device_queue_create_infos,
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = NULL,
    .enabledExtensionCount = 0,
    .ppEnabledExtensionNames = NULL,
    .pEnabledFeatures = NULL
  };
  VkDevice device;
  result = vkCreateDevice(physical_device,
                          &device_create_info,
                          NULL,
                          &device);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }
  volkLoadDevice(device);
  VkQueue queue;
  vkGetDeviceQueue(device,
                   0,
                   0,
                   &queue);
  VmaAllocatorCreateInfo allocator_create_info {
    .flags = 0,
    .physicalDevice = physical_device,
    .device = device,
    .preferredLargeHeapBlockSize = 0,
    .pAllocationCallbacks = NULL,
    .pDeviceMemoryCallbacks = NULL,
    .frameInUseCount = 0,
    .pHeapSizeLimit = NULL,
    .pVulkanFunctions = NULL,
    .pRecordSettings = NULL,
    .instance = instance,
    .vulkanApiVersion =  VK_MAKE_VERSION(1, 1, 0)
  };
  allocator_create_info.instance = instance;
  VmaAllocator allocator;
  result = vmaCreateAllocator(&allocator_create_info,
                              &allocator);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }
  auto map_memory = [&](VmaAllocation &allocation,
                        VmaAllocationInfo &allocation_info,
                        std::function<VkResult(void *&)> operations){
    result = vmaMapMemory(allocator,
                          allocation,
                          &allocation_info.pMappedData);
    if (result != VK_SUCCESS) {
      return result;
    }
    result = vmaInvalidateAllocation(allocator,
                                     allocation,
                                     0,
                                     allocation_info.size);
    if (result != VK_SUCCESS) {
      return result;
    }
    result = operations(allocation_info.pMappedData);
    if (result != VK_SUCCESS) {
      return result;
    }
    result = vmaFlushAllocation(allocator,
                                allocation,
                                0,
                                allocation_info.size);
    if (result != VK_SUCCESS) {
      return result;
    }
    vmaUnmapMemory(allocator,
                   allocation);
    return VK_SUCCESS;
  };

  /* Create Command Pool
   * * * * * * * * * * * * * * * */
  VkCommandPoolCreateInfo command_pool_create_info {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = NULL,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = 0
  };
  VkCommandPool command_pool;
  result = vkCreateCommandPool(device,
                               &command_pool_create_info,
                               NULL,
                               &command_pool);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }

  /* Create Command Buffers
   * * * * * * * * * * * * * * * */
  VkCommandBufferAllocateInfo command_buffer_allocate_info {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext = NULL,
    .commandPool = command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1
  };
  VkCommandBuffer command_buffers[1];
  result = vkAllocateCommandBuffers(device,
                                    &command_buffer_allocate_info,
                                    command_buffers);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }
  auto one_time_submit = [&](std::function<VkResult()> operations){
    VkCommandBufferBeginInfo command_buffer_begin_info {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = NULL,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = NULL
    };
    result = vkBeginCommandBuffer(command_buffers[0],
                                  &command_buffer_begin_info);
    if (result != VK_SUCCESS) {
      return result;
    }
    result = operations();
    if (result != VK_SUCCESS) {
      return result;
    }
    result = vkEndCommandBuffer(command_buffers[0]);
    if (result != VK_SUCCESS) {
      return result;
    }
    VkSubmitInfo submit_infos[] {
      { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = NULL,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = NULL,
        .pWaitDstStageMask = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = command_buffers,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = NULL }
    };
    auto start = std::chrono::steady_clock::now();
    result = vkQueueSubmit(queue,
                           1,
                           submit_infos,
                           NULL);
    if (result != VK_SUCCESS) {
      return result;
    }
    result = vkQueueWaitIdle(queue);
    if (result != VK_SUCCESS) {
      return result;
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = 
      std::chrono::duration_cast<std::chrono::nanoseconds>(end-start); 
    printf("Waited: %f nanoseconds\n", duration.count());
    return VK_SUCCESS;
  };

  /* Create Input Buffer
   * * * * * * * * * * * * * * * */
  VkBufferCreateInfo input_staging_buffer_create_info {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .size = samples * sizeof(float) * 4,
    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL
  };
  VmaAllocationCreateInfo input_staging_allocation_create_info {
    .flags = 0,
    .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
  };
  VkBuffer input_staging_buffer;
  VmaAllocation input_staging_allocation;
  VmaAllocationInfo input_staging_allocation_info;
  result = vmaCreateBuffer(allocator,
                           &input_staging_buffer_create_info,
                           &input_staging_allocation_create_info,
                           &input_staging_buffer,
                           &input_staging_allocation,
                           &input_staging_allocation_info);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }
  float minimum[4] {
    2.0f * float(samples),
    2.0f * float(samples),
    2.0f * float(samples),
    2.0f * float(samples)
  };
  float maximum[4] {
    -2.0f * float(samples),
    -2.0f * float(samples),
    -2.0f * float(samples),
    -2.0f * float(samples)
  };
  result = map_memory(input_staging_allocation,
                      input_staging_allocation_info,
                      [&](void *&pMappedData) {
    std::random_device random_device;
    std::mt19937 random_engine {random_device()};
    std::uniform_real_distribution<float> random_distribution { -float(samples),
                                                                float(samples)};
    for (uint32_t i = 0; i < samples; ++i) {
      for (uint32_t j = 0; j < 4; ++j) {
        float new_number_reciprocal = random_distribution(random_engine);
        if (new_number_reciprocal > 0.0f) {
          new_number_reciprocal += 1.0f;
        } else {
          new_number_reciprocal -= 1.0f;
        }
        float new_number = random_distribution(random_engine) /
                          new_number_reciprocal;
        minimum[j] = std::min(minimum[j], new_number);
        maximum[j] = std::max(maximum[j], new_number);
        ((float *)pMappedData)[i*4+j] = new_number;
      }
    };
    printf("Expected Minimum: %f %f %f %f\nExpected Maximum: %f %f %f %f\n",
          minimum[0], minimum[1], minimum[2], minimum[3],
          maximum[0], maximum[1], maximum[2], maximum[3]);
    return VK_SUCCESS;
  });
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }
  VkBufferCreateInfo input_buffer_create_info {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .size = samples * sizeof(float) * 4,
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL
  };
  VmaAllocationCreateInfo input_allocation_create_info {
    .flags = 0,
    .usage = VMA_MEMORY_USAGE_GPU_ONLY
  };
  VkBuffer input_buffer;
  VmaAllocation input_allocation;
  VmaAllocationInfo input_allocation_info;
  result = vmaCreateBuffer(allocator,
                           &input_buffer_create_info,
                           &input_allocation_create_info,
                           &input_buffer,
                           &input_allocation,
                           &input_allocation_info);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }
  result = one_time_submit([&](){
    VkBufferCopy buffer_copies[] {
      { .srcOffset = 0,
        .dstOffset = 0,
        .size = input_buffer_create_info.size }
    };
    vkCmdCopyBuffer(command_buffers[0],
                    input_staging_buffer,
                    input_buffer,
                    1,
                    buffer_copies);
    return VK_SUCCESS;
  });
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }

  /* Create Intermediary Buffer
   * * * * * * * * * * * * * * * */
  VkBufferCreateInfo intermediary_buffer_create_info {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .size = (samples * sizeof(float) * 4 * 2) / 256,
    .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL
  };
  VmaAllocationCreateInfo intermediary_allocation_create_info {
    .flags = 0,
    .usage = VMA_MEMORY_USAGE_GPU_ONLY
  };
  VkBuffer intermediary_buffer;
  VmaAllocation intermediary_allocation;
  VmaAllocationInfo intermediary_allocation_info;
  result = vmaCreateBuffer(allocator,
                           &intermediary_buffer_create_info,
                           &intermediary_allocation_create_info,
                           &intermediary_buffer,
                           &intermediary_allocation,
                           &intermediary_allocation_info);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }

  /* Create Output Buffer
   * * * * * * * * * * * * * * * */
  VkBufferCreateInfo output_buffer_create_info {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .size = sizeof(float) * 8,
    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL
  };
  VmaAllocationCreateInfo output_allocation_create_info {
    .flags = 0,
    .usage = VMA_MEMORY_USAGE_GPU_ONLY
  };
  VkBuffer output_buffer;
  VmaAllocation output_allocation;
  VmaAllocationInfo output_allocation_info;
  result = vmaCreateBuffer(allocator,
                           &output_buffer_create_info,
                           &output_allocation_create_info,
                           &output_buffer,
                           &output_allocation,
                           &output_allocation_info);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }
  VkBufferCreateInfo output_staging_buffer_create_info {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .size = sizeof(float) * 8,
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL
  };
  VmaAllocationCreateInfo output_staging_allocation_create_info {
    .flags = 0,
    .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
  };
  VkBuffer output_staging_buffer;
  VmaAllocation output_staging_allocation;
  VmaAllocationInfo output_staging_allocation_info;
  result = vmaCreateBuffer(allocator,
                           &output_staging_buffer_create_info,
                           &output_staging_allocation_create_info,
                           &output_staging_buffer,
                           &output_staging_allocation,
                           &output_staging_allocation_info);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }

  /* Create Input Shader Module
   * * * * * * * * * * * * * * * */
  FILE *input_code_pointer = fopen("samples_input_4.spv", "rb");
  if (input_code_pointer == NULL) {
    printf("No code.\n");
    return VK_ERROR_UNKNOWN;
  }
  fseek(input_code_pointer, 0, SEEK_END);
  size_t input_code_size = ftell(input_code_pointer);
  rewind(input_code_pointer);
  uint32_t *input_code = (uint32_t *)malloc(input_code_size);
  fread(input_code, input_code_size, 1, input_code_pointer);
  fclose(input_code_pointer);
  VkShaderModuleCreateInfo input_shader_module_create_info {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .codeSize = input_code_size,
    .pCode = input_code
  };
  VkShaderModule input_shader_module;
  result = vkCreateShaderModule(device,
                                &input_shader_module_create_info,
                                NULL,
                                &input_shader_module);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }

  /* Create Output Shader Module
   * * * * * * * * * * * * * * * */
  FILE *output_code_pointer = fopen("samples_output_4.spv", "rb");
  if (output_code_pointer == NULL) {
    printf("No code.\n");
    return VK_ERROR_UNKNOWN;
  }
  fseek(output_code_pointer, 0, SEEK_END);
  size_t output_code_size = ftell(output_code_pointer);
  rewind(output_code_pointer);
  uint32_t *output_code = (uint32_t *)malloc(output_code_size);
  fread(output_code, output_code_size, 1, output_code_pointer);
  fclose(output_code_pointer);
  VkShaderModuleCreateInfo output_shader_module_create_info {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .codeSize = output_code_size,
    .pCode = output_code
  };
  VkShaderModule output_shader_module;
  result = vkCreateShaderModule(device,
                                &output_shader_module_create_info,
                                NULL,
                                &output_shader_module);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }

  /* Create Descriptor Pool
   * * * * * * * * * * * * * * * */
  VkDescriptorPoolSize descriptor_pool_sizes[] {
    { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 3 }
  };
  VkDescriptorPoolCreateInfo descriptor_pool_create_info {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .pNext = NULL,
    .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
    .maxSets = 1,
    .poolSizeCount = 1,
    .pPoolSizes = descriptor_pool_sizes
  };
  VkDescriptorPool descriptor_pool;
  result = vkCreateDescriptorPool(device,
                                  &descriptor_pool_create_info,
                                  NULL,
                                  &descriptor_pool);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }

  /* Allocate Descriptor Sets
   * * * * * * * * * * * * * * * */
  VkDescriptorSetLayoutBinding descriptor_set_layout_bindings[] {
    { .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .pImmutableSamplers = NULL },
    { .binding = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .pImmutableSamplers = NULL },
    { .binding = 2,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .pImmutableSamplers = NULL }
  };
  VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .bindingCount = 3,
    .pBindings = descriptor_set_layout_bindings
  };
  VkDescriptorSetLayout descriptor_set_layout;
  result = vkCreateDescriptorSetLayout(device,
                                       &descriptor_set_layout_create_info,
                                       NULL,
                                       &descriptor_set_layout);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }
  VkDescriptorSetAllocateInfo descriptor_set_allocate_info {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .pNext = NULL,
    .descriptorPool = descriptor_pool,
    .descriptorSetCount = 1,
    .pSetLayouts = &descriptor_set_layout
  };
  VkDescriptorSet descriptor_sets[1];
  result = vkAllocateDescriptorSets(device,
                                    &descriptor_set_allocate_info,
                                    descriptor_sets);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }

  /* Update Descriptor Sets
   * * * * * * * * * * * * * * * */
  VkDescriptorBufferInfo input_descriptor_buffer_info {
    .buffer = input_buffer,
    .offset = 0,
    .range = VK_WHOLE_SIZE
  };
  VkDescriptorBufferInfo intermediary_descriptor_buffer_info {
    .buffer = intermediary_buffer,
    .offset = 0,
    .range = VK_WHOLE_SIZE
  };
  VkDescriptorBufferInfo output_descriptor_buffer_info {
    .buffer = output_buffer,
    .offset = 0,
    .range = VK_WHOLE_SIZE
  };
  VkWriteDescriptorSet write_descriptor_sets[] {
    { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = NULL,
      .dstSet = descriptor_sets[0],
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pImageInfo = NULL,
      .pBufferInfo = &input_descriptor_buffer_info,
      .pTexelBufferView = NULL },
    { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = NULL,
      .dstSet = descriptor_sets[0],
      .dstBinding = 1,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pImageInfo = NULL,
      .pBufferInfo = &intermediary_descriptor_buffer_info,
      .pTexelBufferView = NULL },
    { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .pNext = NULL,
      .dstSet = descriptor_sets[0],
      .dstBinding = 2,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pImageInfo = NULL,
      .pBufferInfo = &output_descriptor_buffer_info,
      .pTexelBufferView = NULL },
  };
  vkUpdateDescriptorSets(device,
                         3,
                         write_descriptor_sets,
                         0,
                         NULL);

  /* Create Compute Pipeline
   * * * * * * * * * * * * * * * */
  VkPipelineLayoutCreateInfo pipeline_layout_create_info {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .setLayoutCount = 1,
    .pSetLayouts = &descriptor_set_layout,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = NULL
  };
  VkPipelineLayout pipeline_layout;
  result = vkCreatePipelineLayout(device,
                                  &pipeline_layout_create_info,
                                  NULL,
                                  &pipeline_layout);
  VkPipelineShaderStageCreateInfo input_pipeline_shader_stage_create_info {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
    .module = input_shader_module,
    .pName = "main",
    .pSpecializationInfo = NULL
  };
  VkPipelineShaderStageCreateInfo output_pipeline_shader_stage_create_info {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
    .module = output_shader_module,
    .pName = "main",
    .pSpecializationInfo = NULL
  };
  VkComputePipelineCreateInfo compute_pipeline_create_infos[] {
    { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .stage = input_pipeline_shader_stage_create_info,
      .layout = pipeline_layout,
      .basePipelineHandle = NULL,
      .basePipelineIndex = 0 },
    { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .stage = output_pipeline_shader_stage_create_info,
      .layout = pipeline_layout,
      .basePipelineHandle = NULL,
      .basePipelineIndex = 0 }
  };
  VkPipeline pipelines[2];
  result = vkCreateComputePipelines(device,
                                    NULL,
                                    2,
                                    compute_pipeline_create_infos,
                                    NULL,
                                    pipelines);
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }

  /* Submit Compute Pipelines
   * * * * * * * * * * * * * * * */
  result = one_time_submit([&](){
    vkCmdBindDescriptorSets(command_buffers[0],
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipeline_layout,
                            0,
                            1,
                            &descriptor_sets[0],
                            0,
                            NULL);
    vkCmdBindPipeline(command_buffers[0],
                      VK_PIPELINE_BIND_POINT_COMPUTE,
                      pipelines[0]);
    vkCmdDispatch(command_buffers[0],
                  samples,
                  1,
                  1);
    VkBufferMemoryBarrier buffer_memory_barriers[] {
      { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = NULL,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .srcQueueFamilyIndex = 0,
        .dstQueueFamilyIndex = 0,
        .buffer = intermediary_buffer,
        .offset = 0,
        .size = intermediary_buffer_create_info.size }
    };
    vkCmdPipelineBarrier(command_buffers[0],
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0,
                         0,
                         NULL,
                         1,
                         buffer_memory_barriers,
                         0,
                         NULL);
    vkCmdBindPipeline(command_buffers[0],
                      VK_PIPELINE_BIND_POINT_COMPUTE,
                      pipelines[1]);
    vkCmdDispatch(command_buffers[0],
                  1,
                  1,
                  1);
    return VK_SUCCESS;
  });
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }

  /* Copy Calculation
   * * * * * * * * * * * * * * * */
  result = one_time_submit([&](){
    VkBufferCopy buffer_copies[] {
      { .srcOffset = 0,
        .dstOffset = 0,
        .size = output_buffer_create_info.size }
    };
    vkCmdCopyBuffer(command_buffers[0],
                    output_buffer,
                    output_staging_buffer,
                    1,
                    buffer_copies);
    return VK_SUCCESS;
  });
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }
  float calculation[8];
  result = map_memory(output_staging_allocation,
                      output_staging_allocation_info,
                      [&](void *&pMappedData) {
    for (uint32_t i = 0; i < 8; ++i) {
      calculation[i] = ((float *)pMappedData)[i];
    };
    printf("Calculated Minimum: %f %f %f %f\nCalculated Maximum: %f %f %f %f\n",
          calculation[0], calculation[1], calculation[2], calculation[3],
          calculation[4], calculation[5], calculation[6], calculation[7]);
    return VK_SUCCESS;
  });
  if (result != VK_SUCCESS) {
    printf("No success.\n");
    return result;
  }

  /* Validate Calculation
   * * * * * * * * * * * * * * * */
  for (uint32_t i = 0; i < 4; ++i) {
    if (std::fabs(calculation[i]-
                  minimum[i])
          > std::numeric_limits<float>::epsilon() * 16) {
      printf("Calculated minimum does not match expected minimum at %d\n", i);
      return VK_ERROR_MEMORY_MAP_FAILED;
    }
  };
  for (uint32_t i = 0; i < 4; ++i) {
    if (std::fabs(calculation[i+4]-
                  maximum[i])
          > std::numeric_limits<float>::epsilon() * 16) {
      printf("Calculated maximum does not match expected maximum at %d\n", i);
      return VK_ERROR_MEMORY_MAP_FAILED;
    }
  };
  printf("Calculated results match expected results.\n");

  /* Calculate Total Waited
   * * * * * * * * * * * * * * * */
  auto total_end = std::chrono::steady_clock::now();
  std::chrono::duration<double> total_duration =
    std::chrono::duration_cast<std::chrono::nanoseconds>(total_end-
                                                         total_start);
  printf("Total Waited: %f nanoseconds\n", total_duration.count());

  /* Destroy Instance
   * * * * * * * * * * * * * * * */
  vkDestroyPipeline(device,
                    pipelines[1],
                    NULL);
  vkDestroyPipeline(device,
                    pipelines[0],
                    NULL);
  vkDestroyPipelineLayout(device,
                          pipeline_layout,
                          NULL);
  vkFreeDescriptorSets(device,
                       descriptor_pool,
                       1,
                       descriptor_sets);
  vkDestroyDescriptorSetLayout(device,
                               descriptor_set_layout,
                               NULL);
  vkDestroyDescriptorPool(device,
                          descriptor_pool,
                          NULL);
  vkDestroyShaderModule(device,
                        output_shader_module,
                        NULL);
  vkDestroyShaderModule(device,
                        input_shader_module,
                        NULL);
  vmaDestroyBuffer(allocator,
                   output_staging_buffer,
                   output_staging_allocation);
  vmaDestroyBuffer(allocator,
                   output_buffer,
                   output_allocation);
  vmaDestroyBuffer(allocator,
                   intermediary_buffer,
                   intermediary_allocation);
  vmaDestroyBuffer(allocator,
                   input_buffer,
                   input_allocation);
  vmaDestroyBuffer(allocator,
                   input_staging_buffer,
                   input_staging_allocation);
  vkFreeCommandBuffers(device,
                       command_pool,
                       1,
                       command_buffers);
  vkDestroyCommandPool(device,
                       command_pool,
                       NULL);
  vmaDestroyAllocator(allocator);
  vkDestroyDevice(device,
                  NULL);
  vkDestroyInstance(instance,
                    NULL);

  return result;
}