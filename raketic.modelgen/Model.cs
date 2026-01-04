using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace raketic.modelgen;


internal record Point(
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

internal enum SlotType
{
Engine
}


internal record Slot(
    Point Position,
    SlotType Type
);

internal record Model(
    string FileName,
    LineStrip[] LineStrips,
    Slot[] Slots
);
