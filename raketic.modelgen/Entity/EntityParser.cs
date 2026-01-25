using KeraLua;
using raketic.modelgen.Svg;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace raketic.modelgen.Entity;

internal record EntityData(
    string? Type,
    string? ModelRef,
    int? Mass,
    int? Radius,
    Point? Position
)
{
    public static EntityData Empty { get; } = new EntityData(null, null, null, null, null);

    public void WriteToTable(Lua lua)
    {
        if (Type != null)
        {
            lua.PushString("type");
            lua.PushString(Type);
            lua.SetTable(-3);
        }
        if (ModelRef != null)
        {
            lua.PushString("model");
            lua.PushString(ModelRef);
            lua.SetTable(-3);
        }
        if (Mass != null)
        {
            lua.PushString("mass");
            lua.PushInteger(Mass.Value);
            lua.SetTable(-3);
        }
        if (Radius != null)
        {
            lua.PushString("radius");
            lua.PushInteger(Radius.Value);
            lua.SetTable(-3);
        }
    }

    public static EntityData ReadFromTable(Lua lua)
    {
        var ret = Empty;

        lua.PushNil();
        while (lua.Next(-2))
        {
            var key = lua.ToString(-2);
            switch (key)
            {
                case "type":
                    if (lua.Type(-1) != LuaType.String)
                        throw new InvalidOperationException($"Invalid type for 'type' field in entity definition, expected string but got {lua.Type(-1)}");

                    ret = ret with { Type = lua.ToString(-1) };
                    break;
                case "model":
                    if (lua.Type(-1) != LuaType.Table)
                        throw new InvalidOperationException($"Invalid type for 'model' field in entity definition, expected model but got {lua.Type(-1)}");

                    var (_, modelKey) = ModelContext.ModelFromTable(lua);

                    ret = ret with { ModelRef = modelKey };
                    break;
                case "mass":
                    if (lua.Type(-1) != LuaType.Number)
                        throw new InvalidOperationException($"Invalid type for 'mass' field in entity definition, expected number but got {lua.Type(-1)}");

                    ret = ret with { Mass = (int?)lua.ToIntegerX(-1) };
                    break;
                        
                case "radius":
                    if (lua.Type(-1) != LuaType.Number)
                        throw new InvalidOperationException($"Invalid type for 'radius' field in entity definition, expected number but got {lua.Type(-1)}");

                    ret = ret with { Radius = (int?)lua.ToInteger(-1) };
                    break;
                default:
                    throw new InvalidOperationException($"Unknown field '{key}' in entity definition");
            }
            lua.Pop(1);
        }
        return ret;
    }
}


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

        lua.PushCFunction(EntityConstructor);
        lua.SetGlobal("Entity");
    }

    private static void WriteMetaTable(Lua lua)
    {
        if (lua.NewMetaTable("EntityData"))
        {
            // 'record with'
            lua.PushString("__call");
            lua.PushCFunction(EntityWith);
            lua.SetTable(-3);
        }
    }

    private static int EntityWith(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var original = EntityData.ReadFromTable(lua);
        lua.Pop(1);
        var updates = EntityData.ReadFromTable(lua);
        lua.Pop(1);

        var merged = new EntityData(
            Type: updates.Type ?? original.Type,
            ModelRef: updates.ModelRef ?? original.ModelRef,
            Mass: updates.Mass ?? original.Mass,
            Radius: updates.Radius ?? original.Radius,
            Position: updates.Position ?? original.Position
        );

        lua.NewTable();
        merged.WriteToTable(lua);
        WriteMetaTable(lua);
        lua.SetMetaTable(-2);

        return 1;
    }

    private int EntityConstructor(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);

        var entityData = EntityData.ReadFromTable(lua);
        lua.Pop(1);

        EntityDataToLuaStack(lua, entityData);

        return 1;
    }

    private static void EntityDataToLuaStack(Lua lua, EntityData entity)
    {
        lua.NewTable();
        entity.WriteToTable(lua);
        WriteMetaTable(lua);
        lua.SetMetaTable(-2);
    }

    private int EntitiesGetRef(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var key = lua.ToString(2);
        lua.Pop(2);

        var fullPath = Path.Combine(paths.DataDir, "entities", Path.ChangeExtension(key, ".lua"));

        if (!_entityCache.TryGetValue(fullPath, out var cached))
        {
            _entityCache[fullPath] = cached = EntityParser.LoadEntity(fullPath, lua);
        }

        EntityDataToLuaStack(lua, cached);

        return 1;
    }
}
internal class EntityParser
{
    public static EntityData LoadEntity(string entityPath, Lua lua)
    {
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

        var ret = EntityData.ReadFromTable(lua);
        lua.Pop(1);
        return ret;
    }
}
