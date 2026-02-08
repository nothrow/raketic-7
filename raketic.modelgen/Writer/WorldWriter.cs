using raketic.modelgen.Entity;
using raketic.modelgen.Svg;
using raketic.modelgen.World;
using System.Numerics;
using System.Reflection;

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
const uint8_t* _generated_get_radial_profile(uint16_t model_idx);

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
#include ""debug/debug.h""
#include ""debug/profiler.h""

#include <Windows.h>
#include <gl/GL.h>
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

            // Generate velocity for orbiting entities
            if (entity is EntityData entityData && entityData.OrbitTargetSpawnId.HasValue)
            {
                var target = world.Entities[entityData.OrbitTargetSpawnId.Value] as EntityData;
                if (target?.Position != null && entityData.Position != null && target.Mass.HasValue)
                {
                    double dx = entityData.Position.Value.X - target.Position.Value.X;
                    double dy = entityData.Position.Value.Y - target.Position.Value.Y;
                    double r = Math.Sqrt(dx * dx + dy * dy);

                    // Circular orbit velocity: v = sqrt(G * M / r)
                    // G = 6.67430 (matching engine's GRAVITATIONAL_CONSTANT)
                    const double G = 6.67430;
                    double v = Math.Sqrt(G * target.Mass.Value / r);

                    // Perpendicular direction (counterclockwise): rotate (dx,dy) by 90 degrees CCW
                    double nx = -dy / r;
                    double ny = dx / r;

                    double vx = nx * v;
                    double vy = ny * v;

                    _cWriter!.WriteLine($"  od->velocity_x[new_idx] = {vx:0.0#######}f;");
                    _cWriter!.WriteLine($"  od->velocity_y[new_idx] = {vy:0.0#######}f;");
                }
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
