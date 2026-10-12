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
        // The sign tells us whether the second direction turns left or right from the first.
        return aFirst.x * aSecond.y - aFirst.y * aSecond.x;
    }

    float Area(const Vector2f& aOrigin, const Vector2f& aFirst, const Vector2f& aSecond)
    {
        // Use the first two points as a line; the sign tells us which side the third point is on.
        return Cross(aFirst - aOrigin, aSecond - aOrigin);
    }

    bool AreEdgesCollinear(const Vector2f& anEdgeStart, const Vector2f& anEdgeEnd,
        const Vector2f& anOtherEdgeStart, const Vector2f& anOtherEdgeEnd)
    {
        // Allow a small rounding error when checking whether the edges lie on the same line.
        const float tolerance = pointTolerance * (anEdgeEnd - anEdgeStart).Length();
        // Both ends of the other edge must be close to the first edge's line.
        return !(std::abs(Area(anEdgeStart, anEdgeEnd, anOtherEdgeStart)) > tolerance ||
            std::abs(Area(anEdgeStart, anEdgeEnd, anOtherEdgeEnd)) > tolerance);
    }

    bool SamePoint(const Vector2f& aFirst, const Vector2f& aSecond)
    {
        // Treat points that are very close together as the same position.
        return aFirst.DistanceSqr(aSecond) <= pointTolerance * pointTolerance;
    }

    // Only a complete shared edge is a portal; partial overlaps are unsupported.
    bool TryGetEdgeOverLap(const Vector2f& anEdgeStart, const Vector2f& anEdgeEnd,
        const Vector2f& anOtherEdgeStart, const Vector2f& anOtherEdgeEnd,
        Vector2f& aPortalFirst, Vector2f& aPortalSecond)
    {
        // The edges must share both ends, even if their points are listed in reverse order.
        const bool matchingEndpoints =
            (SamePoint(anEdgeStart, anOtherEdgeStart) && SamePoint(anEdgeEnd, anOtherEdgeEnd)) ||
            (SamePoint(anEdgeStart, anOtherEdgeEnd) && SamePoint(anEdgeEnd, anOtherEdgeStart));
        // Reject partial matches and edges that are too short to walk through.
        if (!matchingEndpoints || SamePoint(anEdgeStart, anEdgeEnd))
        {
            return false;
        }
        // Use the shared edge as the opening between the triangles.
        aPortalFirst = anEdgeStart;
        aPortalSecond = anEdgeEnd;
        return true;
    }

    NavPortal OrientPortal(const Vector2f& aPortalFirst, const Vector2f& aPortalSecond,
        const Vector2f& aFromCentre, const Vector2f& aToCentre)
    {
        // Look from the current triangle towards the next one.
        const Vector2f travelDirection = aToCentre - aFromCentre;
        const Vector2f portalMidpoint = (aPortalFirst + aPortalSecond) * 0.5f;
        // Check which end of the opening is on our left, then return left before right.
        if (Cross(travelDirection, aPortalFirst - portalMidpoint) > 0.f)
        {
            return {aPortalFirst, aPortalSecond};
        }
        return {aPortalSecond, aPortalFirst};
    }

    Vector2f ClosestOnEdge(const Vector2f& aPoint, const Vector2f& anEdgeStart, const Vector2f& anEdgeEnd)
    {
        // Find the point on the edge that is closest to the given point.
        const Vector2f edgeVector = anEdgeEnd - anEdgeStart;
        // Find how far along the edge the point falls, keeping it between the two ends.
        const float fractionAlongEdge = std::clamp((aPoint - anEdgeStart).Dot(edgeVector) / edgeVector.LengthSqr(), 0.f, 1.f);
        return anEdgeStart + edgeVector * fractionAlongEdge;
    }

    bool Contains(const NavTriangle& aTriangle, const Vector2f& aPoint)
    {
        // A point is inside only if it is on the inside of all three edges.
        // Loading puts the corners in an order where the inside is to the left.
        for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
        {
            const Vector2f edgeStart = aTriangle.vertices[edgeIndex];
            const Vector2f edgeEnd = aTriangle.vertices[(edgeIndex + 1) % 3];
            if (Area(edgeStart, edgeEnd, aPoint) < -pointTolerance * (edgeEnd - edgeStart).Length())
            {
                return false;
            }
        }
        return true;
    }
}

