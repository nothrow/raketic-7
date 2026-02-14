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

        // Emit compact draw command data per model (instead of individual draw functions)
        foreach (var model in models)
        {
            modelWriter.DumpModelCommands(cWriter, model);
        }
        cWriter.WriteLine();

        // Draw model interpreter: reads command data and issues GL calls
        cWriter.WriteLine($"void _generated_draw_model(color_t color, uint16_t index) {{");
        cWriter.WriteLine($"  static const int16_t* _vtx[] = {{");
        foreach (var model in models)
        {
            cWriter.WriteLine($"    _model_{model.FileName}_vertices,");
        }
        cWriter.WriteLine($"  }};");
        cWriter.WriteLine($"  static const uint16_t* _cmds[] = {{");
        foreach (var model in models)
        {
            cWriter.WriteLine($"    _model_{model.FileName}_cmds,");
        }
        cWriter.WriteLine($"  }};");
        cWriter.WriteLine($"  _ASSERT(index >= 0 && index < {models.Length});");
        cWriter.WriteLine($"  glEnableClientState(GL_VERTEX_ARRAY);");
        cWriter.WriteLine($"  glVertexPointer(2, GL_SHORT, 0, _vtx[index]);");
        cWriter.WriteLine($"  const uint16_t* c = _cmds[index];");
        cWriter.WriteLine($"  while (*c) {{");
        cWriter.WriteLine($"    if (c[3] == 0xFFFE) glColor4ub(0, 0, 0, 255);");
        cWriter.WriteLine($"    else if (c[3] == 0xFFFD) glColor4ubv((GLubyte*)(&color));");
        cWriter.WriteLine($"    else if (c[3] != 0xFFFF) glColor4ubv((GLubyte*)(_model_colors + c[3] * 4));");
        cWriter.WriteLine($"    if (c[4]) glLineWidth(c[4] * 0.1f);");
        cWriter.WriteLine($"    glDrawArrays(c[0], c[1], c[2]);");
        cWriter.WriteLine($"    c += 5;");
        cWriter.WriteLine($"  }}");
        cWriter.WriteLine($"  glDisableClientState(GL_VERTEX_ARRAY);");
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

        // Radial collision profiles: flat 2D array + simple lookup
        cWriter.WriteLine($"static const uint8_t _radial_profiles[][16] = {{");
        for (int j = 0; j < models.Length; j++)
        {
            var profile = models[j].GetRadialProfile();
            if (profile != null)
                cWriter.WriteLine($"  {{ {string.Join(", ", profile)} }}, // {j}: {models[j].FileName}");
            else
                cWriter.WriteLine($"  {{ 0 }}, // {j}: {models[j].FileName} (no profile)");
        }
        cWriter.WriteLine($"}};");
        cWriter.WriteLine();

        cWriter.WriteLine($"const uint8_t* _generated_get_radial_profile(uint16_t model_idx) {{");
        cWriter.WriteLine($"  if (model_idx >= {models.Length}) return 0;");
        cWriter.WriteLine($"  return _radial_profiles[model_idx];");
        cWriter.WriteLine($"}}");
        cWriter.WriteLine();
    }
}
