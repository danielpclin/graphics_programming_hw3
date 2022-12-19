#version 430 core

in vec3 f_viewVertex;
in vec3 f_viewNormal;
in vec3 f_worldVertex;
in vec3 f_worldNormal;
in vec3 f_uv;

layout (location=0) out vec4 fragColor;

uniform sampler2DArray colorTexture;
uniform mat4 viewMat;

vec4 withFog(vec4 color){
	const vec4 FOG_COLOR = vec4(0.0, 0.0, 0.0, 1);
	const float MAX_DIST = 150.0;
	const float MIN_DIST = 120.0;
	
	float dis = length(f_viewVertex);
	float fogFactor = (MAX_DIST - dis) / (MAX_DIST - MIN_DIST);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	fogFactor = fogFactor * fogFactor;
	
	vec4 colorWithFog = mix(FOG_COLOR, color, fogFactor);
	return colorWithFog;
}

void main(){
	vec4 texel = texture(colorTexture, f_uv);
	vec3 lightDir = vec3(0.4, 0.5, 0.8);//(viewMat * vec4(0.4, 0.5, 0.8, 0.0)).xyz;
	vec3 position = f_worldVertex;
	
	if(texel.a < 0.2){
		discard;
	}

	vec3 normalizedNormal = normalize(f_worldNormal);

    // ambient
    vec3 ambient = texel.rgb;

    // diffuse
    vec3 lightDirection = normalize(lightDir - position);
    vec3 diffuse = max(dot(normalizedNormal, lightDirection), 0.0) * texel.rgb;

	fragColor = withFog(vec4((ambient * 0.3 + diffuse * 0.6), 1.0));
}