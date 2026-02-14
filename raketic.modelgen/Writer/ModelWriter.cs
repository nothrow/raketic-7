namespace raketic.modelgen.Writer;

internal class ModelWriter
{
    private Dictionary<System.Drawing.Color, int> _colorIndexes = new();
    public int DumpModelData(StreamWriter w, Model model)
    {
        int points = 0; ;
        w.WriteLine($"static const int16_t _model_{model.FileName}_vertices[] = {{");
        foreach (var linestrip in model.LineStrips)
        {
            w.Write(" ");
            foreach (var point in linestrip.Points)
            {
                checked
                {
                    w.Write($" {(short)point.X}, {(short)point.Y},");
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

    // Command encoding for draw command list:
    // Each command is 5 uint16_t: { gl_mode, start, count, color_idx, width_x10 }
    // gl_mode: 2=GL_LINE_LOOP, 3=GL_LINE_STRIP, 6=GL_TRIANGLE_FAN, 0=END
    // color_idx: 0-253 = palette index, 0xFFFE = black, 0xFFFD = param color, 0xFFFF = no change
    // width_x10: line width * 10, 0 = no change

    private const ushort COLOR_NO_CHANGE = 0xFFFF;
    private const ushort COLOR_PARAM = 0xFFFD;
    private const ushort COLOR_BLACK = 0xFFFE;

    private static ushort GlMode(LineStrip ls) => ls.IsClosed ? (ushort)2 : (ushort)3; // GL_LINE_LOOP : GL_LINE_STRIP

    /// <summary>
    /// Emits compact draw command data array for a model.
    /// </summary>
    public void DumpModelCommands(StreamWriter w, Model model)
    {
        var cmds = new List<ushort>();

        // First pass: hull fill commands
        int hullStart = 0;
        bool hasHull = model.LineStrips.Any(ls => ls.Class == "hull");
        if (hasHull)
        {
            foreach (var linestrip in model.LineStrips)
            {
                if (linestrip.Class == "hull" && linestrip.IsClosed)
                {
                    cmds.AddRange(new ushort[] { 6, (ushort)hullStart, (ushort)linestrip.Points.Length, COLOR_BLACK, 0 });
                }
                hullStart += linestrip.Points.Length;
            }
        }

        // Second pass: wireframe draw commands
        int start = 0;
        int previousColorIndex = -2; // -1 = heat (param color)
        float previousWidth = -1f;

        foreach (var linestrip in model.LineStrips)
        {
            var len = linestrip.Points.Length;
            ushort colorVal = COLOR_NO_CHANGE;
            ushort widthVal = 0;

            if (linestrip.Class == "heat")
            {
                if (previousColorIndex != -1) colorVal = COLOR_PARAM;
                previousColorIndex = -1;
            }
            else
            {
                int cidx = _colorIndexes[linestrip.Color];
                if (previousColorIndex != cidx) colorVal = (ushort)cidx;
                previousColorIndex = cidx;
            }

            if (linestrip.StrokeWidth != previousWidth)
            {
                widthVal = (ushort)(linestrip.StrokeWidth * 10.0f + 0.5f);
                previousWidth = linestrip.StrokeWidth;
            }

            cmds.AddRange(new ushort[] { GlMode(linestrip), (ushort)start, (ushort)len, colorVal, widthVal });
            start += len;
        }

        cmds.Add(0); // END terminator

        w.Write($"static const uint16_t _model_{model.FileName}_cmds[] = {{");
        w.Write($" {string.Join(", ", cmds)}");
        w.WriteLine(" };");
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
