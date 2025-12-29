using Svg;
using Svg.Pathing;
using System.Drawing.Drawing2D;

namespace raketic.modelgen;

internal class SvgParser
{
    private Matrix? _transform = null;

    private IEnumerable<LineStrip> ParseLinestrip(SvgElement elem) => elem switch
    {
        SvgPolyline poly => [ParsePoly(poly)],
        SvgLine line => [ParseLine(line)],
        SvgPath path => ParsePath(path),
        SvgGroup group => ParseGroup(group),
        _ => throw new InvalidOperationException($"Unsupported SVG element: {elem.GetType().Name}"),
    };

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
            foreach(var element in group.Children.SelectMany(ParseLinestrip))
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
        var filename = Path.GetFileNameWithoutExtension(fullPath);
        SvgDocument doc = SvgDocument.Open(fullPath);

        var parser = new SvgParser();

        return new Model(
            FileName: filename,
            doc.Children.SelectMany(parser.ParseLinestrip).ToArray()
        );
    }
}
