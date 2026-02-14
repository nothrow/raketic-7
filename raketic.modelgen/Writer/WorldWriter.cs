using raketic.modelgen.Entity;
using raketic.modelgen.Surface;
using raketic.modelgen.Svg;
using raketic.modelgen.World;
using System.Numerics;

namespace raketic.modelgen.Writer;

internal class WorldWriter(PathInfo paths) : IDisposable
{
    private StreamWriter? _hWriter;
    private StreamWriter? _cWriter;

    public void WriteHeaders()
    {
        _hWriter = new StreamWriter(paths.OutputHRenderer, false);
        _cWriter = new StreamWriter(paths.OutputC, false);

        _hWriter.WriteLine(@"
// generated, do not edit manually

#pragma once
#include ""platform/platform.h""
void _generated_draw_model(color_t color, uint16_t index);
uint16_t _generated_get_model_radius(uint16_t index);
const uint16_t* _generated_get_radial_profile(uint16_t model_idx);

struct surface_polyline;
struct surface_data;
const struct surface_data* _generated_get_surface(uint16_t index);

void _generated_load_map_data(uint16_t index);
");

        _cWriter.WriteLine(@"
// generated, do not edit manually

#include ""renderer.gen.h""
#include ""entity/entity.h""
#include ""entity/controller.h""
#include ""entity/camera.h""
#include ""entity/engine.h""
#include ""entity/weapon.h""
#include ""entity/ai.h""
#include ""entity/radar.h""
#include ""graphics/surface.h""
#include ""debug/debug.h""
#include ""hud/hud.h""
#include ""debug/profiler.h""

#include <Windows.h>
#include <gl/GL.h>
#include <math.h>
");

    }

    public void WriteModels(IEnumerable<Model> models)
    {
        if (_cWriter == null || _hWriter == null)
            throw new InvalidOperationException("Writers not initialized. Call WriteHeaders() first.");

        var svgWriter = new SvgModelWriter(_cWriter!, _hWriter!);

        svgWriter.Write(models.ToArray());
    }

    public void Dispose()
    {
        _hWriter?.Dispose();
        _cWriter?.Dispose();
    }

    internal void WriteSurfaces(IReadOnlyList<SurfaceDataRecord> surfaces)
    {
        if (_cWriter == null || _hWriter == null)
            throw new InvalidOperationException("Writers not initialized. Call WriteHeaders() first.");

        if (surfaces.Count == 0) return;

        var w = _cWriter;

        // Write surface constants to header
        for (int i = 0; i < surfaces.Count; i++)
        {
            _hWriter.WriteLine($"#define SURFACE_{surfaces[i].Name.ToUpper()}_IDX ((uint16_t){i})");
        }
        _hWriter.WriteLine();

        // Write polyline vertex data
        for (int si = 0; si < surfaces.Count; si++)
        {
            var surf = surfaces[si];
            for (int pi = 0; pi < surf.Polylines.Length; pi++)
            {
                var poly = surf.Polylines[pi];
                w.Write($"static const int16_t _surf_{si}_poly_{pi}[] = {{ ");
                for (int vi = 0; vi < poly.Points.Length; vi++)
                {
                    var pt = poly.Points[vi];
                    if (vi > 0) w.Write(", ");
                    w.Write($"{pt.Px}, {pt.Py}, {pt.Pz}");
                }
                w.WriteLine($" }};");
            }
            w.WriteLine();
        }

        // Write polyline descriptor arrays
        for (int si = 0; si < surfaces.Count; si++)
        {
            var surf = surfaces[si];
            w.WriteLine($"static const struct surface_polyline _surf_{si}_polys[] = {{");
            for (int pi = 0; pi < surf.Polylines.Length; pi++)
            {
                w.WriteLine($"  {{ _surf_{si}_poly_{pi}, {surf.Polylines[pi].Points.Length} }},");
            }
            w.WriteLine($"}};");
            w.WriteLine();
        }

        // Write surface data array
        w.WriteLine($"static const struct surface_data _surfaces[] = {{");
        for (int si = 0; si < surfaces.Count; si++)
        {
            var surf = surfaces[si];
            w.WriteLine($"  {{ _surf_{si}_polys, {surf.Polylines.Length}, {{ {surf.ColorR}, {surf.ColorG}, {surf.ColorB}, {surf.ColorA} }} }},  // {si}: {surf.Name}");
        }
        w.WriteLine($"}};");
        w.WriteLine();

        // Write accessor function
        w.WriteLine($"const struct surface_data* _generated_get_surface(uint16_t index) {{");
        w.WriteLine($"  if (index >= {surfaces.Count}) return 0;");
        w.WriteLine($"  return &_surfaces[index];");
        w.WriteLine($"}}");
        w.WriteLine();

        Console.WriteLine($"Written {surfaces.Count} surface(s) with {surfaces.Sum(s => s.Polylines.Length)} polylines, {surfaces.Sum(s => s.Polylines.Sum(p => p.Points.Length))} total vertices.");
    }

    internal void WriteWorlds(IEnumerable<WorldsData> worlds)
    {
        if (_cWriter == null || _hWriter == null)
            throw new InvalidOperationException("Writers not initialized. Call WriteHeaders() first.");

        int i = 0;
        foreach (var world in worlds)
        {
            Console.Write($"Processing world: {world.WorldName} -");

            WriteWorldHeaders(world, i);
            WriteWorldContent(world, i);
            ++i;

            Console.WriteLine($" written {world.Entities.Length} entities");
        }


        _cWriter.WriteLine($"void _generated_load_map_data(uint16_t index) {{");
        _cWriter.WriteLine($"  static void (*_data[])(struct objects_data*, struct parts_data*) = {{");
        foreach (var world in worlds)
        {
            _cWriter.WriteLine($"    _world_{world.WorldName},");
        }
        _cWriter.WriteLine($"  }};");
        _cWriter.WriteLine($"  _ASSERT(index >= 0 && index < {worlds.Count()});");
        _cWriter.WriteLine($"  _data[index](entity_manager_get_objects(), entity_manager_get_parts());");
        _cWriter.WriteLine($"}}");
        _cWriter.WriteLine();
    }

    private void WriteWorldContent(WorldsData world, int i)
    {
        _cWriter!.WriteLine($"static void _world_{world.WorldName}(struct objects_data* od, struct parts_data* pd) {{");
        _cWriter!.WriteLine($"  uint32_t new_idx;");
        _cWriter!.WriteLine($"  uint32_t new_pidx;");

        foreach (EntityData entity in world.Entities)
        {
            _cWriter!.WriteLine();
            _cWriter!.WriteLine($"  new_idx = od->active++;");
            _cWriter!.WriteLine($"  od->type[new_idx] = {entity!.Type};");
            _cWriter!.WriteLine($"  od->model_idx[new_idx] = {entity.Model!.ModelConstantName};");
            _cWriter!.WriteLine($"  od->position_orientation.position_x[new_idx] = {entity.Position?.X:0.0#######}f;");
            _cWriter!.WriteLine($"  od->position_orientation.position_y[new_idx] = {entity.Position?.Y:0.0#######}f;");
            _cWriter!.WriteLine($"  od->position_orientation.orientation_x[new_idx] = {Math.Cos(entity.Rotation ?? 0.0f):0.0#######}f;");
            _cWriter!.WriteLine($"  od->position_orientation.orientation_y[new_idx] = {Math.Sin(entity.Rotation ?? 0.0f):0.0#######}f;");
            _cWriter!.WriteLine($"  od->position_orientation.radius[new_idx] = {entity.Model!.GetRadius()};");
            _cWriter!.WriteLine($"  od->mass[new_idx] = {entity.Mass!};");
            _cWriter!.WriteLine($"  od->health[new_idx] = {entity.Health ?? 1000};");

            // Explicit initial velocity (if specified)
            if (entity.Velocity.HasValue)
            {
                _cWriter!.WriteLine($"  od->velocity_x[new_idx] = {entity.Velocity.Value.X:0.0#######}f;");
                _cWriter!.WriteLine($"  od->velocity_y[new_idx] = {entity.Velocity.Value.Y:0.0#######}f;");
            }

            // Orbit initialization: set tangential velocity for circular orbit
            // Uses the FULL gravitational field (all bodies) to compute the correct
            // centripetal acceleration, not just the orbit target's mass.
            // This accounts for tidal forces (e.g., sun pulling on a moon).
            if (entity is EntityData ed && ed.OrbitTargetSpawnId.HasValue)
            {
                var targetIdx = ed.OrbitTargetSpawnId.Value;
                _cWriter!.WriteLine();
                _cWriter!.WriteLine($"  // Circular orbit velocity around entity {targetIdx} (N-body corrected)");
                _cWriter!.WriteLine($"  {{");
                _cWriter!.WriteLine($"    float _dx = od->position_orientation.position_x[new_idx] - od->position_orientation.position_x[{targetIdx}];");
                _cWriter!.WriteLine($"    float _dy = od->position_orientation.position_y[new_idx] - od->position_orientation.position_y[{targetIdx}];");
                _cWriter!.WriteLine($"    float _dist = sqrtf(_dx * _dx + _dy * _dy);");
                _cWriter!.WriteLine($"    float _rnx = _dx / _dist;");
                _cWriter!.WriteLine($"    float _rny = _dy / _dist;");
                _cWriter!.WriteLine();
                _cWriter!.WriteLine($"    // Relative acceleration of orbiter w.r.t. orbit center (tidal frame)");
                _cWriter!.WriteLine($"    float _arx = 0.0f, _ary = 0.0f;");
                _cWriter!.WriteLine($"    for (uint32_t _k = 0; _k < od->active; _k++) {{");
                _cWriter!.WriteLine($"      if (_k == (uint32_t)new_idx) continue;");
                _cWriter!.WriteLine();
                _cWriter!.WriteLine($"      // Gravity on orbiter from body _k");
                _cWriter!.WriteLine($"      float _ox = od->position_orientation.position_x[_k] - od->position_orientation.position_x[new_idx];");
                _cWriter!.WriteLine($"      float _oy = od->position_orientation.position_y[_k] - od->position_orientation.position_y[new_idx];");
                _cWriter!.WriteLine($"      float _od2 = _ox * _ox + _oy * _oy;");
                _cWriter!.WriteLine($"      if (_od2 < 1.0f) continue;");
                _cWriter!.WriteLine($"      float _od1 = sqrtf(_od2);");
                _cWriter!.WriteLine($"      float _gm = 6.67430f * od->mass[_k] / (_od2 * _od1);");
                _cWriter!.WriteLine($"      _arx += _gm * _ox;");
                _cWriter!.WriteLine($"      _ary += _gm * _oy;");
                _cWriter!.WriteLine();
                _cWriter!.WriteLine($"      // Subtract gravity on orbit center from same body (tidal correction)");
                _cWriter!.WriteLine($"      if (_k != (uint32_t){targetIdx}) {{");
                _cWriter!.WriteLine($"        float _cx = od->position_orientation.position_x[_k] - od->position_orientation.position_x[{targetIdx}];");
                _cWriter!.WriteLine($"        float _cy = od->position_orientation.position_y[_k] - od->position_orientation.position_y[{targetIdx}];");
                _cWriter!.WriteLine($"        float _cd2 = _cx * _cx + _cy * _cy;");
                _cWriter!.WriteLine($"        if (_cd2 >= 1.0f) {{");
                _cWriter!.WriteLine($"          float _cd1 = sqrtf(_cd2);");
                _cWriter!.WriteLine($"          float _gmc = 6.67430f * od->mass[_k] / (_cd2 * _cd1);");
                _cWriter!.WriteLine($"          _arx -= _gmc * _cx;");
                _cWriter!.WriteLine($"          _ary -= _gmc * _cy;");
                _cWriter!.WriteLine($"        }}");
                _cWriter!.WriteLine($"      }}");
                _cWriter!.WriteLine($"    }}");
                _cWriter!.WriteLine();
                _cWriter!.WriteLine($"    // Centripetal component (toward orbit center)");
                _cWriter!.WriteLine($"    float _a_cent = -(_arx * _rnx + _ary * _rny);");
                _cWriter!.WriteLine($"    float _v = _a_cent > 0.0f ? sqrtf(_a_cent * _dist) : 0.0f;");
                _cWriter!.WriteLine($"    od->velocity_x[new_idx] = -_rny * _v + od->velocity_x[{targetIdx}];");
                _cWriter!.WriteLine($"    od->velocity_y[new_idx] = _rnx * _v + od->velocity_y[{targetIdx}];");
                _cWriter!.WriteLine($"  }}");
            }

            // Surface initialization (animated planet/moon surfaces)
            if (entity is EntityData edSurf && edSurf.SurfaceIdx.HasValue)
            {
                _cWriter!.WriteLine();
                _cWriter!.WriteLine($"  // Surface: {edSurf.SurfaceName}");
                _cWriter!.WriteLine($"  od->surface_idx[new_idx] = {edSurf.SurfaceIdx.Value};");
                _cWriter!.WriteLine($"  od->rotation_speed[new_idx] = {(edSurf.RotationSpeed ?? 0.001f):0.0#######}f;");
                _cWriter!.WriteLine($"  od->surface_rotation[new_idx] = 0.0f;");
            }

            // Engage AI orbit keeper for satellites with orbit targets
            if (entity is EntityData ed2 && ed2.OrbitTargetSpawnId.HasValue && entity.Type == "ENTITY_TYPEREF_SATELLITE")
            {
                var targetIdx2 = ed2.OrbitTargetSpawnId.Value;
                _cWriter!.WriteLine();
                _cWriter!.WriteLine($"  // Engage AI orbit keeper for satellite");
                _cWriter!.WriteLine($"  ai_orbit_engage(OBJECT_ID_WITH_TYPE(new_idx, {entity!.Type}._), {targetIdx2});");
            }

            if (entity is EntityWithSlotsData entityWithSlots)
            {
                var slotCount = entityWithSlots.Slots.Length;
                var reservedSlots = slotCount + 7 & ~7;
                var paddingSlots = reservedSlots - slotCount;

                _cWriter!.WriteLine();
                _cWriter!.WriteLine($"  _ASSERT(pd->active + {reservedSlots} < pd->capacity);");
                _cWriter!.WriteLine($"  od->parts_start_idx[new_idx] = pd->active;");
                _cWriter!.WriteLine($"  od->parts_count[new_idx] = {slotCount};");
                _cWriter!.WriteLine();
                _cWriter!.WriteLine($"  memset(&pd->model_idx[pd->active], -1, sizeof(uint16_t) * {reservedSlots});");
                _cWriter!.WriteLine();
                var w = _cWriter!;

                foreach (var slot in entityWithSlots.Slots)
                {
                    var model = entityWithSlots.Model!;
                    var slotRef = slot.SlotRef!.Value;
                    var slotEntity = (PartData)slot.Entity!;

                    w.WriteLine($"  new_pidx = pd->active++;");
                    w.WriteLine($"  pd->parent_id[new_pidx] = OBJECT_ID_WITH_TYPE(new_idx, {entity!.Type}._);");
                    w.WriteLine($"  pd->local_offset_x[new_pidx] = {model.Slots[slotRef].Position.X:0.0#######}f;");
                    w.WriteLine($"  pd->local_offset_y[new_pidx] = {model.Slots[slotRef].Position.Y:0.0#######}f;");
                    w.WriteLine($"  pd->model_idx[new_pidx] = {slotEntity.Model!.ModelConstantName};");

                    w.WriteLine($"  pd->type[new_pidx] = {slotEntity.Type};");

                    w.WriteLine($"  pd->local_orientation_x[new_pidx] = 1.0f;"); // todo: ?!
                    w.WriteLine($"  pd->local_orientation_y[new_pidx] = 0.0f;");

                    slotEntity.DumpPartData(w, "pd->data[new_pidx].data");

                    w.WriteLine();
                }


                _cWriter!.WriteLine($"  pd->active += {paddingSlots};");
            }

            if (entity.SpawnId == world.ControlledEntitySpawnId)
            {
                _cWriter!.WriteLine();

                _cWriter!.WriteLine($"  debug_watch_set(OBJECT_ID_WITH_TYPE(new_idx, {entity!.Type}._));");
                _cWriter!.WriteLine($"  controller_set_entity(OBJECT_ID_WITH_TYPE(new_idx, {entity!.Type}._));");
                _cWriter!.WriteLine($"  camera_set_entity(OBJECT_ID_WITH_TYPE(new_idx, {entity!.Type}._));");
                _cWriter!.WriteLine($"  hud_set_entity(OBJECT_ID_WITH_TYPE(new_idx, {entity!.Type}._));");
                _cWriter!.WriteLine();
            }
        }

        _cWriter!.WriteLine($"}}");
        _cWriter!.WriteLine();
    }

    private void WriteWorldHeaders(WorldsData world, int i)
    {
        _hWriter!.WriteLine($"#define WORLD_{world.WorldName.ToUpper()}_IDX ((uint16_t){i})");
    }
}
