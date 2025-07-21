import API
import sys
from collections import deque

x = 0
y = 0
orient = 0
cells = [[0 for _ in range(16)] for _ in range(16)]



flood = [[1000 for _ in range(16)] for _ in range(16)]

highlighted_path = set()
highlighted_return_path = set()
explored_cells = set()

def markCellAsExplored(x, y):
    """Mark a cell as explored during journey"""
    global explored_cells
    explored_cells.add((x, y))
    # Optional: Visual indication of explored cells
    API.setColor(x, y, "C")  # Cyan for explored


def log(string):
    sys.stderr.write("{}\n".format(string))

DIR_VEC = {'n': (0, 1), 'e': (1, 0), 's': (0, -1), 'w': (-1, 0)}
OPP_DIR = {'n': 's', 's': 'n', 'e': 'w', 'w': 'e'}

def _record_wall(x, y, direction):
    updateWalls.walls[(x, y, direction)] = True
    dx, dy = DIR_VEC[direction]
    xn, yn = x + dx, y + dy
    if 0 <= xn < 16 and 0 <= yn < 16:
        updateWalls.walls[(xn, yn, OPP_DIR[direction])] = True


def updateWalls(x, y, orient, L, R, F):
    if not hasattr(updateWalls, 'walls'):
        updateWalls.walls = {}
    
    walls_changed = False
    rel_sensors = [('L', L), ('F', F), ('R', R)]
    
    for rel_name, present in rel_sensors:
        if not present:
            continue
            
        if rel_name == 'F':
            abs_dir = ['n', 'e', 's', 'w'][orient]
        elif rel_name == 'R':
            abs_dir = ['e', 's', 'w', 'n'][orient]
        else: # 'L'
            abs_dir = ['w', 'n', 'e', 's'][orient]
        
        # Check if this is a new wall
        if (x, y, abs_dir) not in updateWalls.walls:
            walls_changed = True
            
        API.setWall(x, y, abs_dir)
        _record_wall(x, y, abs_dir)
    
    return walls_changed

def markDeadEnds():
    for y in range(16):
        for x in range(16):
            if (x, y) in [(7, 7), (7, 8), (8, 7), (8, 8)]:
                continue  # Skip goal cells
                
            accessible_neighbors = 0
            for dx, dy, direction in [(0, 1, 'n'), (1, 0, 'e'), (0, -1, 's'), (-1, 0, 'w')]:
                nx, ny = x + dx, y + dy
                if 0 <= nx < 16 and 0 <= ny < 16:
                    if not hasWall(x, y, direction):
                        accessible_neighbors += 1
            
            if accessible_neighbors <= 1:
                flood[y][x] = 9999  # Mark as dead end
                if not hasattr(markDeadEnds, 'dead_ends'):
                    markDeadEnds.dead_ends = set()
                markDeadEnds.dead_ends.add((x, y))
                # Visualize dead end with red color and 'X' text
                API.setColor(x, y, "R")
                API.setText(x, y, "X")
                log(f"Dead end marked at ({x}, {y})")


def hasWall(x, y, direction):
    if not hasattr(updateWalls, 'walls'):
        return False
    return updateWalls.walls.get((x, y, direction), False)


def isAccessible(x, y, x1, y1):
    if abs(x - x1) + abs(y - y1) != 1:
        return False
    
    # Block movement to known dead ends
    if hasattr(markDeadEnds, 'dead_ends') and (x1, y1) in markDeadEnds.dead_ends:
        return False
    
    if x1 == x and y1 == y + 1:  # Going north
        return not hasWall(x, y, "n")
    elif x1 == x + 1 and y1 == y:  # Going east
        return not hasWall(x, y, "e")
    elif x1 == x and y1 == y - 1:  # Going south
        return not hasWall(x, y, "s")
    elif x1 == x - 1 and y1 == y:  # Going west
        return not hasWall(x, y, "w")
    
    return False



