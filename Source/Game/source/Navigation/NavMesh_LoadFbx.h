// Included once by Navigation/NavMesh.cpp, after its geometry helpers.
bool NavMesh::LoadFbx(const std::string& aFile, const Vector2f& aMin, const Vector2f& aMax)
{
    // Build in a temporary mesh. A failed reload leaves the current mesh usable.
    NavMesh loaded;
    TGA::FBX::NavMesh imported;
    try
    {
        TGA::FBX::Importer::InitImporter();
        if (!TGA::FBX::Importer::LoadNavMeshA(aFile, imported, true))
        {
            myLoadError = "Could not load " + aFile + ": " + TGA::FBX::Importer::GetLastError();
            return false;
        }
    }
    catch (const std::exception& error)
    {
        myLoadError = "Could not load " + aFile + ": " + error.what();
        return false;
    }

    Vector2f minimum{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    Vector2f maximum{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    for (const TGA::FBX::NavMesh::NavMeshChunk& chunk : imported.Chunks)
    {
        for (const TGA::FBX::NavMesh::NavMeshPolygon& polygon : chunk.Polygons)
        {
            if (polygon.Indices.size() != 3)
            {
                myLoadError = "Navmesh must contain only triangles. Triangulate before exporting.";
                return false;
            }
            NavTriangle triangle;
            for (int corner = 0; corner < 3; ++corner)
            {
                const unsigned int index = polygon.Indices[corner];
                if (index >= chunk.Vertices.size())
                {
                    myLoadError = "Navmesh contains an invalid vertex index.";
                    return false;
                }
                const TGA::FBX::Vertex& vertex = chunk.Vertices[index];
                // Course loader: imported -Z becomes screen X, imported X becomes screen Y.
                const Vector2f point{-vertex.Position[2], vertex.Position[0]};
                if (!std::isfinite(point.x) || !std::isfinite(point.y))
                {
                    myLoadError = "Navmesh contains a non-finite position.";
                    return false;
                }
                triangle.vertices[corner] = point;
                minimum.x = (std::min)(minimum.x, point.x);
                minimum.y = (std::min)(minimum.y, point.y);
                maximum.x = (std::max)(maximum.x, point.x);
                maximum.y = (std::max)(maximum.y, point.y);
            }
            loaded.myTriangles.push_back(triangle);
        }
    }

    const Vector2f extent = maximum - minimum;
    const Vector2f available = aMax - aMin;
    if (loaded.myTriangles.empty() || extent.x <= 0.f || extent.y <= 0.f ||
        available.x <= 0.f || available.y <= 0.f)
    {
        myLoadError = "Empty navmesh or invalid X/Z ground-plane bounds.";
        return false;
    }
    const float scale = (std::min)(available.x / extent.x, available.y / extent.y);
    const Vector2f offset = aMin + (available - extent * scale) * 0.5f;
    for (NavTriangle& triangle : loaded.myTriangles)
    {
        for (Vector2f& vertex : triangle.vertices)
        {
            vertex = offset + (vertex - minimum) * scale;
        }
        const float area = Area(triangle.vertices[0], triangle.vertices[1], triangle.vertices[2]);
        if (std::abs(area) <= pointTolerance)
        {
            myLoadError = "Navmesh contains a degenerate ground-plane triangle.";
            return false;
        }
        if (area < 0.f)
        {
            std::swap(triangle.vertices[1], triangle.vertices[2]); // All triangles counter-clockwise.
        }
        triangle.centre = (triangle.vertices[0] + triangle.vertices[1] + triangle.vertices[2]) / 3.f;
    }
    loaded.SetConnections();
    myTriangles = std::move(loaded.myTriangles);
    myGraph = std::move(loaded.myGraph);
    myLoadError.clear();
    return true;
}

