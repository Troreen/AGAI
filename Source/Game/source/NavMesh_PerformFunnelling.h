// Adapted from the supplied course funnel: replace HD containers with std::vector.
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
    const float cross = Cross(aForward, aVector);
    if (cross > 0.f)
    {
        return VectorRelation::Left;
    }
    if (cross < 0.f)
    {
        return VectorRelation::Right;
    }
    return VectorRelation::Aligned;
}

} // namespace

std::vector<Vector2f> NavMesh::PerformFunnelling(const Vector2f& aStart, const Vector2f& aGoal,
                                               const std::vector<NavPortal>& somePortals) const
{
    if (somePortals.empty())
    {
        return {aStart, aGoal};
    }
    std::vector<NavPortal> portals = somePortals;
    portals.push_back({aGoal, aGoal});
    std::vector<Vector2f> points{aStart};
    Vector2f apex = aStart;
    std::size_t leftIndex = 0;
    std::size_t rightIndex = 0;

    const std::size_t portalCount = portals.size();
    for (std::size_t index = 1; index < portalCount; index++)
    {
        if (portals[index].left != portals[leftIndex].left)
        {
            const Vector2f newSide = portals[index].left - apex;
            const Vector2f oldSide = portals[leftIndex].left - apex;
            const bool makesFunnelWider = GetVectorRelation(oldSide, newSide) == VectorRelation::Left;
            if (!makesFunnelWider)
            {
                const Vector2f rightSide = portals[rightIndex].right - apex;
                const bool crossesOtherSide = GetVectorRelation(rightSide, newSide) == VectorRelation::Right;
                if (crossesOtherSide)
                {
                    points.push_back(portals[rightIndex].right);
                    apex = portals[rightIndex].right;

                    // Find next portal where the right side is not our new apex.
                    bool newPortalFound = false;
                    for (std::size_t nextPortal = rightIndex + 1; nextPortal < portalCount; nextPortal++)
                    {
                        if (portals[nextPortal].right != apex)
                        {
                            leftIndex = nextPortal;
                            rightIndex = nextPortal;
                            index = nextPortal;
                            newPortalFound = true;
                            break;
                        }
                    }

                    if (newPortalFound)
                    {
                        // Restart with both sides of the new portal.
                        continue;
                    }

                    // No more portals: we can go straight to the end.
                    break;
                }
                else
                {
                    leftIndex = index;
                }
            }
        }

        if (portals[index].right != portals[rightIndex].right)
        {
            const Vector2f newSide = portals[index].right - apex;
            const Vector2f oldSide = portals[rightIndex].right - apex;
            const bool makesFunnelWider = GetVectorRelation(oldSide, newSide) == VectorRelation::Right;
            if (!makesFunnelWider)
            {
                const Vector2f leftSide = portals[leftIndex].left - apex;
                const bool crossesOtherSide = GetVectorRelation(leftSide, newSide) == VectorRelation::Left;
                if (crossesOtherSide)
                {
                    points.push_back(portals[leftIndex].left);
                    apex = portals[leftIndex].left;

                    // Find next portal where the left side is not our new apex.
                    bool newPortalFound = false;
                    for (std::size_t nextPortal = leftIndex + 1; nextPortal < portalCount; nextPortal++)
                    {
                        if (portals[nextPortal].left != apex)
                        {
                            leftIndex = nextPortal;
                            rightIndex = nextPortal;
                            index = nextPortal;
                            newPortalFound = true;
                            break;
                        }
                    }

                    if (newPortalFound)
                    {
                        // Restart with both sides of the new portal.
                        continue;
                    }

                    // No more portals: we can go straight to the end.
                    break;
                }
                else
                {
                    rightIndex = index;
                }
            }
        }
    }

    if (!SamePoint(points.back(), aGoal))
    {
        points.push_back(aGoal);
    }
    return points;
}