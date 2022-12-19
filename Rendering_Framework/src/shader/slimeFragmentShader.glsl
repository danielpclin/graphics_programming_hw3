#version 430 core

in vec3 f_viewVertex;
in vec3 f_viewNormal;
in vec3 f_worldVertex;
in vec3 f_worldNormal;
in vec3 f_uv;

layout (location=0) out vec4 fragColor;

uniform mat4 viewMat;

void main(){
	vec3 lightDir = (viewMat * vec4(0.4, 0.5, 0.8, 0.0)).xyz;
	vec3 position = f_worldVertex;
	vec3 cameraPosition = vec3(0.0, 8.0, 10.0);

	vec3 normalizedNormal = normalize(f_worldNormal);

    // ambient
    vec3 ambient = vec3(0.0, 0.5, 0.7);

    // diffuse
    vec3 lightDirection = normalize(lightDir - position);
    vec3 diffuse = max(dot(normalizedNormal, lightDirection), 0.0) * vec3(0.0, 0.5, 0.7);

    // // specular
    vec3 viewDirection = normalize(cameraPosition - position);
    vec3 reflectDirection = reflect(-lightDirection, normalizedNormal);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    vec3 specular = pow(max(dot(normalizedNormal, halfwayDirection), 0.0), 1.0) * vec3(1.0, 1.0, 1.0);

    fragColor = vec4((ambient * 0.2 + diffuse * 0.7 + specular * 0.5), 1.0);

	//fragColor = vec4((ambient * 0.15 + diffuse * 0.75), 1.0);
}