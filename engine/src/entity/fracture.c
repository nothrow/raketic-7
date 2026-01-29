#include "fracture.h"
#include "entity.h"
#include "particles.h"
#include "platform/platform.h"
#include "platform/math.h"
#include "debug/profiler.h"

#include <Windows.h>
#include <gl/GL.h>
#include <immintrin.h>
#include <intrin.h>
#include <string.h>

// Include generated metadata
#include "../generated/models_meta.gen.h"

// Static fragment pool - no malloc
static FragmentPool _fragment_pool;

// Temp buffer for expanded segments during fracture (SoA for SIMD)
typedef struct {
    __declspec(align(32)) float x1[256];
    __declspec(align(32)) float y1[256];
    __declspec(align(32)) float x2[256];
    __declspec(align(32)) float y2[256];
    uint8_t region[256];    // which fragment this segment belongs to
    uint8_t cut_at_p1[256]; // endpoint 1 was created by a cut
    uint8_t cut_at_p2[256]; // endpoint 2 was created by a cut
    int count;
} SegmentBuffer;

static SegmentBuffer _segment_buffer;

// ============================================================================
// Fragment Pool Management
// ============================================================================

void fragment_pool_initialize(void) {
    memset(&_fragment_pool, 0, sizeof(_fragment_pool));
}

int fragment_pool_alloc(void) {
    // Find first free slot using trailing zero count
    uint32_t free_mask = ~_fragment_pool.active_mask;
    if (free_mask == 0) return -1;  // Pool is full

    int idx = (int)_tzcnt_u32(free_mask);
    if (idx >= FRAGMENT_POOL_SIZE) return -1;

    _fragment_pool.active_mask |= (1u << idx);
    _fragment_pool.vertex_counts[idx] = 0;
    return idx;
}

void fragment_pool_free(int pool_idx) {
    if (pool_idx < 0 || pool_idx >= FRAGMENT_POOL_SIZE) return;
    _fragment_pool.active_mask &= ~(1u << pool_idx);
}

int8_t* fragment_get_vertices(int pool_idx) {
    if (pool_idx < 0 || pool_idx >= FRAGMENT_POOL_SIZE) return NULL;
    return _fragment_pool.vertices[pool_idx];
}

void fragment_set_vertex_count(int pool_idx, uint8_t count) {
    if (pool_idx < 0 || pool_idx >= FRAGMENT_POOL_SIZE) return;
    _fragment_pool.vertex_counts[pool_idx] = count;
}

void fragment_draw(color_t color, int pool_idx) {
    if (pool_idx < 0 || pool_idx >= FRAGMENT_POOL_SIZE) return;
    if (!(_fragment_pool.active_mask & (1u << pool_idx))) return;

    uint8_t vertex_count = _fragment_pool.vertex_counts[pool_idx];
    if (vertex_count == 0) return;

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_BYTE, 0, _fragment_pool.vertices[pool_idx]);
    glColor4ubv((GLubyte*)&color);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, vertex_count);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// ============================================================================
// Fracture Algorithm
// ============================================================================

// Cut line represented as ax + by + c = 0 (normalized so a*a + b*b = 1)
typedef struct {
    float a, b, c;
} CutLine;

// Generate random cut lines through bounding box
static int _generate_cut_lines(float min_x, float min_y, float max_x, float max_y,
                               CutLine* cuts, int max_cuts) {
    // Use randf() for random count (2-4 cuts)
    int num_cuts = 2 + (int)(randf() * 3.0f);
    if (num_cuts > max_cuts) num_cuts = max_cuts;

    float cx = (min_x + max_x) * 0.5f;
    float cy = (min_y + max_y) * 0.5f;

    // Spread cuts evenly around the circle with some randomness
    // Use degrees for LUT functions (0-360)
    int base_angle_deg = (int)(randf() * 360.0f);
    int angle_step_deg = 360 / num_cuts;

    for (int i = 0; i < num_cuts; i++) {
        // Angle with jitter for variation
        int angle_deg = base_angle_deg + angle_step_deg * i + (int)(randf_symmetric() * 17.0f);
        float a = lut_cos(angle_deg);
        float b = lut_sin(angle_deg);

        // Small offset from center to ensure cuts go through the model
        float offset = randf_symmetric() * 5.0f;
        float c = -(a * cx + b * cy + offset);

        cuts[i].a = a;
        cuts[i].b = b;
        cuts[i].c = c;
    }

    return num_cuts;
}

