#include "Logika.h"
#include <cmath>
#include <algorithm>
#include <vector>

// --- KONSTRUKTOR I INICJALIZACJA ---

CheckersGame::CheckersGame() {
    Initialize();
}

void CheckersGame::Initialize() {
    m_gameOver = false;
    m_winner = EMPTY;
    m_turn = RED;
    m_multiJumpPiece = { -1, -1 };
    m_anim.active = false;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            m_board[y][x] = EMPTY;
            if ((x + y) % 2 != 0) {
                if (y < 3) {
                    m_board[y][x] = BLACK;
                }
                else if (y > 4) {
                    m_board[y][x] = RED;
                }
            }
        }
    }
}

// --- G£ÓWNA LOGIKA GRY ---

bool CheckersGame::MovePiece(int x1, int y1, int x2, int y2) {
    if (m_gameOver || m_anim.active) return false;

    if (m_multiJumpPiece.x != -1 && (x1 != m_multiJumpPiece.x || y1 != m_multiJumpPiece.y)) {
        return false;
    }

    bool isJump = (std::abs(x2 - x1) == 2);

    // Sprawdzenie przymusu bicia
    if (!isJump && IsJumpAvailable()) {
        return false;
    }

    // Walidacja ruchu przed rozpoczêciem animacji
    if (isJump ? IsValidJump(x1, y1, x2, y2) : IsValidSlide(x1, y1, x2, y2)) {
        m_anim = { true, x1, y1, x2, y2, 0.0f, m_board[y1][x1], isJump };
        m_board[y1][x1] = EMPTY;
        return true;
    }

    return false;
}

void CheckersGame::UpdateAnimation(float step) {
    if (!m_anim.active) return;

    m_anim.progress += step;

    if (m_anim.progress >= 1.0f) {
        ExecuteMove(m_anim.x1, m_anim.y1, m_anim.x2, m_anim.y2, m_anim.isCapture);
        m_anim.active = false;

        // Logika po zakoñczeniu ruchu (multi-jump)
        if (m_anim.isCapture && CanPieceJump(m_anim.x2, m_anim.y2)) {
            m_multiJumpPiece = { m_anim.x2, m_anim.y2 };
        }
        else {
            m_multiJumpPiece = { -1, -1 };
            EndTurn();
        }
    }
}

void CheckersGame::ExecuteMove(int x1, int y1, int x2, int y2, bool isCapture) {
    m_board[y2][x2] = m_anim.piece;

    if (isCapture) {
        m_board[(y1 + y2) / 2][(x1 + x2) / 2] = EMPTY;
    }

    // Promocja na damkê
    if (y2 == 0 && m_board[y2][x2] == RED) {
        m_board[y2][x2] = RED_KING;
    }
    if (y2 == 7 && m_board[y2][x2] == BLACK) {
        m_board[y2][x2] = BLACK_KING;
    }
}

void CheckersGame::EndTurn() {
    m_turn = (m_turn == RED) ? BLACK : RED;
    CheckWinCondition();
}

// --- WALIDACJA RUCHÓW ---

bool CheckersGame::IsValidSlide(int x1, int y1, int x2, int y2) const {
    if (x2 < 0 || x2 >= BOARD_SIZE || y2 < 0 || y2 >= BOARD_SIZE) return false;
    if (m_board[y2][x2] != EMPTY) return false;

    Piece p = m_board[y1][x1];
    if (p == EMPTY && m_anim.active) p = m_anim.piece;
    if (p == EMPTY) return false;

    int dx = x2 - x1;
    int dy = y2 - y1;

    if (std::abs(dx) != 1 || std::abs(dy) != 1) return false;

    // Zwyk³e pionki mog¹ poruszaæ siê tylko do przodu
    if (p == RED && dy != -1 && p != RED_KING) return false;
    if (p == BLACK && dy != 1 && p != BLACK_KING) return false;

    return true;
}

bool CheckersGame::IsValidJump(int x1, int y1, int x2, int y2) const {
    if (x2 < 0 || x2 >= BOARD_SIZE || y2 < 0 || y2 >= BOARD_SIZE) return false;
    if (m_board[y2][x2] != EMPTY) return false;

    Piece p = m_board[y1][x1];
    if (p == EMPTY && m_anim.active) p = m_anim.piece;
    if (p == EMPTY) return false;

    int dx = x2 - x1;
    int dy = y2 - y1;

    if (std::abs(dx) != 2 || std::abs(dy) != 2) return false;

    // Sprawdzenie kierunku dla zwyk³ych pionków
    if (p == RED && dy != -2 && p != RED_KING) return false;
    if (p == BLACK && dy != 2 && p != BLACK_KING) return false;

    int midX = (x1 + x2) / 2;
    int midY = (y1 + y2) / 2;
    Piece midP = m_board[midY][midX];

    return (midP != EMPTY && (midP % 2 != p % 2));
}

// --- FUNKCJE POMOCNICZE STANU GRY ---

bool CheckersGame::IsPlayerPiece(int x, int y, int player) const {
    Piece p = m_board[y][x];
    if (player == RED) return (p == RED || p == RED_KING);
    if (player == BLACK) return (p == BLACK || p == BLACK_KING);
    return false;
}

bool CheckersGame::CanPieceJump(int x, int y) const {
    int dirs[] = { -2, 2 };
    for (int dx : dirs) {
        for (int dy : dirs) {
            if (IsValidJump(x, y, x + dx, y + dy)) return true;
        }
    }
    return false;
}

