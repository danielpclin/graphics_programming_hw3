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

layout (std430, binding=1) buffer InstanceData {
    InstanceProperties rawInstanceProps[];
};

layout (std430, binding=2) buffer CurrValidInstanceData {
    InstanceProperties currValidInstanceProps[];
};

layout (std430, binding=3) buffer DrawCommandsBlock {
    DrawCommand commands[];
};

uniform int numMaxInstance;
uniform mat4 viewProjMat;
uniform vec3 slimePos;

void main() {

    const uint idx = gl_GlobalInvocationID.x;
    // discarding invalid array-access
    if(idx >= numMaxInstance){ return; }
	
	//currValidInstanceProps[idx] = rawInstanceProps[idx];
	//return;

	if (distance(rawInstanceProps[idx].position.xyz, slimePos) < 1) { rawInstanceProps[idx].position.y = 1.0; }
	if (rawInstanceProps[idx].position.y == 1.0){ return; }

    // translate the position to clip space
    vec4 clipSpaceV = viewProjMat * vec4(rawInstanceProps[idx].position.xyz, 1.0);
    clipSpaceV = clipSpaceV / clipSpaceV.w;

    bool frustumCulled = (clipSpaceV.x < -1.0) || (clipSpaceV.x > 1.0) || (clipSpaceV.y < -1.0) || (clipSpaceV.y > 1.0) || (clipSpaceV.z < -1.0) || (clipSpaceV.z > 1.0);

    if(frustumCulled == false){
		uint gidx;
		if(idx < commands[1].baseInstance) {
			const uint UNIQUE_IDX = atomicAdd(commands[0].instanceCount, 1);
			gidx = commands[0].baseInstance + UNIQUE_IDX;
		} else if (idx < commands[2].baseInstance) {
			const uint UNIQUE_IDX = atomicAdd(commands[1].instanceCount, 1);
			gidx = commands[1].baseInstance + UNIQUE_IDX;
		} else {
			const uint UNIQUE_IDX = atomicAdd(commands[2].instanceCount, 1);
			gidx = commands[2].baseInstance + UNIQUE_IDX;
		}
		currValidInstanceProps[gidx] = rawInstanceProps[idx];
    }

}
