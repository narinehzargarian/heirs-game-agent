import time

COL_LETTERS = ['a','b','c','d','e','f','g','h','j','k','m','n']
IDX_TO_COL = {i: c for i, c in enumerate(COL_LETTERS)}

WHITE_SET = frozenset('BPXYGTSN')
BLACK_SET = frozenset('bpxygtsn')

PIECE_VAL = {'P':100000,'p':100000,'X':900,'x':900,'S':500,'s':500,
             'G':400,'g':400,'T':400,'t':400,'Y':300,'y':300,
             'N':250,'n':250,'B':100,'b':100,'.':0}

_CB = [[0]*12 for _ in range(12)]
for _r in range(12):
    for _c in range(12):
        _CB[_r][_c] = max(0, int(6 - abs(_r - 5.5) - abs(_c - 5.5)))

_BAW = [max(0, (10 - r) * 8) for r in range(12)]
_BAB = [max(0, (r - 1) * 8) for r in range(12)]

DIRS8 = [(-1,-1),(-1,0),(-1,1),(0,-1),(0,1),(1,-1),(1,0),(1,1)]
ORTHO = [(-1,0),(1,0),(0,-1),(0,1)]
DIAG = [(-1,-1),(-1,1),(1,-1),(1,1)]


def parse_input():
    with open('input.txt', 'r') as f:
        lines = f.read().strip().split('\n')
    player = lines[0].strip()
    t = lines[1].strip().split()
    my_time, opp_time = float(t[0]), float(t[1])
    board = [list(lines[i].strip()) for i in range(2, 14)]
    return player, my_time, opp_time, board


def pos_to_str(r, c):
    return IDX_TO_COL[c] + str(12 - r)


def gen_moves(board, is_w):
    moves = []
    friendly = WHITE_SET if is_w else BLACK_SET
    enemy = BLACK_SET if is_w else WHITE_SET
    fwd = -1 if is_w else 1

    for r in range(12):
        row = board[r]
        for c in range(12):
            p = row[c]
            if p not in friendly:
                continue
            pu = p if p < 'a' else chr(ord(p) - 32)

            if pu == 'B':
                nr = r + fwd
                if 0 <= nr < 12:
                    t = board[nr][c]
                    if t == '.' or t in enemy:
                        moves.append((r, c, nr, c))
                        if t == '.':
                            nr2 = nr + fwd
                            if 0 <= nr2 < 12:
                                t2 = board[nr2][c]
                                if t2 == '.' or t2 in enemy:
                                    moves.append((r, c, nr2, c))

            elif pu == 'P':
                for dr, dc in DIRS8:
                    nr, nc = r+dr, c+dc
                    if 0 <= nr < 12 and 0 <= nc < 12:
                        t = board[nr][nc]
                        if t == '.' or t in enemy:
                            moves.append((r, c, nr, nc))

            elif pu == 'X':
                for dr, dc in DIRS8:
                    for dist in range(1, 4):
                        nr, nc = r+dr*dist, c+dc*dist
                        if not (0 <= nr < 12 and 0 <= nc < 12):
                            break
                        t = board[nr][nc]
                        if t == '.':
                            moves.append((r, c, nr, nc))
                        elif t in enemy:
                            moves.append((r, c, nr, nc))
                            break
                        else:
                            break

            elif pu == 'Y':
                for dr, dc in DIAG:
                    nr, nc = r+dr, c+dc
                    if 0 <= nr < 12 and 0 <= nc < 12:
                        t = board[nr][nc]
                        if t == '.' or t in enemy:
                            moves.append((r, c, nr, nc))

            elif pu == 'G':
                for dr, dc in ORTHO:
                    for dist in range(1, 3):
                        nr, nc = r+dr*dist, c+dc*dist
                        if not (0 <= nr < 12 and 0 <= nc < 12):
                            break
                        t = board[nr][nc]
                        if t == '.':
                            moves.append((r, c, nr, nc))
                        elif t in enemy:
                            moves.append((r, c, nr, nc))
                            break
                        else:
                            break

            elif pu == 'T':
                for dr, dc in DIAG:
                    for dist in range(1, 3):
                        nr, nc = r+dr*dist, c+dc*dist
                        if not (0 <= nr < 12 and 0 <= nc < 12):
                            break
                        t = board[nr][nc]
                        if t == '.':
                            moves.append((r, c, nr, nc))
                        elif t in enemy:
                            moves.append((r, c, nr, nc))
                            break
                        else:
                            break

            elif pu == 'S':
                for f in range(1, 4):
                    nr = r + f * fwd
                    if not (0 <= nr < 12):
                        break
                    t = board[nr][c]
                    if t == '.' or t in enemy:
                        moves.append((r, c, nr, c))
                    for side in (-1, 1):
                        nc = c + side
                        if 0 <= nc < 12:
                            t = board[nr][nc]
                            if t == '.' or t in enemy:
                                moves.append((r, c, nr, nc))

            elif pu == 'N':
                for dr, dc in DIRS8:
                    nr, nc = r+dr, c+dc
                    if not (0 <= nr < 12 and 0 <= nc < 12):
                        continue
                    t = board[nr][nc]
                    if t != '.' and t not in enemy:
                        continue
                    ok = False
                    for dr2, dc2 in DIRS8:
                        ar, ac = nr+dr2, nc+dc2
                        if 0 <= ar < 12 and 0 <= ac < 12:
                            if ar == r and ac == c:
                                continue
                            if board[ar][ac] in friendly:
                                ok = True
                                break
                    if ok:
                        moves.append((r, c, nr, nc))
    return moves


