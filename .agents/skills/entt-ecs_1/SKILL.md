---
name: entt-ecs
description: Use when working with the EnTT ECS library for entity management, component storage, and system iteration. Covers registry usage, views/groups for high-performance iteration, and reactive signals. Activated when implementing game engine logic, scene management, or gameplay systems.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen).
metadata:
  author: hackathon-team
  version: "1.0"
  project-stage: pre-implementation
  role: library-reference
  activated-by: usage of entt::registry or entt::entity
---

# entt-ecs Skill

Reference for **EnTT 4.x**. A fast and light Entity Component System (ECS).

Use per-aspect references under `references/` for deeper detail on registry operations, views, and lifecycle events.

---

## 1. Core Rules (must know)

1. **Registry is the World.** Almost all operations happen via `entt::registry`.
2. **Entities are IDs.** `entt::entity` is a lightweight integer, not a pointer or object.
3. **Components are POD.** Keep logic out of components; keep data out of systems.
4. **Memory Stability.** Components are **not** stable in memory by default. Never store pointers to components across frames; store `entt::entity` instead.
5. **Iteration is via Views.** Use `registry.view<T>()` for standard iteration. Use `registry.group<T>()` only for performance-critical hotspots.

---

## 2. Basic Operations Cheatsheet

| Task | Pattern |
| :--- | :--- |
| **Create Entity** | `auto entity = registry.create();` |
| **Destroy Entity** | `registry.destroy(entity);` |
| **Add Component** | `registry.emplace<Component>(entity, args...);` |
| **Get Component** | `auto &comp = registry.get<Component>(entity);` |
| **Check Component** | `if (registry.any_of<Component>(entity)) { ... }` |
| **Remove Component** | `registry.remove<Component>(entity);` |

---

## 3. Handle Pattern

Handles wrap an entity and a registry for an object-like interface.

```cpp
entt::handle handle{registry, registry.create()};
handle.emplace<Position>(0, 0, 0);
if (handle.any_of<Velocity>()) {
    auto &vel = handle.get<Velocity>();
}
```

---

## 4. Systems (Views & Groups)

```cpp
// Standard View
auto view = registry.view<Position, Velocity>();
view.each([](auto entity, auto &pos, auto &vel) {
    pos.x += vel.dx;
});

// Performance Group
auto group = registry.group<Position, Velocity>();
for(auto [entity, pos, vel] : group.each()) {
    // contiguous memory access
}
```

---

## 5. Deeper References

For detailed API and patterns, open the relevant reference:

| Aspect | File | When to open |
| :--- | :--- | :--- |
| Registry Basics | `references/registry-basics.md` | Context variables (singletons), handles, and mass operations |
| Component Views | `references/component-views.md` | Complex filters, exclusions, sorting, and groups |
| Entity Lifecycle | `references/entity-lifecycle.md` | Construction/destruction signals and change tracking |
