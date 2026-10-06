#pragma once

#include <Pathfinding/PathfindingTypes.hpp>
#include <Math/Vector2.hpp>
#include <array>
#include <string>
#include <vector>

struct NavTriangle
{
    std::array<CommonUtilities::Vector2f, 3> vertices;
    CommonUtilities::Vector2f centre;
};

// A shared edge has a direction: left/right as seen travelling from one node to the next.
struct NavPortal
{
    CommonUtilities::Vector2f left;
    CommonUtilities::Vector2f right;
};

struct NavigationPath
{
    CommonUtilities::Vector2f requestedTarget;
    CommonUtilities::Vector2f resolvedTarget;
    int startTriangle = -1;
    int goalTriangle = -1;
    std::vector<int> nodes;
    std::vector<CommonUtilities::Vector2f> rawPoints;
    std::vector<NavPortal> portals;
    std::vector<CommonUtilities::Vector2f> smoothPoints;
    std::string error;

    bool Succeeded() const { return error.empty() && !smoothPoints.empty(); }
};

// Small, synchronous gameplay API. Geometry, A*, and smoothing stay separate inside it.
class NavMesh
{
public:
    // TGA imports FBX with Y pointing up. Use (-Z, X) for the 2D ground plane.
    // Fit the file uniformly inside these screen-space bounds (no shape distortion).
    bool LoadFbx(const std::string& aFile, const CommonUtilities::Vector2f& aMin,
                 const CommonUtilities::Vector2f& aMax);
    int FindTriangle(const CommonUtilities::Vector2f& aPoint) const;
    bool ClosestPoint(const CommonUtilities::Vector2f& aPoint,
                      CommonUtilities::Vector2f& aResult, int& aTriangle) const;
    NavigationPath FindPath(const CommonUtilities::Vector2f& aStart,
                            const CommonUtilities::Vector2f& aTarget) const;

    // Check the WHOLE segment, so movement cannot jump across a hole in one frame.
    bool CanTraverseSegment(const CommonUtilities::Vector2f& aStart,
                            const CommonUtilities::Vector2f& anEnd) const;
    CommonUtilities::Vector2f ConstrainMovement(const CommonUtilities::Vector2f& aStart,
                                               const CommonUtilities::Vector2f& anEnd) const;
    const std::vector<NavTriangle>& GetTriangles() const { return myTriangles; }
    const std::vector<CommonUtilities::PathfindingNode>& GetGraph() const { return myGraph; }
    const std::string& GetLoadError() const { return myLoadError; }

private:
    void SetConnections();
    bool GetPortalBetweenNodes(int aFrom, int aTo, NavPortal& aPortal) const;
    std::vector<CommonUtilities::Vector2f> PerformFunnelling(
        const CommonUtilities::Vector2f& aStart, const CommonUtilities::Vector2f& aGoal,
        const std::vector<NavPortal>& somePortals) const;
    float TraversableFraction(const CommonUtilities::Vector2f& aStart,
                              const CommonUtilities::Vector2f& anEnd) const;

    std::vector<NavTriangle> myTriangles;
    std::vector<CommonUtilities::PathfindingNode> myGraph;
    std::string myLoadError;
};
