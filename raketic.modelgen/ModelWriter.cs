using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace raketic.modelgen;

internal static class ModelWriter
{
    public static void DumpModelData(StreamWriter w, Model model)
    {
        w.WriteLine($"static const int8_t _model_{model.FileName}_vertices[] = {{");
        foreach (var linestrip in model.LineStrips)
        {
            w.Write(" ");
            foreach (var point in linestrip.Points)
            {
                checked
                {
                    w.Write($" {(byte)point.X}, {(byte)point.Y},");
                }
            }
            w.WriteLine();
        }
        w.WriteLine("};");
        w.WriteLine();
    }

    private static string GetLineModel(LineStrip linestrip) => linestrip.IsClosed ? "GL_LINE_LOOP" : "GL_LINE_STRIP";

    public static void DumpModel(StreamWriter w, Model model)
    {
        w.WriteLine($"static void _model_{model.FileName}(color_t color) {{");
        w.WriteLine($"  (color);");
        w.WriteLine($"  glEnableClientState(GL_VERTEX_ARRAY);");
        w.WriteLine($"  glVertexPointer(2, GL_BYTE, 0, _model_{model.FileName}_vertices);");

        int start = 0;
        foreach (var linestrip in model.LineStrips)
        {
            var len = linestrip.Points.Length;
            w.WriteLine($"  glDrawArrays({GetLineModel(linestrip)}, {start}, {linestrip.Points.Length});");
            start += len;
        }
        w.WriteLine($"  glDisableClientState(GL_VERTEX_ARRAY);");
        w.WriteLine($"}}");
        w.WriteLine();
    }
}

