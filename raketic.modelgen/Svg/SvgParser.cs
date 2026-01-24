using KeraLua;
using Svg;
using Svg.Pathing;
using System.Drawing.Drawing2D;

namespace raketic.modelgen.Svg;

internal class ModelContext(PathInfo paths)
{
    private readonly Dictionary<string, Model> _modelCache = new();


    public void RegisterForLua(Lua lua)
    {
        lua.NewTable();

        lua.NewTable();

        lua.PushString("__index");
        lua.PushCFunction(ModelsGetRef);

        lua.SetTable(-3);
        lua.SetMetaTable(-2);

        lua.SetGlobal("models");
    }

    private int ModelsGetRef(nint luaState)
    {
        var lua = Lua.FromIntPtr(luaState);
        var key = lua.ToString(2);
        lua.Pop(2);

        var fullPath = Path.Combine(paths.ModelsDir, Path.ChangeExtension(key, ".svg"));

        if (!_modelCache.TryGetValue(fullPath, out var cached))
        {
            _modelCache[fullPath] = cached = SvgParser.ParseSvg(fullPath);
        }

        lua.NewTable();
        lua.PushString("radius");
        lua.PushNumber(cached.GetRadius());
        lua.SetTable(-3);

        lua.PushString("__key");
        lua.PushString(fullPath);
        lua.SetTable(-3);

        return 1;
    }
}

internal class SvgParser
{
    private const int CircleSegments = 64; // number of segments for approximation of circle
    private const int CurveSegments = 16;   // number of segments for approximation of Bezier curve

    private Matrix? _transform = null;
    private Point? _explicitCenter = null;
    private List<(string, Point)> _slots = new();

    private IEnumerable<LineStrip> ParseLinestrip(SvgElement elem) => elem switch
    {
        { ID: "center" } => HandleCenterMarker(elem),
        _ when elem.ContainsAttribute("slot") => HandleSlot(elem),

        SvgPolyline poly => [ParsePoly(poly)],
        SvgLine line => [ParseLine(line)],
        SvgPath path => ParsePath(path),
        SvgGroup group => ParseGroup(group),
        SvgRectangle rectangle => ParseRectangle(rectangle),
        SvgCircle circle => ParseCircle(circle),
        SvgEllipse ellipse => ParseEllipse(ellipse),

        _ => throw new InvalidOperationException($"Unsupported SVG element: {elem.GetType().Name}"),
    };

    private Point GetCenter(SvgElement el)
    {
        if (el is SvgCircle circle)
        {
            return GetPoint(circle.CenterX.Value, circle.CenterY.Value);
        }
        else if (el is SvgEllipse ellipse)
        {
            return GetPoint(ellipse.CenterX.Value, ellipse.CenterY.Value);
        }
        else if (el is SvgRectangle rectangle)
        {
            return GetPoint(rectangle.X + rectangle.Width / 2, rectangle.Y + rectangle.Height / 2);
        }
        else
        {
            throw new InvalidOperationException($"Unsupported center marker element: {el.GetType().Name}");
        }
    }


    private IEnumerable<LineStrip> HandleSlot(SvgElement el)
    {
        var slotName = el.TryGetAttribute("slot", out var slotAttr) ? slotAttr : null;
        if (string.IsNullOrEmpty(slotName))
        {
            throw new InvalidOperationException("Slot element must have a non-empty 'slot' attribute.");
        }

        var center = GetCenter(el);
        _slots.Add((slotName, center));
        return [];
    }

    private IEnumerable<LineStrip> HandleCenterMarker(SvgElement el)
    {
        _explicitCenter = GetCenter(el);
        return [];
    }

    private IEnumerable<LineStrip> ParseGroup(SvgGroup group)
    {
        var oldTransform = _transform;

        if (group.Transforms != null)
        {
            var newTransform = group.Transforms.GetMatrix();
            if (_transform != null)
            {
                newTransform.Multiply(_transform, MatrixOrder.Append);
            }
            _transform = newTransform;
        }
        try
        {
            foreach (var element in group.Children.SelectMany(ParseLinestrip))
            {
                yield return element;
            }
        }
        finally
        {
            if (group.Transforms != null)
            {
                _transform = oldTransform;
            }
        }
    }

    private Point GetPoint(float x, float y)
    {
        if (_transform != null)
        {
            var points = new[] { new System.Drawing.PointF(x, y) };
            _transform.TransformPoints(points);
            return new Point(points[0].X, points[0].Y);
        }
        else
        {
            return new Point(x, y);
        }
    }