def gen_captures(board, is_w):
    """Generate only capture moves for quiescence search."""
    moves = []
    friendly = WHITE_SET if is_w else BLACK_SET
    enemy = BLACK_SET if is_w else WHITE_SET
    fwd = -1 if is_w else 1

    for r in range(12):
        row = board[r]
        for c in range(12):
            p = row[c]
            if p not in friendly:
                continue
            pu = p if p < 'a' else chr(ord(p) - 32)

            if pu == 'B':
                nr = r + fwd
                if 0 <= nr < 12:
                    t = board[nr][c]
                    if t in enemy:
                        moves.append((r, c, nr, c))
                    elif t == '.':
                        nr2 = nr + fwd
                        if 0 <= nr2 < 12 and board[nr2][c] in enemy:
                            moves.append((r, c, nr2, c))

            elif pu == 'P':
                for dr, dc in DIRS8:
                    nr, nc = r+dr, c+dc
                    if 0 <= nr < 12 and 0 <= nc < 12 and board[nr][nc] in enemy:
                        moves.append((r, c, nr, nc))

            elif pu == 'X':
                for dr, dc in DIRS8:
                    for dist in range(1, 4):
                        nr, nc = r+dr*dist, c+dc*dist
                        if not (0 <= nr < 12 and 0 <= nc < 12):
                            break
                        t = board[nr][nc]
                        if t in enemy:
                            moves.append((r, c, nr, nc))
                            break
                        elif t != '.':
                            break

            elif pu == 'Y':
                for dr, dc in DIAG:
                    nr, nc = r+dr, c+dc
                    if 0 <= nr < 12 and 0 <= nc < 12 and board[nr][nc] in enemy:
                        moves.append((r, c, nr, nc))

            elif pu == 'G':
                for dr, dc in ORTHO:
                    for dist in range(1, 3):
                        nr, nc = r+dr*dist, c+dc*dist
                        if not (0 <= nr < 12 and 0 <= nc < 12):
                            break
                        t = board[nr][nc]
                        if t in enemy:
                            moves.append((r, c, nr, nc))
                            break
                        elif t != '.':
                            break

            elif pu == 'T':
                for dr, dc in DIAG:
                    for dist in range(1, 3):
                        nr, nc = r+dr*dist, c+dc*dist
                        if not (0 <= nr < 12 and 0 <= nc < 12):
                            break
                        t = board[nr][nc]
                        if t in enemy:
                            moves.append((r, c, nr, nc))
                            break
                        elif t != '.':
                            break

            elif pu == 'S':
                for f in range(1, 4):
                    nr = r + f * fwd
                    if not (0 <= nr < 12):
                        break
                    if board[nr][c] in enemy:
                        moves.append((r, c, nr, c))
                    for side in (-1, 1):
                        nc = c + side
                        if 0 <= nc < 12 and board[nr][nc] in enemy:
                            moves.append((r, c, nr, nc))

            elif pu == 'N':
                for dr, dc in DIRS8:
                    nr, nc = r+dr, c+dc
                    if not (0 <= nr < 12 and 0 <= nc < 12):
                        continue
                    t = board[nr][nc]
                    if t not in enemy:
                        continue
                    ok = False
                    for dr2, dc2 in DIRS8:
                        ar, ac = nr+dr2, nc+dc2
                        if 0 <= ar < 12 and 0 <= ac < 12:
                            if ar == r and ac == c:
                                continue
                            if board[ar][ac] in friendly:
                                ok = True
                                break
                    if ok:
                        moves.append((r, c, nr, nc))
    return moves


