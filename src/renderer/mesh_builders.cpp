#include "sokol_gfx.h"
#include "mesh_builders.h"
#include "renderer.h"

#include <cstdio>
#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// Module-level mesh store — index 0 reserved (invalid handle id == 0)
// ---------------------------------------------------------------------------

namespace {

static sg_buffer  s_vbufs[512]        = {};
static sg_buffer  s_ibufs[512]        = {};
static uint32_t   s_index_counts[512] = {};
static uint32_t   s_next_id           = 1;

} // namespace

// ---------------------------------------------------------------------------
// Internal accessors (used by renderer.cpp for draw dispatch)
// ---------------------------------------------------------------------------

sg_buffer mesh_vbuf_get(uint32_t id)        { return s_vbufs[id]; }
sg_buffer mesh_ibuf_get(uint32_t id)        { return s_ibufs[id]; }
uint32_t  mesh_index_count_get(uint32_t id) { return s_index_counts[id]; }

void mesh_store_shutdown() {
    for (uint32_t i = 1; i < s_next_id; ++i) {
        sg_destroy_buffer(s_vbufs[i]);
        sg_destroy_buffer(s_ibufs[i]);
        s_vbufs[i] = {};
        s_ibufs[i] = {};
    }
    s_next_id = 1;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

RendererMeshHandle renderer_upload_mesh(
    const Vertex*   vertices,
    uint32_t        vertex_count,
    const uint32_t* indices,
    uint32_t        index_count)
{
    if (s_next_id >= 512) {
        printf("[renderer] ERROR: mesh store full (max 511 meshes)\n");
        return {0};
    }

    sg_buffer_desc vdesc = {};
    vdesc.usage.vertex_buffer = true;
    vdesc.usage.immutable     = true;
    vdesc.data  = { vertices, vertex_count * sizeof(Vertex) };
    vdesc.label = "mesh-vbuf";

    sg_buffer vbuf = sg_make_buffer(&vdesc);
    if (sg_query_buffer_state(vbuf) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: vertex buffer creation failed\n");
        return {0};
    }

    sg_buffer_desc idesc = {};
    idesc.usage.index_buffer = true;
    idesc.usage.immutable    = true;
    idesc.data  = { indices, index_count * sizeof(uint32_t) };
    idesc.label = "mesh-ibuf";

    sg_buffer ibuf = sg_make_buffer(&idesc);
    if (sg_query_buffer_state(ibuf) != SG_RESOURCESTATE_VALID) {
        printf("[renderer] ERROR: index buffer creation failed\n");
        sg_destroy_buffer(vbuf);
        return {0};
    }

    uint32_t id = s_next_id++;
    s_vbufs[id]        = vbuf;
    s_ibufs[id]        = ibuf;
    s_index_counts[id] = index_count;

    return { id };
}

// Stubs — full implementations added in R-021 and R-022
RendererMeshHandle renderer_make_sphere_mesh(float radius, int subdivisions) {
    // UV sphere: (subdivisions+1)^2 vertices, subdivisions^2 * 6 indices (triangle list).
    // Matches R-053 expected counts: vertex=(s+1)*(s+1), index=s*s*6.
    int stacks = subdivisions;
    int slices  = subdivisions;

    int vert_count  = (stacks + 1) * (slices + 1);
    int index_count = stacks * slices * 6;

    std::vector<Vertex>   verts(vert_count);
    std::vector<uint32_t> indices(index_count);

    // Vertices — row-major: lat outer, lon inner
    for (int lat = 0; lat <= stacks; ++lat) {
        float theta     = lat * static_cast<float>(M_PI) / stacks; // [0, PI]
        float sin_theta = sinf(theta);
        float cos_theta = cosf(theta);

        for (int lon = 0; lon <= slices; ++lon) {
            float phi     = lon * 2.0f * static_cast<float>(M_PI) / slices; // [0, 2PI]
            float sin_phi = sinf(phi);
            float cos_phi = cosf(phi);

            // Unit sphere position (also the normal for a sphere)
            float nx = sin_theta * cos_phi;
            float ny = cos_theta;
            float nz = sin_theta * sin_phi;

            Vertex& v = verts[lat * (slices + 1) + lon];
            v.position[0] = radius * nx;
            v.position[1] = radius * ny;
            v.position[2] = radius * nz;

            v.normal[0] = nx;
            v.normal[1] = ny;
            v.normal[2] = nz;

            // UV: longitude → U [0,1], latitude → V [0,1]
            v.uv[0] = static_cast<float>(lon) / slices;
            v.uv[1] = static_cast<float>(lat) / stacks;

            // Tangent = normalize(cross(up=(0,1,0), normal))
            // cross((0,1,0),(nx,ny,nz)) = (nz, 0, -nx)
            float tx = nz, tz = -nx;
            float tlen = sqrtf(tx * tx + tz * tz); // ty == 0
            if (tlen > 1e-6f) {
                v.tangent[0] = tx / tlen;
                v.tangent[1] = 0.0f;
                v.tangent[2] = tz / tlen;
            } else {
                // Pole fallback
                v.tangent[0] = 1.0f;
                v.tangent[1] = 0.0f;
                v.tangent[2] = 0.0f;
            }
        }
    }

    // Indices — two triangles per quad (CCW winding)
    int idx = 0;
    for (int lat = 0; lat < stacks; ++lat) {
        for (int lon = 0; lon < slices; ++lon) {
            uint32_t tl = static_cast<uint32_t>(lat * (slices + 1) + lon);
            uint32_t tr = tl + 1;
            uint32_t bl = static_cast<uint32_t>((lat + 1) * (slices + 1) + lon);
            uint32_t br = bl + 1;

            indices[idx++] = tl;
            indices[idx++] = bl;
            indices[idx++] = tr;

            indices[idx++] = tr;
            indices[idx++] = bl;
            indices[idx++] = br;
        }
    }

    return renderer_upload_mesh(verts.data(),
                                static_cast<uint32_t>(vert_count),
                                indices.data(),
                                static_cast<uint32_t>(index_count));
}

RendererMeshHandle renderer_make_cube_mesh(float half_extent) {
    float h = half_extent;

    // Each face: 4 corners (BL, BR, TR, TL), flat normal, face-local-X tangent.
    struct FaceData {
        float p[4][3]; // positions
        float n[3];    // normal
        float t[3];    // tangent (U = face local X)
    };

    const FaceData faces[6] = {
        // +X
        { {{ h,-h, h},{ h,-h,-h},{ h, h,-h},{ h, h, h}}, { 1, 0, 0}, { 0, 0,-1} },
        // -X
        { {{-h,-h,-h},{-h,-h, h},{-h, h, h},{-h, h,-h}}, {-1, 0, 0}, { 0, 0, 1} },
        // +Y
        { {{-h, h, h},{ h, h, h},{ h, h,-h},{-h, h,-h}}, { 0, 1, 0}, { 1, 0, 0} },
        // -Y
        { {{-h,-h,-h},{ h,-h,-h},{ h,-h, h},{-h,-h, h}}, { 0,-1, 0}, { 1, 0, 0} },
        // +Z
        { {{-h,-h, h},{ h,-h, h},{ h, h, h},{-h, h, h}}, { 0, 0, 1}, { 1, 0, 0} },
        // -Z
        { {{ h,-h,-h},{-h,-h,-h},{-h, h,-h},{ h, h,-h}}, { 0, 0,-1}, {-1, 0, 0} },
    };

    // UV per corner: BL=(0,0), BR=(1,0), TR=(1,1), TL=(0,1)
    const float uvs[4][2] = { {0.0f,0.0f}, {1.0f,0.0f}, {1.0f,1.0f}, {0.0f,1.0f} };

    Vertex    verts[24];
    uint32_t  indices[36];

    for (int f = 0; f < 6; ++f) {
        for (int v = 0; v < 4; ++v) {
            Vertex& vert    = verts[f * 4 + v];
            vert.position[0] = faces[f].p[v][0];
            vert.position[1] = faces[f].p[v][1];
            vert.position[2] = faces[f].p[v][2];
            vert.normal[0]   = faces[f].n[0];
            vert.normal[1]   = faces[f].n[1];
            vert.normal[2]   = faces[f].n[2];
            vert.uv[0]       = uvs[v][0];
            vert.uv[1]       = uvs[v][1];
            vert.tangent[0]  = faces[f].t[0];
            vert.tangent[1]  = faces[f].t[1];
            vert.tangent[2]  = faces[f].t[2];
        }

        uint32_t base    = static_cast<uint32_t>(f * 4);
        uint32_t* idx    = &indices[f * 6];
        idx[0] = base;     idx[1] = base + 2; idx[2] = base + 1; // tri 0 (CCW)
        idx[3] = base;     idx[4] = base + 3; idx[5] = base + 2; // tri 1 (CCW)
    }

    return renderer_upload_mesh(verts, 24, indices, 36);
}
