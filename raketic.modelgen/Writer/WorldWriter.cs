using raketic.modelgen.Svg;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace raketic.modelgen.Writer;

internal class WorldWriter(PathInfo paths) : IDisposable
{
    private StreamWriter? _hWriter;
    private StreamWriter? _cWriter;

    public void WriteHeaders()
    {
        _hWriter = new StreamWriter(paths.OutputHRenderer, false);
        _cWriter = new StreamWriter(paths.OutputC, false);

        _hWriter.WriteLine(@"
// generated, do not edit manually

#pragma once
#include ""platform/platform.h""
void _generated_draw_model(color_t color, uint16_t index);
uint16_t _generated_get_model_radius(uint16_t index);
");

        _cWriter.WriteLine(@"
// generated, do not edit manually

#include ""renderer.gen.h""
#include ""entity/entity.h""

#include <Windows.h>
#include <gl/GL.h>
");

    }

    public void WriteModels(IEnumerable<Model> models)
    {
        if (_cWriter == null || _hWriter == null)
            throw new InvalidOperationException("Writers not initialized. Call WriteHeaders() first.");

        var svgWriter = new SvgModelWriter(_cWriter!, _hWriter!);

        svgWriter.Write(models.ToArray());
    }

    public void Dispose()
    {
        _hWriter?.Dispose();
        _cWriter?.Dispose();
    }
}
