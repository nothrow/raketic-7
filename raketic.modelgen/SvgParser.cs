using Svg;
using Svg.Pathing;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace raketic.modelgen;

internal class SvgParser
{
    private static IEnumerable<LineStrip> ParseLinestrip(SvgElement elem) => elem switch
    {
        SvgPolyline poly => [ParsePoly(poly)],
        SvgLine line => [ParseLine(line)],
        SvgPath path => ParsePath(path),
        _ => throw new InvalidOperationException($"Unsupported SVG element: {elem.GetType().Name}"),
    };

    private static IEnumerable<LineStrip> ParsePath(SvgPath path)
    {
        var currentSegment = new List<Point>();

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
                            Color: path.Stroke.ToString()
                        );
                        currentSegment.Clear();
                    }
                    currentSegment.Add(new Point(moveTo.End.X, moveTo.End.Y));
                    break;
                case SvgLineSegment lineTo:
                    currentSegment.Add(new Point(lineTo.End.X, lineTo.End.Y));
                    break;
                case SvgClosePathSegment closePath:
                    if (currentSegment.Count > 0)
                    {
                        yield return new LineStrip(
                            Points: [.. currentSegment],
                            IsClosed: true,
                            Color: path.Stroke.ToString()
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
                            Color: path.Stroke.ToString()
                        );
        }
    }

    private static LineStrip ParseLine(SvgLine line) => new LineStrip(
            Points:
            [
                new Point(line.StartX, line.StartY),
                new Point(line.EndX, line.EndY)
            ],
            IsClosed: false,
            Color: line.Stroke.ToString()
        );

    private static LineStrip ParsePoly(SvgPolyline poly) => new LineStrip(
            Points: ParsePoints(poly.Points),
            IsClosed: false,
            Color: poly.Stroke.ToString()
        );

    private static Point[] ParsePoints(SvgPointCollection points)
    {
        var ret = new Point[points.Count / 2];
        for (int i = 0; i < points.Count / 2; i++)
        {
            ret[i] = new Point(points[2 * i], points[2 * i + 1]);
        }
        return ret;
    }

    public static Model ParseSvg(string fullPath)
    {
        var filename = Path.GetFileNameWithoutExtension(fullPath);
        SvgDocument doc = SvgDocument.Open(fullPath);

        return new Model(
            FileName: filename,
            doc.Descendants().SelectMany(ParseLinestrip).ToArray()
        );
    }
}
