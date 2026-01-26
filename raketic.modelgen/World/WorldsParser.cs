using KeraLua;
using raketic.modelgen.Entity;
using raketic.modelgen.Svg;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace raketic.modelgen.World;

internal class WorldsParser(ModelContext _modelContext, EntityContext _entityContext)
{
    private List<EntityData> _entities = new();

    public IReadOnlyList<EntityData> Entities => _entities;
    public IReadOnlyList<Model> Models => _entities.Select(x => x.Model!).Where(x => x != null).Distinct().ToList();

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
    }
}
