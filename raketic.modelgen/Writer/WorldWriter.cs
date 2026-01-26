using raketic.modelgen.Svg;
using raketic.modelgen.World;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

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

void _generated_load_world_data(uint16_t index);
");

        _cWriter.WriteLine(@"
// generated, do not edit manually

#include ""renderer.gen.h""
#include ""entity/entity.h""

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
        foreach(var world in worlds)
        {
            Console.Write($"Processing world: {world.WorldName} -");

            WriteWorldHeaders(world, i);
            WriteWorldContent(world, i);
            ++i;

            Console.WriteLine($" written {world.Entities.Length} entities");
        }


        _cWriter.WriteLine($"void _generated_load_map_data(uint16_t index) {{");
        _cWriter.WriteLine($"  static void (*_data[])(struct objects_data*, struct particles_data*) = {{");
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
        _cWriter!.WriteLine($"static void _world_{world.WorldName}(struct objects_data* od, struct particles_data* pd) {{");
        _cWriter!.WriteLine($"  uint32_t new_idx;");

        foreach (var entity in world.Entities)
        {
            _cWriter!.WriteLine();
            _cWriter!.WriteLine($"  new_idx = od->active++;");
            _cWriter!.WriteLine($"  od->type[new_idx] = {entity!.Type};");
            _cWriter!.WriteLine($"  od->model_idx[new_idx] = MODEL_{entity.Model!.FileName.ToUpper()}_IDX;");
            _cWriter!.WriteLine($"  od->position_orientation.position_x[new_idx] = {entity.Position?.X:0.0#######}f;");
            _cWriter!.WriteLine($"  od->position_orientation.position_y[new_idx] = {entity.Position?.Y:0.0#######}f;");
            /*_cWriter!.WriteLine($"  od->position_orientation.orientation_x[new_idx] = 0.0f;");
            _cWriter!.WriteLine($"  od->position_orientation.orientation_y[new_idx] = 0.0f;");*/
            _cWriter!.WriteLine($"  od->position_orientation.radius[new_idx] = {entity.Model!.GetRadius()};");
            _cWriter!.WriteLine($"  od->mass[new_idx] = {entity.Mass!};");
        }

        _cWriter!.WriteLine($"}}");
        _cWriter!.WriteLine();
    }

    private void WriteWorldHeaders(WorldsData world, int i)
    {
        _hWriter!.WriteLine($"#define WORLD_{world.WorldName.ToUpper()}_IDX ((uint16_t){i})");
    }
}
