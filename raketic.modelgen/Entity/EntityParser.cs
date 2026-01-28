using KeraLua;
using raketic.modelgen.Svg;
using raketic.modelgen.Utils;
using System.Reflection;

namespace raketic.modelgen.Entity;

internal abstract record BaseEntityWithModelData
{
    public int? SpawnId { get; init; }
    public string? Type { get; init; }
    public int? ModelRef { get; init; }
    public Model? Model { get; init; }
    public virtual IEnumerable<Model> AllModels
    {
        get
        {
            if (Model != null)
                yield return Model;
        }
    }

    public virtual BaseEntityWithModelData ReadFromTable(string key, LuaType type, Lua lua)
    {
        switch (key)
        {
            case "__dataType": // skip
                return this;

            case "model":
                var modelPtr = lua.CheckUserData(-1, "Model");
                if (modelPtr == IntPtr.Zero)
                    throw new InvalidOperationException($"Invalid type for 'model' field in entity definition, expected Model but got {type}");

                var modelIdx = System.Runtime.InteropServices.Marshal.ReadInt32(modelPtr);
                return this with { ModelRef = modelIdx };

            case "type":
                if (type != LuaType.String)
                    throw new InvalidOperationException($"Invalid type for 'type' field in entity definition, expected string but got {type}");

                return this with { Type = lua.ToString(-1) };
            default:
                throw new InvalidOperationException(
                    $"Unknown field '{key}' in entity definition of type '{this.GetType().Name}'");

        }
    }

    public virtual BaseEntityWithModelData ResolveModels(ModelContext modelContext, EntityContext entityContext)
    {
        if (ModelRef.HasValue)
        {
            var model = modelContext[ModelRef.Value];
            return this with { Model = model };
        }

        return this;
    }
}

internal record PartData : BaseEntityWithModelData
{
    public virtual void DumpPartData(StreamWriter w, string dataref)
    {
    }
}

internal record EnginePartData : PartData
{
    public static EnginePartData Empty { get; } = new EnginePartData();

    public int? ParticleModelRef { get; init; }
    public Model? ParticleModel { get; init; }
    public int? Power { get; init; }

    public override BaseEntityWithModelData ReadFromTable(string key, LuaType type, Lua lua)
    {
        switch (key)
        {
            case "particleModel":
                var modelPtr = lua.CheckUserData(-1, "Model");
                if (modelPtr == IntPtr.Zero)
                    throw new InvalidOperationException($"Invalid type for 'particleModel' field in engine part definition, expected Model but got {type}");
                var modelIdx = System.Runtime.InteropServices.Marshal.ReadInt32(modelPtr);
                return this with { ParticleModelRef = modelIdx };
            case "power":
                if (type != LuaType.Number)
                    throw new InvalidOperationException($"Invalid type for 'power' field in engine part definition, expected number but got {type}");

                return this with { Power = (int?)lua.ToIntegerX(-1) };
            default:
                return base.ReadFromTable(key, type, lua);
        }
    }

    public override IEnumerable<Model> AllModels
    {
        get
        {
            if (Model != null)
                yield return Model;

            if (ParticleModel != null)
                yield return ParticleModel;
        }
    }

    public override void DumpPartData(StreamWriter w, string dataref)
    {
        w.WriteLine($"  ((struct engine_data*)({dataref}))->power = {Power / 100.0:0.0#######}f;");
        w.WriteLine($"  ((struct engine_data*)({dataref}))->particle_model = {ParticleModel!.ModelConstantName};");
    }

    public override BaseEntityWithModelData ResolveModels(ModelContext modelContext, EntityContext entityContext)
    {
        var ret = (EnginePartData)base.ResolveModels(modelContext, entityContext);
        if (ParticleModelRef.HasValue)
        {
            var model = modelContext[ParticleModelRef.Value];
            return ret with { ParticleModel = model };
        }

        return ret;
    }
}

internal record EntityData : BaseEntityWithModelData
{
    public int? Mass { get; init; }
    public int? Radius { get; init; }
    public Point? Position { get; init; }
    public float? Rotation { get; init; }

    public static EntityData Empty { get; } = new EntityData();

    public override BaseEntityWithModelData ReadFromTable(string key, LuaType type, Lua lua)
    {
        switch (key)
        {
            case "dataType":
                // skip
                return this;
            case "position":
                var pointer = lua.CheckUserData(-1, "Point");
                if (pointer == IntPtr.Zero)
                    throw new InvalidOperationException($"Invalid type for 'position' field in entity definition, expected Point but got {type}");

                var point = System.Runtime.InteropServices.Marshal.PtrToStructure<Point>(pointer);
                return this with { Position = point };
            case "rotation":
                if (type != LuaType.Number)
                    throw new InvalidOperationException($"Invalid type for 'rotation' field in entity definition, expected number but got {type}");

                return this with { Rotation = (float?)lua.ToNumberX(-1) };
            case "mass":
                if (type != LuaType.Number)
                    throw new InvalidOperationException($"Invalid type for 'mass' field in entity definition, expected number but got {type}");

                return this with { Mass = (int?)lua.ToIntegerX(-1) };

            case "radius":
                if (type != LuaType.Number)
                    throw new InvalidOperationException($"Invalid type for 'radius' field in entity definition, expected number but got {type}");

                return this with { Radius = (int?)lua.ToInteger(-1) };
            default:
                return base.ReadFromTable(key, type, lua);
        }
    }
}

