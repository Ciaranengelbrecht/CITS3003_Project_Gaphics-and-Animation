#include "DirectionalLightElement.h"

#include <unistd.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/component_wise.hpp>

#include "rendering/imgui/ImGuiManager.h"
#include "scene/SceneContext.h"

/*  Instantiate a default directional light
 */
auto default_light_col = glm::vec3(1.0f);
std::unique_ptr<EditorScene::DirectionalLightElement> EditorScene::DirectionalLightElement::new_default(const SceneContext& scene_context, EditorScene::ElementRef parent) {
    auto light_element = std::make_unique<DirectionalLightElement>(
        NullElementRef,
        "New Directional Light",
        glm::vec3{0.0f, 3.0f, 2.0f},
        glm::vec3{0.0f, 0.0f, 0.0f}, // face down by default
        DirectionalLight::create(
            glm::vec3{},    //position Set via update_instance_data()
            glm::vec3{},
            glm::vec4{default_light_col, 1.0f}
        ),
        EmissiveEntityRenderer::Entity::create(
            scene_context.model_loader.load_from_file<EmissiveEntityRenderer::VertexData>("arrow.obj"),
            EmissiveEntityRenderer::InstanceData{
                glm::mat4{1.0f}, // Set via update_instance_data()
                EmissiveEntityRenderer::EmissiveEntityMaterial{
                    glm::vec4{default_light_col, 1.0f}
                }
            },
            EmissiveEntityRenderer::RenderData{
                scene_context.texture_loader.default_white_texture()
            }
        ),
        EmissiveEntityRenderer::Entity::create(
            scene_context.model_loader.load_from_file<EmissiveEntityRenderer::VertexData>("sphere.obj"),
            EmissiveEntityRenderer::InstanceData{
                glm::mat4{1.0f},
                EmissiveEntityRenderer::EmissiveEntityMaterial{
                    glm::vec4{default_light_col, 1.0f}
                }
            },
            EmissiveEntityRenderer::RenderData{
                scene_context.texture_loader.default_white_texture()
            }));

    light_element->update_instance_data();
    return light_element;
}

/// TODO:
///
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

    static bool linkTransforms = true;

    bool transformUpdated = false;

    //store a seperate variable deltaTranslate
    static glm::vec3 deltaTranslate = {};
    deltaTranslate = position;
    transformUpdated |= ImGui::DragFloat3("Translation", &deltaTranslate[0], 0.01f);
    ImGui::DragDisableCursor(scene_context.window);

    transformUpdated |= ImGui::Checkbox("[Link]", &linkTransforms);

   //check if the transforms need to be linked
    static glm::vec3 newDirection = direction;
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
    glm::mat4 transform_dir = glm::translate(direction);

    //get the object origin
    glm::mat4 model_origin = glm::mat4(         1, 0, 0, 0,
                                                0, 1, 0, 0,
                                                0, 0, 1, 0,
                                                position.x, position.y, position.z, 1);

    glm::mat4 model_orientation = glm::inverse(glm::lookAt(
            position,
            direction,
            glm::vec3(0, 1, 0)
            ));

    if (!EditorScene::is_null(parent)) {
        // Post multiply by transform
        transform = (*parent)->transform * transform;
        transform_dir = (*parent)->transform * transform_dir;
    }

    /*  reset origin if objects are clipping */
    if(direction.x == position.x && direction.z == position.z) {
        model_orientation = rotate(model_origin, glm::radians(90.0f), glm::vec3(-1, 0, 0));
    }

    light->position = glm::vec3(transform[3]);      // Extract translation from matrix

    //Get the forward direction from the model orientation
    light->direction = glm::vec3(model_orientation[2]); // Extract direction translation from matrix
    if (visible) {
        light_cone->instance_data.model_matrix =  transform * glm::scale(glm::vec3{0.1f * visual_scale}) * model_orientation;
        direction_point->instance_data.model_matrix = transform_dir * glm::scale(glm::vec3{0.1f * visual_scale});
    } else {
        // Throw off to infinity as a hacky way to make model invisible
        light_cone->instance_data.model_matrix = glm::scale(glm::vec3{std::numeric_limits<float>::infinity()}) * glm::translate(glm::vec3{std::numeric_limits<float>::infinity()});
        direction_point->instance_data.model_matrix = glm::scale(glm::vec3{std::numeric_limits<float>::infinity()}) * glm::translate(glm::vec3{std::numeric_limits<float>::infinity()});
    }

    glm::vec3 normalised_colour = glm::vec3(light->colour) / glm::compMax(glm::vec3(light->colour));
    light_cone->instance_data.material.emission_tint = glm::vec4(normalised_colour, light_cone->instance_data.material.emission_tint.a);
}

/*  get the element type name
 */
const char* EditorScene::DirectionalLightElement::element_type_name() const {
    return ELEMENT_TYPE_NAME;
}
