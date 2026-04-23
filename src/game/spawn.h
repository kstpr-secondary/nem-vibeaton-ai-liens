#pragma once

#include "components.h"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

entt::entity spawn_player(const glm::vec3& position);

entt::entity spawn_enemy(const glm::vec3& position);

entt::entity spawn_asteroid(const glm::vec3& position,
                              const glm::vec3& linear_velocity,
                              const glm::vec3& angular_velocity,
                              SizeTier tier);

entt::entity spawn_projectile(entt::entity owner,
                                const glm::vec3& position,
                                const glm::vec3& velocity);