def is_attacked_by(board, r, c, by_white):
    """Check if square (r,c) is attacked by any piece of the given color."""
    attacker = WHITE_SET if by_white else BLACK_SET
    fwd = -1 if by_white else 1  # attacker's forward

    # Check baby attacks (baby captures forward)
    br = r - fwd  # row the attacking baby would be on
    if 0 <= br < 12:
        p = board[br][c]
        if p in attacker and (p == 'B' or p == 'b'):
            return True
        # Baby can also capture from 2 away
        br2 = r - 2*fwd
        if 0 <= br2 < 12:
            p2 = board[br2][c]
            mid = board[br][c]
            if p2 in attacker and (p2 == 'B' or p2 == 'b') and mid == '.':
                return True

    # Prince attacks (1 step any dir)
    for dr, dc in DIRS8:
        nr, nc = r+dr, c+dc
        if 0 <= nr < 12 and 0 <= nc < 12:
            p = board[nr][nc]
            if p in attacker and (p == 'P' or p == 'p'):
                return True

    # Princess attacks (1-3 any dir sliding)
    for dr, dc in DIRS8:
        for dist in range(1, 4):
            nr, nc = r+dr*dist, c+dc*dist
            if not (0 <= nr < 12 and 0 <= nc < 12):
                break
            p = board[nr][nc]
            if p == '.':
                continue
            if p in attacker and (p == 'X' or p == 'x'):
                return True
            break

    # Pony (1 diagonal)
    for dr, dc in DIAG:
        nr, nc = r+dr, c+dc
        if 0 <= nr < 12 and 0 <= nc < 12:
            p = board[nr][nc]
            if p in attacker and (p == 'Y' or p == 'y'):
                return True

    # Guard (1-2 orthogonal)
    for dr, dc in ORTHO:
        for dist in range(1, 3):
            nr, nc = r+dr*dist, c+dc*dist
            if not (0 <= nr < 12 and 0 <= nc < 12):
                break
            p = board[nr][nc]
            if p == '.':
                continue
            if p in attacker and (p == 'G' or p == 'g'):
                return True
            break

    # Tutor (1-2 diagonal)
    for dr, dc in DIAG:
        for dist in range(1, 3):
            nr, nc = r+dr*dist, c+dc*dist
            if not (0 <= nr < 12 and 0 <= nc < 12):
                break
            p = board[nr][nc]
            if p == '.':
                continue
            if p in attacker and (p == 'T' or p == 't'):
                return True
            break

    # Scout (1-3 forward + optional sideways, can jump)
    # Scout attacks from behind (attacking forward toward us)
    afwd = fwd  # attacker's forward direction
    for f in range(1, 4):
        sr = r - f * afwd  # row the scout would be on
        if not (0 <= sr < 12):
            break
        # straight
        p = board[sr][c]
        if p in attacker and (p == 'S' or p == 's'):
            return True
        # sideways: scout at (sr, c±1) attacking with sideways shift
        for side in (-1, 1):
            sc = c - side
            if 0 <= sc < 12:
                p = board[sr][sc]
                if p in attacker and (p == 'S' or p == 's'):
                    return True

    # Sibling (1 step any dir, but needs adjacency - just check if adjacent)
    for dr, dc in DIRS8:
        nr, nc = r+dr, c+dc
        if 0 <= nr < 12 and 0 <= nc < 12:
            p = board[nr][nc]
            if p in attacker and (p == 'N' or p == 'n'):
                return True

    return False


