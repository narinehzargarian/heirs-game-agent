#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <vector>
#include <chrono>

using namespace std;

static const char COL_LETTERS[12] = {'a','b','c','d','e','f','g','h','j','k','m','n'};

static char board[12][12];
static bool is_white_player;
static double my_time_left, opp_time_left;

struct Move {
    int r1, c1, r2, c2;
    int score; // for sorting
};

// Precomputed tables
static int CB[12][12];
static int BAW[12];
static int BAB[12];

static const int DIRS8[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
static const int ORTHO[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
static const int DIAG4[4][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};

static auto start_time = chrono::steady_clock::now();
static double deadline_sec;
static int node_count;

static Move killer1[64], killer2[64];
static bool has_killer1[64], has_killer2[64];

inline bool in_bounds(int r, int c) { return r >= 0 && r < 12 && c >= 0 && c < 12; }

inline bool is_upper(char c) { return c >= 'A' && c <= 'Z'; }
inline bool is_lower(char c) { return c >= 'a' && c <= 'z'; }
inline char to_upper(char c) { return is_lower(c) ? c - 32 : c; }

inline bool is_white_piece(char p) {
    return p == 'B' || p == 'P' || p == 'X' || p == 'Y' || p == 'G' || p == 'T' || p == 'S' || p == 'N';
}
inline bool is_black_piece(char p) {
    return p == 'b' || p == 'p' || p == 'x' || p == 'y' || p == 'g' || p == 't' || p == 's' || p == 'n';
}
inline bool is_friendly(char p, bool w) { return w ? is_white_piece(p) : is_black_piece(p); }
inline bool is_enemy(char p, bool w) { return w ? is_black_piece(p) : is_white_piece(p); }

inline int piece_val(char p) {
    switch(to_upper(p)) {
        case 'P': return 100000;
        case 'X': return 900;
        case 'S': return 500;
        case 'G': return 400;
        case 'T': return 400;
        case 'Y': return 300;
        case 'N': return 250;
        case 'B': return 100;
        default: return 0;
    }
}

inline double elapsed_sec() {
    auto now = chrono::steady_clock::now();
    return chrono::duration<double>(now - start_time).count();
}

class TimeUp {};

inline void check_time() {
    if ((node_count & 4095) == 0) {
        if (elapsed_sec() >= deadline_sec)
            throw TimeUp();
    }
}

void init_tables() {
    for (int r = 0; r < 12; r++)
        for (int c = 0; c < 12; c++) {
            double dr = r - 5.5, dc = c - 5.5;
            if (dr < 0) dr = -dr;
            if (dc < 0) dc = -dc;
            int v = (int)(6 - dr - dc);
            CB[r][c] = v > 0 ? v : 0;
        }
    for (int r = 0; r < 12; r++) {
        int v = (10 - r) * 8;
        BAW[r] = v > 0 ? v : 0;
        v = (r - 1) * 8;
        BAB[r] = v > 0 ? v : 0;
    }
}

void parse_input() {
    FILE* f = fopen("input.txt", "r");
    char color[10];
    fscanf(f, "%s", color);
    is_white_player = (strcmp(color, "WHITE") == 0);
    fscanf(f, "%lf %lf", &my_time_left, &opp_time_left);
    for (int r = 0; r < 12; r++) {
        char line[20];
        fscanf(f, "%s", line);
        for (int c = 0; c < 12; c++)
            board[r][c] = line[c];
    }
    fclose(f);
}

void write_output(Move& m) {
    FILE* f = fopen("output.txt", "w");
    fprintf(f, "%c%d %c%d\n", COL_LETTERS[m.c1], 12 - m.r1, COL_LETTERS[m.c2], 12 - m.r2);
    fclose(f);
}

int gen_moves(Move* moves, bool is_w) {
    int cnt = 0;
    int fwd = is_w ? -1 : 1;

    for (int r = 0; r < 12; r++) {
        for (int c = 0; c < 12; c++) {
            char p = board[r][c];
            if (!is_friendly(p, is_w)) continue;
            char pu = to_upper(p);

            if (pu == 'B') {
                int nr = r + fwd;
                if (in_bounds(nr, c)) {
                    char t = board[nr][c];
                    if (t == '.' || is_enemy(t, is_w)) {
                        moves[cnt++] = {r, c, nr, c, 0};
                        if (t == '.') {
                            int nr2 = nr + fwd;
                            if (in_bounds(nr2, c)) {
                                char t2 = board[nr2][c];
                                if (t2 == '.' || is_enemy(t2, is_w))
                                    moves[cnt++] = {r, c, nr2, c, 0};
                            }
                        }
                    }
                }
            } else if (pu == 'P') {
                for (int d = 0; d < 8; d++) {
                    int nr = r + DIRS8[d][0], nc = c + DIRS8[d][1];
                    if (in_bounds(nr, nc)) {
                        char t = board[nr][nc];
                        if (t == '.' || is_enemy(t, is_w))
                            moves[cnt++] = {r, c, nr, nc, 0};
                    }
                }
            } else if (pu == 'X') {
                for (int d = 0; d < 8; d++) {
                    for (int dist = 1; dist <= 3; dist++) {
                        int nr = r + DIRS8[d][0]*dist, nc = c + DIRS8[d][1]*dist;
                        if (!in_bounds(nr, nc)) break;
                        char t = board[nr][nc];
                        if (t == '.') {
                            moves[cnt++] = {r, c, nr, nc, 0};
                        } else if (is_enemy(t, is_w)) {
                            moves[cnt++] = {r, c, nr, nc, 0};
                            break;
                        } else break;
                    }
                }
            } else if (pu == 'Y') {
                for (int d = 0; d < 4; d++) {
                    int nr = r + DIAG4[d][0], nc = c + DIAG4[d][1];
                    if (in_bounds(nr, nc)) {
                        char t = board[nr][nc];
                        if (t == '.' || is_enemy(t, is_w))
                            moves[cnt++] = {r, c, nr, nc, 0};
                    }
                }
            } else if (pu == 'G') {
                for (int d = 0; d < 4; d++) {
                    for (int dist = 1; dist <= 2; dist++) {
                        int nr = r + ORTHO[d][0]*dist, nc = c + ORTHO[d][1]*dist;
                        if (!in_bounds(nr, nc)) break;
                        char t = board[nr][nc];
                        if (t == '.') {
                            moves[cnt++] = {r, c, nr, nc, 0};
                        } else if (is_enemy(t, is_w)) {
                            moves[cnt++] = {r, c, nr, nc, 0};
                            break;
                        } else break;
                    }
                }
            } else if (pu == 'T') {
                for (int d = 0; d < 4; d++) {
                    for (int dist = 1; dist <= 2; dist++) {
                        int nr = r + DIAG4[d][0]*dist, nc = c + DIAG4[d][1]*dist;
                        if (!in_bounds(nr, nc)) break;
                        char t = board[nr][nc];
                        if (t == '.') {
                            moves[cnt++] = {r, c, nr, nc, 0};
                        } else if (is_enemy(t, is_w)) {
                            moves[cnt++] = {r, c, nr, nc, 0};
                            break;
                        } else break;
                    }
                }
            } else if (pu == 'S') {
                for (int fw = 1; fw <= 3; fw++) {
                    int nr = r + fw * fwd;
                    if (!in_bounds(nr, 0)) break;
                    // straight
                    char t = board[nr][c];
                    if (t == '.' || is_enemy(t, is_w))
                        moves[cnt++] = {r, c, nr, c, 0};
                    // sideways
                    for (int side = -1; side <= 1; side += 2) {
                        int nc = c + side;
                        if (in_bounds(nr, nc)) {
                            t = board[nr][nc];
                            if (t == '.' || is_enemy(t, is_w))
                                moves[cnt++] = {r, c, nr, nc, 0};
                        }
                    }
                }
            } else if (pu == 'N') {
                for (int d = 0; d < 8; d++) {
                    int nr = r + DIRS8[d][0], nc = c + DIRS8[d][1];
                    if (!in_bounds(nr, nc)) continue;
                    char t = board[nr][nc];
                    if (t != '.' && !is_enemy(t, is_w)) continue;
                    bool ok = false;
                    for (int d2 = 0; d2 < 8 && !ok; d2++) {
                        int ar = nr + DIRS8[d2][0], ac = nc + DIRS8[d2][1];
                        if (in_bounds(ar, ac) && !(ar == r && ac == c) && is_friendly(board[ar][ac], is_w))
                            ok = true;
                    }
                    if (ok)
                        moves[cnt++] = {r, c, nr, nc, 0};
                }
            }
        }
    }
    return cnt;
}

int gen_captures(Move* moves, bool is_w) {
    int cnt = 0;
    int fwd = is_w ? -1 : 1;

    for (int r = 0; r < 12; r++) {
        for (int c = 0; c < 12; c++) {
            char p = board[r][c];
            if (!is_friendly(p, is_w)) continue;
            char pu = to_upper(p);

            if (pu == 'B') {
                int nr = r + fwd;
                if (in_bounds(nr, c)) {
                    char t = board[nr][c];
                    if (is_enemy(t, is_w)) {
                        moves[cnt++] = {r, c, nr, c, 0};
                    } else if (t == '.') {
                        int nr2 = nr + fwd;
                        if (in_bounds(nr2, c) && is_enemy(board[nr2][c], is_w))
                            moves[cnt++] = {r, c, nr2, c, 0};
                    }
                }
            } else if (pu == 'P') {
                for (int d = 0; d < 8; d++) {
                    int nr = r+DIRS8[d][0], nc = c+DIRS8[d][1];
                    if (in_bounds(nr,nc) && is_enemy(board[nr][nc], is_w))
                        moves[cnt++] = {r, c, nr, nc, 0};
                }
            } else if (pu == 'X') {
                for (int d = 0; d < 8; d++) {
                    for (int dist = 1; dist <= 3; dist++) {
                        int nr = r+DIRS8[d][0]*dist, nc = c+DIRS8[d][1]*dist;
                        if (!in_bounds(nr,nc)) break;
                        char t = board[nr][nc];
                        if (is_enemy(t, is_w)) { moves[cnt++] = {r,c,nr,nc,0}; break; }
                        if (t != '.') break;
                    }
                }
            } else if (pu == 'Y') {
                for (int d = 0; d < 4; d++) {
                    int nr = r+DIAG4[d][0], nc = c+DIAG4[d][1];
                    if (in_bounds(nr,nc) && is_enemy(board[nr][nc], is_w))
                        moves[cnt++] = {r,c,nr,nc,0};
                }
            } else if (pu == 'G') {
                for (int d = 0; d < 4; d++) {
                    for (int dist = 1; dist <= 2; dist++) {
                        int nr = r+ORTHO[d][0]*dist, nc = c+ORTHO[d][1]*dist;
                        if (!in_bounds(nr,nc)) break;
                        char t = board[nr][nc];
                        if (is_enemy(t, is_w)) { moves[cnt++] = {r,c,nr,nc,0}; break; }
                        if (t != '.') break;
                    }
                }
            } else if (pu == 'T') {
                for (int d = 0; d < 4; d++) {
                    for (int dist = 1; dist <= 2; dist++) {
                        int nr = r+DIAG4[d][0]*dist, nc = c+DIAG4[d][1]*dist;
                        if (!in_bounds(nr,nc)) break;
                        char t = board[nr][nc];
                        if (is_enemy(t, is_w)) { moves[cnt++] = {r,c,nr,nc,0}; break; }
                        if (t != '.') break;
                    }
                }
            } else if (pu == 'S') {
                for (int fw = 1; fw <= 3; fw++) {
                    int nr = r + fw*fwd;
                    if (!in_bounds(nr,0)) break;
                    if (is_enemy(board[nr][c], is_w))
                        moves[cnt++] = {r,c,nr,c,0};
                    for (int side = -1; side <= 1; side += 2) {
                        int nc = c+side;
                        if (in_bounds(nr,nc) && is_enemy(board[nr][nc], is_w))
                            moves[cnt++] = {r,c,nr,nc,0};
                    }
                }
            } else if (pu == 'N') {
                for (int d = 0; d < 8; d++) {
                    int nr = r+DIRS8[d][0], nc = c+DIRS8[d][1];
                    if (!in_bounds(nr,nc) || !is_enemy(board[nr][nc], is_w)) continue;
                    bool ok = false;
                    for (int d2 = 0; d2 < 8 && !ok; d2++) {
                        int ar = nr+DIRS8[d2][0], ac = nc+DIRS8[d2][1];
                        if (in_bounds(ar,ac) && !(ar==r && ac==c) && is_friendly(board[ar][ac], is_w))
                            ok = true;
                    }
                    if (ok) moves[cnt++] = {r,c,nr,nc,0};
                }
            }
        }
    }
    return cnt;
}

bool is_attacked_by(int r, int c, bool by_white) {
    int fwd = by_white ? -1 : 1;

    // Baby
    int br = r - fwd;
    if (in_bounds(br, c)) {
        char p = board[br][c];
        if (is_friendly(p, by_white) && to_upper(p) == 'B') return true;
        int br2 = r - 2*fwd;
        if (in_bounds(br2, c) && board[br][c] == '.') {
            char p2 = board[br2][c];
            if (is_friendly(p2, by_white) && to_upper(p2) == 'B') return true;
        }
    }
    // Prince
    for (int d = 0; d < 8; d++) {
        int nr = r+DIRS8[d][0], nc = c+DIRS8[d][1];
        if (in_bounds(nr,nc)) {
            char p = board[nr][nc];
            if (is_friendly(p, by_white) && to_upper(p) == 'P') return true;
        }
    }
    // Princess
    for (int d = 0; d < 8; d++) {
        for (int dist = 1; dist <= 3; dist++) {
            int nr = r+DIRS8[d][0]*dist, nc = c+DIRS8[d][1]*dist;
            if (!in_bounds(nr,nc)) break;
            char p = board[nr][nc];
            if (p == '.') continue;
            if (is_friendly(p, by_white) && to_upper(p) == 'X') return true;
            break;
        }
    }
    // Pony
    for (int d = 0; d < 4; d++) {
        int nr = r+DIAG4[d][0], nc = c+DIAG4[d][1];
        if (in_bounds(nr,nc)) {
            char p = board[nr][nc];
            if (is_friendly(p, by_white) && to_upper(p) == 'Y') return true;
        }
    }
    // Guard
    for (int d = 0; d < 4; d++) {
        for (int dist = 1; dist <= 2; dist++) {
            int nr = r+ORTHO[d][0]*dist, nc = c+ORTHO[d][1]*dist;
            if (!in_bounds(nr,nc)) break;
            char p = board[nr][nc];
            if (p == '.') continue;
            if (is_friendly(p, by_white) && to_upper(p) == 'G') return true;
            break;
        }
    }
    // Tutor
    for (int d = 0; d < 4; d++) {
        for (int dist = 1; dist <= 2; dist++) {
            int nr = r+DIAG4[d][0]*dist, nc = c+DIAG4[d][1]*dist;
            if (!in_bounds(nr,nc)) break;
            char p = board[nr][nc];
            if (p == '.') continue;
            if (is_friendly(p, by_white) && to_upper(p) == 'T') return true;
            break;
        }
    }
    // Scout
    int afwd = fwd;
    for (int fw = 1; fw <= 3; fw++) {
        int sr = r - fw*afwd;
        if (!in_bounds(sr, 0)) break;
        char p = board[sr][c];
        if (is_friendly(p, by_white) && to_upper(p) == 'S') return true;
        for (int side = -1; side <= 1; side += 2) {
            int sc = c - side;
            if (in_bounds(sr, sc)) {
                p = board[sr][sc];
                if (is_friendly(p, by_white) && to_upper(p) == 'S') return true;
            }
        }
    }
    // Sibling
    for (int d = 0; d < 8; d++) {
        int nr = r+DIRS8[d][0], nc = c+DIRS8[d][1];
        if (in_bounds(nr,nc)) {
            char p = board[nr][nc];
            if (is_friendly(p, by_white) && to_upper(p) == 'N') return true;
        }
    }
    return false;
}

int evaluate(bool is_w) {
    int ws = 0, bs = 0;
    int wp_r = -1, wp_c = -1, bp_r = -1, bp_c = -1;

    for (int r = 0; r < 12; r++) {
        for (int c = 0; c < 12; c++) {
            char p = board[r][c];
            if (p == '.') continue;
            if (is_white_piece(p)) {
                ws += piece_val(p) + CB[r][c] * 3;
                if (p == 'B') ws += BAW[r];
                else if (p == 'P') { wp_r = r; wp_c = c; }
            } else {
                bs += piece_val(p) + CB[r][c] * 3;
                if (p == 'b') bs += BAB[r];
                else if (p == 'p') { bp_r = r; bp_c = c; }
            }
        }
    }

    if (wp_r >= 0 && is_attacked_by(wp_r, wp_c, false)) ws -= 500;
    if (bp_r >= 0 && is_attacked_by(bp_r, bp_c, true)) bs -= 500;

    if (wp_r >= 0) {
        for (int d = 0; d < 8; d++) {
            int nr = wp_r+DIRS8[d][0], nc = wp_c+DIRS8[d][1];
            if (in_bounds(nr,nc) && is_white_piece(board[nr][nc])) ws += 15;
        }
    }
    if (bp_r >= 0) {
        for (int d = 0; d < 8; d++) {
            int nr = bp_r+DIRS8[d][0], nc = bp_c+DIRS8[d][1];
            if (in_bounds(nr,nc) && is_black_piece(board[nr][nc])) bs += 15;
        }
    }

    int score = ws - bs;
    return is_w ? score : -score;
}

int quiesce(int alpha, int beta, bool is_w, bool maximizing) {
    node_count++;
    check_time();

    int stand_pat = evaluate(is_w);

    if (maximizing) {
        if (stand_pat >= beta) return beta;
        if (stand_pat > alpha) alpha = stand_pat;
    } else {
        if (stand_pat <= alpha) return alpha;
        if (stand_pat < beta) beta = stand_pat;
    }

    bool current_w = maximizing ? is_w : !is_w;
    Move caps[256];
    int ncaps = gen_captures(caps, current_w);
    if (ncaps == 0) return stand_pat;

    // Sort by MVV-LVA
    for (int i = 0; i < ncaps; i++)
        caps[i].score = piece_val(board[caps[i].r2][caps[i].c2]) - piece_val(board[caps[i].r1][caps[i].c1]) / 10;
    sort(caps, caps + ncaps, [](const Move& a, const Move& b) { return a.score > b.score; });

    for (int i = 0; i < ncaps; i++) {
        Move& m = caps[i];
        char captured = board[m.r2][m.c2];

        if (to_upper(captured) == 'P')
            return maximizing ? 200000 : -200000;

        // Delta pruning
        if (maximizing && stand_pat + piece_val(captured) + 200 < alpha) continue;
        if (!maximizing && stand_pat - piece_val(captured) - 200 > beta) continue;

        char piece = board[m.r1][m.c1];
        board[m.r2][m.c2] = piece;
        board[m.r1][m.c1] = '.';

        int score = quiesce(alpha, beta, is_w, !maximizing);

        board[m.r1][m.c1] = piece;
        board[m.r2][m.c2] = captured;

        if (maximizing) {
            if (score > alpha) alpha = score;
            if (alpha >= beta) return beta;
        } else {
            if (score < beta) beta = score;
            if (alpha >= beta) return alpha;
        }
    }
    return maximizing ? alpha : beta;
}

struct ABResult { int score; Move move; bool has_move; };

ABResult alphabeta(int depth, int alpha, int beta, bool maximizing, bool is_w, int ply) {
    node_count++;
    check_time();

    if (depth <= 0) {
        int s = quiesce(alpha, beta, is_w, maximizing);
        return {s, {}, false};
    }

    bool current_w = maximizing ? is_w : !is_w;
    Move moves[512];
    int nmoves = gen_moves(moves, current_w);

    if (nmoves == 0)
        return {evaluate(is_w), {}, false};

    // Score moves for sorting
    Move prince_cap = {};
    bool found_prince = false;

    for (int i = 0; i < nmoves; i++) {
        char t = board[moves[i].r2][moves[i].c2];
        if (is_enemy(t, current_w)) {
            if (to_upper(t) == 'P') {
                prince_cap = moves[i];
                found_prince = true;
                break;
            }
            moves[i].score = 100000 + piece_val(t) - piece_val(board[moves[i].r1][moves[i].c1]) / 10;
        } else {
            // Check killer
            bool is_k = false;
            if (ply < 64) {
                if (has_killer1[ply] && moves[i].r1 == killer1[ply].r1 && moves[i].c1 == killer1[ply].c1 &&
                    moves[i].r2 == killer1[ply].r2 && moves[i].c2 == killer1[ply].c2) is_k = true;
                if (has_killer2[ply] && moves[i].r1 == killer2[ply].r1 && moves[i].c1 == killer2[ply].c1 &&
                    moves[i].r2 == killer2[ply].r2 && moves[i].c2 == killer2[ply].c2) is_k = true;
            }
            moves[i].score = is_k ? 50000 : CB[moves[i].r2][moves[i].c2];
        }
    }

    if (found_prince) {
        if (maximizing) return {200000 - ply, prince_cap, true};
        else return {-200000 + ply, prince_cap, true};
    }

    sort(moves, moves + nmoves, [](const Move& a, const Move& b) { return a.score > b.score; });

    Move best_move = moves[0];

    if (maximizing) {
        int max_eval = -999999;
        for (int i = 0; i < nmoves; i++) {
            Move& m = moves[i];
            char captured = board[m.r2][m.c2];
            char piece = board[m.r1][m.c1];
            board[m.r2][m.c2] = piece;
            board[m.r1][m.c1] = '.';

            ABResult res = alphabeta(depth - 1, alpha, beta, false, is_w, ply + 1);

            board[m.r1][m.c1] = piece;
            board[m.r2][m.c2] = captured;

            if (res.score > max_eval) {
                max_eval = res.score;
                best_move = m;
            }
            if (res.score > alpha) alpha = res.score;
            if (alpha >= beta) {
                if (captured == '.' && ply < 64) {
                    if (!has_killer1[ply] || !(m.r1==killer1[ply].r1 && m.c1==killer1[ply].c1 &&
                        m.r2==killer1[ply].r2 && m.c2==killer1[ply].c2)) {
                        killer2[ply] = killer1[ply];
                        has_killer2[ply] = has_killer1[ply];
                        killer1[ply] = m;
                        has_killer1[ply] = true;
                    }
                }
                break;
            }
        }
        return {max_eval, best_move, true};
    } else {
        int min_eval = 999999;
        for (int i = 0; i < nmoves; i++) {
            Move& m = moves[i];
            char captured = board[m.r2][m.c2];
            char piece = board[m.r1][m.c1];
            board[m.r2][m.c2] = piece;
            board[m.r1][m.c1] = '.';

            ABResult res = alphabeta(depth - 1, alpha, beta, true, is_w, ply + 1);

            board[m.r1][m.c1] = piece;
            board[m.r2][m.c2] = captured;

            if (res.score < min_eval) {
                min_eval = res.score;
                best_move = m;
            }
            if (res.score < beta) beta = res.score;
            if (alpha >= beta) {
                if (captured == '.' && ply < 64) {
                    if (!has_killer1[ply] || !(m.r1==killer1[ply].r1 && m.c1==killer1[ply].c1 &&
                        m.r2==killer1[ply].r2 && m.c2==killer1[ply].c2)) {
                        killer2[ply] = killer1[ply];
                        has_killer2[ply] = has_killer1[ply];
                        killer1[ply] = m;
                        has_killer1[ply] = true;
                    }
                }
                break;
            }
        }
        return {min_eval, best_move, true};
    }
}

Move solve() {
    bool is_w = is_white_player;

    // Time budget
    int pc = 0;
    for (int r = 0; r < 12; r++)
        for (int c = 0; c < 12; c++)
            if (board[r][c] != '.') pc++;

    int est_moves = pc > 30 ? pc : 30;
    double budget = my_time_left / est_moves;
    if (budget > 8.0) budget = 8.0;
    if (budget > my_time_left - 1.0) budget = my_time_left - 1.0;
    if (budget < 0.05) budget = 0.05;
    if (my_time_left < 5.0) {
        budget = my_time_left * 0.1;
        if (budget < 0.01) budget = 0.01;
        if (budget > 0.3) budget = 0.3;
    }

    start_time = chrono::steady_clock::now();
    deadline_sec = budget;

    Move moves[512];
    int nmoves = gen_moves(moves, is_w);
    if (nmoves == 0) return {0,0,0,0,0};

    // Instant prince capture
    for (int i = 0; i < nmoves; i++) {
        char t = board[moves[i].r2][moves[i].c2];
        if (to_upper(t) == 'P' && is_enemy(t, is_w))
            return moves[i];
    }

    Move best_move = moves[0];
    memset(has_killer1, 0, sizeof(has_killer1));
    memset(has_killer2, 0, sizeof(has_killer2));

    for (int depth = 1; depth < 50; depth++) {
        node_count = 0;
        try {
            ABResult res = alphabeta(depth, -999999, 999999, true, is_w, 0);
            if (res.has_move)
                best_move = res.move;
            if (res.score >= 190000)
                break;
        } catch (TimeUp&) {
            break;
        }

        if (elapsed_sec() > budget * 0.5)
            break;
    }

    return best_move;
}

int main() {
    init_tables();
    parse_input();
    Move best = solve();
    write_output(best);
    return 0;
}