// Expand LINE_LOOP/LINE_STRIP commands into individual segments
static void _expand_to_segments(const int8_t* vertices, const DrawCommand* commands,
                                int command_count, SegmentBuffer* buf) {
    buf->count = 0;

    for (int cmd_idx = 0; cmd_idx < command_count; cmd_idx++) {
        const DrawCommand* cmd = &commands[cmd_idx];
        int start = cmd->start;
        int count = cmd->count;

        if (count < 2) continue;

        // Add segments for this strip/loop
        for (int i = 0; i < count - 1; i++) {
            int idx = buf->count;
            if (idx >= 256) break;

            buf->x1[idx] = (float)vertices[(start + i) * 2];
            buf->y1[idx] = (float)vertices[(start + i) * 2 + 1];
            buf->x2[idx] = (float)vertices[(start + i + 1) * 2];
            buf->y2[idx] = (float)vertices[(start + i + 1) * 2 + 1];
            buf->region[idx] = 0;
            buf->cut_at_p1[idx] = 0;  // original endpoints, not cut
            buf->cut_at_p2[idx] = 0;
            buf->count++;
        }

        // Close the loop if LINE_LOOP
        if (cmd->type == CMD_LINE_LOOP && count >= 2) {
            int idx = buf->count;
            if (idx < 256) {
                buf->x1[idx] = (float)vertices[(start + count - 1) * 2];
                buf->y1[idx] = (float)vertices[(start + count - 1) * 2 + 1];
                buf->x2[idx] = (float)vertices[start * 2];
                buf->y2[idx] = (float)vertices[start * 2 + 1];
                buf->region[idx] = 0;
                buf->cut_at_p1[idx] = 0;
                buf->cut_at_p2[idx] = 0;
                buf->count++;
            }
        }
    }
}

