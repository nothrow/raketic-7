using KeraLua;
using raketic.modelgen.Svg;
using raketic.modelgen.Utils;

namespace raketic.modelgen.Entity;

internal abstract class BaseData
{
    public virtual BaseData ReadFromTable(string key, LuaType type, Lua lua)
    {
        throw new InvalidOperationException($"Unknown field '{key}' in entity definition");
    }
}

internal class EntityData : BaseData
{
    public string? Type { get; init; }
    public int? ModelRef { get; init; }
    public Model? Model { get; init; }
    public int? Mass { get; init; }
    public int? Radius { get; init; }
    public Point? Position { get; init; }

    public static EntityData Empty { get; } = new EntityData();

    public override BaseData ReadFromTable(string key, LuaType type, Lua lua)
    {
        switch (key)
        {
            case "dataType":
                // skip
                return this;
            case "type":
                if (type != LuaType.String)
                    throw new InvalidOperationException($"Invalid type for 'type' field in entity definition, expected string but got {type}");

                return Extend(new EntityData { Type = lua.ToString(-1) });
            case "position":
                var pointer = lua.CheckUserData(-1, "Point");
                if (pointer == IntPtr.Zero)
                    throw new InvalidOperationException($"Invalid type for 'position' field in entity definition, expected Point but got {type}");

                var point = System.Runtime.InteropServices.Marshal.PtrToStructure<Point>(pointer);
                return Extend(new EntityData { Position = point });
            case "model":
                var modelPtr = lua.CheckUserData(-1, "Model");
                if (modelPtr == IntPtr.Zero)
                    throw new InvalidOperationException($"Invalid type for 'model' field in entity definition, expected Model but got {type}");

                var modelIdx = System.Runtime.InteropServices.Marshal.ReadInt32(modelPtr);
                return Extend(new EntityData { ModelRef = modelIdx });
            case "mass":
                if (type != LuaType.Number)
                    throw new InvalidOperationException($"Invalid type for 'mass' field in entity definition, expected number but got {type}");

                return Extend(new EntityData { Mass = (int?)lua.ToIntegerX(-1) });

            case "radius":
                if (type != LuaType.Number)
                    throw new InvalidOperationException($"Invalid type for 'radius' field in entity definition, expected number but got {type}");

                return Extend(new EntityData { Radius = (int?)lua.ToInteger(-1) });
            default:
                return base.ReadFromTable(key, type, lua);
        }
    }

    public virtual EntityData Extend(EntityData other)
    {
        return new EntityData
        {
            Type = other.Type ?? Type,
            ModelRef = other.ModelRef ?? ModelRef,
            Model = other.Model ?? Model,
            Mass = other.Mass ?? Mass,
            Radius = other.Radius ?? Radius,
            Position = other.Position ?? Position
        };
    }
}

internal class SlotData
{
    public required string SlotRef { get; init; }
    public EntityData? Entity { get; init; }
}

internal class EntityWithSlotsData : EntityData
{
    public SlotData[] Slots { get; init; } = Array.Empty<SlotData>();

    public static new EntityWithSlotsData Empty { get; } = new EntityWithSlotsData();

    public override EntityData Extend(EntityData other)
    {
        var slots = (other as EntityWithSlotsData)?.Slots ?? Slots;
        return new EntityWithSlotsData
        {
            Type = other.Type ?? Type,
            ModelRef = other.ModelRef ?? ModelRef,
            Model = other.Model ?? Model,
            Mass = other.Mass ?? Mass,
            Radius = other.Radius ?? Radius,
            Position = other.Position ?? Position,
            Slots = slots
        };
    }
}

internal class EntityContext(PathInfo paths)
{
    private readonly List<BaseData> _entityCache = new();
    private readonly Dictionary<string, int> _entityCacheKeys = new();

    private readonly List<BaseData> _partsCache = new();
    private readonly Dictionary<string, int> _partsCacheKeys = new();

    private BaseData ReadFromTable(BaseData start, Lua lua)
    {
        BaseData ret = start;
        lua.PushNil();
        while (lua.Next(-2))
        {
            var key = lua.ToString(-2);
            ret = ret.ReadFromTable(key, lua.Type(-1), lua);
            lua.Pop(1);
        }
        return ret;
    }

