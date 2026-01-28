using KeraLua;
using raketic.modelgen.Entity;
using raketic.modelgen.Svg;

namespace raketic.modelgen.World;

internal record WorldsData(string WorldName, BaseEntityWithModelData[] Entities, int? ControlledEntitySpawnId);

internal class WorldsParser(PathInfo _paths, ModelContext _modelContext, EntityContext _entityContext)
{
    private List<WorldsData> _worlds = new();
    private List<BaseEntityWithModelData> _entities = new();
    private int? _controlledEntity = null;

    public IReadOnlyList<WorldsData> Worlds => _worlds;
    public IReadOnlyList<Model> Models => _worlds.SelectMany(x => x.Entities.SelectMany(y => y.AllModels)).Distinct().ToList();

    private int Control(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);

        _controlledEntity = _entityContext.GetEntityData(lua, 1).SpawnId;

        if (_controlledEntity == null)
        {
            lua.PushString("control: entity must have a spawnId");
            lua.Error();
        }

        return 0;
    }

    private int Spawn(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var argc = lua.GetTop();
        for(int i = 1; i <= argc; i++)
        {
            var entity = _entityContext.GetEntityData(lua, i);

            var resolvedEntity = entity.ResolveModels(_modelContext, _entityContext) with { SpawnId = _entities.Count };
            // todo: replace the variable on stack with the resolved entity?
            _entities.Add(
                resolvedEntity
            );

            _entityContext.GetEntityData
        }
        return argc;
    }

    private void RegisterForLua(Lua lua)
    {
        lua.PushCFunction(Spawn);
        lua.SetGlobal("spawn");

        lua.PushCFunction(Control);
        lua.SetGlobal("control");
    }

    internal void ParseWorld(string worldPath)
    {
        _entities = new();
        _controlledEntity = null;

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
                _entities.ToArray(),
                _controlledEntity
            )
        );
    }
}
