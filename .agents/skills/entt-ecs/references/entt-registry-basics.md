# EnTT — Registry Basics

The `entt::registry` is the central hub for all ECS data.

---

## Singletons (Context Variables)

Store global data like Time, Config, or Camera in the registry context instead of using global variables.

```cpp
// Store
registry.ctx().emplace<GameConfig>(1920, 1080);

// Retrieve (reference; throws if not present)
auto& config = registry.ctx().get<GameConfig>();

// Safe retrieve (returns pointer; nullptr if not present)
auto* cfg = registry.ctx().find<GameConfig>();
```

---

## Mass Operations

```cpp
// Clear all instances of a specific component (remove from all entities)
registry.clear<TemporaryTag>();

// Destroy all entities and all components
registry.clear();

// Check if an entity ID is valid and currently alive
if (registry.valid(entity)) { ... }

// Count living entities
size_t total = registry.alive();
```

---

## Handles

Handles are useful for passing an "object" around that contains both the entity ID and its registry.

```cpp
entt::handle h = { registry, registry.create() };

// Construct component in place
h.emplace<Position>(10.f, 10.f, 0.f);

// Safe check and retrieval
if (auto* vel = h.try_get<Velocity>()) {
    vel->x += 1.0f;
}

// Destroy entity via handle
h.destroy();
```

For engine-internal code, use raw `entt::entity` + registry calls. Reserve handles for helpers that need both entity + registry without passing them separately.

---

## Component Storage Access

```cpp
auto& storage = registry.storage<Position>();
// storage.each(callback)  — iterate all entities with this component
// storage.contains(entity) — check presence without creating a view
// storage.size()           — count without a view
```

Useful for bulk introspection or serialization; not needed for normal gameplay code.
