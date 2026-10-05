#pragma once

#include "Pathfinding/PathfindingTypes.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <queue>
#include <vector>

namespace CommonUtilities
{
	// --- Find the cheapest route without guessing how far is left ---
	// Use the same nodes and connection costs as A*. The result includes the
	// start and destination, or is empty when no route exists.
	// Connection costs must be finite and zero or greater.
	//
	// Dijkstra always checks the place with the lowest cost FROM THE START.
	// Unlike A*, it does not add an estimate of the cost still left to travel.
	inline std::vector<int> Dijkstra(
		const std::vector<PathfindingNode>& someNodes,
		int aStartIndex,
		int anEndIndex
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

		// --- Keep a note for each place waiting to be checked ---
		struct QueueNode
		{
			// Only the actual cost so far. There is no remaining-cost estimate.
			float myDistance = 0.0f;
			// If costs are equal, check the older note first.
			std::size_t myOrder = 0;
			int myIndex = 0;
		};

		// priority_queue normally puts the biggest value first. Using > here
		// makes it put the smallest travel cost first instead.
		struct QueueCompare
		{
			bool operator()(const QueueNode& aLeft, const QueueNode& aRight) const
			{
				if (aLeft.myDistance == aRight.myDistance)
				{
					return aLeft.myOrder > aRight.myOrder;
				}

				return aLeft.myDistance > aRight.myDistance;
			}
		};

		// --- Remember our cheapest route to each place ---
		// Infinity means we have not reached that place yet.
		// previous remembers where we came FROM so we can rebuild the route.
		static const int Unvisited = -1;
		std::vector<float> distances(someNodes.size(), std::numeric_limits<float>::infinity());
		std::vector<int> previous(someNodes.size(), Unvisited);

		std::priority_queue<QueueNode, std::vector<QueueNode>, QueueCompare> openNodes;
		std::size_t nextQueueOrder = 0;

		// Start at the starting place without spending any travel cost.
		distances[aStartIndex] = 0.0f;
		openNodes.push({ 0.0f, nextQueueOrder++, aStartIndex });

		// --- Explore in order of cost from the start ---
		while (!openNodes.empty())
		{
			const QueueNode current = openNodes.top();
			openNodes.pop();

			// If we found a cheaper route after adding this note, skip the old note.
			if (current.myDistance != distances[current.myIndex])
			{
				continue;
			}

			// The destination is the cheapest waiting place, so its route is settled.
			if (current.myIndex == anEndIndex)
			{
				break;
			}

			// Try each place we can travel to directly from this one.
			for (const PathfindingConnection& connection : someNodes[current.myIndex].myConnections)
			{
				const int neighborIndex = connection.myNodeIndex;
				// Ignore connections pointing outside the node list.
				if (neighborIndex < 0 || static_cast<std::size_t>(neighborIndex) >= someNodes.size())
				{
					continue;
				}

				// Cost to reach here + this next step's cost = cost to reach there.
				const float newDistance = current.myDistance + connection.myCost;
				if (distances[neighborIndex] <= newDistance)
				{
					continue;
				}

				// Save the cheaper route and add a new note to the waiting list.
				distances[neighborIndex] = newDistance;
				previous[neighborIndex] = current.myIndex;
				openNodes.push({ newDistance, nextQueueOrder++, neighborIndex });
			}
		}

		// --- Rebuild the route from the saved steps ---
		if (previous[anEndIndex] == Unvisited)
		{
			return {};
		}

		// Follow the steps backwards from the destination to the start.
		std::vector<int> path;
		for (int nodeIndex = anEndIndex; nodeIndex != Unvisited; nodeIndex = previous[nodeIndex])
		{
			path.push_back(nodeIndex);
		}

		// Flip the list so the caller gets start -> ... -> destination.
		std::reverse(path.begin(), path.end());
		return path;
	}
}
