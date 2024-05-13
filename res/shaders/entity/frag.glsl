#version 410 core
#include "../common/lights.glsl"

//get light pipeline mode
uniform int shader_mode;

in VertexOut {
    LightingResult lighting_result;

    //texture coordinates
    vec2 texture_coordinate;

    //position and normals from vertex shader
    vec3 ws_position;
    vec3 ws_normal;

} frag_in;

layout(location = 0) out vec4 out_colour;

// Global Data
uniform float inverse_gamma;

//material properties
uniform vec3 diffuse_tint;
uniform vec3 specular_tint;
uniform vec3 ambient_tint;
uniform float shininess;

//texture properties
uniform sampler2D diffuse_texture;
uniform sampler2D specular_map_texture;

// Global data
uniform vec3 ws_view_position;

// Get Light Data
#if NUM_PL > 0
layout (std140) uniform PointLightArray {
    PointLightData point_lights[NUM_PL];
};
#endif

vec3 resolveFragmentLighting() {
    vec3 ws_view_dir = normalize(ws_view_position - frag_in.ws_position);

    // Prepare the material based on the shader uniforms
    Material material = Material(diffuse_tint, specular_tint, ambient_tint, shininess);

    // Calculate lighting result based on the point lights
    LightCalculatioData light_calc_data = LightCalculatioData(frag_in.ws_position, ws_view_dir, frag_in.ws_normal);
    LightingResult lighting_result = total_light_calculation(light_calc_data, material
        #if NUM_PL > 0
        , point_lights
        #endif
    );

    // Calculate the contribution from the directional light
    vec3 dirLightContribution = calculateDirectionalLight(dirLight, frag_in.ws_normal, ws_view_dir, material);

    // Combine the results from point lights and directional light
    lighting_result.total_diffuse += dirLightContribution * material.diffuse_tint;
    lighting_result.total_specular += dirLightContribution * material.specular_tint;
    lighting_result.total_ambient += dirLightContribution * material.ambient_tint;

    // Resolve the final color using the texture and calculated light
    return resolve_textured_light_calculation(lighting_result, diffuse_texture, specular_map_texture, frag_in.texture_coordinate);
}


void main() {

    out_colour = vec4(resolveFragmentLighting(), 1.0f);
    out_colour.rgb = pow(out_colour.rgb, vec3(inverse_gamma));
}