#include "NavMesh_LoadFbx.h"

bool NavMesh::GetPortalBetweenNodes(int aFromTriangleIndex, int aToTriangleIndex, NavPortal& aPortal) const
{
    const NavTriangle& fromTriangle = myTriangles[aFromTriangleIndex];
    const NavTriangle& toTriangle = myTriangles[aToTriangleIndex];

    // Compare each edge of the current triangle with each edge of the next triangle.
    for (int fromEdgeIndex = 0; fromEdgeIndex < 3; ++fromEdgeIndex)
    {
        const Vector2f edgeStart = fromTriangle.vertices[fromEdgeIndex];
        const Vector2f edgeEnd = fromTriangle.vertices[(fromEdgeIndex + 1) % 3];

        for (int toEdgeIndex = 0; toEdgeIndex < 3; ++toEdgeIndex)
        {
            const Vector2f otherEdgeStart = toTriangle.vertices[toEdgeIndex];
            const Vector2f otherEdgeEnd = toTriangle.vertices[(toEdgeIndex + 1) % 3];

            // The edges must lie on the same line, with the triangles on opposite sides.
            // The product is negative only when the centres are on opposite sides.
            if (!AreEdgesCollinear(edgeStart, edgeEnd, otherEdgeStart, otherEdgeEnd) ||
                Area(edgeStart, edgeEnd, fromTriangle.centre) * Area(edgeStart, edgeEnd, toTriangle.centre) >= 0.f)
            {
                continue;
            }
            Vector2f portalFirst;
            Vector2f portalSecond;
            // Check that the triangles share the whole edge.
            if (!TryGetEdgeOverLap(edgeStart, edgeEnd, otherEdgeStart, otherEdgeEnd, portalFirst, portalSecond))
            {
                continue;
            }
            // Put the opening's ends in left/right order for this direction of travel.
            aPortal = OrientPortal(portalFirst, portalSecond, fromTriangle.centre, toTriangle.centre);
            return true;
        }
    }
    // No shared edge means there is no opening between these triangles.
    return false;
}

void NavMesh::SetConnections()
{
    // Give every triangle a graph node, then check each pair of triangles once.
    myGraph.resize(myTriangles.size());
    for (int fromTriangleIndex = 0; fromTriangleIndex < static_cast<int>(myTriangles.size()); ++fromTriangleIndex)
    {
        for (int toTriangleIndex = fromTriangleIndex + 1; toTriangleIndex < static_cast<int>(myTriangles.size()); ++toTriangleIndex)
        {
            NavPortal portal;
            if (GetPortalBetweenNodes(fromTriangleIndex, toTriangleIndex, portal))
            {
                // Allow travel in both directions, using the distance between centres as the cost.
                const float connectionCost = myTriangles[fromTriangleIndex].centre.Distance(myTriangles[toTriangleIndex].centre);
                myGraph[fromTriangleIndex].myConnections.push_back({toTriangleIndex, connectionCost});
                myGraph[toTriangleIndex].myConnections.push_back({fromTriangleIndex, connectionCost});
            }
        }
    }
}

int NavMesh::FindTriangle(const Vector2f& aPoint) const
{
    // Return the first triangle containing the point, or -1 if it is outside the mesh.
    for (int triangleIndex = 0; triangleIndex < static_cast<int>(myTriangles.size()); ++triangleIndex)
    {
        if (Contains(myTriangles[triangleIndex], aPoint))
        {
            return triangleIndex;
        }
    }
    return -1;
}

