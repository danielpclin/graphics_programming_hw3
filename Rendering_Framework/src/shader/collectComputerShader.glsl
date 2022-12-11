#version 430 core

layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

struct DrawCommand {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
};

struct InstanceProperties {
    vec4 position;
};

layout (std430, binding=3) buffer DrawCommandsBlock {
    DrawCommand commands[];
};

layout (std430, binding=1) buffer InstanceData {
    InstanceProperties rawInstanceProps[] ;
};

layout (std430, binding=2) buffer CurrValidInstanceData {
    InstanceProperties currValidInstanceProps[] ;
};

uniform int numMaxInstance;
uniform mat4 viewProjMat;

void main() {

    const uint idx = gl_GlobalInvocationID.x;
    // discarding invalid array-access
    if(idx >= numMaxInstance){ return; }

    // translate the position to clip space
    vec4 clipSpaceV = viewProjMat * vec4(rawInstanceProps[idx].position.xyz, 1.0);
    clipSpaceV = clipSpaceV / clipSpaceV.w;

    bool frustumCulled = (clipSpaceV.x < -1.0) || (clipSpaceV.x > 1.0) || (clipSpaceV.y < -1.0) || (clipSpaceV.y > 1.0) || (clipSpaceV.z < -1.0) || (clipSpaceV.z > 1.0);


    if(frustumCulled == false){
        // get UNIQUE buffer location for assigning the instance data
        // it also updates instanceCount
        const int UNIQUE_IDX = atomicAdd(commands[0].instanceCount, 1);
        // put data into valid-instance buffer
        currValidInstanceProps[UNIQUE_IDX] = rawInstanceProps[idx];
    }

}
