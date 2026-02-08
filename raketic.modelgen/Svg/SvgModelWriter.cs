using raketic.modelgen.Writer;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace raketic.modelgen.Svg;

internal class SvgModelWriter(StreamWriter cWriter, StreamWriter hWriter)
{
    private ModelWriter modelWriter = new ModelWriter();

    public void Write(Model[] models)
    {
        int i = 0;
        foreach (var model in models)
        {
            Console.Write($"Processing model: {model.FileName}");

            var written = modelWriter.DumpModelData(cWriter, model);

            Console.WriteLine($" - written {written} vertices");

            hWriter.WriteLine($"#define {model.ModelConstantName}    ((uint16_t){i++})");
        }

        modelWriter.DumpModelColors(cWriter, models);

        foreach (var model in models)
        {
            modelWriter.DumpModel(cWriter, model);
        }

        cWriter.WriteLine();
        cWriter.WriteLine($"void _generated_draw_model(color_t color, uint16_t index) {{");

        cWriter.WriteLine($"  static void (*_data[])(color_t) = {{");
        foreach (var model in models)
        {
            cWriter.WriteLine($"    _model_{model.FileName},");
        }
        cWriter.WriteLine($"  }};");

        cWriter.WriteLine($"  _ASSERT(index >= 0 && index < {models.Length});");
        cWriter.WriteLine($"  _data[index](color);");
        cWriter.WriteLine($"}}");
        cWriter.WriteLine();

        cWriter.WriteLine($"uint16_t _generated_get_model_radius(uint16_t index) {{");
        cWriter.WriteLine($"  static uint16_t _data[] = {{");
        foreach (var model in models)
        {
            cWriter.WriteLine($"    {model.GetRadius()},");
        }
        cWriter.WriteLine($"  }};");

        cWriter.WriteLine($"  _ASSERT(index >= 0 && index < {models.Length});");
        cWriter.WriteLine($"  return _data[index];");
        cWriter.WriteLine($"}}");
        cWriter.WriteLine();

        // Collision hull data arrays
        foreach (var model in models)
        {
            var hull = model.GetCollisionHull();
            if (hull != null)
            {
                cWriter.Write($"static const int8_t _model_{model.FileName}_collision_hull[] = {{");
                foreach (var p in hull)
                {
                    checked
                    {
                        cWriter.Write($" {(sbyte)p.X}, {(sbyte)p.Y},");
                    }
                }
                cWriter.WriteLine(" };");
            }
        }
        cWriter.WriteLine();

        // Collision hull lookup function
        cWriter.WriteLine($"const int8_t* _generated_get_collision_hull(uint16_t model_idx, uint32_t* count) {{");
        cWriter.WriteLine($"  switch (model_idx) {{");
        for (int j = 0; j < models.Length; j++)
        {
            var hull = models[j].GetCollisionHull();
            if (hull != null)
            {
                cWriter.WriteLine($"    case {j}: *count = {hull.Length}; return _model_{models[j].FileName}_collision_hull;");
            }
        }
        cWriter.WriteLine($"    default: *count = 0; return 0;");
        cWriter.WriteLine($"  }}");
        cWriter.WriteLine($"}}");
        cWriter.WriteLine();
    }
}
