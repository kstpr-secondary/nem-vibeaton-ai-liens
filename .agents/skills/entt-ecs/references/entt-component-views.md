# EnTT — Component Views and Groups

Views and groups are the primary way to write systems that iterate over entities.

---

## Views

Views are non-owning and represent entities that have all of the specified components. They are cheap to create — no allocation, no sorting.

### Include & Exclude Filtering

```cpp
// All entities with Position, Velocity, but NOT HiddenTag
auto view = registry.view<Position, Velocity>(entt::exclude<HiddenTag>);

// Structured bindings (preferred, C++17)
for (auto [entity, pos, vel] : view.each()) {
    pos.x += vel.dx;
}

// Lambda form (equivalent)
view.each([](auto entity, auto& pos, auto& vel) {
    // ...
});
```

### Single-Component Access from a View

```cpp
for (auto entity : view) {
    // More efficient than get<T>(entity) on the registry when you only need one component:
    auto& pos = view.get<Position>(entity);
}
```

### Multi-Component Structured Get

```cpp
auto [pos, vel] = registry.get<Position, Velocity>(entity);
```

---

## Groups

Groups are for high-performance contiguous memory access. They "own" one or more component types and reorder storage so iteration is cache-friendly.

```cpp
// Define a group (expensive to create — do this once, not per-tick)
auto group = registry.group<Position, Velocity>();

for (auto [entity, pos, vel] : group.each()) {
    // Guaranteed contiguous memory for both components
}
```

**When to use groups:** physics integration or rendering loops over thousands of entities where cache misses dominate. For MVP, views are sufficient everywhere.

---

## Sorting

Useful for rendering order (transparency, UI layers):

```cpp
// Sort component storage globally
registry.sort<Renderable>([](const auto& lhs, const auto& rhs) {
    return lhs.depth < rhs.depth; // back-to-front
});

// Sort within a view specifically
auto view = registry.view<Renderable>();
view.sort<Renderable>([](const auto& lhs, const auto& rhs) {
    return lhs.depth < rhs.depth;
});
```
