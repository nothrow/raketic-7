using KeraLua;
using raketic.modelgen.Entity;
using raketic.modelgen.Svg;

namespace raketic.modelgen.World;

internal record WorldsData(string WorldName, EntityData[] Entities);

internal class WorldsParser(ModelContext _modelContext, EntityContext _entityContext)
{
    private List<WorldsData> _worlds = new();
    private List<EntityData> _entities = new();
    
    public IReadOnlyList<WorldsData> Worlds => _worlds;
    public IReadOnlyList<Model> Models => _worlds.SelectMany(x => x.Entities.Select(y => y.Model!)).Where(x => x != null).Distinct().ToList();

    private int Spawn(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var argc = lua.GetTop();
        for(int i = 1; i <= argc; i++)
        {
            var entity = _entityContext.GetEntityData(lua, i);
            if (entity.ModelRef.HasValue)
            {
                var model = _modelContext[entity.ModelRef.Value];
                entity = entity with { Model = model };
            }

            _entities.Add(
                entity
            );
        }
        return argc;
    }

    private void RegisterForLua(Lua lua)
    {
        lua.PushCFunction(Spawn);
        lua.SetGlobal("spawn");
    }

    internal void ParseWorld(string worldPath)
    {
        _entities = new();
        using var lua = new Lua();

        _modelContext.RegisterForLua(lua);
        _entityContext.RegisterForLua(lua);

        RegisterForLua(lua);
        PointLuaBinding.RegisterForLua(lua);

        var worldName = Path.GetFileNameWithoutExtension(worldPath);

        if (lua.LoadFile(worldPath) != LuaStatus.OK)
        {
            var error = lua.ToString(-1);
            throw new InvalidOperationException($"Error loading world file '{worldPath}': {error}");
        }

        var result = lua.PCall(0, 0, 0);
        if (result != LuaStatus.OK)
        {
            var error = lua.ToString(-1);
            throw new InvalidOperationException($"Error executing world file '{worldPath}': {error}");
        }

        _worlds.Add(
            new WorldsData(
                worldName,
                _entities.ToArray()
            )
        );
    }
}
