#pragma once

#include "components.h"
#include "collider.h"

#include <entt/entt.hpp>
#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// Impulse-based elastic collision response
// ---------------------------------------------------------------------------

// Resolve a single collision between two dynamic bodies
void resolve_collision(Transform& tr_a, RigidBody& rb_a,
                       Transform& tr_b, RigidBody& rb_b,
                       const CollisionPair& contact);

// Positional correction to prevent sinking (Baumgarte)
void positional_correction(Transform& tr_a, const RigidBody& rb_a,
                           Transform& tr_b, const RigidBody& rb_b,
                           const CollisionPair& contact);

// ---------------------------------------------------------------------------
// Collision resolution pipeline
// ---------------------------------------------------------------------------

// Resolve all collisions in the registry using detect_collisions() + response
void resolve_all_collisions(entt::registry& reg);