// SIMD: Test 8 segments against 1 cut line, split intersecting segments
// Returns number of new segments added
static int _simd_cut_segments(SegmentBuffer* buf, const CutLine* cut, int start_idx, int count) {
    if (count <= 0) return 0;

    int new_segments = 0;
    int batches = (count + 7) / 8;

    __m256 cut_a = _mm256_set1_ps(cut->a);
    __m256 cut_b = _mm256_set1_ps(cut->b);
    __m256 cut_c = _mm256_set1_ps(cut->c);
    __m256 zero = _mm256_setzero_ps();
    __m256 one = _mm256_set1_ps(1.0f);
    __m256 eps = _mm256_set1_ps(0.001f);

    for (int batch = 0; batch < batches; batch++) {
        int i = start_idx + batch * 8;
        int batch_count = (count - batch * 8);
        if (batch_count > 8) batch_count = 8;

        // Load segment endpoints
        __m256 x1 = _mm256_loadu_ps(&buf->x1[i]);
        __m256 y1 = _mm256_loadu_ps(&buf->y1[i]);
        __m256 x2 = _mm256_loadu_ps(&buf->x2[i]);
        __m256 y2 = _mm256_loadu_ps(&buf->y2[i]);

        // Compute signed distances from cut line: d = a*x + b*y + c
        __m256 d1 = _mm256_add_ps(_mm256_add_ps(
            _mm256_mul_ps(cut_a, x1),
            _mm256_mul_ps(cut_b, y1)), cut_c);
        __m256 d2 = _mm256_add_ps(_mm256_add_ps(
            _mm256_mul_ps(cut_a, x2),
            _mm256_mul_ps(cut_b, y2)), cut_c);

        // Check if segment crosses the line (d1 and d2 have opposite signs)
        __m256 cross_mask = _mm256_cmp_ps(_mm256_mul_ps(d1, d2), zero, _CMP_LT_OQ);

        // Compute intersection parameter t = d1 / (d1 - d2)
        __m256 denom = _mm256_sub_ps(d1, d2);
        __m256 denom_safe = _mm256_blendv_ps(denom, one, _mm256_cmp_ps(_mm256_andnot_ps(
            _mm256_set1_ps(-0.0f), denom), eps, _CMP_LT_OQ));
        __m256 t = _mm256_div_ps(d1, denom_safe);

        // Clamp t to [0, 1]
        t = _mm256_max_ps(zero, _mm256_min_ps(one, t));

        // Compute intersection point: ix = x1 + t*(x2-x1), iy = y1 + t*(y2-y1)
        __m256 ix = _mm256_add_ps(x1, _mm256_mul_ps(t, _mm256_sub_ps(x2, x1)));
        __m256 iy = _mm256_add_ps(y1, _mm256_mul_ps(t, _mm256_sub_ps(y2, y1)));

        // Extract results and split crossing segments
        __declspec(align(32)) float ix_arr[8], iy_arr[8];
        __declspec(align(32)) float cross_arr[8];
        _mm256_store_ps(ix_arr, ix);
        _mm256_store_ps(iy_arr, iy);
        _mm256_store_ps(cross_arr, cross_mask);

        for (int j = 0; j < batch_count; j++) {
            if (*(uint32_t*)&cross_arr[j] != 0) {
                // Segment crosses - split it
                int seg_idx = i + j;
                float orig_x2 = buf->x2[seg_idx];
                float orig_y2 = buf->y2[seg_idx];
                uint8_t orig_cut_p2 = buf->cut_at_p2[seg_idx];

                // Shorten original segment to intersection - mark p2 as cut point
                buf->x2[seg_idx] = ix_arr[j];
                buf->y2[seg_idx] = iy_arr[j];
                buf->cut_at_p2[seg_idx] = 1;  // new endpoint is a cut point

                // Add new segment from intersection to original end
                int new_idx = buf->count + new_segments;
                if (new_idx < 256) {
                    buf->x1[new_idx] = ix_arr[j];
                    buf->y1[new_idx] = iy_arr[j];
                    buf->x2[new_idx] = orig_x2;
                    buf->y2[new_idx] = orig_y2;
                    buf->region[new_idx] = buf->region[seg_idx];
                    buf->cut_at_p1[new_idx] = 1;  // this endpoint is a cut point
                    buf->cut_at_p2[new_idx] = orig_cut_p2;  // preserve original flag
                    new_segments++;
                }
            }
        }
    }

    return new_segments;
}

// SIMD: Classify 8 segment midpoints against all cut lines to determine region
static void _simd_classify_regions(SegmentBuffer* buf, const CutLine* cuts, int num_cuts) {
    int count = buf->count;

    for (int i = 0; i < count; i += 8) {
        int batch_count = (count - i);
        if (batch_count > 8) batch_count = 8;

        // Load segment endpoints and compute midpoints
        __m256 x1 = _mm256_loadu_ps(&buf->x1[i]);
        __m256 y1 = _mm256_loadu_ps(&buf->y1[i]);
        __m256 x2 = _mm256_loadu_ps(&buf->x2[i]);
        __m256 y2 = _mm256_loadu_ps(&buf->y2[i]);

        __m256 half = _mm256_set1_ps(0.5f);
        __m256 mx = _mm256_mul_ps(_mm256_add_ps(x1, x2), half);
        __m256 my = _mm256_mul_ps(_mm256_add_ps(y1, y2), half);

        // Compute region code based on which side of each cut line
        __m256i region = _mm256_setzero_si256();
        __m256 zero = _mm256_setzero_ps();

        for (int c = 0; c < num_cuts; c++) {
            __m256 cut_a = _mm256_set1_ps(cuts[c].a);
            __m256 cut_b = _mm256_set1_ps(cuts[c].b);
            __m256 cut_c = _mm256_set1_ps(cuts[c].c);

            // d = a*mx + b*my + c
            __m256 d = _mm256_add_ps(_mm256_add_ps(
                _mm256_mul_ps(cut_a, mx),
                _mm256_mul_ps(cut_b, my)), cut_c);

            // Set bit c if d >= 0
            __m256 pos_mask = _mm256_cmp_ps(d, zero, _CMP_GE_OQ);
            __m256i bit = _mm256_and_si256(_mm256_castps_si256(pos_mask),
                                           _mm256_set1_epi32(1 << c));
            region = _mm256_or_si256(region, bit);
        }

        // Store regions
        __declspec(align(32)) int region_arr[8];
        _mm256_store_si256((__m256i*)region_arr, region);

        for (int j = 0; j < batch_count; j++) {
            buf->region[i + j] = (uint8_t)region_arr[j];
        }
    }
}

