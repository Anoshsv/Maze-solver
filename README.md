Maze-solver

Implements a maze-solving algorithm using flood fill logic and simulates it with the MMS (Micromouse Simulator) API.

The robot starts at the top-left corner of a 16×16 maze and explores toward the center using a flood-fill matrix that stores the estimated distance to the goal. At each step, it:

    Senses walls in the front, left, and right using API functions

    Updates an internal wall map with bidirectional wall recording to ensure accurate pathfinding

    Recalculates the flood matrix using BFS propagation from goal cells to maintain optimal distance values

    Chooses the direction with the lowest flood value to move toward, allowing backtracking when all accessible neighbors have higher values

    Stops when it reaches the center of the maze (any of the four center cells)

    Displays flood values and detected walls in the simulator interface using the MMS API

This approach ensures the robot finds a valid path to the goal and updates its internal map dynamically as it discovers new walls. The enhanced algorithm handles dead ends by automatically backtracking and exploring alternative routes.
