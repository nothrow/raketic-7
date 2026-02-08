using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace raketic.modelgen;


internal record struct Point(
    float X,
    float Y
);

internal record LineStrip(
    Point[] Points,
    bool IsClosed,
    float StrokeWidth,
    string Class,
    System.Drawing.Color Color
);

internal record Slot(
    Point Position,
    string Name
);

internal record Model(
    string FileName,
    LineStrip[] LineStrips,
    Slot[] Slots
)
{
    public string ModelConstantName => $"MODEL_{FileName.ToUpper()}_IDX";

    /// <summary>
    /// computes radius as the max distance from origin to any point in the model
    /// </summary>
    /// <returns></returns>
    public int GetRadius()
    {
        double maxDistSq = 0;
        foreach (var linestrip in LineStrips)
        {
            foreach (var point in linestrip.Points)
            {
                double distSq = point.X * point.X + point.Y * point.Y;
                if (distSq > maxDistSq)
                    maxDistSq = distSq;
            }
        }

        // Ceiling to be conservative, clamp to uint8 range
        return (int)Math.Ceiling(Math.Sqrt(maxDistSq));
    }

    /// <summary>
    /// Computes the convex hull of the model's hull/outline polygon for collision detection.
    /// Returns null for models with no closed polygon (e.g. particles).
    /// </summary>
    public Point[]? GetCollisionHull()
    {
        // Prefer explicit hull class, otherwise first closed polygon
        var sourceStrip = LineStrips.FirstOrDefault(ls => ls.Class == "hull" && ls.IsClosed)
                       ?? LineStrips.FirstOrDefault(ls => ls.IsClosed);

        if (sourceStrip == null)
            return null;

        return ComputeConvexHull(sourceStrip.Points);
    }

    /// <summary>
    /// Andrew's monotone chain convex hull algorithm. Returns points in CCW order.
    /// </summary>
    private static Point[] ComputeConvexHull(Point[] points)
    {
        if (points.Length <= 2)
            return points;

        var sorted = points.OrderBy(p => p.X).ThenBy(p => p.Y).ToArray();
        var hull = new Point[sorted.Length * 2];
        int k = 0;

        // Lower hull
        for (int i = 0; i < sorted.Length; i++)
        {
            while (k >= 2 && Cross(hull[k - 2], hull[k - 1], sorted[i]) <= 0)
                k--;
            hull[k++] = sorted[i];
        }

        // Upper hull
        int lower = k + 1;
        for (int i = sorted.Length - 2; i >= 0; i--)
        {
            while (k >= lower && Cross(hull[k - 2], hull[k - 1], sorted[i]) <= 0)
                k--;
            hull[k++] = sorted[i];
        }

        return hull.Take(k - 1).ToArray(); // -1 to remove duplicate last point
    }

    private static float Cross(Point o, Point a, Point b) =>
        (a.X - o.X) * (b.Y - o.Y) - (a.Y - o.Y) * (b.X - o.X);
}
