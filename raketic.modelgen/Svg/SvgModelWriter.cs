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

        // Radial collision profile data (16 distances from center at 22.5° intervals)
        foreach (var model in models)
        {
            var profile = model.GetRadialProfile();
            if (profile != null)
            {
                cWriter.Write($"static const uint8_t _model_{model.FileName}_radial[16] = {{");
                cWriter.Write($" {string.Join(", ", profile)}");
                cWriter.WriteLine(" };");
            }
        }
        cWriter.WriteLine();

        // Radial profile lookup function
        cWriter.WriteLine($"const uint8_t* _generated_get_radial_profile(uint16_t model_idx) {{");
        cWriter.WriteLine($"  switch (model_idx) {{");
        for (int j = 0; j < models.Length; j++)
        {
            var profile = models[j].GetRadialProfile();
            if (profile != null)
            {
                cWriter.WriteLine($"    case {j}: return _model_{models[j].FileName}_radial;");
            }
        }
        cWriter.WriteLine($"    default: return 0;");
        cWriter.WriteLine($"  }}");
        cWriter.WriteLine($"}}");
        cWriter.WriteLine();
    }
}
