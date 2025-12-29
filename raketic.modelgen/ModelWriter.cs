namespace raketic.modelgen;

internal class ModelWriter
{
    private Dictionary<System.Drawing.Color, int> _colorIndexes = new();
    public void DumpModelData(StreamWriter w, Model model)
    {
        w.WriteLine($"static const int8_t _model_{model.FileName}_vertices[] = {{");
        foreach (var linestrip in model.LineStrips)
        {
            w.Write(" ");
            foreach (var point in linestrip.Points)
            {
                checked
                {
                    w.Write($" {(sbyte)point.X}, {(sbyte)point.Y},");
                }
            }
            w.WriteLine();
        }
        w.WriteLine("};");
        w.WriteLine();
    }

    public void DumpModelColors(StreamWriter w, IEnumerable<Model> models)
    {
        w.WriteLine($"static const uint8_t _model_colors[] = {{");
        var colors = models.SelectMany(x => x.LineStrips).Select(x => x.Color).Distinct();
        int i = 0;
        foreach (var color in colors)
        {
            _colorIndexes[color] = i++;
            w.WriteLine($"  {color.R}, {color.G}, {color.B}, {color.A}, // idx {i - 1}");
        }
        w.WriteLine("};");
        w.WriteLine();
    }

    private static string GetLineModel(LineStrip linestrip) => linestrip.IsClosed ? "GL_LINE_LOOP" : "GL_LINE_STRIP";

    public void DumpModel(StreamWriter w, Model model)
    {
        w.WriteLine($"static void _model_{model.FileName}(color_t color) {{");
        w.WriteLine($"  (color);");
        w.WriteLine($"  glEnableClientState(GL_VERTEX_ARRAY);");
        w.WriteLine($"  glVertexPointer(2, GL_BYTE, 0, _model_{model.FileName}_vertices);");

        int start = 0;
        foreach (var linestrip in model.LineStrips)
        {
            var len = linestrip.Points.Length;

            if (linestrip.Class == "heat")
            {
                w.WriteLine($"  glColor4ubv((GLubyte*)(&color));");
            }
            else
            {
                w.WriteLine($"  glColor4ubv((GLubyte*)(_model_colors + {_colorIndexes[linestrip.Color] * 4}));");
            }

            w.WriteLine($"  glDrawArrays({GetLineModel(linestrip)}, {start}, {linestrip.Points.Length});");
            start += len;
        }
        w.WriteLine($"  glDisableClientState(GL_VERTEX_ARRAY);");
        w.WriteLine($"}}");
        w.WriteLine();
    }
}

