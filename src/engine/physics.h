#pragma once

#include "components.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// Force accumulator — per-entity, cleared after each substep
// ---------------------------------------------------------------------------

struct ForceAccum {
    glm::vec3 force  = glm::vec3(0.0f);
    glm::vec3 torque = glm::vec3(0.0f);
};

// ---------------------------------------------------------------------------
// Inertia tensor helpers
// ---------------------------------------------------------------------------

// Compute body-space inverse inertia for a solid box (AABB half-extents)
glm::mat3 make_box_inv_inertia_body(float mass, glm::vec3 half_extents);

// Compute body-space inverse inertia for a solid sphere
glm::mat3 make_sphere_inv_inertia_body(float mass, float radius);

// Update world-space inverse inertia from body-space tensor and current rotation
void update_world_inertia(RigidBody& rb, const glm::quat& rotation,
                          const glm::mat3& inv_inertia_body);

// ---------------------------------------------------------------------------
// Integration
// ---------------------------------------------------------------------------

// Semi-implicit Euler: velocity first, then position
void integrate_linear(Transform& tr, RigidBody& rb, const glm::vec3& force_accum, float h);

// Angular integration with quaternion renormalization
void integrate_angular(Transform& tr, RigidBody& rb, const glm::vec3& torque_accum, float h);

// ---------------------------------------------------------------------------
// Physics substep loop
// ---------------------------------------------------------------------------

// Run one fixed-timestep physics substep
void physics_substep(entt::registry& reg, float h);

// Main physics tick: accumulator loop with dt-cap
void physics_system_tick(entt::registry& reg, float dt, float physics_hz, float dt_cap);