def getSurrounds(x, y):
    neighbors = []
    directions = [(0, 1), (1, 0), (0, -1), (-1, 0)]  # N, E, S, W
    for dx, dy in directions:
        nx, ny = x + dx, y + dy
        if 0 <= nx < 16 and 0 <= ny < 16:
            neighbors.append((nx, ny))
        else:
            neighbors.append((-1, -1))
    return neighbors



def floodFill():
    # Initialize all cells to high value
    for y in range(16):
        for x in range(16):
            flood[y][x] = 1000
    
    # Set goal cells to 0 and start BFS from all goal cells simultaneously
    goal_cells = [(7, 7), (7, 8), (8, 7), (8, 8)]
    queue = deque()
    
    for gx, gy in goal_cells:
        flood[gy][gx] = 0
        queue.append((gx, gy))
    
    # BFS flood fill from all goal cells
    while queue:
        x_curr, y_curr = queue.popleft()
        curr_val = flood[y_curr][x_curr]
        
        for dx, dy, direction in [(0, 1, 'n'), (1, 0, 'e'), (0, -1, 's'), (-1, 0, 'w')]:
            x_n, y_n = x_curr + dx, y_curr + dy
            if 0 <= x_n < 16 and 0 <= y_n < 16:
                if not hasWall(x_curr, y_curr, direction):
                    new_val = curr_val + 1
                    if flood[y_n][x_n] > new_val:
                        flood[y_n][x_n] = new_val
                        queue.append((x_n, y_n))



def highlightPath(x, y):
    global highlighted_path
    # Clear previous highlights
    for (hx, hy) in highlighted_path:
        API.clearColor(hx, hy)
    highlighted_path.clear()

    visited = set()
    new_path = set()
    while flood[y][x] != 0:
        API.setColor(x, y, "B")
        new_path.add((x, y))
        visited.add((x, y))
        minVal = flood[y][x]
        next_cell = None
        for xi, yi in getSurrounds(x, y):
            if (xi >= 0 and yi >= 0 and isAccessible(x, y, xi, yi) and
                    flood[yi][xi] < minVal and (xi, yi) not in visited):
                minVal = flood[yi][xi]
                next_cell = (xi, yi)
        if next_cell:
            x, y = next_cell
        else:
            break
    API.setColor(x, y, "B")
    new_path.add((x, y))
    highlighted_path = new_path


def toMove(x, y, xprev, yprev, orient):
    directions = [(0, 1), (1, 0), (0, -1), (-1, 0)]  # N, E, S, W
    
    candidates = []
    min_flood_val = 10000
    
    # Find minimum flood value first, excluding dead ends
    for i, (dx, dy) in enumerate(directions):
        xi, yi = x + dx, y + dy
        if not (0 <= xi < 16 and 0 <= yi < 16):
            continue
        if not isAccessible(x, y, xi, yi):
            continue
        
        # Skip known dead ends
        if hasattr(markDeadEnds, 'dead_ends') and (xi, yi) in markDeadEnds.dead_ends:
            continue
            
        v = flood[yi][xi]
        if v < min_flood_val:
            min_flood_val = v
    
    # Collect all cells with minimum flood value, excluding dead ends
    for i, (dx, dy) in enumerate(directions):
        xi, yi = x + dx, y + dy
        if not (0 <= xi < 16 and 0 <= yi < 16):
            continue
        if not isAccessible(x, y, xi, yi):
            continue
            
        # Skip known dead ends
        if hasattr(markDeadEnds, 'dead_ends') and (xi, yi) in markDeadEnds.dead_ends:
            continue
        
        v = flood[yi][xi]
        if v == min_flood_val:
            candidates.append((i, xi, yi))
    
    if not candidates:
        log(f"No non-dead-end moves available from ({x}, {y})")
        return None
    
    # Prefer straight movement (no turn)
    for dir_idx, xi, yi in candidates:
        if dir_idx == orient:  # Straight ahead
            return 'F'
    
    # If no straight option, choose direction requiring minimal turns
    best_dir = candidates[0][0]
    diff = (best_dir - orient) % 4
    return ('F', 'R', 'B', 'L')[diff]


######################################################################################
# Trace back to start position (0,0) using shortest discovered path
# Returns True if successfully reached start, False if failed