bool NavMesh::ClosestPoint(const Vector2f& aPoint, Vector2f& aClosestPoint, int& aTriangleIndex) const
{
    // Keep the requested position if it is already on the walkable surface.
    aTriangleIndex = FindTriangle(aPoint);
    if (aTriangleIndex >= 0)
    {
        aClosestPoint = aPoint; // Interior targets stay exactly where clicked.
        return true;
    }
    // Otherwise, check every triangle edge and keep the nearest position.
    float closestDistanceSquared = std::numeric_limits<float>::max();
    for (int triangleIndex = 0; triangleIndex < static_cast<int>(myTriangles.size()); ++triangleIndex)
    {
        const NavTriangle& triangle = myTriangles[triangleIndex];
        for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
        {
            const Vector2f closestPointOnEdge = ClosestOnEdge(aPoint, triangle.vertices[edgeIndex], triangle.vertices[(edgeIndex + 1) % 3]);
            const float distanceSquared = closestPointOnEdge.DistanceSqr(aPoint);
            if (distanceSquared < closestDistanceSquared)
            {
                closestDistanceSquared = distanceSquared;
                aClosestPoint = closestPointOnEdge;
                aTriangleIndex = triangleIndex;
            }
        }
    }
    return aTriangleIndex >= 0;
}

NavigationPath NavMesh::FindPath(const Vector2f& aStart, const Vector2f& aTarget) const
{
    NavigationPath navigationPath;
    navigationPath.requestedTarget = aTarget;
    // The start must be on the mesh; move an outside target to its nearest walkable position.
    navigationPath.startTriangle = FindTriangle(aStart);
    if (!ClosestPoint(aTarget, navigationPath.resolvedTarget, navigationPath.goalTriangle) || navigationPath.startTriangle < 0)
    {
        navigationPath.error = "No navmesh, or path start is outside the navmesh.";
        return navigationPath;
    }

    // Find a connected route through the triangles, using distances between their centres.
    navigationPath.nodes = CommonUtilities::AStar(myGraph, navigationPath.startTriangle, navigationPath.goalTriangle,
        [this](int aFromTriangleIndex, int aToTriangleIndex) { return myTriangles[aFromTriangleIndex].centre.Distance(myTriangles[aToTriangleIndex].centre); });
    if (navigationPath.nodes.empty())
    {
        navigationPath.error = "No connected route to the closest target triangle.";
        return navigationPath;
    }
    // Keep the route through triangle centres so it can be drawn for debugging.
    navigationPath.rawPoints.push_back(aStart);
    for (int triangleIndex : navigationPath.nodes)
    {
        navigationPath.rawPoints.push_back(myTriangles[triangleIndex].centre);
    }
    navigationPath.rawPoints.push_back(navigationPath.resolvedTarget);

    // Turn each shared edge along the route into an opening with a left and right end.
    for (std::size_t pathNodeIndex = 1; pathNodeIndex < navigationPath.nodes.size(); ++pathNodeIndex)
    {
        NavPortal portal;
        if (!GetPortalBetweenNodes(navigationPath.nodes[pathNodeIndex - 1], navigationPath.nodes[pathNodeIndex], portal))
        {
            navigationPath.error = "A* route contains nodes without a shared edge.";
            return navigationPath;
        }
        navigationPath.portals.push_back(portal);
    }
    // Remove unnecessary corners while staying within the openings along the route.
    navigationPath.smoothPoints = PerformFunnelling(aStart, navigationPath.resolvedTarget, navigationPath.portals);
    // Check every part of the finished path to make sure it stays on the walkable surface.
    for (std::size_t waypointIndex = 1; waypointIndex < navigationPath.smoothPoints.size(); ++waypointIndex)
    {
        if (!CanTraverseSegment(navigationPath.smoothPoints[waypointIndex - 1], navigationPath.smoothPoints[waypointIndex]))
        {
            navigationPath.error = "Funnel produced a segment outside the navmesh.";
            navigationPath.smoothPoints.clear();
            break;
        }
    }
    return navigationPath;
}

