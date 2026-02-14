namespace raketic.modelgen.Surface;

internal class SurfaceContext(PathInfo paths)
{
    private readonly List<SurfaceDataRecord> _surfaceCache = new();
    private readonly Dictionary<string, int> _surfaceCacheKeys = new();

    public IReadOnlyList<SurfaceDataRecord> Surfaces => _surfaceCache;

    /// <summary>
    /// Load a surface by name (file stem), returning its index.
    /// </summary>
    public int LoadSurface(string name)
    {
        if (_surfaceCacheKeys.TryGetValue(name, out var idx))
            return idx;

        var fullPath = Path.Combine(paths.ModelsDir, name + ".lua");
        if (!File.Exists(fullPath))
            throw new FileNotFoundException($"Surface file not found: {fullPath}");

        Console.WriteLine($"Parsing surface file: {fullPath}");
        var data = SurfaceParser.Parse(fullPath);
        idx = _surfaceCache.Count;
        _surfaceCacheKeys[name] = idx;
        _surfaceCache.Add(data);
        return idx;
    }
}