    public void RegisterForLua(Lua lua)
    {
        lua.RegisterGlobalCache("entities", EntitiesRetrieve);
        lua.RegisterGlobalCache("parts", PartsRetrieve);

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


    private int PartWith(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var self = lua.CheckUserData(1, "Part");
        if (self == IntPtr.Zero)
            throw new InvalidOperationException("Expected Part instance as first argument to 'with'");

        var partIndex = System.Runtime.InteropServices.Marshal.ReadInt32(self);
        var original = _partsCache[partIndex];

        var overrideData = ReadFromTable(original, lua);

        _partsCache.Add(overrideData);

        PushPart(lua, _partsCache.Count - 1);
        return 1;
    }

    private static EntityData GetEntityDataFromDataType(string dataType)
    {
        switch (dataType)
        {
            case "EntityWithSlotsData":
                return new EntityWithSlotsData();
            case "EntityData":
                return EntityData.Empty;
            default:
                throw new InvalidOperationException($"Unknown data type: {dataType}");
        }
    }

    private int EntityConstructor(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);

        lua.PushString("__dataType");
        lua.GetTable(-2);

        if (lua.Type(-1) != LuaType.String)
            throw new InvalidOperationException($"Invalid type for '__dataType' field in entity definition, expected string but got {lua.Type(-1)}");

        var dataType = lua.ToString(-1);
        lua.Pop(1);

        var entityData = ReadFromTable(GetEntityDataFromDataType(dataType), lua);
        lua.Pop(1);

        var entityIndex = _entityCache.Count;
        _entityCache.Add(entityData);

        PushEntity(lua, entityIndex);
        return 1;
    }

    private void PushPart(Lua lua, int partIndex)
    {
        var userDataPtr = lua.NewUserData(sizeof(int));

        System.Runtime.InteropServices.Marshal.WriteInt32(userDataPtr, partIndex);

        if (lua.NewMetaTable("Part"))
        {
            lua.PushString("__index");
            lua.PushCFunction((nint state) =>
            {
                var l = Lua.FromIntPtr(state);
                var partIdx = System.Runtime.InteropServices.Marshal.ReadInt32(l.ToUserData(1));
                var part = _partsCache[partIdx];
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
            lua.PushCFunction(PartWith);
            lua.SetTable(-3);
        }
        lua.SetMetaTable(-2);
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

    public BaseData GetEntityData(Lua lua, int id)
    {
        var entityRef = lua.CheckUserData(id, "Entity");
        if (entityRef == IntPtr.Zero)
            throw new InvalidOperationException($"Expected Entity instance at index {id}");

        var entityIdx = System.Runtime.InteropServices.Marshal.ReadInt32(entityRef);
        return _entityCache[entityIdx];
    }

    private int PartsRetrieve(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var key = lua.ToString(2);
        lua.Pop(2);

        var fullPath = Path.Combine(paths.DataDir, "parts", Path.ChangeExtension(key, ".lua"));

        if (!_partsCacheKeys.TryGetValue(fullPath, out var partIndex))
        {
            LoadObjectFromLuaFile(fullPath, lua);

            var partRef = lua.CheckUserData(1, "Part");
            if (partRef == IntPtr.Zero)
                throw new InvalidOperationException($"Part file '{fullPath}' did not return an Part instance");
            var partIdx = System.Runtime.InteropServices.Marshal.ReadInt32(partRef);
            _partsCacheKeys[fullPath] = partIdx;
            // keep the entity on top of the stack
        }
        else
        {
            PushPart(lua, partIndex);
        }

        return 1;
    }

    private int EntitiesRetrieve(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var key = lua.ToString(2);
        lua.Pop(2);

        var fullPath = Path.Combine(paths.DataDir, "entities", Path.ChangeExtension(key, ".lua"));

        if (!_entityCacheKeys.TryGetValue(fullPath, out var entityIndex))
        {
            LoadObjectFromLuaFile(fullPath, lua);


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

    private static void LoadObjectFromLuaFile(string objectPath, Lua lua)
    {
        if (lua.LoadFile(objectPath) != LuaStatus.OK)
        {
            var error = lua.ToString(-1);
            throw new InvalidOperationException($"Error loading object file '{objectPath}': {error}");
        }

        var result = lua.PCall(0, 1, 0);
        if (result != LuaStatus.OK)
        {
            var error = lua.ToString(-1);
            throw new InvalidOperationException($"Error executing object file '{objectPath}': {error}");
        }
    }
}