def traceBackToStartWithTracking(current_x, current_y, current_orient):
    """
    Return journey with cell tracking for optimal path calculation
    """
    global explored_cells
    x, y, orient = current_x, current_y, current_orient
    
    floodFillForReturn()
    log(f"Starting return journey with tracking from ({x}, {y}) to (0, 0)")
    API.setColor(0, 0, "G")
    
    move_count = 0
    max_moves = 500
    visited_positions = []
    
    while (x, y) != (0, 0) and move_count < max_moves:
        move_count += 1
        
        # Track position history for oscillation detection
        visited_positions.append((x, y))
        if len(visited_positions) > 10:
            visited_positions.pop(0)
            
        # Oscillation detection
        if len(visited_positions) >= 4:
            if (visited_positions[-1] == visited_positions[-3] and 
                visited_positions[-2] == visited_positions[-4]):
                log(f"OSCILLATION DETECTED - recalculating")
                floodFillForReturn()
        
        # Wall detection
        L = API.wallLeft()
        R = API.wallRight()  
        F = API.wallFront()
        
        walls_changed = updateWalls(x, y, orient, L, R, F)
        if walls_changed:
            log(f"New walls discovered during return at ({x}, {y})")
            floodFillForReturn()
        
        direction = toMoveForReturn(x, y, orient)
        if direction is None:
            log(f"No valid return move found at ({x}, {y})")
            return False
        
        # Execute movement
        if direction == 'L':
            API.turnLeft()
            orient = API.orientation(orient, 'L')
        elif direction == 'R':
            API.turnRight()
            orient = API.orientation(orient, 'R')
        elif direction == 'B':
            API.turnLeft()
            orient = API.orientation(orient, 'L')
            API.turnLeft()
            orient = API.orientation(orient, 'L')
        
        # Check and move
        if API.wallFront():
            log(f"BLOCKED during return at ({x}, {y})")
            abs_dir = ['n', 'e', 's', 'w'][orient]
            API.setWall(x, y, abs_dir)
            _record_wall(x, y, abs_dir)
            floodFillForReturn()
            continue
        
        try:
            xnext, ynext = API.updateCoordinates(x, y, orient)
            log(f"Return move {move_count}: ({x}, {y}) -> ({xnext}, {ynext})")
            
            API.moveForward()
            x, y = xnext, ynext
            
            # **TRACK RETURN JOURNEY CELLS**
            markCellAsExplored(x, y)
            API.setColor(x, y, "Y")
            
            if (x, y) == (0, 0):
                log(f"STARTING POINT REACHED!")
                return True
                
        except API.MouseCrashedError:
            log(f"Mouse crashed during return at ({x}, {y})")
            return False
    
    return (x, y) == (0, 0)

def floodFillForReturn():
    """
    Modified flood fill with (0,0) as goal for return journey
    """
    # Initialize all cells to high value
    for y in range(16):
        for x in range(16):
            flood[y][x] = 1000
    
    # Set start position (0,0) as goal
    flood[0][0] = 0
    queue = deque([(0, 0)])
    
    # BFS flood fill from start position
    while queue:
        x_curr, y_curr = queue.popleft()
        curr_val = flood[y_curr][x_curr]
        
        for dx, dy, direction in [(0, 1, 'n'), (1, 0, 'e'), (0, -1, 's'), (-1, 0, 'w')]:
            x_n, y_n = x_curr + dx, y_curr + dy
            if 0 <= x_n < 16 and 0 <= y_n < 16:
                if not hasWall(x_curr, y_curr, direction):
                    new_val = curr_val + 1
                    if flood[y_n][x_n] > new_val:
                        flood[y_n][x_n] = new_val
                        queue.append((x_n, y_n))


def isAccessibleForReturn(x, y, x1, y1):
    """Special accessibility check for return journey - ignores dead end blocking"""
    if abs(x - x1) + abs(y - y1) != 1:
        return False
    
    # **DO NOT BLOCK DEAD ENDS DURING RETURN** - they might be part of shortest path
    
    if x1 == x and y1 == y + 1:  # Going north
        return not hasWall(x, y, "n")
    elif x1 == x + 1 and y1 == y:  # Going east
        return not hasWall(x, y, "e")
    elif x1 == x and y1 == y - 1:  # Going south
        return not hasWall(x, y, "s")
    elif x1 == x - 1 and y1 == y:  # Going west
        return not hasWall(x, y, "w")
    
    return False


