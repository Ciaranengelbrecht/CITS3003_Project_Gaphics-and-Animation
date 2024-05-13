#ifndef NUM_PL
#define NUM_PL 0
#endif

// Material Properties
struct Material {
    vec3 diffuse_tint;
    vec3 specular_tint;
    vec3 ambient_tint;
    float shininess;
};

// Light Data
struct LightCalculatioData {
    vec3 ws_frag_position;
    vec3 ws_view_dir;
    vec3 ws_normal;
};

struct PointLightData {
    vec3 position;
    vec3 colour;
};

// Calculations
// Directional Light Structure
struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

uniform DirectionalLight dirLight;

vec3 calculateDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir, Material material) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    vec3 ambient = 0.1 * light.color * light.intensity;
    vec3 diffuse = diff * light.color * light.intensity;
    vec3 specular = spec * light.color * light.intensity;
    
    return ambient + diffuse + specular;
}

const float ambient_factor = 0.002f;

// Point Lights
void point_light_calculation(PointLightData point_light, LightCalculatioData calculation_data, float shininess, inout vec3 total_diffuse, inout vec3 total_specular, inout vec3 total_ambient) {
    vec3 ws_light_offset = point_light.position - calculation_data.ws_frag_position;
    float distance = length(ws_light_offset);
    //similar to video:formula derived from https://gamedev.stackexchange.com/questions/131372/light-attenuation-formula-derivation
    float attenuation = 1.0 / (1.0 + distance * distance);
    //potentially better formula derives from: https://gamedev.stackexchange.com/questions/56897/glsl-light-attenuation-color-and-intensity-formula
    //float attenuation = 1.0 / (1.0 + 0.1*distance * 0.01*distance*distance);

    // Ambient
    vec3 ambient_component = ambient_factor * point_light.colour * attenuation;

    // Diffuse
    vec3 ws_light_dir = normalize(ws_light_offset);
    float diffuse_factor = max(dot(ws_light_dir, calculation_data.ws_normal), 0.0f);
    vec3 diffuse_component = diffuse_factor * point_light.colour * attenuation;

    // Specular
    vec3 ws_halfway_dir = normalize(ws_light_dir + calculation_data.ws_view_dir);
    float specular_factor = pow(max(dot(calculation_data.ws_normal, ws_halfway_dir), 0.0f), shininess);
    vec3 specular_component = specular_factor * point_light.colour * attenuation;

    total_diffuse += diffuse_component;
    total_specular += specular_component;
    total_ambient += ambient_component;
}

// Total Calculation

struct LightingResult {
    vec3 total_diffuse;
    vec3 total_specular;
    vec3 total_ambient;
};

LightingResult total_light_calculation(LightCalculatioData light_calculation_data, Material material
        #if NUM_PL > 0
        ,PointLightData point_lights[NUM_PL]
        #endif
    ) {

    vec3 total_diffuse = vec3(0.0f);
    vec3 total_specular = vec3(0.0f);
    vec3 total_ambient = vec3(0.0f);

    #if NUM_PL > 0
    for (int i = 0; i < NUM_PL; i++) {
        point_light_calculation(point_lights[i], light_calculation_data, material.shininess, total_diffuse, total_specular, total_ambient);
    }
    #endif

    #if NUM_PL > 0
    total_ambient /= float(NUM_PL);
    #endif

    total_diffuse *= material.diffuse_tint;
    total_specular *= material.specular_tint;
    total_ambient *= material.ambient_tint;
 // Calculate directional light contribution
vec3 dirLightContribution = calculateDirectionalLight(dirLight, light_calculation_data.ws_normal, light_calculation_data.ws_view_dir, material);
    total_diffuse += dirLightContribution; 
    total_specular += dirLightContribution; 
    total_ambient += dirLightContribution; 
    return LightingResult(total_diffuse, total_specular, total_ambient);
}

vec3 resolve_textured_light_calculation(LightingResult result, sampler2D diffuse_texture, sampler2D specular_map, vec2 texture_coordinate) {
    vec3 texture_colour = texture(diffuse_texture, texture_coordinate).rgb;
    vec3 specular_map_sample = texture(specular_map, texture_coordinate).rgb;

    vec3 textured_diffuse = result.total_diffuse * texture_colour;
    vec3 sampled_specular = result.total_specular * specular_map_sample;
    vec3 textured_ambient = result.total_ambient * texture_colour;

    // Mix the diffuse and ambient so that there is no ambient in bright scenes
    return max(textured_diffuse, textured_ambient) + sampled_specular;
}