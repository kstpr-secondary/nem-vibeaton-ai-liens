#include "projectile.h"
#include "components.h"
#include <engine.h>

void projectile_update(float /*dt*/) {
    auto& reg  = engine_registry();
    auto  view = reg.view<ProjectileTag, ProjectileData>();

    for (auto e : view) {
        // Skip entities already queued for destruction.
        if (reg.all_of<DestroyPending>(e))
            continue;

        const auto& pd = view.get<ProjectileData>(e);
        const double age = engine_now() - pd.spawn_time;

        if (age > static_cast<double>(pd.lifetime))
            reg.emplace<DestroyPending>(e);
    }
}
