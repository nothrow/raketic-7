using KeraLua;

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
}

internal class PointLuaBinding
{
    private static int PointConstructor(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var argc = lua.GetTop();

        double x, y;
        if (argc == 1 && lua.IsTable(1))
        {
            // vec { x = 10, y = 20 } nebo vec { 10, 20 }
            lua.GetField(1, "x");
            if (lua.IsNumber(-1))
            {
                x = lua.ToNumber(-1);
                lua.Pop(1);
                lua.GetField(1, "y");
                if (!lua.IsNumber(-1))
                {
                    lua.PushString("vec: missing y");
                    lua.Error();
                }
                y = lua.ToNumber(-1);
                lua.Pop(1);
            }
            else
            {
                lua.Pop(1);
                lua.GetInteger(1, 1);
                if (!lua.IsNumber(-1))
                {
                    lua.PushString("vec: missing [1]");
                    lua.Error();
                }
                x = lua.ToNumber(-1);
                lua.Pop(1);
                lua.GetInteger(1, 2);
                if (!lua.IsNumber(-1))
                {
                    lua.PushString("vec: missing [2]");
                    lua.Error();
                }
                y = lua.ToNumber(-1);
                lua.Pop(1);
            }
        }
        else if (argc == 2 && lua.IsNumber(1) && lua.IsNumber(2))
        {
            x = lua.ToNumber(1);
            y = lua.ToNumber(2);
        }
        else
        {
            lua.PushString("vec: expected vec(x, y) or vec { x = .., y = .. }");
            lua.Error();
            return 1;
        }

        var px = (int)x;
        var py = (int)y;

        var userDataPtr = lua.NewUserData(System.Runtime.InteropServices.Marshal.SizeOf<Point>());
        System.Runtime.InteropServices.Marshal.StructureToPtr(new Point(px, py), userDataPtr, false);
        if (lua.NewMetaTable("Point"))
        {
            // add methods or properties if needed
        }
        lua.SetMetaTable(-2);
        return 1;
    }


    public static void RegisterForLua(Lua lua)
    {
        lua.PushCFunction(PointConstructor);
        lua.SetGlobal("vec");
    }
}

internal class EntityContext(PathInfo paths)
{
    private readonly List<EntityData> _entityCache = new();
    private readonly Dictionary<string, int> _entityCacheKeys = new();

    private static EntityData ReadFromTable(EntityData start, Lua lua)
    {
        var ret = start;

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
                case "position":
                    var pointer = lua.CheckUserData(-1, "Point");
                    if (pointer == IntPtr.Zero)
                        throw new InvalidOperationException($"Invalid type for 'position' field in entity definition, expected Point but got {lua.Type(-1)}");

                    var point = System.Runtime.InteropServices.Marshal.PtrToStructure<Point>(pointer);
                    ret = ret with { Position = point };
                    break;
                case "model":

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

    public void RegisterForLua(Lua lua)
    {
        lua.NewTable();
        lua.NewTable();

        lua.PushString("__index");
        lua.PushCFunction(EntitiesRetrieve);

        lua.SetTable(-3);
        lua.SetMetaTable(-2);

        lua.SetGlobal("entities");

        lua.PushCFunction(EntityConstructor);
        lua.SetGlobal("Entity");
    }

    private int EntityWith(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var self = lua.CheckUserData(1, "Entity");
        if (self == IntPtr.Zero)
            throw new InvalidOperationException("Expected Entity instance as first argument to 'with'");

        var entityIndex = System.Runtime.InteropServices.Marshal.ReadInt32(self);
        var original = _entityCache[entityIndex];

        var overrideData = ReadFromTable(original, lua);

        _entityCache.Add(overrideData);

        PushEntity(lua, _entityCache.Count - 1);
        return 1;
    }

    private int EntityConstructor(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);

        var entityData = ReadFromTable(EntityData.Empty, lua);
        lua.Pop(1);

        var entityIndex = _entityCache.Count;
        _entityCache.Add(entityData);

        PushEntity(lua, entityIndex);
        return 1;
    }

    private void PushEntity(Lua lua, int entityIndex)
    {
        var userDataPtr = lua.NewUserData(sizeof(int));

        System.Runtime.InteropServices.Marshal.WriteInt32(userDataPtr, entityIndex);

        if (lua.NewMetaTable("Entity"))
        {
            lua.PushString("__index");
            lua.PushCFunction((nint state) =>
            {
                var l = Lua.FromIntPtr(state);
                var entityIdx = System.Runtime.InteropServices.Marshal.ReadInt32(l.ToUserData(1));
                var entity = _entityCache[entityIdx];
                var key = l.ToString(2);

                l.Pop(2);
                switch (key)
                {
                    default:
                        l.PushString($"Unknown field {key}");
                        l.Error();
                        return 1;
                }
            });
            lua.SetTable(-3);

            lua.PushString("__call");
            lua.PushCFunction(EntityWith);
            lua.SetTable(-3);
        }
        lua.SetMetaTable(-2);
    }

    private int EntitiesRetrieve(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var key = lua.ToString(2);
        lua.Pop(2);

        var fullPath = Path.Combine(paths.DataDir, "entities", Path.ChangeExtension(key, ".lua"));

        if (!_entityCacheKeys.TryGetValue(fullPath, out var entityIndex))
        {
            LoadEntity(fullPath, lua);


            var entityRef = lua.CheckUserData(1, "Entity");
            if (entityRef == IntPtr.Zero)
                throw new InvalidOperationException($"Entity file '{fullPath}' did not return an Entity instance");
            var entityIdx = System.Runtime.InteropServices.Marshal.ReadInt32(entityRef);
            _entityCacheKeys[fullPath] = entityIdx;
            // keep the entity on top of the stack
        }
        else
        {
            PushEntity(lua, entityIndex);
        }


        return 1;
    }

    private static void LoadEntity(string entityPath, Lua lua)
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
    }
}