def toMoveForReturn(x, y, orient):
    """
    Return movement with oscillation prevention
    """
    directions = [(0, 1), (1, 0), (0, -1), (-1, 0)]  # N, E, S, W
    
    candidates = []
    min_flood_val = 10000
    
    # Find minimum flood value using return-specific accessibility
    for i, (dx, dy) in enumerate(directions):
        xi, yi = x + dx, y + dy
        if not (0 <= xi < 16 and 0 <= yi < 16):
            continue
        if not isAccessibleForReturn(x, y, xi, yi):  # **USE RETURN-SPECIFIC FUNCTION**
            continue
        
        v = flood[yi][xi]
        if v < min_flood_val:
            min_flood_val = v
    
    # Collect all cells with minimum flood value
    for i, (dx, dy) in enumerate(directions):
        xi, yi = x + dx, y + dy
        if not (0 <= xi < 16 and 0 <= yi < 16):
            continue
        if not isAccessibleForReturn(x, y, xi, yi):  # **USE RETURN-SPECIFIC FUNCTION**
            continue
        
        v = flood[yi][xi]
        if v == min_flood_val:
            candidates.append((i, xi, yi))
    
    if not candidates:
        return None
    
    # **ANTI-OSCILLATION: Prefer straight movement strongly**
    for dir_idx, xi, yi in candidates:
        if dir_idx == orient:  # Straight ahead
            return 'F'
    
    # **TIE-BREAKING: Choose direction that moves towards (0,0) when flood values are equal**
    if len(candidates) > 1:
        best_candidate = None
        best_distance = 1000
        
        for dir_idx, xi, yi in candidates:
            # Calculate Manhattan distance to (0,0)
            distance_to_start = abs(xi - 0) + abs(yi - 0)
            if distance_to_start < best_distance:
                best_distance = distance_to_start
                best_candidate = (dir_idx, xi, yi)
        
        if best_candidate:
            best_dir = best_candidate[0]
            diff = (best_dir - orient) % 4
            return ('F', 'R', 'B', 'L')[diff]
    
    # Default to first candidate
    best_dir = candidates[0][0]
    diff = (best_dir - orient) % 4
    return ('F', 'R', 'B', 'L')[diff]

def highlightReturnPath(start_x, start_y):
    """
    Visualize return path using return-specific accessibility
    """
    x, y = start_x, start_y
    path_cells = []
    visited = set()  # **PREVENT CYCLES**
    
    while (x, y) != (0, 0) and len(path_cells) < 256:
        if (x, y) in visited:  # **BREAK CYCLES**
            log(f"Cycle detected at ({x}, {y}) - breaking")
            break
            
        path_cells.append((x, y))
        visited.add((x, y))
        
        # Find next cell with lowest flood value
        min_val = flood[y][x]
        next_cell = None
        best_distance = 1000
        
        for dx, dy in [(0, 1), (1, 0), (0, -1), (-1, 0)]:
            nx, ny = x + dx, y + dy
            if 0 <= nx < 16 and 0 <= ny < 16:
                if isAccessibleForReturn(x, y, nx, ny):  # **USE RETURN-SPECIFIC FUNCTION**
                    if flood[ny][nx] < min_val:
                        min_val = flood[ny][nx]
                        next_cell = (nx, ny)
                    elif flood[ny][nx] == min_val:  # **TIE-BREAKING**
                        distance_to_start = abs(nx - 0) + abs(ny - 0)
                        if distance_to_start < best_distance:
                            best_distance = distance_to_start
                            next_cell = (nx, ny)
        
        if next_cell and next_cell not in visited:
            x, y = next_cell
        else:
            break
    
    # Highlight the return path
    for px, py in path_cells:
        API.setColor(px, py, "Y")
    
    API.setColor(0, 0, "G")
    log(f"Return path length: {len(path_cells)} cells")
    
    return len(path_cells)

