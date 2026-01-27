using KeraLua;
using raketic.modelgen.Entity;
using raketic.modelgen.Svg;

namespace raketic.modelgen.World;

internal record WorldsData(string WorldName, BaseEntityWithModelData[] Entities);

internal class WorldsParser(PathInfo _paths, ModelContext _modelContext, EntityContext _entityContext)
{
    private List<WorldsData> _worlds = new();
    private List<BaseEntityWithModelData> _entities = new();
    
    public IReadOnlyList<WorldsData> Worlds => _worlds;
    public IReadOnlyList<Model> Models => _worlds.SelectMany(x => x.Entities.Select(y => y.Model!)).Where(x => x != null).Distinct().ToList();

    private int Spawn(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var argc = lua.GetTop();
        for(int i = 1; i <= argc; i++)
        {
            var entity = _entityContext.GetEntityData(lua, i);

            entity.ResolveModels(_modelContext);

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

        // Load common library first
        if (lua.LoadFile(_paths.CommonLibLua) != LuaStatus.OK)
        {
            var error = lua.ToString(-1);
            throw new InvalidOperationException($"Error loading commonlib.lua: {error}");
        }
        if (lua.PCall(0, 0, 0) != LuaStatus.OK)
        {
            var error = lua.ToString(-1);
            throw new InvalidOperationException($"Error executing commonlib.lua: {error}");
        }

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
