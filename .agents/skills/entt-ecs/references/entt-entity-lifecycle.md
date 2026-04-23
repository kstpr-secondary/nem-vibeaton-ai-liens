# EnTT — Entity Lifecycle Edge Cases

---

## Handle Reuse After `destroy`

The same `entt::entity` integer value may be reassigned to a new entity after `registry.destroy(e)`. The version bits in the handle make old IDs invalid, but stale entity variables remain dangerous:

```cpp
registry.destroy(e);
// e still holds the old integer — registry.valid(e) now returns false.
// Never store entt::entity across ticks without re-validating:
if (registry.valid(e)) { ... }
```

---

## Deferred Destruction Pattern

Never call `registry.destroy()` inside a view iteration for entities of the types in that view — it causes undefined behaviour (skipped or double-processed entities). Queue destroys for end-of-tick:

```cpp
// Mark during iteration (DestroyPending is an empty tag)
registry.view<Health>().each([&](auto e, Health& hp) {
    if (hp.value <= 0) registry.emplace_or_replace<DestroyPending>(e);
});

// Apply after all views are done
auto dead = registry.view<DestroyPending>();
std::vector<entt::entity> to_destroy(dead.begin(), dead.end());
for (auto e : to_destroy) registry.destroy(e);
```

---

## Reactive Signals

Connect callbacks to construction, destruction, or patch events for reactive one-time setup (e.g., upload mesh to GPU when a `Mesh` component is added).

```cpp
// Called when emplace / emplace_or_replace creates a component
registry.on_construct<Mesh>().connect<&OnMeshAdded>();

// Called when remove / destroy deletes a component
registry.on_destroy<Mesh>().connect<&OnMeshRemoved>();

// Called when replace or patch modifies a component
registry.on_update<Position>().connect<&OnPositionUpdated>();

void OnMeshAdded(entt::registry& reg, entt::entity entity) {
    auto& mesh = reg.get<Mesh>(entity);
    // e.g., upload to GPU via sokol_gfx
}
```

Use `patch` to ensure `on_update` signals fire correctly:
```cpp
registry.patch<Position>(entity, [](auto& pos) {
    pos.x += 1.0f;  // triggers on_update<Position>
});
```

---

## Change Tracking (Observers)

Process only entities whose component changed since the last frame, without iterating the whole view:

```cpp
// 1. Setup a reactive storage (observer) — do this once
auto& observer = registry.storage<entt::reactive>("physics_updates"_hs);
observer.on_update<Position>();

// 2. In your system, iterate only changed entities
for (auto entity : observer) {
    // Only entities whose Position was patched/replaced since last clear
}

// 3. Clear for next frame
observer.clear();
```

---

## `destroy` Triggers All `on_destroy` Signals

`registry.destroy(entity)` removes the entity ID and triggers `on_destroy` for **every** component attached to it. No manual per-component cleanup needed if signals are wired correctly.

```cpp
// Always guard with valid() when the entity ID comes from external code
if (registry.valid(entity)) {
    registry.destroy(entity);
}
```
