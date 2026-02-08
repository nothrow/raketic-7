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

    // Command encoding for compact draw command list:
    // Each command is 5 bytes: { gl_mode, start, count, color_idx, width_x10 }
    // gl_mode: 2=GL_LINE_LOOP, 3=GL_LINE_STRIP, 6=GL_TRIANGLE_FAN, 0=END
    // color_idx: 0-253 = palette index, 0xFE = black, 0xFD = param color, 0xFF = no change
    // width_x10: line width * 10, 0 = no change

    private const byte COLOR_NO_CHANGE = 0xFF;
    private const byte COLOR_PARAM = 0xFD;
    private const byte COLOR_BLACK = 0xFE;

    private static byte GlMode(LineStrip ls) => ls.IsClosed ? (byte)2 : (byte)3; // GL_LINE_LOOP : GL_LINE_STRIP

    /// <summary>
    /// Emits compact draw command data array for a model.
    /// </summary>
    public void DumpModelCommands(StreamWriter w, Model model)
    {
        var cmds = new List<byte>();

        // First pass: hull fill commands
        int hullStart = 0;
        bool hasHull = model.LineStrips.Any(ls => ls.Class == "hull");
        if (hasHull)
        {
            foreach (var linestrip in model.LineStrips)
            {
                if (linestrip.Class == "hull" && linestrip.IsClosed)
                {
                    cmds.AddRange(new byte[] { 6, (byte)hullStart, (byte)linestrip.Points.Length, COLOR_BLACK, 0 });
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
            byte colorByte = COLOR_NO_CHANGE;
            byte widthByte = 0;

            if (linestrip.Class == "heat")
            {
                if (previousColorIndex != -1) colorByte = COLOR_PARAM;
                previousColorIndex = -1;
            }
            else
            {
                int cidx = _colorIndexes[linestrip.Color];
                if (previousColorIndex != cidx) colorByte = (byte)cidx;
                previousColorIndex = cidx;
            }

            if (linestrip.StrokeWidth != previousWidth)
            {
                widthByte = (byte)(linestrip.StrokeWidth * 10.0f + 0.5f);
                previousWidth = linestrip.StrokeWidth;
            }

            cmds.AddRange(new byte[] { GlMode(linestrip), (byte)start, (byte)len, colorByte, widthByte });
            start += len;
        }

        cmds.Add(0); // END terminator

        w.Write($"static const uint8_t _model_{model.FileName}_cmds[] = {{");
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
