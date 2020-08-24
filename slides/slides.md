---
marp: true
title: WebGPU Simdgroup 20200828
description: WebGPU Simdgroup 20200828 Mehmet Oguz Derin
theme: uncover
paginate: true
_paginate: false
---

# <!--fit--> SIMD operations in WebGPU for ML

How subgroup operations will help ML in the browser

<!-- _footer: "Mehmet **Oguz** Derin **@mehmetoguzderin**" -->

---

# Table of Contents

+ Concepts
+ Impact on ML
+ Web Interface
+ Operations
+ Hardware Support
+ Roadmap

---

# Subgroups

+ Subdivision of threadgroups

+ Also named as **simdgroups**, **warps**, **waves**

+ Subgroup operations make sharing and reducing data across threads in a subgroup **measurably faster**

+ Their size vary across GPUs

+ We can have these operations in WGSL (WebGPU Shading Language)

---

![bg 75%](nvidia-register-cache-0.jpg)

<!-- _footer: "https://developer.nvidia.com/blog/register-cache-warp-cuda/" -->

---

![bg 90%](amd-rdna-0.png)

<!-- _footer: "https://www.amd.com/system/files/documents/rdna-whitepaper.pdf" -->

---

# Impact on ML

+ Exploratory Data Analysis
+ Model Fine Tuning
+ Edge Inference

How:
+ ~2x reduced runtime
+ Reduced power consumption
+ Intuitive calculations [0]

<!-- _footer: "[0] It is important to note that GPU has no atomics or advisable locking mechanism for floating point numbers." -->

---

# Web Interface

Subgroup operations exposed to web are
+ Compute stage only
+ Active threads only
+ Non-uniform

---

![bg 90%](nvidia-using-warp-0.png)

<!-- _footer: "https://developer.nvidia.com/blog/using-cuda-warp-level-primitives/" -->

---

| Basic Operations |
|:---|
| `subgroup_size` |
| `subgroup_invocation_idx` |
| `subgroupIsFirst` |

<!-- _footer: "https://mehmetoguzderin.github.io/webgpu/wgsl.html#subgroup-builtin-functions" -->

---

| Vote Operations |
|:---|
| `subgroupAll` |
| `subgroupAny` |

<!-- _footer: "https://mehmetoguzderin.github.io/webgpu/wgsl.html#subgroup-builtin-functions" -->

---

| Arithmetic Operations |
|:---|
| `subgroupAdd` |
| `subgroupMul` |
| `subgroupMin` |
| `subgroupMax` |
| `subgroupAnd` |
| `subgroupOr` |
| `subgroupXor` |

<!-- _footer: "https://mehmetoguzderin.github.io/webgpu/wgsl.html#subgroup-builtin-functions" -->

---

| Arithmetic Prefix Operations |
|:---|
| `subgroupPrefixAdd` |
| `subgroupPrefixMul` |

<!-- _footer: "https://mehmetoguzderin.github.io/webgpu/wgsl.html#subgroup-builtin-functions" -->

---

| Ballot Operations |
|:---|
| `subgroupBallot` |
| `subgroupBroadcastFirst` |

<!-- _footer: "https://mehmetoguzderin.github.io/webgpu/wgsl.html#subgroup-builtin-functions" -->

---

# Hardware Support

**Desktop:** available everywhere
**Mobile:** most of the next generation chips support

---

# Roadmap

Raised concerns mostly fall out-of-scope for this PR, not blockers for adoption as-is

Technically can make into MVP as a good addition in the standard library of WGSL

---

![bg right 60%](mehmetoguzderin.png)

## **Thanks for your attention!**

You can check out the PR itself [#954](https://github.com/gpuweb/gpuweb/pull/954)

<!-- _footer: "Twitter: [**@mehmetoguzderin**](https://twitter.com/mehmetoguzderin)" -->