bool CheckersGame::CanPieceSlide(int x, int y) const {
    int dirs[] = { -1, 1 };
    for (int dx : dirs) {
        for (int dy : dirs) {
            if (IsValidSlide(x, y, x + dx, y + dy)) return true;
        }
    }
    return false;
}

bool CheckersGame::CanPlayerMove(int p) const {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (IsPlayerPiece(x, y, p) && (CanPieceJump(x, y) || CanPieceSlide(x, y))) {
                return true;
            }
        }
    }
    return false;
}

bool CheckersGame::IsJumpAvailable() const {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (IsPlayerPiece(x, y, m_turn) && CanPieceJump(x, y)) return true;
        }
    }
    return false;
}

void CheckersGame::CheckWinCondition() {
    bool redExists = false, blackExists = false;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (IsPlayerPiece(x, y, RED)) redExists = true;
            if (IsPlayerPiece(x, y, BLACK)) blackExists = true;
        }
    }

    if (!redExists || (m_turn == RED && !CanPlayerMove(RED))) {
        m_gameOver = true;
        m_winner = BLACK;
    }
    else if (!blackExists || (m_turn == BLACK && !CanPlayerMove(BLACK))) {
        m_gameOver = true;
        m_winner = RED;
    }
}

// --- LOGIKA AI (MINIMAX) ---

void CheckersGame::ExecuteBotMove() {
    if (m_turn == BLACK && !m_gameOver && !m_anim.active) {
        std::vector<Move> m = GetAllValidMoves(BLACK);
        if (!m.empty()) {
            Move b = GetBestMove(4);
            MovePiece(b.x1, b.y1, b.x2, b.y2);
        }
    }
}

std::vector<Move> CheckersGame::GetAllValidMoves(int p) const {
    std::vector<Move> jumps, slides;

    // Przymus bicia przy multi-jump
    if (m_multiJumpPiece.x != -1) {
        for (int dx : { -2, 2 }) {
            for (int dy : { -2, 2 }) {
                if (IsValidJump(m_multiJumpPiece.x, m_multiJumpPiece.y, m_multiJumpPiece.x + dx, m_multiJumpPiece.y + dy)) {
                    jumps.push_back({ m_multiJumpPiece.x, m_multiJumpPiece.y, m_multiJumpPiece.x + dx, m_multiJumpPiece.y + dy, true });
                }
            }
        }
        return jumps;
    }

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (IsPlayerPiece(x, y, p)) {
                // Szukaj biæ
                for (int dx : { -2, 2 }) {
                    for (int dy : { -2, 2 }) {
                        if (IsValidJump(x, y, x + dx, y + dy)) {
                            jumps.push_back({ x, y, x + dx, y + dy, true });
                        }
                    }
                }
                // Szukaj zwyk³ych ruchów (tylko jeœli nie znaleziono ¿adnego bicia)
                if (jumps.empty()) {
                    for (int dx : { -1, 1 }) {
                        for (int dy : { -1, 1 }) {
                            if (IsValidSlide(x, y, x + dx, y + dy)) {
                                slides.push_back({ x, y, x + dx, y + dy, false });
                            }
                        }
                    }
                }
            }
        }
    }
    return jumps.empty() ? slides : jumps;
}

int CheckersGame::Evaluate() const {
    int score = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (m_board[y][x] == BLACK) score += 10;
            else if (m_board[y][x] == BLACK_KING) score += 30;
            else if (m_board[y][x] == RED) score -= 10;
            else if (m_board[y][x] == RED_KING) score -= 30;
        }
    }
    return score;
}

int CheckersGame::Minimax(int depth, bool isMaximizing, int alpha, int beta) {
    if (depth == 0 || m_gameOver) return Evaluate();

    std::vector<Move> moves = GetAllValidMoves(isMaximizing ? BLACK : RED);
    if (moves.empty()) return isMaximizing ? -1000 : 1000;

    if (isMaximizing) {
        int maxEval = -10000;
        for (auto& mv : moves) {
            CheckersGame temp = *this;
            temp.ExecuteMove(mv.x1, mv.y1, mv.x2, mv.y2, mv.isJump);
            maxEval = std::max(maxEval, temp.Minimax(depth - 1, false, alpha, beta));
            alpha = std::max(alpha, maxEval);
            if (beta <= alpha) break;
        }
        return maxEval;
    }
    else {
        int minEval = 10000;
        for (auto& mv : moves) {
            CheckersGame temp = *this;
            temp.ExecuteMove(mv.x1, mv.y1, mv.x2, mv.y2, mv.isJump);
            minEval = std::min(minEval, temp.Minimax(depth - 1, true, alpha, beta));
            beta = std::min(beta, minEval);
            if (beta <= alpha) break;
        }
        return minEval;
    }
}

Move CheckersGame::GetBestMove(int depth) {
    std::vector<Move> moves = GetAllValidMoves(BLACK);
    Move bestMove = moves[0];
    int maxEval = -10000;

    for (auto& mv : moves) {
        CheckersGame temp = *this;
        temp.ExecuteMove(mv.x1, mv.y1, mv.x2, mv.y2, mv.isJump);
        int eval = temp.Minimax(depth - 1, false, -10000, 10000);
        if (eval > maxEval) {
            maxEval = eval;
            bestMove = mv;
        }
    }
    return bestMove;
}

// --- GETTERY ---

Piece CheckersGame::GetPiece(int x, int y) const { return m_board[y][x]; }
int CheckersGame::GetTurn() const { return m_turn; }
bool CheckersGame::IsGameOver() const { return m_gameOver; }
int CheckersGame::GetWinner() const { return m_winner; }
Point CheckersGame::GetMultiJumpPiece() const { return m_multiJumpPiece; }