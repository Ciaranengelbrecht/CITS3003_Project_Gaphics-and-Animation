#include "DirectionalLightElement.h"

#include <unistd.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/component_wise.hpp>

#include "rendering/imgui/ImGuiManager.h"
#include "scene/SceneContext.h"

/*  Instantiate a default directional light
 */
std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::new_default(const SceneContext& scene_context, EditorScene::ElementRef parent) {
    auto light_element = std::make_unique<DirectionalLightElement>(
        parent,
        "New Directional Light",
        glm::vec3{0.0f, 1.0f, 0.0f},
        glm::vec3{0.0f, 0.0f, 0.0f}, // face down by default
        DirectionalLight::create(
            glm::vec3{},    //position Set via update_instance_data()
            glm::vec3{},    //TODO: direction set via update_instance_data()
            glm::vec4{1.0f}
        ),
        EmissiveEntityRenderer::Entity::create(
            scene_context.model_loader.load_from_file<EmissiveEntityRenderer::VertexData>("arrow.obj"),
            EmissiveEntityRenderer::InstanceData{
                glm::mat4{}, // Set via update_instance_data()
                EmissiveEntityRenderer::EmissiveEntityMaterial{
                    glm::vec4{1.0f}
                }
            },
            EmissiveEntityRenderer::RenderData{
                scene_context.texture_loader.default_white_texture()
            }
        ),
        EmissiveEntityRenderer::Entity::create(
            scene_context.model_loader.load_from_file<EmissiveEntityRenderer::VertexData>("sphere.obj"),
            EmissiveEntityRenderer::InstanceData{
                glm::mat4{},
                EmissiveEntityRenderer::EmissiveEntityMaterial{
                    glm::vec4{1.0f}
                }
            },
            EmissiveEntityRenderer::RenderData{
                scene_context.texture_loader.default_white_texture()
            }
            )
    );

    light_element->update_instance_data();
    return light_element;
}

std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::from_json(const SceneContext& scene_context, EditorScene::ElementRef parent, const json& j) {
    auto light_element = new_default(scene_context, parent);

    light_element->position = j["position"];
    light_element->direction = j["direction"];
    light_element->light->colour = j["colour"];
    light_element->visible = j["visible"];
    light_element->visual_scale = j["visual_scale"];

    light_element->update_instance_data();
    return light_element;
}

/*  Get the element variables into JSON format
 *  Compared to the Point Light element, an extra "direction" element is stored
 *  This is the offset from the object center where the (local) Z-axis aligns to
 */
json EditorScene::DirectionalLightElement::into_json() const {
    return {
        {"position",     position},
        {"colour",       light->colour},
        {"direction",    direction},
        {"visible",      visible},
        {"visual_scale", visual_scale},
    };
}

/*  add_imgui_edit_section defines the imgui elements required for the directional light
 *  "Local Transformation": the position of the object relative to global center
 *  "Direction": the offset coordinates which the z-axis points to
 *  "Light Properties": the color properties of the light
 *  "Visuals": the viewport properties of the light (scale, visibility, etc)
 */
void EditorScene::DirectionalLightElement::add_imgui_edit_section(MasterRenderScene& render_scene, const SceneContext& scene_context) {
    ImGui::Text("Directional Light");
    SceneElement::add_imgui_edit_section(render_scene, scene_context);

    static bool linkTransforms = false;

    bool transformUpdated = false;

    //store a seperate variable deltaTranslate
    static glm::vec3 deltaTranslate = position;
    transformUpdated |= ImGui::DragFloat3("Translation", &deltaTranslate[0], 0.01f);
    ImGui::DragDisableCursor(scene_context.window);

    transformUpdated |= ImGui::Checkbox("[Link]", &linkTransforms);

    // TODO: update direction facing
    glm::vec3 newDirection = direction;
    if(linkTransforms) {
        newDirection += (deltaTranslate - position);
    }

    transformUpdated |= ImGui::DragFloat3("Direction", &newDirection[0], 0.01f);
    ImGui::DragDisableCursor(scene_context.window);


    ImGui::Text("Light Properties");
    transformUpdated |= ImGui::ColorEdit3("Colour", &light->colour[0]);
    ImGui::Spacing();
    ImGui::DragFloat("Intensity", &light->colour.a, 0.01f, 0.0f, FLT_MAX);
    ImGui::DragDisableCursor(scene_context.window);

    ImGui::Spacing();
    ImGui::Text("Visuals");

    transformUpdated |= ImGui::Checkbox("Show Visuals", &visible);
    transformUpdated |= ImGui::DragFloat("Visual Scale", &visual_scale, 0.01f, 0.0f, FLT_MAX);
    ImGui::DragDisableCursor(scene_context.window);

    if (transformUpdated) {

        position = deltaTranslate;
        direction = newDirection;
        update_instance_data();

    }
}

/*  update_instance_data updates the data of the object
 *  the transform matrix is obtained and applied to the object
 *  this is where the get_direction function is called and the returning value is applied to the object
 */
void EditorScene::DirectionalLightElement::update_instance_data() {

    transform = glm::translate(position);
    const glm::mat4 transform_dir = glm::translate(direction);

    glm::mat4 object_rot = glm::lookAt(position, direction - position, glm::vec3(0.0, 0.0, -1.0));

    if (!EditorScene::is_null(parent)) {
        // Post multiply by transform
        transform = (*parent)->transform * transform;
    }

    light->position = glm::vec3(transform[3]);      // Extract translation from matrix
    light->direction = glm::vec3(transform_dir[3]); // Extract direction translation from matrix
    if (visible) {
        light_cone->instance_data.model_matrix =  transform * glm::inverse(object_rot) * glm::scale(glm::vec3{0.1f * visual_scale});
        direction_point->instance_data.model_matrix = transform_dir * glm::scale(glm::vec3{0.1f * visual_scale});
    } else {
        // Throw off to infinity as a hacky way to make model invisible
        light_cone->instance_data.model_matrix = glm::scale(glm::vec3{std::numeric_limits<float>::infinity()}) * glm::translate(glm::vec3{std::numeric_limits<float>::infinity()});
        direction_point->instance_data.model_matrix = glm::scale(glm::vec3{std::numeric_limits<float>::infinity()}) * glm::translate(glm::vec3{std::numeric_limits<float>::infinity()});
    }




    /*  Get the rotation and apply to the transformation matrix

    glm::mat4 directionPointTransform =  glm::translate(direction + light->position);
    light->direction = glm::vec3(directionPointTransform[3]); //get direction point transform by the offset

    glm::qua<float> rotate_to_qua = glm::rotation(glm::normalize(light->position), glm::normalize(glm::vec3(directionPointTransform[3])));
    glm::mat4 rotate_to = glm::toMat4(rotate_to_qua);
    transform = transform * rotate_to;
    if (visible) {
        direction_point->instance_data.model_matrix =  transform * glm::scale(glm::vec3{0.1f * visual_scale});
    } else {
        // Throw off to infinity as a hacky way to make model invisible
        direction_point->instance_data.model_matrix = glm::scale(glm::vec3{std::numeric_limits<float>::infinity()}) * glm::translate(glm::vec3{std::numeric_limits<float>::infinity()});
    }
    */



    glm::vec3 normalised_colour = glm::vec3(light->colour) / glm::compMax(glm::vec3(light->colour));
    light_cone->instance_data.material.emission_tint = glm::vec4(normalised_colour, light_cone->instance_data.material.emission_tint.a);
}

/*  get the element type name
 */
const char* EditorScene::DirectionalLightElement::element_type_name() const {
    return ELEMENT_TYPE_NAME;
}
