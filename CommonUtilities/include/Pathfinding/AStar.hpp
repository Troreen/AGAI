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
	// --- Find a route through connected places ---
	// Each node is a place we can visit; each connection is a way to another place.
	// The result is a list of node indices, including the start and destination.
	// An empty list means we could not find a route.
	//
	// someNodes: all the places and their connections. We only read this list.
	// aStartIndex / anEndIndex: the positions of those nodes in the list.
	// aHeuristic: an optional function taking two node indices and returning a
	// float estimate of the remaining travel cost. The = {} lets callers omit it.
	// For example, a navmesh can estimate using the distance between node centres.
	// To guarantee the cheapest route, the estimate must never be higher than the
	// real remaining cost, and must be zero at the destination. With no estimate,
	// we use zero and choose which place to check using only the cost so far.
	// All connection costs must be finite and zero or greater.
	inline std::vector<int> AStar(
		const std::vector<PathfindingNode>& someNodes,
		int aStartIndex,
		int anEndIndex,
		const std::function<float(int, int)>& aHeuristic = {}
	)
	{
		// --- Check that both requested places actually exist ---
		if (aStartIndex < 0 || static_cast<std::size_t>(aStartIndex) >= someNodes.size())
		{
			return {};
		}

		if (anEndIndex < 0 || static_cast<std::size_t>(anEndIndex) >= someNodes.size())
		{
			return {};
		}

		// Already there? The whole route is just this one place.
		if (aStartIndex == anEndIndex)
		{
			return { aStartIndex };
		}

		// A small local helper: ask the caller for an estimate, or use zero.
		// [&] lets this helper use the variables from the surrounding function.
		const auto estimateRemainingCost = [&](int aNodeIndex)
		{
			return aHeuristic ? aHeuristic(aNodeIndex, anEndIndex) : 0.0f;
		};

		// --- Keep a note for each place waiting to be checked ---
		// This is a search note, separate from the actual node in someNodes.
		struct QueueNode
		{
			// Cost so far + our estimate of how much travel is still left.
			float myEstimatedTotalDistance = 0.0f;
			// The actual cost of the route used to reach this place.
			float myDistance = 0.0f;
			// If two notes have equal estimates, check the older one first.
			std::size_t myOrder = 0;
			int myIndex = 0;
		};

		// Tell the waiting list which note should come first.
		// priority_queue normally puts the biggest value first. Using > here
		// makes it put the smallest estimated total cost first instead.
		struct QueueCompare
		{
			bool operator()(const QueueNode& aLeft, const QueueNode& aRight) const
			{
				if (aLeft.myEstimatedTotalDistance == aRight.myEstimatedTotalDistance)
				{
					return aLeft.myOrder > aRight.myOrder;
				}

				return aLeft.myEstimatedTotalDistance > aRight.myEstimatedTotalDistance;
			}
		};

		// --- Remember our best route to each place ---
		// Infinity means no route has reached that place yet.
		// previous remembers where we came FROM so we can rebuild the route later.
		static const int Unvisited = -1;
		std::vector<float> distances(someNodes.size(), std::numeric_limits<float>::infinity());
		std::vector<int> previous(someNodes.size(), Unvisited);

		// The waiting list automatically keeps the most promising note at the top.
		std::priority_queue<QueueNode, std::vector<QueueNode>, QueueCompare> openNodes;
		std::size_t nextQueueOrder = 0;

		// Start at the starting place. We have not spent any travel cost yet.
		distances[aStartIndex] = 0;
		openNodes.push({ estimateRemainingCost(aStartIndex), 0.0f, nextQueueOrder++, aStartIndex });

		// --- Check promising places until we reach the destination or run out ---
		while (!openNodes.empty())
		{
			const QueueNode current = openNodes.top();
			openNodes.pop();

			// A place can have several notes if we find a cheaper route to it later.
			// Skip an old note when its cost no longer matches our best saved cost.
			if (current.myDistance != distances[current.myIndex])
			{
				continue;
			}

			// The destination is now the most promising place. We can stop searching.
			if (current.myIndex == anEndIndex)
			{
				break;
			}

			// Try each place we can travel to directly from this one.
			for (const PathfindingConnection& connection : someNodes[current.myIndex].myConnections)
			{
				const int neighborIndex = connection.myNodeIndex;
				// Ignore a connection pointing outside the node list.
				if (neighborIndex < 0 || static_cast<std::size_t>(neighborIndex) >= someNodes.size())
				{
					continue;
				}

				// Cost to reach here + cost of this next connection = cost to reach there.
				const float newDistance = current.myDistance + connection.myCost;
				// Only save this route if it is cheaper than the one we already know.
				if (distances[neighborIndex] <= newDistance)
				{
					continue;
				}

				// Remember the improvement and put a new note on the waiting list.
				distances[neighborIndex] = newDistance;
				previous[neighborIndex] = current.myIndex;
				openNodes.push({
					newDistance + estimateRemainingCost(neighborIndex),
					newDistance,
					nextQueueOrder++,
					neighborIndex });
			}
		}

		// --- Turn the saved steps into the finished route ---
		// No previous place for the destination means we never reached it.
		if (previous[anEndIndex] == Unvisited)
		{
			return {};
		}

		// Walk backwards: destination -> the place before it -> ... -> start.
		std::vector<int> path;
		for (int nodeIndex = anEndIndex; nodeIndex != Unvisited; nodeIndex = previous[nodeIndex])
		{
			path.push_back(nodeIndex);
		}

		// Flip that backwards list so callers receive start -> ... -> destination.
		std::reverse(path.begin(), path.end());
		return path;
	}
}
