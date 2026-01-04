

namespace raketic.modelgen;

internal class ModelWriter
{
    private Dictionary<System.Drawing.Color, int> _colorIndexes = new();
    public void DumpModelData(StreamWriter w, Model model)
    {
        w.WriteLine($"static const int8_t _model_{model.FileName}_vertices[] = {{");
        foreach (var linestrip in model.LineStrips)
        {
            w.Write(" ");
            foreach (var point in linestrip.Points)
            {
                checked
                {
                    w.Write($" {(sbyte)point.X}, {(sbyte)point.Y},");
                }
            }
            w.WriteLine();
        }
        w.WriteLine("};");
        w.WriteLine();
    }

    public void DumpModelColors(StreamWriter w, IEnumerable<Model> models)
    {
        w.WriteLine($"static const uint8_t _model_colors[] = {{");
        var colors = models.SelectMany(x => x.LineStrips).Select(x => x.Color).Distinct();
        int i = 0;
        foreach (var color in colors)
        {
            _colorIndexes[color] = i++;
            w.WriteLine($"  {color.R}, {color.G}, {color.B}, {color.A}, // idx {i - 1}");
        }
        w.WriteLine("};");
        w.WriteLine();
    }

    private static string GetLineModel(LineStrip linestrip) => linestrip.IsClosed ? "GL_LINE_LOOP" : "GL_LINE_STRIP";

    public void DumpModel(StreamWriter w, Model model)
    {
        w.WriteLine($"static void _model_{model.FileName}(color_t color) {{");
        w.WriteLine($"  (color);");
        w.WriteLine($"  glEnableClientState(GL_VERTEX_ARRAY);");
        w.WriteLine($"  glVertexPointer(2, GL_BYTE, 0, _model_{model.FileName}_vertices);");

        int start = 0;
        float previousWidth = -1f;

        int previousColorIndex = -2; // -1 is heat

        foreach (var linestrip in model.LineStrips)
        {
            var len = linestrip.Points.Length;

            if (linestrip.Class == "heat")
            {
                if (previousColorIndex != -1)
                    w.WriteLine($"  glColor4ubv((GLubyte*)(&color));");

                previousColorIndex = -1;
            }
            else
            {
                if (previousColorIndex != _colorIndexes[linestrip.Color])
                    w.WriteLine($"  glColor4ubv((GLubyte*)(_model_colors + {_colorIndexes[linestrip.Color] * 4}));");

                previousColorIndex = _colorIndexes[linestrip.Color];
            }
            if (linestrip.StrokeWidth != previousWidth)
            {
                w.WriteLine($"  glLineWidth({linestrip.StrokeWidth:0.0}f);");
                previousWidth = linestrip.StrokeWidth;
            }

            w.WriteLine($"  glDrawArrays({GetLineModel(linestrip)}, {start}, {linestrip.Points.Length});");
            start += len;
        }
        w.WriteLine($"  glDisableClientState(GL_VERTEX_ARRAY);");
        w.WriteLine($"}}");
        w.WriteLine();
    }

    public void DumpModelSlots(StreamWriter w, Model model)
    {
        if (model.Slots.Length == 0)
            return;

        //struct objects_data*od = entity_manager_get_objects();
        w.WriteLine($"static void _model_{model.FileName}_slots(entity_id_t parent) {{");
        w.WriteLine($"  struct objects_data* od = entity_manager_get_objects();");
        w.WriteLine($"  struct parts_data* pd = entity_manager_get_parts();");
        w.WriteLine($"  uint32_t parent_idx = GET_ORDINAL(parent);");
        w.WriteLine($"  uint32_t i = pd->active;");

        // reserve nearest multiple of 8 slots (for better computations)
        int slotCount = model.Slots.Length;
        int reservedSlots = (slotCount + 7) & ~7;

        w.WriteLine();

        w.WriteLine($"  _ASSERT(pd->active + {reservedSlots} < pd->capacity);");
        w.WriteLine($"  od->parts_start_idx[parent_idx] = i;");
        w.WriteLine($"  od->parts_count[parent_idx] = {slotCount};");
        w.WriteLine($"  pd->active += {reservedSlots};");

        w.WriteLine();
        w.WriteLine($"  memset(&pd->model_idx[i], -1, sizeof(uint16_t) * {reservedSlots});");

        w.WriteLine();
        for (int i = 0; i < model.Slots.Length; i++)
        {
            w.WriteLine($"  pd->parent_id[i + {i}] = parent;");
            w.WriteLine($"  pd->local_offset_x[i + {i}] = {model.Slots[i].Position.X};");
            w.WriteLine($"  pd->local_offset_y[i + {i}] = {model.Slots[i].Position.Y};");

            w.WriteLine($"  pd->type[i + {i}] = {GetTypeRef(model.Slots[i].Type)};");

            w.WriteLine($"  pd->local_orientation_x[i + {i}] = 1.0f;");
            w.WriteLine($"  pd->local_orientation_y[i + {i}] = 0.0f;");

            w.WriteLine();
        }

        w.WriteLine($"}}");
        w.WriteLine();
    }

    private string GetTypeRef(SlotType type) => type switch
    {
        SlotType.Engine => "PART_TYPEREF_ENGINE",
        _ => throw new InvalidOperationException($"Unknown slot type: {type}"),
    };
}
