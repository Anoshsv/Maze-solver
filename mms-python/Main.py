import API
import sys
from collections import deque

x = 0
y = 0
orient = 0
cells = [[0 for _ in range(16)] for _ in range(16)]

flood = [[abs(7 - x) + abs(7 - y) if x <= 7 and y <= 7 else
          abs(8 - x) + abs(7 - y) if x > 7 and y <= 7 else
          abs(7 - x) + abs(8 - y) if x <= 7 and y > 7 else
          abs(8 - x) + abs(8 - y)
          for x in range(16)] for y in range(16)]


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

    rel_sensors = [('L', L), ('F', F), ('R', R)]
    for rel_name, present in rel_sensors:
        if not present:
            continue
        if rel_name == 'F':
            abs_dir = ['n', 'e', 's', 'w'][orient]
        elif rel_name == 'R':
            abs_dir = ['e', 's', 'w', 'n'][orient]
        else:  # 'L'
            abs_dir = ['w', 'n', 'e', 's'][orient]

        API.setWall(x, y, abs_dir)
        _record_wall(x, y, abs_dir)


def hasWall(x, y, direction):
    if not hasattr(updateWalls, 'walls'):
        return False
    return updateWalls.walls.get((x, y, direction), False)


def isAccessible(x, y, x1, y1):
    if abs(x - x1) + abs(y - y1) != 1:
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
    x0, y0 = x, y + 1
    x1, y1 = x + 1, y
    x2, y2 = x, y - 1
    x3, y3 = x - 1, y
    return (
        (x0 if y0 < 16 else -1, y0 if y0 < 16 else -1),
        (x1 if x1 < 16 else -1, y1),
        (x2, y2 if y2 >= 0 else -1),
        (x3 if x3 >= 0 else -1, y3),
    )


def floodFill():
    # Reset all flood values except goal cells
    for y_iter in range(16):
        for x_iter in range(16):
            if (x_iter, y_iter) in [(7, 7), (7, 8), (8, 7), (8, 8)]:
                flood[y_iter][x_iter] = 0
            else:
                flood[y_iter][x_iter] = 1000

    # BFS from goal cells outward
    queue = deque([(7, 7), (7, 8), (8, 7), (8, 8)])

    while queue:
        x_curr, y_curr = queue.popleft()
        curr_val = flood[y_curr][x_curr]

        # Check all 4 directions
        for dx, dy, direction in [(0, 1, 'n'), (1, 0, 'e'), (0, -1, 's'), (-1, 0, 'w')]:
            x_n = x_curr + dx
            y_n = y_curr + dy
            
            if 0 <= x_n < 16 and 0 <= y_n < 16:
                # Check if there's no wall between current and neighbor
                if not hasWall(x_curr, y_curr, direction):
                    if flood[y_n][x_n] > curr_val + 1:
                        flood[y_n][x_n] = curr_val + 1
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
    dirs = getSurrounds(x, y)            # N, E, S, W
    best_dir = None
    best_val = 10000

    for i, (xi, yi) in enumerate(dirs):
        if xi < 0 or yi < 0:
            continue
        if not isAccessible(x, y, xi, yi):
            continue
        v = flood[yi][xi]
        if v < best_val:
            best_val = v
            best_dir = i

    if best_dir is None:
        log(f"No accessible neighbour from ({x}, {y})")
        return None

    # Convert best_dir (0-3) to turn command
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
        updateWalls(x, y, orient, L, R, F)

        # Run flood fill after updating walls
        floodFill()  # No parameters needed now

        highlightPath(x, y)

        debugFlood(x, y)

        direction = toMove(x, y, xprev, yprev, orient)
        if direction is None:
            log(f"No valid move found at ({x}, {y})")
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

