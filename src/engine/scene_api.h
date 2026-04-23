#pragma once

// Flush all entities tagged DestroyPending.
// Called at end of engine_tick() to avoid mid-iteration invalidation.
void scene_flush_pending_destroys();
