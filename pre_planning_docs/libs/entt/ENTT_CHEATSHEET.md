# EnTT ECS Cheatsheet (Hackathon Edition)

This guide provides a rapid reference for **EnTT 4.x** (Entity Component System) specifically tailored for the **Vibe Coding Hackathon** (3D Space Shooter project).

---

## 🚀 1. Core Concepts

| Concept | Description |
| :--- | :--- |
| **Registry** | The "world" or "database" that stores all entities and components. |
| **Entity** | A lightweight identifier (`entt::entity`). Not an object. |
| **Component** | Plain Old Data (POD) structs attached to entities. No logic. |
| **System** | Logic that iterates over entities with specific components. |
| **Handle** | A convenient wrapper around an entity and a registry. |

---

## 🏗️ 2. Registry & Entity Management

### Basic Operations
```cpp
entt::registry registry;

// Create a naked entity
auto entity = registry.create();

// Check if valid
if (registry.valid(entity)) { /* ... */ }

// Destroy entity (removes all components)
registry.destroy(entity);

// Clear the entire registry
registry.clear();
```

### Handles (The "Object-like" approach)
Handles are useful for passing "entities" around without losing the registry reference.
```cpp
entt::handle entity = { registry, registry.create() };

// Use like an object:
entity.emplace<Position>(0.0f, 0.0f, 0.0f);
if (entity.any_of<Velocity>()) {
    auto &vel = entity.get<Velocity>();
}
entity.destroy();
```

---

## 📦 3. Component Lifecycle

### Adding & Removing
```cpp
// Emplace (Construct in place)
registry.emplace<Position>(entity, 10.0f, 20.0f, 0.0f);

// Emplace or Replace (Safe way to ensure component exists)
registry.emplace_or_replace<Health>(entity, 100);

// Remove
registry.remove<Collision>(entity);

// Clear all instances of a component type
registry.clear<EnemyAI>();
```

### Retrieval & Checking
```cpp
// Check existence
if (registry.any_of<Shield>(entity)) { ... }

// Single retrieval
auto &pos = registry.get<Position>(entity);

// Multi-retrieval (structured bindings)
auto [pos, vel] = registry.get<Position, Velocity>(entity);

// Try get (returns pointer or nullptr)
if (auto *shields = registry.try_get<Shield>(entity)) {
    shields->amount -= damage;
}
```

### Modification
```cpp
// Direct update
registry.get<Health>(entity).value -= 10;

// Patch (Trigger update signals)
registry.patch<Position>(entity, [](auto &pos) {
    pos.x += 1.0f;
});
```

---

## 🔍 4. Iteration & Systems (Views)

Views are the primary way to write systems. They are extremely fast.

### Simple View (Read/Write)
```cpp
auto view = registry.view<Position, Velocity>();

// Lambda iteration (recommended)
view.each([](auto entity, auto &pos, auto &vel) {
    pos.value += vel.value * deltaTime;
});

// For-loop iteration
for(auto [entity, pos, vel] : view.each()) {
    // ...
}
```

### Filtering (Exclude)
```cpp
// Find entities with Position but NOT Velocity (e.g., static obstacles)
auto view = registry.view<Position>(entt::exclude<Velocity>);
```

### Groups (Extreme Performance)
Use groups for high-frequency systems (Physics/Rendering) to force component storage to be perfectly contiguous.
```cpp
// Define a group that owns Position and Velocity
auto group = registry.group<Position, Velocity>();

for(auto [entity, pos, vel] : group.each()) {
    // ...
}
```

---

## 📡 5. Reactive Programming (Signals)

Great for one-time setup (e.g., uploading mesh to GPU when `Mesh` is added).

```cpp
// Setup (do this once)
registry.on_construct<Mesh>().connect<&OnMeshAdded>();
registry.on_destroy<Mesh>().connect<&OnMeshRemoved>();

void OnMeshAdded(entt::registry &reg, entt::entity entity) {
    auto &mesh = reg.get<Mesh>(entity);
    // Upload mesh to sokol_gfx...
}
```

---

## 🌌 6. Space Shooter Scenarios

### Collision System (AABB vs AABB)
```cpp
void CollisionSystem(entt::registry &registry) {
    auto view = registry.view<Position, CollisionBox>();
    
    for(auto entityA : view) {
        auto &posA = view.get<Position>(entityA);
        auto &boxA = view.get<CollisionBox>(entityA);
        
        for(auto entityB : view) {
            if (entityA == entityB) continue;
            
            auto &posB = view.get<Position>(entityB);
            auto &boxB = view.get<CollisionBox>(entityB);
            
            if (CheckAABB(posA, boxA, posB, boxB)) {
                // Trigger event or handle damage
            }
        }
    }
}
```

### Spawning an Asteroid (Prefab Pattern)
```cpp
entt::entity SpawnAsteroid(entt::registry &registry, glm::vec3 pos) {
    auto entity = registry.create();
    registry.emplace<Position>(entity, pos);
    registry.emplace<Velocity>(entity, RandomDir() * 5.0f);
    registry.emplace<Rotation>(entity, 0.0f, 1.0f, 0.0f);
    registry.emplace<Health>(entity, 50);
    registry.emplace<AsteroidTag>(entity);
    registry.emplace<Mesh>(entity, "asteroid_lowpoly.obj");
    return entity;
}
```

---

## ⚡ 7. Performance & Advanced Tips

### Context Variables (Singletons)
Store global game state (Time, Input, Camera) in the registry's context.
```cpp
// Store
registry.ctx().emplace<GameConfig>(1920, 1080);

// Retrieve
auto &config = registry.ctx().get<GameConfig>();
```

### Sorting
If you need to render back-to-front (Transparency):
```cpp
registry.sort<Renderable>([](const auto &lhs, const auto &rhs) {
    return lhs.depth < rhs.depth;
});
```

### Fast Entity Check
```cpp
if (registry.valid(entity) && registry.all_of<Position>(entity)) { ... }
```

### Reactive Storage (Change Tracking)
If you only want to process things that *changed*:
```cpp
// Setup a reactive storage (observer)
auto &observer = registry.storage<entt::reactive>("physics_updates"_hs);
observer.on_update<Position>();

// In your system:
for(auto entity : observer) {
    // Only entities whose Position was patched since last clear
}
observer.clear();
```

---

## ⚠️ Common Pitfalls

1. **Memory Stability:** Components are **not** stable in memory unless using `entt::storage<T>::pointer_stable`. Don't store pointers to components; store `entt::entity` and fetch them.
2. **Deletion while Iterating:** It is safe to destroy the *current* entity being iterated in a `view.each()`, but avoid creating/destroying *other* entities of the same view type during the loop.
3. **Empty Components:** Tagging entities (e.g., `struct PlayerTag {};`) is free. Use them liberally for filtering!
