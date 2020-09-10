cmake_minimum_required(VERSION 3.16.0)
project(webgpu_20200828_simdgroup VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 20)

add_executable(webgpu_20200828_simdgroup samples.cpp)

add_subdirectory(Vulkan-Headers EXCLUDE_FROM_ALL)
target_link_libraries(webgpu_20200828_simdgroup PRIVATE Vulkan-Headers)

add_subdirectory(volk EXCLUDE_FROM_ALL)
target_link_libraries(webgpu_20200828_simdgroup PRIVATE volk_headers)

add_library(vma_headers INTERFACE)
target_include_directories(vma_headers
  INTERFACE VulkanMemoryAllocator/src
)
target_link_libraries(webgpu_20200828_simdgroup PRIVATE vma_headers)

file(GLOB kernel_files "${CMAKE_SOURCE_DIR}/*.comp")
foreach(kernel_file ${kernel_files})
  string(REGEX REPLACE "^.*[\\/|\\\\]|.[^.]*$" "" kernel ${kernel_file})
  add_custom_command(TARGET webgpu_20200828_simdgroup POST_BUILD
                     COMMAND glslangValidator --target-env vulkan1.1 -V ${CMAKE_SOURCE_DIR}/${kernel}.comp
                     -o ${CMAKE_SOURCE_DIR}/${kernel}.spv)

  add_custom_command(TARGET webgpu_20200828_simdgroup POST_BUILD
                   COMMAND spirv-opt -O ${CMAKE_SOURCE_DIR}/${kernel}.spv
                   -o ${CMAKE_SOURCE_DIR}/${kernel}.spv)

  add_custom_command(TARGET webgpu_20200828_simdgroup POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${CMAKE_SOURCE_DIR}/${kernel}.spv
                   $<TARGET_FILE_DIR:${PROJECT_NAME}>/${kernel}.spv)
endforeach()
