using System.Runtime.CompilerServices;

static class Paths
{
    public static PathInfo Get([CallerFilePath] string thisFile = "")
    {
        var projDir = Path.GetDirectoryName(thisFile)!;
        var slnDir = Path.GetFullPath(Path.Combine(projDir, ".."));

        return new PathInfo
        {
            DataDir = Path.Combine(slnDir, "data"),
            ModelsDir = Path.Combine(slnDir, "data", "models"),
            OutputC = Path.Combine(slnDir, "engine", "generated", "models.gen.c"),
            OutputMetaC = Path.Combine(slnDir, "engine", "generated", "models_meta.gen.c"),
            OutputMetaH = Path.Combine(slnDir, "engine", "generated", "models_meta.gen.h"),
            OutputHRenderer = Path.Combine(slnDir, "engine", "generated", "renderer.gen.h"),
            OutputHSlots = Path.Combine(slnDir, "engine", "generated", "slots.gen.h"),
            CommonLibLua = Path.Combine(slnDir, "raketic.modelgen", "commonlib.lua"),
        };
    }
}