    private IEnumerable<LineStrip> ParseRectangle(SvgRectangle rectangle)
    {
        return [
            new LineStrip(
                Points: [
                    GetPoint(rectangle.X, rectangle.Y),
                    GetPoint(rectangle.X + rectangle.Width, rectangle.Y),
                    GetPoint(rectangle.X + rectangle.Width, rectangle.Y + rectangle.Height),
                    GetPoint(rectangle.X, rectangle.Y + rectangle.Height)
                ],
                IsClosed: true,
                Color: GetColor(rectangle.Stroke),
                StrokeWidth: rectangle.StrokeWidth,
                Class: rectangle.TryGetAttribute("class", out var classAttr) ? classAttr : ""
            )
        ];
    }

    private IEnumerable<LineStrip> ParseCircle(SvgCircle circle)
    {
        Console.WriteLine($"  ⚠️ CIRCLE! Rasterizing to {CircleSegments} segments.");

        var cx = circle.CenterX.Value;
        var cy = circle.CenterY.Value;
        var r = circle.Radius.Value;

        var points = new Point[CircleSegments];
        for (int i = 0; i < CircleSegments; i++)
        {
            var angle = 2 * Math.PI * i / CircleSegments;
            var x = cx + r * (float)Math.Cos(angle);
            var y = cy + r * (float)Math.Sin(angle);
            points[i] = GetPoint(x, y);
        }

        return [
            new LineStrip(
                Points: points,
                IsClosed: true,
                Color: GetColor(circle.Stroke),
                StrokeWidth: circle.StrokeWidth,
                Class: circle.TryGetAttribute("class", out var classAttr) ? classAttr : ""
            )
        ];
    }

    private IEnumerable<LineStrip> ParseEllipse(SvgEllipse ellipse)
    {
        Console.WriteLine($"  ⚠️ ELLIPSE! Rasterizing to {CircleSegments} segments.");

        var cx = ellipse.CenterX.Value;
        var cy = ellipse.CenterY.Value;
        var rx = ellipse.RadiusX.Value;
        var ry = ellipse.RadiusY.Value;

        var points = new Point[CircleSegments];
        for (int i = 0; i < CircleSegments; i++)
        {
            var angle = 2 * Math.PI * i / CircleSegments;
            var x = cx + rx * (float)Math.Cos(angle);
            var y = cy + ry * (float)Math.Sin(angle);
            points[i] = GetPoint(x, y);
        }

        return [
            new LineStrip(
                Points: points,
                IsClosed: true,
                Color: GetColor(ellipse.Stroke),
                StrokeWidth: ellipse.StrokeWidth,
                Class: ellipse.TryGetAttribute("class", out var classAttr) ? classAttr : ""
            )
        ];
    }

    private IEnumerable<LineStrip> ParsePath(SvgPath path)
    {
        var currentSegment = new List<Point>();
        var @class = path.TryGetAttribute("class", out var classAttr) ? classAttr : "";

        foreach (var data in path.PathData)
        {
            switch (data)
            {
                case SvgMoveToSegment moveTo:
                    if (currentSegment.Count > 0)
                    {
                        yield return new LineStrip(
                            Points: [.. currentSegment],
                            IsClosed: false,
                            Color: GetColor(path.Stroke),
                            StrokeWidth: path.StrokeWidth,
                            Class: @class
                        );
                        currentSegment.Clear();
                    }
                    currentSegment.Add(GetPoint(moveTo.End.X, moveTo.End.Y));
                    break;
                case SvgLineSegment lineTo:
                    currentSegment.Add(GetPoint(lineTo.End.X, lineTo.End.Y));
                    break;
                case SvgQuadraticCurveSegment quadratic:
                    Console.WriteLine($"  ⚠️ QUADRATIC CURVE! Rasterizing to {CurveSegments} segments.");
                    RasterizeQuadraticBezier(currentSegment, quadratic);
                    break;
                case SvgClosePathSegment closePath:
                    if (currentSegment.Count > 0)
                    {
                        yield return new LineStrip(
                            Points: [.. currentSegment],
                            IsClosed: true,
                            Color: GetColor(path.Stroke),
                            StrokeWidth: path.StrokeWidth,
                            Class: @class
                        );
                        currentSegment.Clear();
                    }
                    break;
                default:
                    throw new InvalidOperationException($"Unsupported SVG path segment: {data.GetType().Name}");
            }
        }

        if (currentSegment.Count > 0)
        {
            yield return new LineStrip(
                            Points: [.. currentSegment],
                            IsClosed: false,
                            Color: GetColor(path.Stroke),
                            StrokeWidth: path.StrokeWidth,
                            Class: @class
                        );
        }
    }

