# Heirs Game Agent

## Overview
AI agent for "Heirs", a chess-like 12x12 board game. Uses alpha-beta minimax with iterative deepening.
Goal: capture the opponent's Prince.

## Files
- `homework.py` — Python implementation
- `homework.cpp` — C++ implementation (faster, for tournament)
- `input.txt` — game engine writes this, agent reads it
- `output.txt` — agent writes its move here

## Board Representation
- 12x12 grid stored as array, index 0 = row 12 (top), index 11 = row 1 (bottom)
- Column mapping: a,b,c,d,e,f,g,h,j,k,m,n (skip 'i' and 'l')
- White pieces: uppercase (B,P,X,Y,G,T,S,N), Black: lowercase
- `.` = empty square

## Piece Movement Rules

| Piece | Symbol (W/B) | Movement |
|-------|-------------|----------|
| Baby | B/b | 1-2 forward, no jump, captures forward only |
| Prince | P/p | 1 step any direction (8 dirs). If captured = game over |
| Princess | X/x | 1-3 steps any direction, sliding (no jump) |
| Pony | Y/y | Exactly 1 diagonal step |
| Guard | G/g | 1-2 orthogonal (no jump) |
| Tutor | T/t | 1-2 diagonal (no jump) |
| Scout | S/s | 1-3 forward + optional +/-1 sideways, CAN jump, capture only on landing |
| Sibling | N/n | 1 step any direction, must end adjacent to a friendly piece |

Forward = up for White (decreasing row index), down for Black (increasing row index).

## I/O Format

### input.txt
```
WHITE              <- player color
300.0 300.0        <- your time, opponent time
gytsnxpnstyg       <- row 12 (top)
bbbbbbbbbbbb       <- row 11
............       <- rows 10-3
............
............
............
............
............
............
............
BBBBBBBBBBBB       <- row 2
GYTSNXPNSTYG       <- row 1 (bottom)
```

### output.txt
```
f2 f4              <- source destination
```

## Architecture

### Core Components

**1. Move Generation (`gen_moves`)**
- Iterates all 144 squares, generates legal moves per piece type
- Returns list of (r1,c1,r2,c2) tuples
- `gen_captures` — same but only returns capture moves (for quiescence)

**2. Evaluation Function (`evaluate`)**
- Material values: Prince=100000, Princess=900, Scout=500, Guard=400, Tutor=400, Pony=300, Sibling=250, Baby=100
- Center control bonus: Manhattan distance from center, multiplied by 3
- Baby advancement bonus: +8 per row advanced (max +80)
- Prince safety: -500 penalty if prince is under attack (`is_attacked_by`)
- Prince protection: +15 per friendly piece adjacent to prince

**3. Alpha-Beta Minimax (`alphabeta`)**
- Maximizing = our turn (pick highest score)
- Minimizing = opponent's turn (pick lowest score)
- Alpha = best score we can guarantee
- Beta = best score opponent can guarantee
- Prune when alpha >= beta (opponent won't allow this branch)
- Make/unmake move in-place (no board copying)

**4. Quiescence Search (`quiesce`)**
- Called when main search reaches depth 0
- Continues searching capture moves only until position is quiet
- Stand pat: option to not capture if current eval is good enough
- Delta pruning: skip captures that can't possibly improve alpha
- Prevents horizon effect (stopping mid-capture sequence)

**5. Iterative Deepening (`solve`)**
- Search depth 1, then 2, then 3... until time runs out
- Always has a complete result from previous depth as fallback
- Stops early if winning move found (score >= 190000)

**6. Time Management**
- Budget = remaining_time / max(30, piece_count)
- Capped at 5s (Python) or 8s (C++) per move
- Safety margin: budget < remaining_time - 1.0
- Low time mode (<5s): uses 10% of remaining
- Time checked every 2048-4096 nodes via bitwise AND

## Search Optimizations

| Technique | Description | Effect |
|-----------|-------------|--------|
| **Alpha-beta pruning** | Skip branches that can't affect result | Effectively doubles search depth |
| **Iterative deepening** | Increase depth each round, stop on time | Safe time control, always has an answer |
| **Move ordering (MVV-LVA)** | Most Valuable Victim - Least Valuable Attacker. Try best captures first | More alpha-beta cutoffs |
| **Killer moves** | Remember 2 quiet moves per ply that caused cutoffs, try them early | ~15-20% faster search |
| **Quiescence search** | Don't evaluate mid-capture; keep searching captures at depth 0 | Avoids tactical blunders |
| **Delta pruning** | Skip captures in quiescence that can't raise alpha (with 200pt margin) | Faster quiescence |
| **Stand pat** | In quiescence, option to not capture if eval is already good | Avoids forced bad trades |
| **Prince capture detection** | Instant return on prince capture, prefer faster wins (200000 - ply) | No wasted search on won positions |
| **Make/unmake** | Modify board in-place, undo after recursion | Zero memory allocation in search |
| **Bitwise time check** | `(node_count & 4095) == 0` — check clock every 4096 nodes | Minimal timing overhead |

## Move Ordering Priority
1. Prince captures (instant win/loss)
2. Regular captures sorted by MVV-LVA
3. Killer moves (quiet moves that caused cutoffs at this ply)
4. Non-captures sorted by center bonus

## Key Design Decisions
- **200000 for prince value**: Must be larger than any possible eval sum (~10000 max) so prince capture always dominates
- **200000 - ply for wins**: Prefer capturing prince sooner (ply 3 scores higher than ply 7)
- **-500 prince safety penalty**: Large enough to influence decisions but not override material advantage
- **Frozenset (Python)**: Immutable set for piece membership checks, slightly faster than regular set
- **No transposition table**: Kept simple; could add Zobrist hashing for further improvement


## Game Over Conditions
- Prince captured → capturing side wins
- Time runs out → that player loses
- Illegal move / no output.txt → that player loses
- Draw: no legal moves, threefold repetition, or 50 moves without capture/baby move (engine handles draws)
