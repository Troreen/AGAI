// Pull a string through the portals to remove unnecessary path corners.
// A* builds the directed portals separately so both stages can be inspected.
enum class VectorRelation
{
    Left,
    Right,
    Aligned
};

namespace
{
VectorRelation GetVectorRelation(const Vector2f& aForward, const Vector2f& aVector)
{
    // Check whether the second direction points left, right, or along the first.
    const float turnDirection = Cross(aForward, aVector);
    if (turnDirection > 0.f)
    {
        return VectorRelation::Left;
    }
    if (turnDirection < 0.f)
    {
        return VectorRelation::Right;
    }
    return VectorRelation::Aligned;
}

} // namespace

std::vector<Vector2f> NavMesh::PerformFunnelling(const Vector2f& aStart, const Vector2f& aGoal,
                                               const std::vector<NavPortal>& somePortals) const
{
    // With no openings to pass through, the path goes straight to the goal.
    if (somePortals.empty())
    {
        return {aStart, aGoal};
    }
    std::vector<NavPortal> routePortals = somePortals;
    // Treat the goal as a final opening whose two ends are the same point.
    routePortals.push_back({aGoal, aGoal});
    std::vector<Vector2f> waypoints{aStart};
    // Look ahead from the last chosen corner, keeping a left and right limit.
    Vector2f lastCorner = aStart;
    std::size_t leftPortalIndex = 0;
    std::size_t rightPortalIndex = 0;

    const std::size_t portalCount = routePortals.size();
    // The two limits mark the directions we can walk before needing another turn.
    for (std::size_t currentPortalIndex = 1; currentPortalIndex < portalCount; currentPortalIndex++)
    {
        // Try to bring the left limit inward as the next opening comes into view.
        if (routePortals[currentPortalIndex].left != routePortals[leftPortalIndex].left)
        {
            const Vector2f candidateLeftDirection = routePortals[currentPortalIndex].left - lastCorner;
            const Vector2f currentLeftDirection = routePortals[leftPortalIndex].left - lastCorner;
            const bool makesFunnelWider = GetVectorRelation(currentLeftDirection, candidateLeftDirection) == VectorRelation::Left;
            // A wider opening adds no restriction; only a tighter one can change the path.
            if (!makesFunnelWider)
            {
                const Vector2f currentRightDirection = routePortals[rightPortalIndex].right - lastCorner;
                const bool crossesOtherSide = GetVectorRelation(currentRightDirection, candidateLeftDirection) == VectorRelation::Right;
                if (crossesOtherSide)
                {
                    // The limits crossed, so the path must turn at the right corner.
                    waypoints.push_back(routePortals[rightPortalIndex].right);
                    lastCorner = routePortals[rightPortalIndex].right;

                    // Start looking ahead again at the next opening with a different right endpoint.
                    bool restartPortalFound = false;
                    for (std::size_t nextPortalIndex = rightPortalIndex + 1; nextPortalIndex < portalCount; nextPortalIndex++)
                    {
                        if (routePortals[nextPortalIndex].right != lastCorner)
                        {
                            leftPortalIndex = nextPortalIndex;
                            rightPortalIndex = nextPortalIndex;
                            currentPortalIndex = nextPortalIndex;
                            restartPortalFound = true;
                            break;
                        }
                    }

                    if (restartPortalFound)
                    {
                        // Restart with both sides of the new portal.
                        continue;
                    }

                    // No more portals: we can go straight to the end.
                    break;
                }
                else
                {
                    // Keep the tighter left limit for the next opening.
                    leftPortalIndex = currentPortalIndex;
                }
            }
        }

        // Do the same check for the right limit.
        if (routePortals[currentPortalIndex].right != routePortals[rightPortalIndex].right)
        {
            const Vector2f candidateRightDirection = routePortals[currentPortalIndex].right - lastCorner;
            const Vector2f currentRightDirection = routePortals[rightPortalIndex].right - lastCorner;
            const bool makesFunnelWider = GetVectorRelation(currentRightDirection, candidateRightDirection) == VectorRelation::Right;
            if (!makesFunnelWider)
            {
                const Vector2f currentLeftDirection = routePortals[leftPortalIndex].left - lastCorner;
                const bool crossesOtherSide = GetVectorRelation(currentLeftDirection, candidateRightDirection) == VectorRelation::Left;
                if (crossesOtherSide)
                {
                    // The limits crossed, so the path must turn at the left corner.
                    waypoints.push_back(routePortals[leftPortalIndex].left);
                    lastCorner = routePortals[leftPortalIndex].left;

                    // Start looking ahead again at the next opening with a different left endpoint.
                    bool restartPortalFound = false;
                    for (std::size_t nextPortalIndex = leftPortalIndex + 1; nextPortalIndex < portalCount; nextPortalIndex++)
                    {
                        if (routePortals[nextPortalIndex].left != lastCorner)
                        {
                            leftPortalIndex = nextPortalIndex;
                            rightPortalIndex = nextPortalIndex;
                            currentPortalIndex = nextPortalIndex;
                            restartPortalFound = true;
                            break;
                        }
                    }

                    if (restartPortalFound)
                    {
                        // Restart with both sides of the new portal.
                        continue;
                    }

                    // No more portals: we can go straight to the end.
                    break;
                }
                else
                {
                    // Keep the tighter right limit for the next opening.
                    rightPortalIndex = currentPortalIndex;
                }
            }
        }
    }

    // Finish at the goal without adding the same point twice.
    if (!SamePoint(waypoints.back(), aGoal))
    {
        waypoints.push_back(aGoal);
    }
    return waypoints;
}