def safeMove(x, y, orient):
    """Check if front movement is actually safe"""
    # Always check front wall before any move
    if API.wallFront():
        abs_dir = ['n', 'e', 's', 'w'][orient]
        if (x, y, abs_dir) not in updateWalls.walls:
            log(f"SAFETY: Front wall discovered at ({x}, {y}) direction {abs_dir}")
            API.setWall(x, y, abs_dir)
            _record_wall(x, y, abs_dir)
            return False
    return True


def completeStop(x, y, message="STOPPED"):
    """
    Completely stop the bot and clean up visualization
    """
    log(f"COMPLETE STOP at ({x}, {y}): {message}")
    
    # Clear all previous visualizations
    API.clearAllColor()
    API.clearAllText()
    
    # Mark final position
    API.setColor(x, y, "G")
    API.setText(x, y, message)
    
    # Mark start position if we're there
    if (x, y) == (0, 0):
        API.setText(0, 0, "START-END")
    
    log("ALL BOT OPERATIONS TERMINATED")
    return True

######################################################################################

def findShortestExploredPath():
    """
    Find shortest path from (0,0) to goal using only explored cells
    """
    global explored_cells
    
    if (0, 0) not in explored_cells:
        log("ERROR: Starting position not in explored cells")
        return None
    
    # Goal cells in explored territory
    goal_cells = [(7, 7), (7, 8), (8, 7), (8, 8)]
    explored_goals = [goal for goal in goal_cells if goal in explored_cells]
    
    if not explored_goals:
        log("ERROR: No goal cells in explored territory")
        return None
    
    # BFS to find shortest path through explored cells only
    from collections import deque
    
    queue = deque([(0, 0, [])])  # (x, y, path)
    visited = set()
    visited.add((0, 0))
    
    while queue:
        x, y, path = queue.popleft()
        current_path = path + [(x, y)]
        
        # Check if we reached any goal
        if (x, y) in explored_goals:
            log(f"Shortest explored path found: {len(current_path)} cells")
            return current_path
        
        # Explore neighbors (only through explored cells)
        for dx, dy in [(0, 1), (1, 0), (0, -1), (-1, 0)]:
            nx, ny = x + dx, y + dy
            
            if (nx, ny) not in visited and (nx, ny) in explored_cells:
                # Check if movement is possible (no walls)
                if isAccessibleForReturn(x, y, nx, ny):
                    visited.add((nx, ny))
                    queue.append((nx, ny, current_path))
    
    log("No path found through explored cells")
    return None

def highlightOptimalPath(path):
    """
    Highlight the optimal path in distinct color
    """
    if not path:
        return
        
    # Clear previous highlights
    API.clearAllColor()
    
    # # Highlight explored territory in light color
    # for x, y in explored_cells:
    #     API.setColor(x, y, "C")  # Cyan for explored
    
    # Highlight optimal path in bright color
    for i, (x, y) in enumerate(path):
        if (x, y) == (0, 0):
            API.setColor(x, y, "G")  # Green for start
            API.setText(x, y, "START")
        elif (x, y) in [(7, 7), (7, 8), (8, 7), (8, 8)]:
            API.setColor(x, y, "R")  # Red for goal
            API.setText(x, y, "GOAL")
        else:
            API.setColor(x, y, "O")  # Orange for optimal path
            API.setText(x, y, str(i))  # Number the path
    
    log(f"OPTIMAL PATH HIGHLIGHTED: {len(path)} cells total")
    
    # Log the complete path
    path_str = " -> ".join([f"({x},{y})" for x, y in path])
    log(f"Optimal path: {path_str}")