// Pack segments by region into fragment pool slots
static int _pack_fragments(SegmentBuffer* buf, FractureResult* result) {
    // Count unique regions
    uint32_t region_mask = 0;
    for (int i = 0; i < buf->count; i++) {
        region_mask |= (1u << buf->region[i]);
    }

    result->count = 0;

    // Process each region
    for (int r = 0; r < 16 && result->count < 8; r++) {
        if (!(region_mask & (1u << r))) continue;

        // Allocate pool slot
        int pool_idx = fragment_pool_alloc();
        if (pool_idx < 0) break;

        int8_t* verts = _fragment_pool.vertices[pool_idx];
        int vert_count = 0;
        float sum_x = 0, sum_y = 0;
        int point_count = 0;

        // Pack all segments in this region as GL_LINES pairs
        // Also add short perpendicular "break" lines at cut points
        for (int i = 0; i < buf->count && vert_count < FRAGMENT_MAX_VERTICES - 6; i++) {
            if (buf->region[i] != r) continue;

            float fx1 = buf->x1[i];
            float fy1 = buf->y1[i];
            float fx2 = buf->x2[i];
            float fy2 = buf->y2[i];

            int8_t x1 = (int8_t)fx1;
            int8_t y1 = (int8_t)fy1;
            int8_t x2 = (int8_t)fx2;
            int8_t y2 = (int8_t)fy2;

            // Add the main segment
            verts[vert_count * 2] = x1;
            verts[vert_count * 2 + 1] = y1;
            verts[vert_count * 2 + 2] = x2;
            verts[vert_count * 2 + 3] = y2;
            vert_count += 2;

            // Compute perpendicular direction for break lines
            float dx = fx2 - fx1;
            float dy = fy2 - fy1;
            float len = dx * dx + dy * dy;
            if (len > 0.1f) {
                len = 1.0f / (len > 1.0f ? len : 1.0f);  // approx normalize
                float px = -dy * 3.0f * len;  // perpendicular, length ~3
                float py = dx * 3.0f * len;

                // Add break line at p1 if it was cut
                if (buf->cut_at_p1[i] && vert_count < FRAGMENT_MAX_VERTICES - 2) {
                    verts[vert_count * 2] = (int8_t)(fx1 - px);
                    verts[vert_count * 2 + 1] = (int8_t)(fy1 - py);
                    verts[vert_count * 2 + 2] = (int8_t)(fx1 + px);
                    verts[vert_count * 2 + 3] = (int8_t)(fy1 + py);
                    vert_count += 2;
                }

                // Add break line at p2 if it was cut
                if (buf->cut_at_p2[i] && vert_count < FRAGMENT_MAX_VERTICES - 2) {
                    verts[vert_count * 2] = (int8_t)(fx2 - px);
                    verts[vert_count * 2 + 1] = (int8_t)(fy2 - py);
                    verts[vert_count * 2 + 2] = (int8_t)(fx2 + px);
                    verts[vert_count * 2 + 3] = (int8_t)(fy2 + py);
                    vert_count += 2;
                }
            }

            // Accumulate for centroid
            sum_x += fx1 + fx2;
            sum_y += fy1 + fy2;
            point_count += 2;
        }

        if (vert_count == 0) {
            fragment_pool_free(pool_idx);
            continue;
        }

        _fragment_pool.vertex_counts[pool_idx] = (uint8_t)vert_count;

        // Compute centroid
        float centroid_x = (point_count > 0) ? sum_x / point_count : 0;
        float centroid_y = (point_count > 0) ? sum_y / point_count : 0;

        // Store result
        int idx = result->count;
        result->pool_indices[idx] = (uint8_t)pool_idx;
        result->centroid_x[idx] = centroid_x;
        result->centroid_y[idx] = centroid_y;
        result->count++;
    }

    return result->count;
}

