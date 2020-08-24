hey, i am oguz, and in this presentation I will talk about simd operations in webgpu for machine learning


this is a high level presentation that doesn't go into too much technical detail


subgroups are subdivision of threadgroups they're also named as simdgroups, warps, and waves and their operations can make sharing and reducing data across threads in a subgroup measurably faster and we can have these operations in webgpu shading language


it is fast to share data between threads in a threadgroup thanks to shared memory but it is faster share data between threads in a subgroup because they don't even have to go to shared memory


the desire is containing as much sharing and calculation as possible in these simd32 blocks and subgroup operations are a great way to do that


subgroup operations reduce runtime and power consumption which can have critical impact on exploratory data analysis, model fine tuning, and edge inference applications but besides subgroups also bring an intuitive mapping to hardware for algorithms because gpus have no atomics or advisable locking mechanism for floating point numbers or at least one that will get exposed in webgpu


subgroup operations in webgpu shading language are compute stage only active threads only and have a non-uniform execution model


the meaning of active threads here is in case of a divergence inside a subgroup the operations provided will only execute across threads that can make to these operations together


let's get started with the basic operations subgroup size gives us the number of threads in a subgroup invocation index gives us the index of our thread inside subgroup and subgroup is first gives whether we are the first invocation among the active threads


subgroup all returns true to all invocations in case all invocations provided with true and any returns true in case at least one invocation provides true


arithmetic operations essentially provide you with reduction addition across invocations multiplication, minimum, maximum, and, or, exclusive or but it is important to note that these operations will take place across active threads and they can besides scalar numerical values they can take vectors too


prefix operations provide us with the summation or multiplication of invocations' with an index less than the one we are inspecting


subgroup ballot returns a bitfield where the bit is one in the corresponding invocation in case the invocation provided true to the ballot operation and subgroup broadcast first broadcasts the value in the first lane the first active lane to rest of the invocations


on desktop subgroup operations are available everywhere at least the target of webgpu and on the mobile most of the next generation chips support them


technically subgroup operations can make it into mvp as a addition in the standard library of webgpu shading language because the raised concerns mostly fall out of scope for this pr and not blockers for adoption as is


thanks and you can check the pr itself to see more and reach out to me for anything related to subgroups have a nice and healthy day