def evaluate(board, is_w):
    ws = 0
    bs = 0
    wp_r, wp_c = -1, -1
    bp_r, bp_c = -1, -1

    for r in range(12):
        row = board[r]
        cbr = _CB[r]
        for c in range(12):
            p = row[c]
            if p == '.':
                continue
            if p in WHITE_SET:
                ws += PIECE_VAL[p] + cbr[c] * 3
                if p == 'B':
                    ws += _BAW[r]
                elif p == 'P':
                    wp_r, wp_c = r, c
            else:
                bs += PIECE_VAL[p] + cbr[c] * 3
                if p == 'b':
                    bs += _BAB[r]
                elif p == 'p':
                    bp_r, bp_c = r, c

    # Prince safety: penalize if prince is under attack
    if wp_r >= 0 and is_attacked_by(board, wp_r, wp_c, False):
        ws -= 500
    if bp_r >= 0 and is_attacked_by(board, bp_r, bp_c, True):
        bs -= 500

    # Bonus for friendly pieces adjacent to prince (protection)
    if wp_r >= 0:
        for dr, dc in DIRS8:
            nr, nc = wp_r+dr, wp_c+dc
            if 0 <= nr < 12 and 0 <= nc < 12 and board[nr][nc] in WHITE_SET:
                ws += 15
    if bp_r >= 0:
        for dr, dc in DIRS8:
            nr, nc = bp_r+dr, bp_c+dc
            if 0 <= nr < 12 and 0 <= nc < 12 and board[nr][nc] in BLACK_SET:
                bs += 15

    score = ws - bs
    return score if is_w else -score


class TimeUp(Exception):
    pass


_node_count = 0
_killer1 = [None] * 64  # killer moves indexed by depth
_killer2 = [None] * 64


