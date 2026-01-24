using KeraLua;
using raketic.modelgen.Svg;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace raketic.modelgen.Entity;

internal record EntityData();

internal class EntityContext(PathInfo paths, ModelContext models)
{
    private Dictionary<string, EntityData> _entityCache = new Dictionary<string, EntityData>();

    public void RegisterForLua(Lua lua)
    {
        lua.NewTable();

        lua.NewTable();

        lua.PushString("__index");
        lua.PushCFunction(EntitiesGetRef);

        lua.SetTable(-3);
        lua.SetMetaTable(-2);

        lua.SetGlobal("entities");
    }

    private int EntitiesGetRef(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var key = lua.ToString(2);
        lua.Pop(2);

        var fullPath = Path.Combine(paths.DataDir, "entities", Path.ChangeExtension(key, ".lua"));
        if (_entityCache.TryGetValue(fullPath, out var cached))
        {
            // Push cached entity
            lua.PushLightUserData(nint.Zero); // Placeholder for cached entity
            return 1;
        }
        else
        {
            var entity = EntityParser.LoadEntity(fullPath, models);
            _entityCache[fullPath] = entity;

            // Push new entity
            lua.PushLightUserData(nint.Zero); // Placeholder for new entity
            return 1;
        }
    }

}
internal class EntityParser
{
    public static EntityData LoadEntity(string entityPath, ModelContext models)
    {
        using var lua = new KeraLua.Lua();

        models.RegisterForLua(lua);

        if (lua.LoadFile(entityPath) != LuaStatus.OK)
        {
            var error = lua.ToString(-1);
            throw new InvalidOperationException($"Error loading entity file '{entityPath}': {error}");
        }

        var result = lua.PCall(0, 1, 0);
        if (result != LuaStatus.OK)
        {
            var error = lua.ToString(-1);
            throw new InvalidOperationException($"Error executing entity file '{entityPath}': {error}");
        }

        return new EntityData();
    }

    
}
