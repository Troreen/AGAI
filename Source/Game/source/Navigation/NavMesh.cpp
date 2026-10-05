#include "NavMesh.h"

#include <Pathfinding/AStar.hpp>
#include <TGAFBXImporter/source/Importer.h>
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>

namespace
{
using CommonUtilities::Vector2f;
constexpr float pointTolerance = 0.001f; // In gameplay pixels.

float Cross(const Vector2f& aFirst, const Vector2f& aSecond)
{
    return aFirst.x * aSecond.y - aFirst.y * aSecond.x;
}

float Area(const Vector2f& aOrigin, const Vector2f& aFirst, const Vector2f& aSecond)
{
    return Cross(aFirst - aOrigin, aSecond - aOrigin);
}

bool SamePoint(const Vector2f& aFirst, const Vector2f& aSecond)
{
    return aFirst.DistanceSqr(aSecond) <= pointTolerance * pointTolerance;
}

Vector2f ClosestOnEdge(const Vector2f& aPoint, const Vector2f& aStart, const Vector2f& anEnd)
{
    const Vector2f edge = anEnd - aStart;
    const float amount = std::clamp((aPoint - aStart).Dot(edge) / edge.LengthSqr(), 0.f, 1.f);
    return aStart + edge * amount;
}

bool Contains(const NavTriangle& aTriangle, const Vector2f& aPoint)
{
    for (int edge = 0; edge < 3; ++edge)
    {
        const Vector2f start = aTriangle.vertices[edge];
        const Vector2f end = aTriangle.vertices[(edge + 1) % 3];
        if (Area(start, end, aPoint) < -pointTolerance * (end - start).Length())
        {
            return false;
        }
    }
    return true;
}
}

#include "NavMesh_LoadFbx.h"

bool NavMesh::GetPortalBetweenNodes(int aFrom, int aTo, NavPortal& aPortal) const
{
    const NavTriangle& from = myTriangles[aFrom];
    const NavTriangle& to = myTriangles[aTo];
    // Compare positions, not chunk-local indices: FBX may split shared vertices/chunks.
    // The course asset also has T-junctions: one edge meets only part of a longer
    // edge. Their common, collinear span is still a valid portal.
    for (int firstEdge = 0; firstEdge < 3; ++firstEdge)
    {
        const Vector2f first = from.vertices[firstEdge];
        const Vector2f second = from.vertices[(firstEdge + 1) % 3];
        for (int secondEdge = 0; secondEdge < 3; ++secondEdge)
        {
            const Vector2f otherFirst = to.vertices[secondEdge];
            const Vector2f otherSecond = to.vertices[(secondEdge + 1) % 3];
            const Vector2f edge = second - first;
            const float length = edge.Length();
            if (std::abs(Area(first, second, otherFirst)) > pointTolerance * length ||
                std::abs(Area(first, second, otherSecond)) > pointTolerance * length ||
                Area(first, second, from.centre) * Area(first, second, to.centre) >= 0.f)
            {
                continue;
            }
            const Vector2f unit = edge / length;
            const float firstAmount = (otherFirst - first).Dot(unit);
            const float secondAmount = (otherSecond - first).Dot(unit);
            const float overlapStart = (std::max)(0.f, (std::min)(firstAmount, secondAmount));
            const float overlapEnd = (std::min)(length, (std::max)(firstAmount, secondAmount));
            if (overlapEnd - overlapStart <= pointTolerance)
            {
                continue;
            }
            const Vector2f portalFirst = first + unit * overlapStart;
            const Vector2f portalSecond = first + unit * overlapEnd;
            const Vector2f direction = to.centre - from.centre;
            const Vector2f midpoint = (portalFirst + portalSecond) * 0.5f;
            if (Cross(direction, portalFirst - midpoint) > 0.f)
            {
                aPortal = {portalFirst, portalSecond};
            }
            else
            {
                aPortal = {portalSecond, portalFirst};
            }
            return true;
        }
    }
    return false;
}

void NavMesh::SetConnections()
{
    myGraph.resize(myTriangles.size());
    for (int first = 0; first < static_cast<int>(myTriangles.size()); ++first)
    {
        for (int second = first + 1; second < static_cast<int>(myTriangles.size()); ++second)
        {
            NavPortal portal;
            if (GetPortalBetweenNodes(first, second, portal))
            {
                const float cost = myTriangles[first].centre.Distance(myTriangles[second].centre);
                myGraph[first].myConnections.push_back({second, cost});
                myGraph[second].myConnections.push_back({first, cost});
            }
        }
    }
}

int NavMesh::FindTriangle(const Vector2f& aPoint) const
{
    for (int index = 0; index < static_cast<int>(myTriangles.size()); ++index)
    {
        if (Contains(myTriangles[index], aPoint))
        {
            return index;
        }
    }
    return -1;
}

