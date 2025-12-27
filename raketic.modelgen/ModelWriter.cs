using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace raketic.modelgen;

internal static class ModelWriter
{
    public static void DumpModel(StreamWriter w, Model model)
    {
        foreach(var linestrip in model.LineStrips)
        {         
            w.WriteLine(linestrip.IsClosed ? "    glBegin(GL_LINE_LOOP);" : "    glBegin(GL_LINE_STRIP);");
            foreach (var point in linestrip.Points)
            {
                w.WriteLine($"        glVertex2d({(int)point.X}, {(int)point.Y});");
            }
            w.WriteLine("    glEnd();");
            w.WriteLine();
        }
    }
}

