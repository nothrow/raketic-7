-- Simplified lunar surface features (maria and craters)
-- Craters are defined as center (lon, lat) + angular radius
-- The modelgen expands them into circular polylines on the unit sphere
return {
  color = {100, 100, 90},
  craters = {
    -- Major maria (large dark basins)
    {-20, 32, 18},      -- Mare Imbrium
    {18, 22, 12},        -- Mare Serenitatis
    {55, 15, 10},        -- Mare Crisium
    {-5, -5, 14},        -- Mare Nubium region
    {25, 5, 10},         -- Mare Tranquillitatis

    -- Prominent craters (smaller)
    {-10, -43, 5},       -- Tycho
    {-22, 10, 4},        -- Copernicus
    {-35, -10, 3},       -- Kepler
  },
}
