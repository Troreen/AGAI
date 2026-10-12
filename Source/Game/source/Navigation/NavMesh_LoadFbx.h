// Included once by Navigation/NavMesh.cpp, after its geometry helpers.
bool NavMesh::LoadFbx(const std::string& aFilePath, const Vector2f& aScreenMin, const Vector2f& aScreenMax)
{
    // Build in a temporary mesh. A failed reload leaves the current mesh usable.
    NavMesh loadedMesh;
    TGA::FBX::NavMesh importedMesh;
    try
    {
        // Read the file and ask the importer to split its shapes into triangles.
        TGA::FBX::Importer::InitImporter();
        if (!TGA::FBX::Importer::LoadNavMeshA(aFilePath, importedMesh, true))
        {
            myLoadError = "Could not load " + aFilePath + ": " + TGA::FBX::Importer::GetLastError();
            return false;
        }
    }
    catch (const std::exception& error)
    {
        myLoadError = "Could not load " + aFilePath + ": " + error.what();
        return false;
    }

    // Collect the ground positions and track the outer bounds of the imported mesh.
    Vector2f meshMin{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    Vector2f meshMax{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    for (const TGA::FBX::NavMesh::NavMeshChunk& meshChunk : importedMesh.Chunks)
    {
        for (const TGA::FBX::NavMesh::NavMeshPolygon& meshPolygon : meshChunk.Polygons)
        {
            // Every walkable piece must have exactly three valid corners.
            if (meshPolygon.Indices.size() != 3)
            {
                myLoadError = "Navmesh must contain only triangles. Triangulate before exporting.";
                return false;
            }
            NavTriangle triangle;
            for (int cornerIndex = 0; cornerIndex < 3; ++cornerIndex)
            {
                const unsigned int vertexIndex = meshPolygon.Indices[cornerIndex];
                if (vertexIndex >= meshChunk.Vertices.size())
                {
                    myLoadError = "Navmesh contains an invalid vertex index.";
                    return false;
                }
                const TGA::FBX::Vertex& importedVertex = meshChunk.Vertices[vertexIndex];
                // Imported -Z becomes screen X, and imported X becomes screen Y.
                const Vector2f groundPosition{-importedVertex.Position[2], importedVertex.Position[0]};
                if (!std::isfinite(groundPosition.x) || !std::isfinite(groundPosition.y))
                {
                    myLoadError = "Navmesh contains a non-finite position.";
                    return false;
                }
                triangle.vertices[cornerIndex] = groundPosition;
                meshMin.x = (std::min)(meshMin.x, groundPosition.x);
                meshMin.y = (std::min)(meshMin.y, groundPosition.y);
                meshMax.x = (std::max)(meshMax.x, groundPosition.x);
                meshMax.y = (std::max)(meshMax.y, groundPosition.y);
            }
            loadedMesh.myTriangles.push_back(triangle);
        }
    }

    // Make sure the mesh and its intended screen area both have usable sizes.
    const Vector2f meshSize = meshMax - meshMin;
    const Vector2f availableScreenSize = aScreenMax - aScreenMin;
    if (loadedMesh.myTriangles.empty() || meshSize.x <= 0.f || meshSize.y <= 0.f ||
        availableScreenSize.x <= 0.f || availableScreenSize.y <= 0.f)
    {
        myLoadError = "Empty navmesh or invalid X/Z ground-plane bounds.";
        return false;
    }
    // Fit the mesh into the screen area without stretching it, then centre it.
    const float screenScale = (std::min)(availableScreenSize.x / meshSize.x, availableScreenSize.y / meshSize.y);
    const Vector2f screenOffset = aScreenMin + (availableScreenSize - meshSize * screenScale) * 0.5f;
    for (NavTriangle& triangle : loadedMesh.myTriangles)
    {
        for (Vector2f& vertexPosition : triangle.vertices)
        {
            vertexPosition = screenOffset + (vertexPosition - meshMin) * screenScale;
        }
        // Reject flattened triangles, which have no walkable space inside them.
        const float signedTriangleArea = Area(triangle.vertices[0], triangle.vertices[1], triangle.vertices[2]);
        if (std::abs(signedTriangleArea) <= pointTolerance)
        {
            myLoadError = "Navmesh contains a degenerate ground-plane triangle.";
            return false;
        }
        // Put all corners in counterclockwise order so inside/outside checks agree.
        if (signedTriangleArea < 0.f)
        {
            std::swap(triangle.vertices[1], triangle.vertices[2]); // All triangles counter-clockwise.
        }
        // Average the three corners to get a centre for path planning.
        triangle.centre = (triangle.vertices[0] + triangle.vertices[1] + triangle.vertices[2]) / 3.f;
    }
    // Connect the triangles, then replace the current mesh only after everything succeeds.
    loadedMesh.SetConnections();
    myTriangles = std::move(loadedMesh.myTriangles);
    myGraph = std::move(loadedMesh.myGraph);
    myLoadError.clear();
    return true;
}

