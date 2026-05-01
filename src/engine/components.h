#pragma once

#include <renderer.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// ---------------------------------------------------------------------------
// Core components
// ---------------------------------------------------------------------------

struct Transform {
    glm::vec3 position = {0.f, 0.f, 0.f};
    glm::quat rotation = glm::quat(1.f, 0.f, 0.f, 0.f);  // identity
    glm::vec3 scale    = {1.f, 1.f, 1.f};
};

struct Mesh {
    RendererMeshHandle handle = {};  // {0} = no mesh; draw is skipped
};

struct EntityMaterial {
    Material mat = {};  // renderer Material value — shading model, base_color, etc.
};

struct RigidBody {
    float     mass             = 1.0f;
    float     inv_mass         = 1.0f;   // 1/mass; precomputed at spawn
    glm::vec3 linear_velocity  = {0.f, 0.f, 0.f};
    glm::vec3 angular_velocity = {0.f, 0.f, 0.f};  // world-space ω (rad/s)
    glm::mat3 inv_inertia_body = glm::mat3(1.f);    // body-space I⁻¹; constant after spawn
    glm::mat3 inv_inertia      = glm::mat3(1.f);    // world-space I⁻¹; updated each substep
    float     restitution      = 1.0f;               // 1.0 = perfectly elastic
};

struct Collider {
    glm::vec3 half_extents = {0.5f, 0.5f, 0.5f};  // local-space AABB half-sizes
};

struct Camera {
    float fov        = 60.0f;   // vertical FOV in degrees
    float near_plane = 0.1f;
    float far_plane  = 1000.0f;
};

struct Light {
    glm::vec3 direction = {0.f, -1.f, 0.f};  // world-space, toward light source
    glm::vec3 color     = {1.f, 1.f, 1.f};   // linear RGB
    float     intensity = 1.0f;
};

// ---------------------------------------------------------------------------
// Tag components — presence on entity is the semantic, no data
// ---------------------------------------------------------------------------

struct Static {};         // Excluded from physics integration; still collidable
struct Dynamic {};        // Under physics simulation (expects RigidBody + Collider)
struct Interactable {};   // Visible to raycast() and overlap_aabb() queries
struct CameraActive {};   // Marks the one active camera entity per scene
struct DestroyPending {}; // Queued for end-of-tick registry.destroy() sweep
struct SpawnTarget {};    // Spawner-created entity (for stress-test cleanup)

struct CollisionFlash {
    float              timer       = 0.f;
    glm::vec3          flash_color = {1.f, 0.15f, 0.15f};
    glm::vec3          base_color  = {1.f, 1.f, 1.f};
};

struct OutOfBounds {}; // Debug flag — entity found outside arena bounds after physics tick
