# EnTT Registry Basics

The `entt::registry` is the central hub for all ECS data.

## 1. Singletons (Context Variables)

Store global data like Time, Config, or Camera in the registry context.

```cpp
// Store
registry.ctx().emplace<GameConfig>(1920, 1080);

// Retrieve
auto &config = registry.ctx().get<GameConfig>();
```

## 2. Mass Operations

Efficiently handle multiple entities or component types.

```cpp
// Clear all instances of a specific component
registry.clear<TemporaryTag>();

// Destroy all entities
registry.clear();

// Check if an entity ID is valid and currently "alive"
if (registry.valid(entity)) { ... }
```

## 3. Handles

Handles are useful for passing an "object" around that contains both the entity ID and its registry.

```cpp
entt::handle entity = { registry, registry.create() };

// Construct component
entity.emplace<Position>(10.f, 10.f);

// Safe check and retrieval
if (auto* vel = entity.try_get<Velocity>()) {
    vel->x += 1.0f;
}

// Destroy entity via handle
entity.destroy();
```

## 4. Component Storage Pointers

While component memory isn't stable, you can get the raw storage for mass operations if needed (rare for hackathon).

```cpp
auto &storage = registry.storage<Position>();
// storage.each(...), storage.contains(entity), etc.
```
