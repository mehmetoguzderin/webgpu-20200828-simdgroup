# WebGPU Simdgroup Samples

Please install Vulkan SDK (with `vulkan-tools` and `glslang-tools`)
before building the samples as build process requires it. But
build artifacts do not require Vulkan SDK to run, unless they
are debug builds which requires a validation layer from SDK.

Tested on Windows 10 and Ubuntu 20.04.


## Debug

```
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```


## Release

```
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```