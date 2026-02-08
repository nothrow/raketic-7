using KeraLua;
using raketic.modelgen.Entity;
using raketic.modelgen.Svg;

namespace raketic.modelgen.World;

internal record WorldsData(string WorldName, BaseEntityWithModelData[] Entities, int? ControlledEntitySpawnId);

internal class WorldsParser(PathInfo _paths, ModelContext _modelContext, EntityContext _entityContext)
{
    private List<WorldsData> _worlds = new();
    private List<BaseEntityWithModelData> _entities = new();
    private List<Model> _requiredModels = new();
    private int? _controlledEntity = null;

    public IReadOnlyList<WorldsData> Worlds => _worlds;
    public IReadOnlyList<Model> Models => _worlds.SelectMany(x => x.Entities.SelectMany(y => y.AllModels)).Concat(_requiredModels).Distinct().ToList();

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

            // Resolve orbit target SpawnId for celestial body orbits
            if (resolvedEntity is EntityData ed && ed.OrbitTargetCacheIdx.HasValue)
            {
                var target = _entityContext[ed.OrbitTargetCacheIdx.Value];
                if (target.SpawnId == null)
                    throw new InvalidOperationException("Orbit target must be spawned before the orbiting entity");
                resolvedEntity = ((EntityData)resolvedEntity) with { OrbitTargetSpawnId = target.SpawnId };
            }

            _entities.Add(
                resolvedEntity
            );

            _entityContext.PushEntityData(lua, resolvedEntity);
            lua.Replace(i);
        }
        return argc;
    }

    private int RequireModel(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var model = _modelContext.GetModelData(lua, 1);
        if (!_requiredModels.Contains(model))
            _requiredModels.Add(model);
        return 0;
    }

    private void RegisterForLua(Lua lua)
    {
        lua.PushCFunction(Spawn);
        lua.SetGlobal("spawn");

        lua.PushCFunction(Control);
        lua.SetGlobal("control");

        lua.PushCFunction(RequireModel);
        lua.SetGlobal("require_model");
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
