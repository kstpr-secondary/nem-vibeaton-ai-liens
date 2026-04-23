#pragma once

// Read the CameraActive entity, compute view + projection matrices, and push
// them to renderer_set_camera(). Called once per engine_tick().
void camera_update();
