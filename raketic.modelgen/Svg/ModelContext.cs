using KeraLua;
using raketic.modelgen.Entity;

namespace raketic.modelgen.Svg;

internal class ModelContext(PathInfo paths)
{
    private const string ModelMetaTableName = "Model";

    private readonly List<Model> _modelCache = new();
    private readonly Dictionary<string, int> _modelCacheKeys = new();

    public Model this[int index]
    {
        get
        {
            if (index < 0 || index >= _modelCache.Count)
                throw new IndexOutOfRangeException($"Model index {index} is out of range (0..{_modelCache.Count - 1})");
            return _modelCache[index];
        }
    }

    public void RegisterForLua(Lua lua)
    {
        lua.NewTable();

        lua.NewTable();

        lua.PushString("__index");
        lua.PushCFunction(ModelsRetrieve);

        lua.SetTable(-3);
        lua.SetMetaTable(-2);

        lua.SetGlobal("models");
    }

    private void PushModel(Lua lua, int modelIndex)
    {
        var userDataPtr = lua.NewUserData(sizeof(int));

        System.Runtime.InteropServices.Marshal.WriteInt32(userDataPtr, modelIndex);

        if (lua.NewMetaTable(ModelMetaTableName))
        {
            lua.PushString("__index");
            lua.PushCFunction((nint state) =>
            {
                var l = Lua.FromIntPtr(state);
                var modelIdx = System.Runtime.InteropServices.Marshal.ReadInt32(l.ToUserData(1));
                var model = _modelCache[modelIdx];
                var key = l.ToString(2);

                l.Pop(2);
                switch (key)
                {
                    case "radius":
                        l.PushNumber(model.GetRadius());
                        return 1;
                    default:
                        l.PushString($"Unknown field {key}");
                        l.Error();
                        return 1;
                }
            });
            lua.SetTable(-3);
        }
        lua.SetMetaTable(-2);
    }

    public Model GetModelData(Lua lua, int id)
    {
        var entityRef = lua.CheckUserData(id, ModelMetaTableName);
        if (entityRef == IntPtr.Zero)
            throw new InvalidOperationException($"Expected Model instance on stack at index {id}");

        var modelIdx = System.Runtime.InteropServices.Marshal.ReadInt32(entityRef);
        return _modelCache[modelIdx];
    }

    private int ModelsRetrieve(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var key = lua.ToString(2);
        lua.Pop(2);

        var fullPath = Path.Combine(paths.ModelsDir, Path.ChangeExtension(key, ".svg"));

        if (!_modelCacheKeys.TryGetValue(fullPath, out var modelIndex))
        {
            var modelParsed = SvgParser.ParseSvg(fullPath);
            modelIndex = _modelCache.Count;
            _modelCacheKeys[fullPath] = modelIndex;
            _modelCache.Add(modelParsed);
        }

        PushModel(lua, modelIndex);
        return 1;
    }
}
