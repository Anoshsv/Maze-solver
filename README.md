# Maze-solver

Implements a maze-solving algorithm using flood fill logic and simulates it with the MMS (Micromouse Simulator) API.

The robot starts at the top-left corner of a 16×16 maze and explores toward the center using a flood-fill matrix that stores the estimated distance to the goal. At each step, it:
1.Senses walls in the front, left, and right using API functions.
2.Updates an internal wall map and encodes cell wall configurations using integer values.
3.Checks for flood-fill consistency, and if needed, updates the flood matrix to ensure each cell has a value one greater than the minimum of its accessible neighbors.
4.Chooses the direction with the lowest flood value to move toward, avoiding backtracking unless no other option exists.
5.Stops when it reaches the center of the maze (any of the four center cells).
6.Displays flood values and detected walls in the simulator interface using the MMS API.

This approach ensures the robot finds a valid path to the goal, and updates its internal map dynamically as it discovers new walls. The code also includes logic for tracking orientation, marking visited cells, and visualizing flood values.
