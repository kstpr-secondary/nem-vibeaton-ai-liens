# EnTT Component Views and Groups

Views and groups are used to iterate over entities and their components.

## 1. Views

Views are non-owning and represent entities that have all of the specified components.

### Filtering (Include & Exclude)
```cpp
// All entities with Position, Velocity, but NOT HiddenTag
auto view = registry.view<Position, Velocity>(entt::exclude<HiddenTag>);

// Iterate via each
view.each([](auto entity, auto &pos, auto &vel) {
    // ...
});

// Iterate via structured bindings
for(auto [entity, pos, vel] : view.each()) {
    // ...
}
```

### Retrieval from View
```cpp
for(auto entity : view) {
    // More efficient if you only need one of the components
    auto &pos = view.get<Position>(entity);
}
```

## 2. Groups

Groups are for high-performance contiguous access. They "own" the components and reorder storage.

```cpp
// Define a group (should be done once or rarely changed)
auto group = registry.group<Position, Velocity>();

for(auto [entity, pos, vel] : group.each()) {
    // Guaranteed contiguous memory
}
```

## 3. Sorting

Useful for rendering order (transparency, UI layers).

```cpp
registry.sort<Renderable>([](const auto &lhs, const auto &rhs) {
    return lhs.depth < rhs.depth; // Back-to-front
});

// Sort a view specifically
auto view = registry.view<Renderable>();
view.sort<Renderable>([](const auto &lhs, const auto &rhs) {
    return lhs.depth < rhs.depth;
});
```

## 4. Multi-Component Get

```cpp
// Structured bindings on retrieval
auto [pos, vel] = registry.get<Position, Velocity>(entity);
```