#include "NavMesh_PerformFunnelling.h"

float NavMesh::TraversableFraction(const Vector2f& aStart, const Vector2f& anEnd) const
{
    // Find the parts of the movement line that lie inside each triangle.
    // Fractions run from 0 at the start to 1 at the end of the movement.
    std::vector<std::pair<float, float>> coveredRanges;
    for (const NavTriangle& triangle : myTriangles)
    {
        float entryFraction = 0.f;
        float exitFraction = 1.f;
        bool segmentTouchesTriangle = true;
        for (int edgeIndex = 0; edgeIndex < 3; ++edgeIndex)
        {
            const Vector2f edgeStart = triangle.vertices[edgeIndex];
            const Vector2f edgeEnd = triangle.vertices[(edgeIndex + 1) % 3];
            const float edgeLength = (edgeEnd - edgeStart).Length();
            // Positive distances are inside this edge; negative distances are outside.
            float startDistanceFromEdge = Area(edgeStart, edgeEnd, aStart) / edgeLength;
            float endDistanceFromEdge = Area(edgeStart, edgeEnd, anEnd) / edgeLength;
            // Treat tiny rounding errors as being exactly on the edge.
            if (std::abs(startDistanceFromEdge) <= pointTolerance)
            {
                startDistanceFromEdge = 0.f;
            }
            if (std::abs(endDistanceFromEdge) <= pointTolerance)
            {
                endDistanceFromEdge = 0.f;
            }
            // If both ends are outside the same edge, the line misses this triangle.
            if (startDistanceFromEdge < 0.f && endDistanceFromEdge < 0.f)
            {
                segmentTouchesTriangle = false;
                break;
            }
            // When the line crosses an edge, shorten the part that is inside the triangle.
            if (startDistanceFromEdge < 0.f)
            {
                entryFraction = (std::max)(entryFraction, startDistanceFromEdge / (startDistanceFromEdge - endDistanceFromEdge));
            }
            if (endDistanceFromEdge < 0.f)
            {
                exitFraction = (std::min)(exitFraction, startDistanceFromEdge / (startDistanceFromEdge - endDistanceFromEdge));
            }
        }
        // Save the part of the movement covered by this triangle.
        if (segmentTouchesTriangle && entryFraction <= exitFraction)
        {
            coveredRanges.push_back({entryFraction, exitFraction});
        }
    }
    // Join covered parts in order, stopping at the first gap in the walkable surface.
    std::sort(coveredRanges.begin(), coveredRanges.end());
    float coveredFraction = 0.f;
    for (const std::pair<float, float>& coveredRange : coveredRanges)
    {
        // Each pair stores where a covered part starts (first) and ends (second).
        if (coveredRange.first > coveredFraction + 0.000001f)
        {
            break;
        }
        coveredFraction = (std::max)(coveredFraction, coveredRange.second);
    }
    return coveredFraction;
}

bool NavMesh::CanTraverseSegment(const Vector2f& aStart, const Vector2f& anEnd) const
{
    // The start and the entire movement line must be on the walkable surface.
    return FindTriangle(aStart) >= 0 && TraversableFraction(aStart, anEnd) >= 1.f;
}

Vector2f NavMesh::ConstrainMovement(const Vector2f& aStart, const Vector2f& anEnd) const
{
    // Allow the full movement when it stays on the mesh.
    const float traversableFraction = TraversableFraction(aStart, anEnd);
    if (traversableFraction >= 1.f)
    {
        return anEnd;
    }
    // Stay at the start if the requested movement is too small to matter.
    const float movementLength = aStart.Distance(anEnd);
    if (movementLength <= pointTolerance)
    {
        return aStart;
    }
    // Stop a tiny distance before the boundary to avoid floating-point drift outside.
    const float safeMovementFraction = (std::max)(0.f, traversableFraction - pointTolerance / movementLength);
    return aStart + (anEnd - aStart) * safeMovementFraction;
}
