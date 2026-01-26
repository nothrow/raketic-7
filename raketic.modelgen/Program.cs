using raketic.modelgen;
using raketic.modelgen.Entity;
using raketic.modelgen.Svg;
using raketic.modelgen.World;
using raketic.modelgen.Writer;

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

Console.WriteLine($"Worlds contain {worldsParser.Entities.Count} entities using {worldsParser.Models.Count} unique models.");

using var writer = new WorldWriter(paths);

writer.WriteHeaders();
writer.WriteModels(worldsParser.Models);

return;

/*

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
*/
