using raketic.modelgen;
using raketic.modelgen.Entity;
using raketic.modelgen.Svg;
using raketic.modelgen.World;

// relative paths:
// sln/models/ for all svgs
// sln/engine/generated/renderer.gen.c for output
// sln/engine/generated/renderer.gen.h for output

var paths = Paths.Get();

Console.WriteLine($"Data directory:   {paths.DataDir}");
Console.WriteLine($"Output C file:    {paths.OutputC}");
Console.WriteLine($"Output H file:    {paths.OutputHRenderer}");
Console.WriteLine($"                  {paths.OutputHSlots}");
Console.WriteLine();


var modelContext = new ModelContext(paths);
var entityContext = new EntityContext(paths);
var worldsParser = new WorldsParser(modelContext, entityContext);

foreach(var world in Directory.GetFiles(Path.Combine(paths.DataDir, "worlds"), "*.lua"))
{
    Console.WriteLine($"Found world: {Path.GetFileName(world)}");

    worldsParser.ParseWorld(world);
}
return;




var svgFiles = Directory.GetFiles(paths.ModelsDir, "*.svg");

Console.WriteLine($"Found {svgFiles.Length} SVG file(s):");
var models = svgFiles.Select(SvgParser.ParseSvg).ToArray();

int i = 0;

using var hsWriter = new StreamWriter(paths.OutputHSlots, false);

using var hWriter = new StreamWriter(paths.OutputHRenderer, false);
using var cWriter = new StreamWriter(paths.OutputC, false);

Console.WriteLine();

hsWriter.WriteLine("#pragma once");
hsWriter.WriteLine("#include \"platform/platform.h\"");
hsWriter.WriteLine("#include \"entity/entity.h\"");
hsWriter.WriteLine("void _generated_fill_slots(uint16_t index, entity_id_t parent);");

hWriter.WriteLine("#pragma once");
hWriter.WriteLine("#include \"platform/platform.h\"");
hWriter.WriteLine("void _generated_draw_model(color_t color, uint16_t index);");
hWriter.WriteLine("uint16_t _generated_get_model_radius(uint16_t index);");
hWriter.WriteLine();

cWriter.WriteLine("#include \"renderer.gen.h\"");
cWriter.WriteLine("#include \"slots.gen.h\"");
cWriter.WriteLine("#include <Windows.h>");
cWriter.WriteLine("#include <gl/GL.h>");

var svgWriter = new SvgModelWriter(cWriter, hWriter);
svgWriter.Write(models);

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