def quiesce(board, alpha, beta, is_w, maximizing, deadline):
    global _node_count
    _node_count += 1
    if (_node_count & 2047) == 0:
        if time.time() >= deadline:
            raise TimeUp()

    stand_pat = evaluate(board, is_w)

    if maximizing:
        if stand_pat >= beta:
            return beta
        if stand_pat > alpha:
            alpha = stand_pat
    else:
        if stand_pat <= alpha:
            return alpha
        if stand_pat < beta:
            beta = stand_pat

    current_w = is_w if maximizing else not is_w
    caps = gen_captures(board, current_w)
    if not caps:
        return stand_pat

    # Sort captures by MVV-LVA
    caps.sort(key=lambda m: -PIECE_VAL[board[m[2]][m[3]]] + PIECE_VAL[board[m[0]][m[1]]] // 10)

    for move in caps:
        r1, c1, r2, c2 = move
        captured = board[r2][c2]

        # Prince capture = game over
        if captured == 'p' or captured == 'P':
            return 200000 if maximizing else -200000

        # Delta pruning: skip if capture can't raise alpha
        if maximizing:
            if stand_pat + PIECE_VAL[captured] + 200 < alpha:
                continue
        else:
            if stand_pat - PIECE_VAL[captured] - 200 > beta:
                continue

        piece = board[r1][c1]
        board[r2][c2] = piece
        board[r1][c1] = '.'

        score = quiesce(board, alpha, beta, is_w, not maximizing, deadline)

        board[r1][c1] = piece
        board[r2][c2] = captured

        if maximizing:
            if score > alpha:
                alpha = score
            if alpha >= beta:
                return beta
        else:
            if score < beta:
                beta = score
            if alpha >= beta:
                return alpha

    return alpha if maximizing else beta


def alphabeta(board, depth, alpha, beta, maximizing, is_w, deadline, ply):
    global _node_count
    _node_count += 1
    if (_node_count & 2047) == 0:
        if time.time() >= deadline:
            raise TimeUp()

    if depth <= 0:
        return quiesce(board, alpha, beta, is_w, maximizing, deadline), None

    current_w = is_w if maximizing else not is_w
    moves = gen_moves(board, current_w)
    if not moves:
        return evaluate(board, is_w), None

    enemy = BLACK_SET if current_w else WHITE_SET

    # Sort moves: prince captures > captures (MVV-LVA) > killer moves > non-captures
    prince_caps = []
    captures = []
    killers = []
    non_captures = []
    k1 = _killer1[ply] if ply < 64 else None
    k2 = _killer2[ply] if ply < 64 else None

    for m in moves:
        t = board[m[2]][m[3]]
        if t in enemy:
            if t == 'p' or t == 'P':
                prince_caps.append(m)
            else:
                captures.append((PIECE_VAL[t] - PIECE_VAL[board[m[0]][m[1]]] // 10, m))
        elif m == k1 or m == k2:
            killers.append(m)
        else:
            non_captures.append((_CB[m[2]][m[3]], m))

    # Instant win
    if prince_caps:
        m = prince_caps[0]
        if maximizing:
            return 200000 - ply, m
        else:
            return -200000 + ply, m

    captures.sort(key=lambda x: -x[0])
    non_captures.sort(key=lambda x: -x[0])
    sorted_moves = [m for _, m in captures] + killers + [m for _, m in non_captures]

    best_move = sorted_moves[0]

    if maximizing:
        max_eval = -999999
        for move in sorted_moves:
            r1, c1, r2, c2 = move
            captured = board[r2][c2]
            piece = board[r1][c1]
            board[r2][c2] = piece
            board[r1][c1] = '.'

            ev, _ = alphabeta(board, depth - 1, alpha, beta, False, is_w, deadline, ply + 1)

            board[r1][c1] = piece
            board[r2][c2] = captured

            if ev > max_eval:
                max_eval = ev
                best_move = move
            if ev > alpha:
                alpha = ev
            if alpha >= beta:
                # Store killer move
                if captured == '.' and ply < 64:
                    if move != _killer1[ply]:
                        _killer2[ply] = _killer1[ply]
                        _killer1[ply] = move
                break
        return max_eval, best_move
    else:
        min_eval = 999999
        for move in sorted_moves:
            r1, c1, r2, c2 = move
            captured = board[r2][c2]
            piece = board[r1][c1]
            board[r2][c2] = piece
            board[r1][c1] = '.'

            ev, _ = alphabeta(board, depth - 1, alpha, beta, True, is_w, deadline, ply + 1)

            board[r1][c1] = piece
            board[r2][c2] = captured

            if ev < min_eval:
                min_eval = ev
                best_move = move
            if ev < beta:
                beta = ev
            if alpha >= beta:
                if captured == '.' and ply < 64:
                    if move != _killer1[ply]:
                        _killer2[ply] = _killer1[ply]
                        _killer1[ply] = move
                break
        return min_eval, best_move


def solve(board, is_w, my_time):
    global _node_count, _killer1, _killer2

    # Time budget
    pc = sum(1 for r in range(12) for c in range(12) if board[r][c] != '.')
    est_moves = max(30, pc)
    budget = my_time / est_moves
    budget = min(budget, 5.0)
    budget = min(budget, my_time - 1.0)
    budget = max(budget, 0.05)
    if my_time < 5.0:
        budget = min(0.2, max(0.01, my_time * 0.1))

    deadline = time.time() + budget

    moves = gen_moves(board, is_w)
    if not moves:
        return None

    enemy = BLACK_SET if is_w else WHITE_SET
    for m in moves:
        t = board[m[2]][m[3]]
        if (t == 'p' or t == 'P') and t in enemy:
            return m

    best_move = moves[0]
    _killer1 = [None] * 64
    _killer2 = [None] * 64

    for depth in range(1, 50):
        _node_count = 0
        try:
            score, move = alphabeta(board, depth, -999999, 999999, True, is_w, deadline, 0)
            if move is not None:
                best_move = move
            if score >= 190000:
                break
        except TimeUp:
            break

        elapsed = time.time() - (deadline - budget)
        if elapsed > budget * 0.5:
            break

    return best_move


def main():
    player, my_time, opp_time, board = parse_input()
    is_w = (player == 'WHITE')
    best = solve(board, is_w, my_time)
    if best is None:
        moves = gen_moves(board, is_w)
        best = moves[0]
    r1, c1, r2, c2 = best
    with open('output.txt', 'w') as f:
        f.write(pos_to_str(r1, c1) + ' ' + pos_to_str(r2, c2) + '\n')


if __name__ == '__main__':
    main()
