#version 430 core

in vec3 f_viewVertex;
in vec3 f_viewNormal;
in vec3 f_worldVertex;
in vec3 f_worldNormal;
in vec3 f_uv;

layout (location=0) out vec4 fragColor;

uniform sampler2D colorTexture;

void main(){
	vec4 texel = texture(colorTexture, f_uv.xy);
	vec3 lightDir = vec3(0.4, 0.5, 0.8);
	vec3 position = f_worldVertex;
	
	//if(texel.a < 0.3){
	//	discard;
	//}

	vec3 normalizedNormal = normalize(f_worldNormal);

    // ambient
    vec3 ambient = texel.rgb;

    // diffuse
    vec3 lightDirection = normalize(lightDir - position);
    vec3 diffuse = max(dot(normalizedNormal, lightDirection), 0.0) * texel.rgb;

    // // specular
    // vec3 viewDirection = normalize(cameraPosition - position);
    // vec3 reflectDirection = reflect(-lightDirection, normalizedNormal);
    // vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    // vec3 specular = pow(max(dot(normalizedNormal, halfwayDirection), 0.0), material.shininess * 2) * material.specular;

    //color = vec4((ambient * 0.2 + diffuse * 0.7 + specular * 0.5), 1.0);

	fragColor = vec4((ambient * 0.15 + diffuse * 0.75), 1.0);
	if (f_uv.z != 0){
		fragColor = vec4(vec3(f_uv.z/4), 1.0);
	}
}