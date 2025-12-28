using raketic.modelgen;
using System.Reflection;
using System.Runtime.CompilerServices;

// relative paths:
// sln/models/ for all svgs
// sln/engine/generated/renderer.gen.c for output
// sln/engine/generated/renderer.gen.h for output

var paths = Paths.Get();

Console.WriteLine($"Models directory: {paths.ModelsDir}");
Console.WriteLine($"Output C file:    {paths.OutputC}");
Console.WriteLine($"Output H file:    {paths.OutputH}");
Console.WriteLine();

var svgFiles = Directory.GetFiles(paths.ModelsDir, "*.svg");

Console.WriteLine($"Found {svgFiles.Length} SVG file(s):");
var models = svgFiles.Select(SvgParser.ParseSvg);

int i = 0;

using var hWriter = new StreamWriter(paths.OutputH, false);
using var cWriter = new StreamWriter(paths.OutputC, false);

hWriter.WriteLine("#pragma once");
hWriter.WriteLine("#include \"platform/platform.h\"");
hWriter.WriteLine("void _generated_draw_model(color_t color, uint16_t index);");

cWriter.WriteLine("#include \"renderer.gen.h\"");
cWriter.WriteLine("#include <Windows.h>");
cWriter.WriteLine("#include <gl/GL.h>");


foreach (var model in models)
{
    Console.WriteLine($"Processing model: {model.FileName}");

    ModelWriter.DumpModelData(cWriter, model);

    hWriter.WriteLine($"#define MODEL_{model.FileName.ToUpper()}_IDX {i++}");
}

foreach (var model in models)
{
    ModelWriter.DumpModel(cWriter, model);
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


static class Paths
{
    public static PathInfo Get([CallerFilePath] string thisFile = "")
    {
        var projDir = Path.GetDirectoryName(thisFile)!;
        var slnDir = Path.GetFullPath(Path.Combine(projDir, ".."));

        return new PathInfo
        {
            ModelsDir = Path.Combine(slnDir, "models"),
            OutputC = Path.Combine(slnDir, "engine", "generated", "renderer.gen.c"),
            OutputH = Path.Combine(slnDir, "engine", "generated", "renderer.gen.h")
        };
    }
}

record PathInfo
{
    public required string ModelsDir { get; init; }
    public required string OutputC { get; init; }
    public required string OutputH { get; init; }
}