int fracture_model(uint16_t model_idx, FractureResult* result) {
    PROFILE_ZONE("fracture_model");

    result->count = 0;

    // Get model data from generated metadata
    const int8_t* vertices = _model_vertices[model_idx];
    const DrawCommand* commands = _model_commands[model_idx];
    int command_count = _model_command_counts[model_idx];

    // Expand all line strips/loops to individual segments
    _expand_to_segments(vertices, commands, command_count, &_segment_buffer);

    if (_segment_buffer.count == 0) {
        PROFILE_ZONE_END();
        return 0;
    }

    // Compute bounding box
    float min_x = _segment_buffer.x1[0], max_x = _segment_buffer.x1[0];
    float min_y = _segment_buffer.y1[0], max_y = _segment_buffer.y1[0];
    for (int i = 0; i < _segment_buffer.count; i++) {
        if (_segment_buffer.x1[i] < min_x) min_x = _segment_buffer.x1[i];
        if (_segment_buffer.x2[i] < min_x) min_x = _segment_buffer.x2[i];
        if (_segment_buffer.x1[i] > max_x) max_x = _segment_buffer.x1[i];
        if (_segment_buffer.x2[i] > max_x) max_x = _segment_buffer.x2[i];
        if (_segment_buffer.y1[i] < min_y) min_y = _segment_buffer.y1[i];
        if (_segment_buffer.y2[i] < min_y) min_y = _segment_buffer.y2[i];
        if (_segment_buffer.y1[i] > max_y) max_y = _segment_buffer.y1[i];
        if (_segment_buffer.y2[i] > max_y) max_y = _segment_buffer.y2[i];
    }

    // Generate random cut lines
    CutLine cuts[4];
    int num_cuts = _generate_cut_lines(min_x, min_y, max_x, max_y, cuts, 4);

    // Apply each cut line, splitting segments that cross it
    for (int c = 0; c < num_cuts; c++) {
        int original_count = _segment_buffer.count;
        int new_segs = _simd_cut_segments(&_segment_buffer, &cuts[c], 0, original_count);
        _segment_buffer.count += new_segs;
    }

    // Classify all segments by region (which side of each cut line)
    _simd_classify_regions(&_segment_buffer, cuts, num_cuts);

    // Pack segments into fragment pool slots
    int fragments = _pack_fragments(&_segment_buffer, result);

    PROFILE_ZONE_END();
    return fragments;
}

// ============================================================================
// Explode Entity API
// ============================================================================

