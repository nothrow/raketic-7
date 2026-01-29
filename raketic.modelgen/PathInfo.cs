record PathInfo
{
    public required string ModelsDir { get; init; }
    public required string OutputC { get; init; }
    public required string OutputMetaC { get; init; }
    public required string OutputMetaH { get; init; }
    public required string OutputHRenderer { get; init; }
    public required string OutputHSlots { get; init; }
    public required string DataDir { get; init; }
    public required string CommonLibLua { get; init; }
}
