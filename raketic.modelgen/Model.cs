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
    /// Computes a 16-sample radial distance profile for collision detection.
    /// For each of 16 angles (0, 22.5, ..., 337.5 degrees), casts a ray from origin outward
    /// and finds the farthest intersection with the hull polygon edges.
    /// Returns null for models with no closed polygon (e.g. particles).
    /// </summary>
    public byte[]? GetRadialProfile()
    {
        // Prefer explicit hull class, otherwise first closed polygon
        var sourceStrip = LineStrips.FirstOrDefault(ls => ls.Class == "hull" && ls.IsClosed)
                       ?? LineStrips.FirstOrDefault(ls => ls.IsClosed);

        if (sourceStrip == null)
            return null;

        var points = sourceStrip.Points;
        var profile = new byte[16];

        for (int sector = 0; sector < 16; sector++)
        {
            double angle = sector * (2.0 * Math.PI / 16.0);
            double dirX = Math.Cos(angle);
            double dirY = Math.Sin(angle);

            double maxT = 0;

            // Test ray from origin in direction (dirX, dirY) against each polygon edge
            for (int i = 0; i < points.Length; i++)
            {
                int j = (i + 1) % points.Length;

                double ex = points[j].X - points[i].X;
                double ey = points[j].Y - points[i].Y;

                double denom = dirX * ey - dirY * ex;
                if (Math.Abs(denom) < 1e-10)
                    continue; // parallel

                double invDenom = 1.0 / denom;

                // t = distance along ray, u = distance along edge [0..1]
                double t = (points[i].X * ey - points[i].Y * ex) * invDenom;
                double u = (points[i].X * dirY - points[i].Y * dirX) * invDenom;

                if (t > 0 && u >= 0 && u <= 1)
                {
                    if (t > maxT)
                        maxT = t;
                }
            }

            // Ceiling for conservative collision (don't underestimate), minimum 1
            profile[sector] = (byte)Math.Min(255, Math.Max(1, (int)Math.Ceiling(maxT)));
        }

        return profile;
    }
}
