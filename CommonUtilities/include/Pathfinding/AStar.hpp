#pragma once

#include "Pathfinding/PathfindingTypes.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <queue>
#include <vector>

namespace CommonUtilities
{
	// Find a route through nodes using their existing connections.
	// Return the node indices from start to goal, or an empty list if no route exists.

	// The optional estimate function gives a rough cost from a node to the goal.
	// It guides the search towards promising nodes. If omitted, the estimate is zero.
	// To guarantee the cheapest route, the estimate must never exceed the actual
	// remaining cost and must be zero at the goal.
	// Connection costs must be finite and zero or greater, using the same units as the estimate.
	inline std::vector<int> AStar(
		const std::vector<PathfindingNode>& someNodes,
		int aStartNodeIndex,
		int aGoalNodeIndex,
		const std::function<float(int, int)>& aEstimateRemainingCost = {}
	)
	{
		// Both the start and goal must refer to nodes in the supplied list.
		if (aStartNodeIndex < 0 || static_cast<std::size_t>(aStartNodeIndex) >= someNodes.size())
		{
			return {};
		}

		if (aGoalNodeIndex < 0 || static_cast<std::size_t>(aGoalNodeIndex) >= someNodes.size())
		{
			return {};
		}

		// If we are already at the goal, the route contains just that node.
		if (aStartNodeIndex == aGoalNodeIndex)
		{
			return { aStartNodeIndex };
		}

		// Ask the caller for a distance/cost estimate from this node to the goal.
		// [&] lets this small function use the goal and estimate function from above.
		const auto estimateCostToGoal = [&](int aNodeIndex)
		{
			return aEstimateRemainingCost ? aEstimateRemainingCost(aNodeIndex, aGoalNodeIndex) : 0.0f;
		};

		// Keep a search note for each route waiting to be explored.
		// A node can have several notes if we find better routes to it later.0
		struct SearchEntry
		{
			// Cost to reach this node plus the estimated cost to reach the goal from here.
			float myEstimatedTotalCost = 0.0f;
			float myCostFromStart = 0.0f;
			// Remember when the note was added, so equal estimates are checked in order.
			std::size_t myInsertionOrder = 0;
			int myNodeIndex = 0;
		};

		struct SearchPriorityCompare
		{
			bool operator()(const SearchEntry& aFirstEntry, const SearchEntry& aSecondEntry) const
			{
				// With equal estimated costs, explore the older note first.
				if (aFirstEntry.myEstimatedTotalCost == aSecondEntry.myEstimatedTotalCost)
				{
					return aFirstEntry.myInsertionOrder > aSecondEntry.myInsertionOrder;
				}

				// The queue normally puts the biggest value first; > makes the lowest cost come first.
				return aFirstEntry.myEstimatedTotalCost > aSecondEntry.myEstimatedTotalCost;
			}
		};

		// Keep the cheapest known cost to each node and the node we came from to reach it.
		// Infinity means we have not reached the node yet; -1 means no previous node is saved.
		static const int noPreviousNodeIndex = -1;
		std::vector<float> bestCostFromStart(someNodes.size(), std::numeric_limits<float>::infinity());
		std::vector<int> previousNodeIndices(someNodes.size(), noPreviousNodeIndex);

		// The queue keeps the most promising route at the front automatically.
		std::priority_queue<SearchEntry, std::vector<SearchEntry>, SearchPriorityCompare> nodesToExplore;
		std::size_t nextInsertionOrder = 0;

		// Start searching at the start node, with no travel cost spent yet.
		bestCostFromStart[aStartNodeIndex] = 0;
		nodesToExplore.push({ estimateCostToGoal(aStartNodeIndex), 0.0f, nextInsertionOrder++, aStartNodeIndex });

		// Explore connections until we reach the goal or have no routes left to check.
		while (!nodesToExplore.empty())
		{
			// Take the most promising note off the waiting list.
			const SearchEntry currentEntry = nodesToExplore.top();
			nodesToExplore.pop();

			// Skip an old note if we have already found a cheaper route to this node.
			if (currentEntry.myCostFromStart != bestCostFromStart[currentEntry.myNodeIndex])
			{
				continue;
			}

			// We have reached the goal through the best route waiting to be checked.
			if (currentEntry.myNodeIndex == aGoalNodeIndex)
			{
				break;
			}

			// Try every connection leading directly from the current node to another node.
			for (const PathfindingConnection& connection : someNodes[currentEntry.myNodeIndex].myConnections)
			{
				const int neighbourNodeIndex = connection.myNodeIndex;
				// Ignore connections that point outside the node list.
				if (neighbourNodeIndex < 0 || static_cast<std::size_t>(neighbourNodeIndex) >= someNodes.size())
				{
					continue;
				}

				// Add the next connection's cost to the cost of reaching the current node.
				const float candidateCostFromStart = currentEntry.myCostFromStart + connection.myCost;
				// Keep the route we already know if this new route is no cheaper.
				if (bestCostFromStart[neighbourNodeIndex] <= candidateCostFromStart)
				{
					continue;
				}

				// Save this better route and remember where its last step came from.
				bestCostFromStart[neighbourNodeIndex] = candidateCostFromStart;
				previousNodeIndices[neighbourNodeIndex] = currentEntry.myNodeIndex;

				// Add it to the waiting list, using cost so far + estimated cost left as its priority.
				nodesToExplore.push({
					candidateCostFromStart + estimateCostToGoal(neighbourNodeIndex),
					candidateCostFromStart,
					nextInsertionOrder++,
					neighbourNodeIndex });
			}
		}

		// Without a saved step into the goal, the search never found a route to it.
		if (previousNodeIndices[aGoalNodeIndex] == noPreviousNodeIndex)
		{
			return {};
		}

		// Follow the saved steps backwards from the goal to the start.
		std::vector<int> pathNodeIndices;
		for (int routeNodeIndex = aGoalNodeIndex; routeNodeIndex != noPreviousNodeIndex; routeNodeIndex = previousNodeIndices[routeNodeIndex])
		{
			pathNodeIndices.push_back(routeNodeIndex);
		}

		// Turn that backwards list around so the caller receives start -> ... -> goal.
		std::reverse(pathNodeIndices.begin(), pathNodeIndices.end());
		return pathNodeIndices;
	}
}
