#include "renderer.h"

int main() {
    RendererConfig cfg;
    renderer_init(cfg);
    renderer_run();
    return 0;
}
