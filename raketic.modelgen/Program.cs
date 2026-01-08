using raketic.modelgen;
using System.Runtime.CompilerServices;

// relative paths:
// sln/models/ for all svgs
// sln/engine/generated/renderer.gen.c for output
// sln/engine/generated/renderer.gen.h for output

var paths = Paths.Get();

Console.WriteLine($"Models directory: {paths.ModelsDir}");
Console.WriteLine($"Output C file:    {paths.OutputC}");
Console.WriteLine($"Output H file:    {paths.OutputHRenderer}");
Console.WriteLine($"                  {paths.OutputHSlots}");
Console.WriteLine();

var svgFiles = Directory.GetFiles(paths.ModelsDir, "*.svg");

Console.WriteLine($"Found {svgFiles.Length} SVG file(s):");
var models = svgFiles.Select(SvgParser.ParseSvg).ToArray();

int i = 0;

using var hWriter = new StreamWriter(paths.OutputHRenderer, false);
using var hsWriter = new StreamWriter(paths.OutputHSlots, false);
using var cWriter = new StreamWriter(paths.OutputC, false);

Console.WriteLine();

hsWriter.WriteLine("#pragma once");
hsWriter.WriteLine("#include \"platform/platform.h\"");
hsWriter.WriteLine("#include \"entity/entity.h\"");
hsWriter.WriteLine("void _generated_fill_slots(uint16_t index, entity_id_t parent);");

hWriter.WriteLine("#pragma once");
hWriter.WriteLine("#include \"platform/platform.h\"");
hWriter.WriteLine("void _generated_draw_model(color_t color, uint16_t index);");

cWriter.WriteLine("#include \"renderer.gen.h\"");
cWriter.WriteLine("#include \"slots.gen.h\"");
cWriter.WriteLine("#include <Windows.h>");
cWriter.WriteLine("#include <gl/GL.h>");

var modelWriter = new ModelWriter();

foreach (var model in models)
{
    Console.Write($"Processing model: {model.FileName}");

    var written = modelWriter.DumpModelData(cWriter, model);
    Console.WriteLine($" - written {written} vertices");

    hWriter.WriteLine($"#define MODEL_{model.FileName.ToUpper()}_IDX ((uint16_t){i++})");
}

modelWriter.DumpModelColors(cWriter, models);

foreach (var model in models)
{
    modelWriter.DumpModel(cWriter, model);
}

foreach(var model in models)
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

cWriter.WriteLine($"void _generated_fill_slots(uint16_t index, entity_id_t parent) {{");
cWriter.WriteLine($"  switch (index) {{");
foreach (var model in models)
{
    if (model.Slots.Length == 0)
        cWriter.WriteLine($"    case MODEL_{model.FileName.ToUpper()}_IDX: return;");
    else
    {
        cWriter.WriteLine($"    case MODEL_{model.FileName.ToUpper()}_IDX:");
        cWriter.WriteLine($"      _model_{model.FileName}_slots(parent); break;");
    }
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
            OutputC = Path.Combine(slnDir, "engine", "generated", "models.gen.c"),
            OutputHRenderer = Path.Combine(slnDir, "engine", "generated", "renderer.gen.h"),
            OutputHSlots = Path.Combine(slnDir, "engine", "generated", "slots.gen.h")
        };
    }
}

record PathInfo
{
    public required string ModelsDir { get; init; }
    public required string OutputC { get; init; }
    public required string OutputHRenderer { get; init; }
    public required string OutputHSlots { get; init; }
}