internal record SlotData
{
    public required string SlotName { get; init; }
    public int? SlotRef { get; init; }
    public int EntityRef { get; init; }
    public BaseEntityWithModelData? Entity { get; init; }
}

internal record EntityWithSlotsData : EntityData
{
    public SlotData[] Slots { get; init; } = Array.Empty<SlotData>();

    public static new EntityWithSlotsData Empty { get; } = new EntityWithSlotsData();

    public override IEnumerable<Model> AllModels
    {
        get
        {
            if (Model != null)
                yield return Model;
            foreach (var slot in Slots)
            {
                if (slot.Entity != null)
                {
                    foreach (var model in slot.Entity.AllModels)
                    {
                        yield return model;
                    }
                }
            }
        }
    }

    private static SlotData[] ReadSlotsFromTable(Lua lua)
    {
        var slots = new List<SlotData>();

        lua.PushNil();
        while (lua.Next(-2))
        {
            var key = lua.ToString(-2);

            var valuePtr = lua.CheckUserData(-1, "BaseEntity");
            if (valuePtr == IntPtr.Zero)
                throw new InvalidOperationException($"Invalid type for 'slot' field in entity definition, expected BaseEntity but got {lua.Type(-1)}");

            var objectIndex = System.Runtime.InteropServices.Marshal.ReadInt32(valuePtr);
            slots.Add(new SlotData
            {
                SlotName = key,
                EntityRef = objectIndex
            });
            lua.Pop(1);
        }

        return [.. slots];
    }

    public override BaseEntityWithModelData ResolveModels(ModelContext modelContext, EntityContext entityContext)
    {
        var ret = (EntityWithSlotsData)base.ResolveModels(modelContext, entityContext);

        if (ret.Model != null)
        {
            var slots = ResolveSlots(ret.Slots, ret.Model, modelContext, entityContext);
            return ret with { Slots = slots };
        }
        return ret;
    }

    private static SlotData[] ResolveSlots(SlotData[] slots, Model model, ModelContext modelContext, EntityContext entityContext)
    {
        return slots.Select(slot =>
        {
            var index = model.Slots.Index().FirstOrDefault(x => x.Item.Name == slot.SlotName);
            if (index == default)
                throw new InvalidOperationException($"Model '{model.FileName}' does not have a slot named '{slot.SlotName}'");

            return slot with { SlotRef = index.Index, Entity = entityContext[slot.EntityRef].ResolveModels(modelContext, entityContext) };
        }).ToArray();
    }

    public override BaseEntityWithModelData ReadFromTable(string key, LuaType type, Lua lua)
    {
        switch (key)
        {
            case "slots":
                if (type != LuaType.Table)
                    throw new InvalidOperationException($"Invalid type for 'mass' field in entity definition, expected number but got {type}");

                return this with
                {
                    Slots = ReadSlotsFromTable(lua)
                };
            default:
                return base.ReadFromTable(key, type, lua);
        }
    }
}

internal class EntityContext(PathInfo paths)
{
    private readonly List<BaseEntityWithModelData> _entityCache = new();
    private readonly Dictionary<string, int> _entityCacheKeys = new();
    private readonly Dictionary<string, int> _partsCacheKeys = new();


    public BaseEntityWithModelData this[int index]
    {
        get
        {
            if (index < 0 || index >= _entityCache.Count)
                throw new IndexOutOfRangeException($"Entity index {index} is out of range (0..{_entityCache.Count - 1})");
            return _entityCache[index];
        }
    }

    private BaseEntityWithModelData ReadFromTable(BaseEntityWithModelData start, Lua lua)
    {
        BaseEntityWithModelData ret = start;
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
        var self = lua.CheckUserData(1, "BaseEntity");
        if (self == IntPtr.Zero)
            throw new InvalidOperationException("Expected Entity instance as first argument to 'with'");

        var entityIndex = System.Runtime.InteropServices.Marshal.ReadInt32(self);
        var original = _entityCache[entityIndex];

        var overrideData = ReadFromTable(original, lua);

        _entityCache.Add(overrideData);

        PushEntity(lua, _entityCache.Count - 1);
        return 1;
    }

    private static BaseEntityWithModelData GetEntityDataFromDataType(string dataType)
    {
        switch (dataType)
        {
            case "EntityWithSlotsData":
                return new EntityWithSlotsData();
            case "EntityData":
                return EntityData.Empty;
            case "EngineData":
                return EnginePartData.Empty;
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

    private void PushEntity(Lua lua, int entityIndex)
    {
        var userDataPtr = lua.NewUserData(sizeof(int));

        System.Runtime.InteropServices.Marshal.WriteInt32(userDataPtr, entityIndex);

        if (lua.NewMetaTable("BaseEntity"))
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

    // returns index of the entity
    public void PushEntityData(Lua lua, BaseEntityWithModelData entity)
    {
        _entityCache.Add(entity);
        PushEntity(lua, _entityCache.Count - 1);
    }

    public BaseEntityWithModelData GetEntityData(Lua lua, int id)
    {
        var entityRef = lua.CheckUserData(id, "BaseEntity");
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

            var partRef = lua.CheckUserData(1, "BaseEntity");
            if (partRef == IntPtr.Zero)
                throw new InvalidOperationException($"Part file '{fullPath}' did not return an Part instance");
            var partIdx = System.Runtime.InteropServices.Marshal.ReadInt32(partRef);
            _partsCacheKeys[fullPath] = partIdx;
            // keep the entity on top of the stack
        }
        else
        {
            PushEntity(lua, partIndex);
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


            var entityRef = lua.CheckUserData(1, "BaseEntity");
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
