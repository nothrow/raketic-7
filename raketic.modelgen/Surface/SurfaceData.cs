namespace raketic.modelgen.Surface;

/// <summary>
/// A point on the unit sphere, precomputed from (longitude, latitude).
/// Stored as int16_t × 10000 for compact C code generation.
/// </summary>
internal record struct SpherePoint(short Px, short Py, short Pz);

/// <summary>
/// A polyline on the unit sphere surface.
/// </summary>
internal record SurfacePolyline(SpherePoint[] Points);

/// <summary>
/// Complete surface data for a celestial body.
/// </summary>
internal record SurfaceDataRecord(
    SurfacePolyline[] Polylines,
    byte ColorR,
    byte ColorG,
    byte ColorB,
    byte ColorA
)
{
    public string Name { get; init; } = "";
    
    /// <summary>
    /// Convert (longitude, latitude) in degrees to a unit sphere point.
    /// px = cos(lat) * cos(lon)  (depth/visibility)
    /// py = cos(lat) * sin(lon)  (horizontal on screen)
    /// pz = sin(lat)             (vertical on screen)
    /// </summary>
    public static SpherePoint LonLatToSphere(double lonDeg, double latDeg)
    {
        double lonRad = lonDeg * Math.PI / 180.0;
        double latRad = latDeg * Math.PI / 180.0;
        
        double cosLat = Math.Cos(latRad);
        double px = cosLat * Math.Cos(lonRad);
        double py = cosLat * Math.Sin(lonRad);
        double pz = Math.Sin(latRad);
        
        return new SpherePoint(
            (short)Math.Round(px * 10000),
            (short)Math.Round(py * 10000),
            (short)Math.Round(pz * 10000)
        );
    }
    
    /// <summary>
    /// Generate a circular polyline (crater) on the unit sphere at given center and angular radius.
    /// </summary>
    public static SpherePoint[] GenerateCrater(double centerLonDeg, double centerLatDeg, double radiusDeg, int segments = 12)
    {
        var points = new SpherePoint[segments + 1]; // +1 to close the loop
        double radRad = radiusDeg * Math.PI / 180.0;
        
        for (int i = 0; i <= segments; i++)
        {
            double angle = i * 2.0 * Math.PI / segments;
            
            // Point on small circle around (0,0,1) then rotated to (centerLon, centerLat)
            double sinR = Math.Sin(radRad);
            double cosR = Math.Cos(radRad);
            
            // Start with point on sphere at angular distance 'radiusDeg' from north pole
            double x = sinR * Math.Cos(angle);
            double y = sinR * Math.Sin(angle);
            double z = cosR;
            
            // Rotate from north pole to (centerLon, centerLat):
            // First rotate by latitude around Y axis (to tilt from pole to equator)
            double latRad = (90.0 - centerLatDeg) * Math.PI / 180.0; // co-latitude
            double cosLat = Math.Cos(latRad);
            double sinLat = Math.Sin(latRad);
            
            double x2 = x * cosLat + z * sinLat;
            double y2 = y;
            double z2 = -x * sinLat + z * cosLat;
            
            // Then rotate by longitude around Z axis  
            double lonRad = centerLonDeg * Math.PI / 180.0;
            double cosLon = Math.Cos(lonRad);
            double sinLon = Math.Sin(lonRad);
            
            double px = x2 * cosLon - y2 * sinLon;
            double py = x2 * sinLon + y2 * cosLon;
            double pz = z2;
            
            points[i] = new SpherePoint(
                (short)Math.Round(px * 10000),
                (short)Math.Round(py * 10000),
                (short)Math.Round(pz * 10000)
            );
        }
        
        return points;
    }
}