bool NavMesh::ClosestPoint(const Vector2f& aPoint, Vector2f& aResult, int& aTriangle) const
{
    aTriangle = FindTriangle(aPoint);
    if (aTriangle >= 0)
    {
        aResult = aPoint; // Interior targets stay exactly where the user clicked.
        return true;
    }
    float bestDistance = std::numeric_limits<float>::max();
    for (int index = 0; index < static_cast<int>(myTriangles.size()); ++index)
    {
        const NavTriangle& triangle = myTriangles[index];
        for (int edge = 0; edge < 3; ++edge)
        {
            const Vector2f candidate = ClosestOnEdge(aPoint, triangle.vertices[edge], triangle.vertices[(edge + 1) % 3]);
            const float distance = candidate.DistanceSqr(aPoint);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                aResult = candidate;
                aTriangle = index;
            }
        }
    }
    return aTriangle >= 0;
}

NavigationPath NavMesh::FindPath(const Vector2f& aStart, const Vector2f& aTarget) const
{
    NavigationPath path;
    path.requestedTarget = aTarget;
    path.startTriangle = FindTriangle(aStart);
    if (!ClosestPoint(aTarget, path.resolvedTarget, path.goalTriangle) || path.startTriangle < 0)
    {
        path.error = "No navmesh, or path start is outside the navmesh.";
        return path;
    }

    // Stage 1: A* uses centre distances for both movement costs and its estimate.
    path.nodes = CommonUtilities::AStar(myGraph, path.startTriangle, path.goalTriangle,
        [this](int aFrom, int aTo) { return myTriangles[aFrom].centre.Distance(myTriangles[aTo].centre); });
    if (path.nodes.empty())
    {
        path.error = "No connected route to the closest target triangle.";
        return path;
    }
    path.rawPoints.push_back(aStart);
    for (int node : path.nodes)
    {
        path.rawPoints.push_back(myTriangles[node].centre);
    }
    path.rawPoints.push_back(path.resolvedTarget);

    // Stage 2: The A* corridor's shared edges become directed portals.
    for (std::size_t index = 1; index < path.nodes.size(); ++index)
    {
        NavPortal portal;
        if (!GetPortalBetweenNodes(path.nodes[index - 1], path.nodes[index], portal))
        {
            path.error = "A* route contains nodes without a shared edge.";
            return path;
        }
        path.portals.push_back(portal);
    }
    // Stage 3: Pull a string through that corridor, keeping the raw route untouched.
    path.smoothPoints = PerformFunnelling(aStart, path.resolvedTarget, path.portals);
    for (std::size_t index = 1; index < path.smoothPoints.size(); ++index)
    {
        if (!CanTraverseSegment(path.smoothPoints[index - 1], path.smoothPoints[index]))
        {
            path.error = "Funnel produced a segment outside the navmesh.";
            path.smoothPoints.clear();
            break;
        }
    }
    return path;
}

#include "NavMesh_PerformFunnelling.h"

float NavMesh::TraversableFraction(const Vector2f& aStart, const Vector2f& anEnd) const
{
    // Clip the segment against each convex triangle, then merge the covered intervals.
    // Testing only the endpoint would let a large timestep skip right over a hole.
    std::vector<std::pair<float, float>> intervals;
    for (const NavTriangle& triangle : myTriangles)
    {
        float enter = 0.f;
        float exit = 1.f;
        bool intersects = true;
        for (int edge = 0; edge < 3; ++edge)
        {
            const Vector2f first = triangle.vertices[edge];
            const Vector2f second = triangle.vertices[(edge + 1) % 3];
            const float length = (second - first).Length();
            float startSide = Area(first, second, aStart) / length;
            float endSide = Area(first, second, anEnd) / length;
            if (std::abs(startSide) <= pointTolerance)
            {
                startSide = 0.f;
            }
            if (std::abs(endSide) <= pointTolerance)
            {
                endSide = 0.f;
            }
            if (startSide < 0.f && endSide < 0.f)
            {
                intersects = false;
                break;
            }
            if (startSide < 0.f)
            {
                enter = (std::max)(enter, startSide / (startSide - endSide));
            }
            if (endSide < 0.f)
            {
                exit = (std::min)(exit, startSide / (startSide - endSide));
            }
        }
        if (intersects && enter <= exit)
        {
            intervals.push_back({enter, exit});
        }
    }
    std::sort(intervals.begin(), intervals.end());
    float covered = 0.f;
    for (const std::pair<float, float>& interval : intervals)
    {
        if (interval.first > covered + 0.000001f)
        {
            break;
        }
        covered = (std::max)(covered, interval.second);
    }
    return covered;
}

bool NavMesh::CanTraverseSegment(const Vector2f& aStart, const Vector2f& anEnd) const
{
    return FindTriangle(aStart) >= 0 && TraversableFraction(aStart, anEnd) >= 1.f;
}

Vector2f NavMesh::ConstrainMovement(const Vector2f& aStart, const Vector2f& anEnd) const
{
    const float fraction = TraversableFraction(aStart, anEnd);
    if (fraction >= 1.f)
    {
        return anEnd;
    }
    const float length = aStart.Distance(anEnd);
    if (length <= pointTolerance)
    {
        return aStart;
    }
    // Stop a tiny distance before the boundary to avoid floating-point drift outside.
    const float safeFraction = (std::max)(0.f, fraction - pointTolerance / length);
    return aStart + (anEnd - aStart) * safeFraction;
}
