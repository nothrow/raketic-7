namespace raketic.modelgen.Writer;

internal class ModelWriter
{
    private Dictionary<System.Drawing.Color, int> _colorIndexes = new();
    public int DumpModelData(StreamWriter w, Model model)
    {
        int points = 0; ;
        w.WriteLine($"static const int8_t _model_{model.FileName}_vertices[] = {{");
        foreach (var linestrip in model.LineStrips)
        {
            w.Write(" ");
            foreach (var point in linestrip.Points)
            {
                checked
                {
                    w.Write($" {(sbyte)point.X}, {(sbyte)point.Y},");
                    ++points;
                }
            }
            w.WriteLine();
        }
        w.WriteLine("};");
        w.WriteLine();
        return points;
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

        // First pass: draw filled black polygons for "hull" class (to occlude stars)
        int hullStart = 0;
        bool hasHull = model.LineStrips.Any(ls => ls.Class == "hull");
        if (hasHull)
        {
            w.WriteLine();
            w.WriteLine($"  //  hull fill (occludes background)");
            w.WriteLine($"  glColor4ub(0, 0, 0, 255);");
            foreach (var linestrip in model.LineStrips)
            {
                if (linestrip.Class == "hull" && linestrip.IsClosed)
                {
                    w.WriteLine($"  glDrawArrays(GL_TRIANGLE_FAN, {hullStart}, {linestrip.Points.Length});");
                }
                hullStart += linestrip.Points.Length;
            }
        }

        w.WriteLine();
        w.WriteLine($"  //  wireframe lines");

        // Second pass: draw wireframe lines
        int start = 0;
        float previousWidth = -1f;

        int previousColorIndex = -2; // -1 is heat

        foreach (var linestrip in model.LineStrips)
        {
            var len = linestrip.Points.Length;

            if (linestrip.Class == "heat")
            {
                if (previousColorIndex != -1)
                    w.WriteLine($"  glColor4ubv((GLubyte*)(&color));");

                previousColorIndex = -1;
            }
            else
            {
                if (previousColorIndex != _colorIndexes[linestrip.Color])
                    w.WriteLine($"  glColor4ubv((GLubyte*)(_model_colors + {_colorIndexes[linestrip.Color] * 4}));");

                previousColorIndex = _colorIndexes[linestrip.Color];
            }
            if (linestrip.StrokeWidth != previousWidth)
            {
                w.WriteLine($"  glLineWidth({linestrip.StrokeWidth:0.0}f);");
                previousWidth = linestrip.StrokeWidth;
            }

            w.WriteLine($"  glDrawArrays({GetLineModel(linestrip)}, {start}, {linestrip.Points.Length});");
            start += len;
        }
        w.WriteLine($"  glDisableClientState(GL_VERTEX_ARRAY);");
        w.WriteLine($"}}");
        w.WriteLine();
    }

    public void DumpModelRadius(StreamWriter w, Model model)
    {
        // Ceiling to be conservative, clamp to uint8 range
        int radius = model.GetRadius();
        if (radius > 65535)
            throw new InvalidOperationException($"Model {model.FileName} has a radius greater than 65535");
        else if (radius > 255)
            w.WriteLine($"static const uint16_t _model_{model.FileName}_radius = (uint16_t){radius};");
        else
            w.WriteLine($"static const uint8_t _model_{model.FileName}_radius = (uint8_t){radius};");

        w.WriteLine();
    }
}