def analyzePathEfficiency():
    """
    Compare exploration efficiency with optimal path
    """
    global explored_cells
    
    optimal_path = findShortestExploredPath()
    if not optimal_path:
        return
        
    total_explored = len(explored_cells)
    optimal_length = len(optimal_path)
    efficiency = (optimal_length / total_explored) * 100
    
    log(f"=== PATH ANALYSIS ===")
    log(f"Total cells explored: {total_explored}")
    log(f"Optimal path length: {optimal_length}")
    log(f"Path efficiency: {efficiency:.1f}%")
    log(f"Exploration overhead: {total_explored - optimal_length} extra cells")
    
    return {
        'total_explored': total_explored,
        'optimal_length': optimal_length,
        'efficiency': efficiency
    }

###############################################################################################################


def showFlood(xrun, yrun):
    for x in range(16):
        for y in range(16):
            API.setText(x, y, str(flood[y][x]))

def debugFlood(x, y):
    log(f"At ({x}, {y}), flood value: {flood[y][x]}")
    dirs = getSurrounds(x, y)
    for i, (xi, yi) in enumerate(dirs):
        if xi >= 0 and yi >= 0:
            accessible = isAccessible(x, y, xi, yi)
            log(f"  Dir {i}: ({xi}, {yi}) - Accessible: {accessible}, Flood: {flood[yi][xi]}")
def main():
    global explored_cells
    x = 0
    y = 0
    xprev = 0
    yprev = 0
    orient = 0
    bot_terminated = False
    
    # Mark starting cell as explored
    markCellAsExplored(x, y)

    while not bot_terminated:
        # Check goal IMMEDIATELY after moving
        if (x, y) in [(7, 7), (7, 8), (8, 7), (8, 8)]:
            log(f"GOAL REACHED at ({x}, {y})")
            API.setColor(x, y, "G")
            
            # Show return path before executing
            floodFillForReturn()
            path_length = highlightReturnPath(x, y)
            
            # Execute return journey with cell tracking
            success = traceBackToStartWithTracking(x, y, orient)
            if success:
                log("RETURN JOURNEY COMPLETED - CALCULATING OPTIMAL PATH")
                
                # **NEW: Calculate and highlight shortest path through explored cells**
                optimal_path = findShortestExploredPath()
                if optimal_path:
                    highlightOptimalPath(optimal_path)
                    log(f"OPTIMAL PATH FOUND: {len(optimal_path)} cells")
                else:
                    log("No valid optimal path found through explored cells")
                
                bot_terminated = True
                break
            else:
                log("Return journey failed!")
                bot_terminated = True
                break

        L = API.wallLeft()
        R = API.wallRight()
        F = API.wallFront()

        walls_changed = updateWalls(x, y, orient, L, R, F)

        if walls_changed:
            floodFill()
            if len(updateWalls.walls) % 5 == 0:
                markDeadEnds()

        highlightPath(x, y)
        debugFlood(x, y)

        direction = toMove(x, y, xprev, yprev, orient)
        if direction is None:
            log(f"No valid move found at ({x}, {y}) - TERMINATING")
            if hasattr(markDeadEnds, 'dead_ends'):
                floodFill()
                direction = toMove(x, y, xprev, yprev, orient)
            if direction is None:
                log("BOT STUCK - TERMINATING")
                bot_terminated = True
                break

        # Handle turning
        if direction == 'L':
            API.turnLeft()
            orient = API.orientation(orient, 'L')
        elif direction == 'R':
            API.turnRight()
            orient = API.orientation(orient, 'R')
        elif direction == 'B':
            API.turnLeft()
            orient = API.orientation(orient, 'L')
            API.turnLeft()
            orient = API.orientation(orient, 'L')

        # Move forward and track explored cells
        try:
            xnext, ynext = API.updateCoordinates(x, y, orient)
            log(f"Moving from ({x}, {y}) to -> ({xnext}, {ynext}), facing {orient}")
            
            API.moveForward()
            xprev = x
            yprev = y
            x = xnext
            y = ynext
            
            # **TRACK NEWLY EXPLORED CELL**
            markCellAsExplored(x, y)
            
        except API.MouseCrashedError:
            log("MOUSE CRASHED - TERMINATING")
            bot_terminated = True
            break

        showFlood(x, y)
    
    log("BOT EXECUTION COMPLETED - OPTIMAL PATH HIGHLIGHTED")

if __name__ == "__main__":
    main()

