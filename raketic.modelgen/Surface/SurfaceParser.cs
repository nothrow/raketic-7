using KeraLua;

namespace raketic.modelgen.Surface;

internal class SurfaceParser
{
    /// <summary>
    /// Parse a surface Lua file into SurfaceDataRecord.
    /// Supports 'polylines' (explicit lon/lat outlines) and 'craters' (auto-generated circles).
    /// </summary>
    public static SurfaceDataRecord Parse(string filePath)
    {
        var lua = new Lua();
        lua.DoFile(filePath);

        // The file returns a table on the stack
        if (!lua.IsTable(-1))
            throw new InvalidOperationException($"Surface file '{filePath}' did not return a table");

        byte colorR = 200, colorG = 200, colorB = 200, colorA = 200;
        var polylines = new List<SurfacePolyline>();

        lua.PushNil();
        while (lua.Next(-2))
        {
            if (lua.IsString(-2))
            {
                string key = lua.ToString(-2)!;
                switch (key)
                {
                    case "color":
                        (colorR, colorG, colorB) = ReadColor(lua);
                        break;
                    case "polylines":
                        polylines.AddRange(ReadPolylines(lua));
                        break;
                    case "craters":
                        polylines.AddRange(ReadCraters(lua));
                        break;
                }
            }
            else if (lua.IsInteger(-2))
            {
                // Numeric keys at top level = polylines (legacy format)
                if (lua.IsTable(-1))
                {
                    polylines.Add(ReadSinglePolyline(lua));
                }
            }
            lua.Pop(1);
        }

        var name = Path.GetFileNameWithoutExtension(filePath);
        lua.Close();

        return new SurfaceDataRecord(polylines.ToArray(), colorR, colorG, colorB, colorA) { Name = name };
    }

    private static (byte r, byte g, byte b) ReadColor(Lua lua)
    {
        if (!lua.IsTable(-1))
            return (200, 200, 200);

        lua.RawGetInteger(-1, 1);
        byte r = (byte)(lua.ToInteger(-1));
        lua.Pop(1);

        lua.RawGetInteger(-1, 2);
        byte g = (byte)(lua.ToInteger(-1));
        lua.Pop(1);

        lua.RawGetInteger(-1, 3);
        byte b = (byte)(lua.ToInteger(-1));
        lua.Pop(1);

        return (r, g, b);
    }

    private static IEnumerable<SurfacePolyline> ReadPolylines(Lua lua)
    {
        if (!lua.IsTable(-1))
            yield break;

        lua.PushNil();
        while (lua.Next(-2))
        {
            if (lua.IsTable(-1))
            {
                yield return ReadSinglePolyline(lua);
            }
            lua.Pop(1);
        }
    }

    private static SurfacePolyline ReadSinglePolyline(Lua lua)
    {
        var points = new List<SpherePoint>();

        lua.PushNil();
        while (lua.Next(-2))
        {
            if (lua.IsTable(-1))
            {
                // Each point is {lon, lat}
                lua.RawGetInteger(-1, 1);
                double lon = lua.ToNumber(-1);
                lua.Pop(1);

                lua.RawGetInteger(-1, 2);
                double lat = lua.ToNumber(-1);
                lua.Pop(1);

                points.Add(SurfaceDataRecord.LonLatToSphere(lon, lat));
            }
            lua.Pop(1);
        }

        return new SurfacePolyline(points.ToArray());
    }

    private static IEnumerable<SurfacePolyline> ReadCraters(Lua lua)
    {
        if (!lua.IsTable(-1))
            yield break;

        lua.PushNil();
        while (lua.Next(-2))
        {
            if (lua.IsTable(-1))
            {
                // Each crater: {lon, lat, radius[, segments]}
                lua.RawGetInteger(-1, 1);
                double lon = lua.ToNumber(-1);
                lua.Pop(1);

                lua.RawGetInteger(-1, 2);
                double lat = lua.ToNumber(-1);
                lua.Pop(1);

                lua.RawGetInteger(-1, 3);
                double radius = lua.ToNumber(-1);
                lua.Pop(1);

                int segments = 12;
                lua.RawGetInteger(-1, 4);
                if (lua.IsNumber(-1))
                    segments = (int)lua.ToInteger(-1);
                lua.Pop(1);

                var craterPoints = SurfaceDataRecord.GenerateCrater(lon, lat, radius, segments);
                yield return new SurfacePolyline(craterPoints);
            }
            lua.Pop(1);
        }
    }
}
