#include "asset_bridge.h"

#include <renderer.h>

#include <cstdio>
#include <vector>

RendererMeshHandle asset_bridge_upload(const ImportedMesh& mesh) {
    if (mesh.empty() || mesh.indices.empty()) {
        fprintf(stderr, "[ENGINE] asset_bridge: empty mesh, skipping upload\n");
        return {};
    }

    size_t n         = mesh.positions.size() / 3;
    bool has_normals = (mesh.normals.size() == n * 3);
    bool has_uvs     = (mesh.uvs.size()     == n * 2);

    std::vector<Vertex> verts(n);
    for (size_t i = 0; i < n; ++i) {
        // Position
        verts[i].position[0] = mesh.positions[i*3 + 0];
        verts[i].position[1] = mesh.positions[i*3 + 1];
        verts[i].position[2] = mesh.positions[i*3 + 2];

        // Normal — Y-up default when absent
        if (has_normals) {
            verts[i].normal[0] = mesh.normals[i*3 + 0];
            verts[i].normal[1] = mesh.normals[i*3 + 1];
            verts[i].normal[2] = mesh.normals[i*3 + 2];
        } else {
            verts[i].normal[0] = 0.f;
            verts[i].normal[1] = 1.f;
            verts[i].normal[2] = 0.f;
        }

        // UV — (0,0) default when absent
        if (has_uvs) {
            verts[i].uv[0] = mesh.uvs[i*2 + 0];
            verts[i].uv[1] = mesh.uvs[i*2 + 1];
        } else {
            verts[i].uv[0] = 0.f;
            verts[i].uv[1] = 0.f;
        }

        // Tangent — zero-filled (not used at MVP shading level)
        verts[i].tangent[0] = 0.f;
        verts[i].tangent[1] = 0.f;
        verts[i].tangent[2] = 0.f;
    }

    RendererMeshHandle h = renderer_upload_mesh(
        verts.data(),
        static_cast<uint32_t>(n),
        mesh.indices.data(),
        static_cast<uint32_t>(mesh.indices.size())
    );

    if (!renderer_handle_valid(h))
        fprintf(stderr, "[ENGINE] asset_bridge: renderer_upload_mesh returned invalid handle\n");

    return h;
}
