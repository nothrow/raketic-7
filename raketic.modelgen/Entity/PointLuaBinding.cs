using KeraLua;

namespace raketic.modelgen.Entity;

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