    private void RasterizeQuadraticBezier(List<Point> currentSegment, SvgQuadraticCurveSegment quadratic)
    {
        if (currentSegment.Count == 0)
            throw new InvalidOperationException("Quadratic curve segment requires a starting point.");

        // P0 = start (last point in segment), P1 = control, P2 = end
        var p0 = currentSegment[^1];
        var p1x = quadratic.ControlPoint.X;
        var p1y = quadratic.ControlPoint.Y;
        var p2x = quadratic.End.X;
        var p2y = quadratic.End.Y;

        // B(t) = (1-t)²P0 + 2(1-t)tP1 + t²P2
        for (int i = 1; i <= CurveSegments; i++)
        {
            float t = (float)i / CurveSegments;
            float oneMinusT = 1 - t;

            float x = oneMinusT * oneMinusT * p0.X
                    + 2 * oneMinusT * t * p1x
                    + t * t * p2x;

            float y = oneMinusT * oneMinusT * p0.Y
                    + 2 * oneMinusT * t * p1y
                    + t * t * p2y;

            currentSegment.Add(GetPoint(x, y));
        }
    }

    private System.Drawing.Color GetColor(SvgPaintServer paintServer) => paintServer switch
    {
        SvgColourServer colorServer => colorServer.Colour,
        _ => System.Drawing.Color.Black
    };

    private LineStrip ParseLine(SvgLine line) => new LineStrip(
            Points:
            [
                GetPoint(line.StartX, line.StartY),
                GetPoint(line.EndX, line.EndY)
            ],
            IsClosed: false,
            Color: GetColor(line.Stroke),
            StrokeWidth: line.StrokeWidth,
            Class: line.TryGetAttribute("class", out var classAttr) ? classAttr : ""
        );

    private LineStrip ParsePoly(SvgPolyline poly) => new LineStrip(
            Points: ParsePoints(poly.Points),
            IsClosed: false,
            Color: GetColor(poly.Stroke),
            StrokeWidth: poly.StrokeWidth,
            Class: poly.TryGetAttribute("class", out var classAttr) ? classAttr : ""
        );

    private Point[] ParsePoints(SvgPointCollection points)
    {
        var ret = new Point[points.Count / 2];
        for (int i = 0; i < points.Count / 2; i++)
        {
            ret[i] = GetPoint(points[2 * i], points[2 * i + 1]);
        }
        return ret;
    }

    public static Model ParseSvg(string fullPath)
    {
        Console.WriteLine($"Parsing SVG file: {fullPath}");
        var filename = Path.GetFileNameWithoutExtension(fullPath);
        SvgDocument doc = SvgDocument.Open(fullPath);

        var parser = new SvgParser();
        var lineStrips = doc.Children.SelectMany(parser.ParseLinestrip).ToArray();

        // Determine center: explicit marker or bounding box center
        Point center;
        if (parser._explicitCenter != null)
        {
            center = parser._explicitCenter;
            Console.WriteLine($"  Using explicit center: ({center.X}, {center.Y})");
        }
        else
        {
            center = ComputeBoundingBoxCenter(lineStrips);
            Console.WriteLine($"  Computed bounding box center: ({center.X}, {center.Y})");
        }

        // Translate all points so center is at (0, 0)
        var centeredStrips = lineStrips
            .Select(ls => ls with
            {
                Points = ls.Points.Select(p => new Point(p.X - center.X, p.Y - center.Y)).ToArray()
            })
            .ToArray();



        return new Model(
            FileName: filename,
            centeredStrips,
            Slots: parser._slots.Select(x => new Slot(x.Item2 with { X = x.Item2.X - center.X, Y = x.Item2.Y - center.Y }, ParseSlotType(x.Item1))).ToArray()
        );
    }

    private static SlotType ParseSlotType(string item) => item.ToLower() switch
    {
        "engine" => SlotType.Engine,
        _ => throw new InvalidOperationException($"Unknown slot type: {item}"),
    };

    private static Point ComputeBoundingBoxCenter(LineStrip[] lineStrips)
    {
        var allPoints = lineStrips.SelectMany(ls => ls.Points).ToArray();

        if (allPoints.Length == 0)
            return new Point(0, 0);

        float minX = allPoints.Min(p => p.X);
        float maxX = allPoints.Max(p => p.X);
        float minY = allPoints.Min(p => p.Y);
        float maxY = allPoints.Max(p => p.Y);

        return new Point((minX + maxX) / 2, (minY + maxY) / 2);
    }
}
