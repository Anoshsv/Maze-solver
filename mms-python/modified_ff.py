import API
import sys
from collections import deque

x = 0
y = 0
orient = 0
cells = [[0 for _ in range(16)] for _ in range(16)]



flood = [[1000 for _ in range(16)] for _ in range(16)]

highlighted_path = set()


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
    x = 0
    y = 0
    xprev = 0
    yprev = 0
    orient = 0

    while True:
        # Check goal IMMEDIATELY after moving
        if (x, y) in [(7, 7), (7, 8), (8, 7), (8, 8)]:
            log(f"GOAL REACHED at ({x}, {y})")
            API.setColor(x, y, "G")
            break

        L = API.wallLeft()
        R = API.wallRight()
        F = API.wallFront()

        walls_changed = updateWalls(x, y, orient, L, R, F)

        if walls_changed:
            floodFill()
            # Mark dead ends more frequently for better detection
            if len(updateWalls.walls) % 5 == 0:  # Every 5 walls instead of 10
                markDeadEnds()

        highlightPath(x, y)
        debugFlood(x, y)

        direction = toMove(x, y, xprev, yprev, orient)
        if direction is None:
            log(f"No valid move found at ({x}, {y})")
            # Try to clear dead end marks and recalculate
            if hasattr(markDeadEnds, 'dead_ends'):
                floodFill()
                direction = toMove(x, y, xprev, yprev, orient)
            
            if direction is None:
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

        # Move forward
        xnext, ynext = API.updateCoordinates(x, y, orient)
        log(f"Moving from ({x}, {y}) to -> ({xnext}, {ynext}), facing {orient}")
        
        API.moveForward()
        xprev = x
        yprev = y
        x = xnext
        y = ynext

        showFlood(x, y)

if __name__ == "__main__":
    main()

