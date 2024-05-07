#version 410 core
#include "../common/lights.glsl"

// Per vertex data
layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinate;

//get light pipeline mode
uniform int shader_mode;

out VertexOut {
    LightingResult lighting_result;

    //texture coordinates
    vec2 texture_coordinate;

    //position and normals from vertex shader
    vec3 ws_position;
    vec3 ws_normal;

} vertex_out;

// Per instance data
uniform mat4 model_matrix;
uniform mat3 normal_matrix;

// Material properties
uniform vec3 diffuse_tint;
uniform vec3 specular_tint;
uniform vec3 ambient_tint;
uniform float shininess;

//texture scaling attribute
uniform vec2 texture_scale;

// Get Light Data
#if NUM_PL > 0
layout (std140) uniform PointLightArray {
    PointLightData point_lights[NUM_PL];
};
#endif

// Global data
uniform vec3 ws_view_position;
uniform mat4 projection_view_matrix;

LightingResult resolveVertexLighting(vec3 ws_position, vec3 ws_normal){
    // Per vertex lighting
    vec3 ws_view_dir = normalize(ws_view_position - ws_position);
    LightCalculatioData light_calculation_data = LightCalculatioData(ws_position, ws_view_dir, ws_normal);
    Material material = Material(diffuse_tint, specular_tint, ambient_tint, shininess);

    return total_light_calculation(light_calculation_data, material
    #if NUM_PL > 0
    ,point_lights
    #endif
    );
}

void main() {

    // Transform vertices
    vec3 ws_position = (model_matrix * vec4(vertex_position, 1.0f)).xyz;
    vertex_out.ws_position = ws_position;
    vec3 ws_normal = normalize(normal_matrix * normal);
    vertex_out.ws_normal = ws_normal;

    //apply texture scaling on texture coordinate space
    vertex_out.texture_coordinate = texture_coordinate * texture_scale;

    gl_Position = projection_view_matrix * vec4(vertex_out.ws_position, 1.0f);

    if(shader_mode == 1){
        vertex_out.lighting_result = resolveVertexLighting(ws_position, ws_normal);
    }

}

