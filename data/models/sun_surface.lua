-- Solar surface features: sunspots and granulation patterns
-- Craters used as "sunspot" circles, polylines as turbulence arcs
return {
  color = {200, 120, 0},
  craters = {
    -- Sunspots (dark regions on solar disc)
    {-35, 15, 8},       -- large sunspot group
    {50, -20, 6},        -- medium sunspot
    {-10, -40, 5},       -- small sunspot
    {30, 35, 4},         -- small sunspot
    {-55, -10, 3},       -- tiny spot
    {70, 10, 3},         -- tiny spot near limb
  },
  polylines = {
    -- Granulation arcs (turbulent convection patterns)
    {
      {-40, -20}, {-20, -35}, {10, -25},
    },
    {
      {30, 10}, {50, 30}, {25, 45},
    },
    {
      {-15, 35}, {-35, 50}, {-55, 35},
    },
    {
      {10, -60}, {35, -65}, {55, -50},
    },
  },
}
