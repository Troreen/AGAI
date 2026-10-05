#pragma once

#include <vector>

namespace CommonUtilities
{
	// --- One way to travel from a node to another node ---
	struct PathfindingConnection
	{
		// The destination's position in the node list, not a world coordinate.
		int myNodeIndex = -1;
		// How expensive this step is (for example, distance or travel time).
		// Use the same units for every connection and the A* estimate.
		float myCost = 1.0f;
	};

	// --- A place we can visit, such as one area of a navmesh ---
	// A* only needs its connections. Your game can keep positions or shapes separately.
	struct PathfindingNode
	{
		// Connections are one-way. Add a connection at both ends for two-way travel.
		std::vector<PathfindingConnection> myConnections;
	};
}
