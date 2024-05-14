#include "Lights.h"

#include <algorithm>

std::vector<PointLight> LightScene::get_nearest_point_lights(glm::vec3 target, size_t max_count, size_t min_count) const {
    if (point_lights.size() <= max_count) {
        // No need to store if we are just going to return them all anyway.

        std::vector<PointLight> result{};
        result.reserve(std::max(point_lights.size(), min_count));

        for (const auto& point_light: point_lights) {
            result.push_back(*point_light);
        }

        while (result.size() < min_count) {
            result.push_back(PointLight::off());
        }

        return result;
    }

    size_t result_count = std::min(point_lights.size(), max_count);

    std::vector<std::pair<float, PointLight>> sorted_vector{};
    sorted_vector.reserve(point_lights.size());

    for (const auto& point_light: point_lights) {
        glm::vec3 diff = point_light->position - target;
        // dot(a, a) == |a|^2, but doesn't require a sqrt()
        // and since (|a| < |b|) <-> (|a|^2 < |b|^2) so sorting works with squares
        float distance_squared = glm::dot(diff, diff);
        sorted_vector.emplace_back(distance_squared, *point_light);
    }

    // Can use partial_sort to just get the `result_count` smallest, best used with smallish max_count
    std::partial_sort(sorted_vector.begin(), sorted_vector.begin() + (long) result_count, sorted_vector.end(), [](const std::pair<float, PointLight>& lhs, const std::pair<float, PointLight>& rhs) -> bool {
        return lhs.first < rhs.first;
    });

    std::vector<PointLight> result{};
    result.reserve(std::max(result_count, min_count));
    for (auto i = 0u; i < result_count; ++i) {
        result.push_back(sorted_vector[i].second);
    }
    while (result.size() < min_count) {
        result.push_back(PointLight::off());
    }
    return result;
}

/* Its virtually the same function but with some new pizazz.
 */
std::vector<DirectionalLight> LightScene::get_nearest_directional_lights(glm::vec3 target, size_t max_count, size_t min_count) const {
    if (directional_lights.size() <= max_count) {
        // No need to store if we are just going to return them all anyway.

        std::vector<DirectionalLight> result{};
        result.reserve(std::max(directional_lights.size(), min_count));

        // FOR DIRECTIONAL_LIGHTS
        for (const auto& directional_light: directional_lights) {
            result.push_back(*directional_light);
        }

        while (result.size() < min_count) {
            result.push_back(DirectionalLight::off());
        }

        return result;
    }

    size_t result_count = std::min(directional_lights.size(), max_count);

    std::vector<std::pair<float, DirectionalLight>> sorted_vector{};
    sorted_vector.reserve(directional_lights.size());

    //DIRECTIONAL_LIGHTS
    for (const auto& directional_light: directional_lights) {
        glm::vec3 diff = directional_light->position - target;
        // dot(a, a) == |a|^2, but doesn't require a sqrt()
        // and since (|a| < |b|) <-> (|a|^2 < |b|^2) so sorting works with squares
        float distance_squared = glm::dot(diff, diff);
        sorted_vector.emplace_back(distance_squared, *directional_light);
    }

    // Can use partial_sort to just get the `result_count` smallest, best used with smallish max_count
    std::partial_sort(sorted_vector.begin(), sorted_vector.begin() + (long) result_count, sorted_vector.end(), [](const std::pair<float, DirectionalLight>& lhs, const std::pair<float, DirectionalLight>& rhs) -> bool {
        return lhs.first < rhs.first;
    });

    std::vector<DirectionalLight> result{};
    result.reserve(std::max(result_count, min_count));
    for (auto i = 0u; i < result_count; ++i) {
        result.push_back(sorted_vector[i].second);
    }
    while (result.size() < min_count) {
        result.push_back(DirectionalLight::off());
    }

    return result;

}

template<typename Light>
std::vector<Light> LightScene::get_nearest_lights(const std::unordered_set<std::shared_ptr<Light>>& lights, glm::vec3 target, size_t max_count, size_t min_count) {
    if (lights.size() <= max_count) {
        // No need to store if we are just going to return them all anyway.

        std::vector<Light> result{};
        result.reserve(std::max(lights.size(), min_count));

        for (const auto& point_light: lights) {
            result.push_back(*point_light);
        }

        while (result.size() < min_count) {
            result.push_back(Light::off());
        }

        return result;
    }

    size_t result_count = std::min(lights.size(), max_count);

    std::vector<std::pair<float, Light>> sorted_vector{};
    sorted_vector.reserve(lights.size());

    for (const auto& point_light: lights) {
        glm::vec3 diff = point_light->position - target;
        // dot(a, a) == |a|^2, but doesn't require a sqrt()
        // and since (|a| < |b|) <-> (|a|^2 < |b|^2) so sorting works with squares
        float distance_squared = glm::dot(diff, diff);
        sorted_vector.emplace_back(distance_squared, *point_light);
    }

    // Can use partial_sort to just get the `result_count` smallest, best used with smallish max_count
    std::partial_sort(sorted_vector.begin(), sorted_vector.begin() + (long) result_count, sorted_vector.end(), [](const std::pair<float, Light>& lhs, const std::pair<float, Light>& rhs) -> bool {
        return lhs.first < rhs.first;
    });

    std::vector<Light> result{};
    result.reserve(std::max(result_count, min_count));
    for (auto i = 0u; i < result_count; ++i) {
        result.push_back(sorted_vector[i].second);
    }
    while (result.size() < min_count) {
        result.push_back(Light::off());
    }
    return result;
}
