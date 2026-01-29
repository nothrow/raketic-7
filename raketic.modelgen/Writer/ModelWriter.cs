namespace raketic.modelgen.Writer;

internal class ModelWriter
{
    private Dictionary<System.Drawing.Color, int> _colorIndexes = new();

    /// <summary>
    /// Generates models_meta.gen.h header with declarations for fracture system
    /// </summary>
    public void DumpModelMetadataHeader(StreamWriter h, Model[] models)
    {
        h.WriteLine(@"
// generated, do not edit manually - fracture system metadata

#pragma once
#include <stdint.h>

// Draw command types for fracture expansion
typedef enum { CMD_LINE_LOOP = 0, CMD_LINE_STRIP = 1 } DrawCmdType;
typedef struct { uint8_t type; uint8_t start; uint8_t count; } DrawCommand;
");

        // Extern declarations for vertex arrays (defined in models.gen.c)
        foreach (var model in models)
        {
            h.WriteLine($"extern const int8_t _model_{model.FileName}_vertices[];");
        }
        h.WriteLine();

        // Extern declarations for metadata arrays (defined in models_meta.gen.c)
        h.WriteLine("extern const int8_t* _model_vertices[];");
        h.WriteLine("extern const DrawCommand* _model_commands[];");
        h.WriteLine("extern const uint8_t _model_command_counts[];");
        h.WriteLine();
    }

    /// <summary>
    /// Generates models_meta.gen.c with vertex pointers and draw commands for fracture system
    /// </summary>
    public void DumpModelMetadata(StreamWriter w, Model[] models)
    {
        w.WriteLine(@"
// generated, do not edit manually - fracture system metadata

#include ""models_meta.gen.h""
");

        // Generate draw commands for each model
        foreach (var model in models)
        {
            w.WriteLine($"static const DrawCommand _model_commands_{model.FileName}[] = {{");
            int start = 0;
            foreach (var linestrip in model.LineStrips)
            {
                var cmdType = linestrip.IsClosed ? "CMD_LINE_LOOP" : "CMD_LINE_STRIP";
                w.WriteLine($"  {{{cmdType}, {start}, {linestrip.Points.Length}}},");
                start += linestrip.Points.Length;
            }
            w.WriteLine("};");
            w.WriteLine();
        }

        // Array of vertex pointers
        w.WriteLine("const int8_t* _model_vertices[] = {");
        foreach (var model in models)
        {
            w.WriteLine($"  _model_{model.FileName}_vertices,");
        }
        w.WriteLine("};");
        w.WriteLine();

        // Array of command array pointers
        w.WriteLine("const DrawCommand* _model_commands[] = {");
        foreach (var model in models)
        {
            w.WriteLine($"  _model_commands_{model.FileName},");
        }
        w.WriteLine("};");
        w.WriteLine();

        // Command counts per model
        w.WriteLine("const uint8_t _model_command_counts[] = {");
        foreach (var model in models)
        {
            w.WriteLine($"  {model.LineStrips.Length},");
        }
        w.WriteLine("};");
        w.WriteLine();
    }
    public int DumpModelData(StreamWriter w, Model model)
    {
        int points = 0; ;
        // Not static - needed by models_meta.gen.c for fracture system
        w.WriteLine($"const int8_t _model_{model.FileName}_vertices[] = {{");
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
