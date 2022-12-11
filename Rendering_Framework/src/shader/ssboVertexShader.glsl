#version 430 core

layout(location=0) in vec3 v_vertex;
layout(location=1) in vec3 v_normal;
layout(location=2) in vec3 v_uv;
layout(location=3) in vec4 v_worldPosOffset;

out vec3 f_viewVertex;
out vec3 f_viewNormal;
out vec3 f_worldVertex;
out vec3 f_worldNormal;
out vec3 f_uv;

uniform mat4 projMat;
uniform mat4 viewMat;
uniform mat4 modelMat;

void main() {
	vec4 worldVertex = modelMat * vec4(v_vertex, 1.0) + v_worldPosOffset;
	vec4 worldNormal = modelMat * vec4(v_normal, 0.0);
	vec4 viewVertex = viewMat * worldVertex;
	vec4 viewNormal = viewMat * worldNormal;

	f_worldVertex = worldVertex.xyz;
	f_worldNormal = worldNormal.xyz;
	f_viewVertex = viewVertex.xyz;
	f_viewNormal = viewNormal.xyz;
	f_uv = v_uv;

	gl_Position = projMat * viewVertex;
} 