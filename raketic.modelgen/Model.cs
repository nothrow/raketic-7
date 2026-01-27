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
}
