using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace raketic.modelgen;

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
            modelWriter.DumpModelRadius(cWriter, model);

            Console.WriteLine($" - written {written} vertices");

            hWriter.WriteLine($"#define MODEL_{model.FileName.ToUpper()}_IDX ((uint16_t){i++})");
        }

        modelWriter.DumpModelColors(cWriter, models);

        foreach (var model in models)
        {
            modelWriter.DumpModel(cWriter, model);
        }

        foreach (var model in models)
        {
            modelWriter.DumpModelSlots(cWriter, model);
        }



        cWriter.WriteLine($"void _generated_draw_model(color_t color, uint16_t index) {{");
        cWriter.WriteLine($"  switch (index) {{");
        foreach (var model in models)
        {
            cWriter.WriteLine($"    case MODEL_{model.FileName.ToUpper()}_IDX:");
            cWriter.WriteLine($"      _model_{model.FileName}(color); break;");
        }
        cWriter.WriteLine($"    default: _ASSERT(0);");
        cWriter.WriteLine($"  }}");
        cWriter.WriteLine($"}}");
        cWriter.WriteLine();
        cWriter.WriteLine($"uint16_t _generated_get_model_radius(uint16_t index) {{");
        cWriter.WriteLine($"  switch (index) {{");
        foreach (var model in models)
        {
            cWriter.WriteLine($"    case MODEL_{model.FileName.ToUpper()}_IDX: return _model_{model.FileName}_radius;");
        }
        cWriter.WriteLine($"    default: _ASSERT(0); return 0;");
        cWriter.WriteLine($"  }}");
        cWriter.WriteLine($"}}");
        cWriter.WriteLine();
    }
}
