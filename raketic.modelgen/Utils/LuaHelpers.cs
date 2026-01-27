using KeraLua;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace raketic.modelgen.Utils;

internal static class LuaHelpers
{
    public static void RegisterGlobalCache(this Lua lua, string cacheName, LuaFunction resolver)
    {
        lua.NewTable();
        lua.NewTable();

        lua.PushString("__index");
        lua.PushCFunction(resolver);

        lua.SetTable(-3);
        lua.SetMetaTable(-2);

        lua.SetGlobal(cacheName);
    }
}