int explode_entity(uint32_t object_idx) {
    PROFILE_ZONE("explode_entity");

    struct objects_data* od = entity_manager_get_objects();

    if (object_idx >= od->active) {
        PROFILE_ZONE_END();
        return 0;
    }

    // Get entity data
    uint16_t model_idx = od->model_idx[object_idx];
    float entity_x = od->position_orientation.position_x[object_idx];
    float entity_y = od->position_orientation.position_y[object_idx];
    float entity_vx = od->velocity_x[object_idx];
    float entity_vy = od->velocity_y[object_idx];
    float entity_ox = od->position_orientation.orientation_x[object_idx];
    float entity_oy = od->position_orientation.orientation_y[object_idx];

    // Can't fracture dynamic fragments
    if (model_idx >= FRAGMENT_MODEL_BASE) {
        PROFILE_ZONE_END();
        return 0;
    }

    // Fracture the model
    FractureResult result;
    int num_fragments = fracture_model(model_idx, &result);

    if (num_fragments == 0) {
        PROFILE_ZONE_END();
        return 0;
    }

    // Batch compute world positions and velocities using SIMD
    // Pad to 8 for SIMD alignment
    __declspec(align(32)) float world_x[8];
    __declspec(align(32)) float world_y[8];
    __declspec(align(32)) float radial_x[8];
    __declspec(align(32)) float radial_y[8];
    __declspec(align(32)) float vx[8];
    __declspec(align(32)) float vy[8];

    // Load entity orientation into SIMD registers
    __m256 ox = _mm256_set1_ps(entity_ox);
    __m256 oy = _mm256_set1_ps(entity_oy);
    __m256 ex = _mm256_set1_ps(entity_x);
    __m256 ey = _mm256_set1_ps(entity_y);
    __m256 evx = _mm256_set1_ps(entity_vx);
    __m256 evy = _mm256_set1_ps(entity_vy);

    // Load centroids (pad with zeros)
    __declspec(align(32)) float cx_pad[8] = {0};
    __declspec(align(32)) float cy_pad[8] = {0};
    for (int i = 0; i < num_fragments; i++) {
        cx_pad[i] = result.centroid_x[i];
        cy_pad[i] = result.centroid_y[i];
    }
    __m256 cx = _mm256_load_ps(cx_pad);
    __m256 cy = _mm256_load_ps(cy_pad);

    // Transform centroids: world = entity + rotate(centroid, orientation)
    // rotate: wx = cx*ox - cy*oy, wy = cx*oy + cy*ox
    __m256 rot_x = _mm256_sub_ps(_mm256_mul_ps(cx, ox), _mm256_mul_ps(cy, oy));
    __m256 rot_y = _mm256_add_ps(_mm256_mul_ps(cx, oy), _mm256_mul_ps(cy, ox));

    _mm256_store_ps(world_x, _mm256_add_ps(ex, rot_x));
    _mm256_store_ps(world_y, _mm256_add_ps(ey, rot_y));

    // Radial direction is the rotated centroid (already computed)
    _mm256_store_ps(radial_x, rot_x);
    _mm256_store_ps(radial_y, rot_y);

    // Normalize radial directions (batch of 8)
    vec2_normalize_i(radial_x, radial_y, 8);

    // Generate random speeds and spreads using SIMD
    __m256 rand_spd = randf_8();           // [0, 1)
    __m256 rand_sx = randf_symmetric_8();  // [-1, 1)
    __m256 rand_sy = randf_symmetric_8();  // [-1, 1)

    // speeds = 15 + rand * 10, spread = rand * 8 (slower, more dramatic)
    __m256 spd = _mm256_add_ps(_mm256_set1_ps(15.0f), _mm256_mul_ps(rand_spd, _mm256_set1_ps(10.0f)));
    __m256 sx = _mm256_mul_ps(rand_sx, _mm256_set1_ps(8.0f));
    __m256 sy = _mm256_mul_ps(rand_sy, _mm256_set1_ps(8.0f));

    // vx = entity_vx + radial_x * speed + spread_x
    __m256 rx = _mm256_load_ps(radial_x);
    __m256 ry = _mm256_load_ps(radial_y);

    _mm256_store_ps(vx, _mm256_add_ps(evx, _mm256_add_ps(_mm256_mul_ps(rx, spd), sx)));
    _mm256_store_ps(vy, _mm256_add_ps(evy, _mm256_add_ps(_mm256_mul_ps(ry, spd), sy)));

    // Spawn particles
    for (int i = 0; i < num_fragments; i++) {
        particle_create_t pc = {
            .x = world_x[i],
            .y = world_y[i],
            .vx = vx[i],
            .vy = vy[i],
            .ox = entity_ox + randf_symmetric() * 0.3f,
            .oy = entity_oy + randf_symmetric() * 0.3f,
            .ttl = 90 + (uint16_t)(randf() * 60.0f),
            .model_idx = FRAGMENT_MODEL_BASE + result.pool_indices[i]
        };
        particles_create_particle(&pc);
    }

    PROFILE_ZONE_END();
    return num_fragments;
}

