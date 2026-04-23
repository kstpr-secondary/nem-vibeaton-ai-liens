# EnTT Entity Lifecycle and Signals

Use signals for reactive programming: doing work only when something changes.

## 1. Lifecycle Signals

Connect functions to construction, destruction, or update events.

```cpp
// Construction: Called when emplace/emplace_or_replace creates a component
registry.on_construct<Mesh>().connect<&OnMeshAdded>();

// Destruction: Called when remove or destroy deletes a component
registry.on_destroy<Mesh>().connect<&OnMeshRemoved>();

// Update: Called when replace or patch modifies a component
registry.on_update<Position>().connect<&OnPositionUpdated>();

void OnMeshAdded(entt::registry &reg, entt::entity entity) {
    auto &mesh = reg.get<Mesh>(entity);
    // e.g. Upload to GPU
}
```

## 2. Structured Modifications (Patch)

Use `patch` to ensure `on_update` signals are triggered correctly.

```cpp
registry.patch<Position>(entity, [](auto &pos) {
    pos.x += 1.0f; // Triggers on_update<Position>
});
```

## 3. Change Tracking (Observers)

If you need to process all entities that changed since the last frame.

```cpp
// 1. Setup a reactive storage (observer)
auto &observer = registry.storage<entt::reactive>("physics_updates"_hs);
observer.on_update<Position>();

// 2. In your system:
for(auto entity : observer) {
    // Only entities whose Position was patched/replaced
}

// 3. Clear for next frame
observer.clear();
```

## 4. Entity Destruction

`registry.destroy(entity)` removes the entity ID and triggers `on_destroy` for **every** component attached to it.

```cpp
// Safe destruction
if (registry.valid(entity)) {
    registry.destroy(entity);
}
```
