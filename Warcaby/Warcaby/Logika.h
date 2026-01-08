#pragma once
#include <vector>

enum Piece { EMPTY = 0, RED = 1, BLACK = 2, RED_KING = 3, BLACK_KING = 4 };
enum GameState { MENU, PLAYING_2P, PLAYING_AI };

struct Point { int x, y; };
struct Move { int x1, y1, x2, y2; bool isJump; };

struct AnimationData {
    bool active = false;
    int x1, y1, x2, y2;
    float progress = 0.0f;
    Piece piece = EMPTY;
    bool isCapture = false;
};

class CheckersGame {
public:
    GameState m_state = MENU;
    static const int BOARD_SIZE = 8;
    AnimationData m_anim;

    CheckersGame();
    void Initialize();
    void ExecuteBotMove();
    bool MovePiece(int x1, int y1, int x2, int y2);
    void UpdateAnimation(float step);

    Piece GetPiece(int x, int y) const;
    int GetTurn() const;
    bool IsGameOver() const;
    int GetWinner() const;
    Point GetMultiJumpPiece() const;

private:
    Piece m_board[BOARD_SIZE][BOARD_SIZE];
    int m_turn;
    bool m_gameOver;
    int m_winner;
    Point m_multiJumpPiece;

    // Metody walidacji
    bool IsPlayerPiece(int x, int y, int player) const;
    bool IsValidSlide(int x1, int y1, int x2, int y2) const;
    bool IsValidJump(int x1, int y1, int x2, int y2) const;
    bool CanPieceJump(int x, int y) const;
    bool CanPieceSlide(int x, int y) const;
    bool CanPlayerMove(int player) const;
    bool IsJumpAvailable() const;

    void ExecuteMove(int x1, int y1, int x2, int y2, bool isCapture);
    void EndTurn();
    void CheckWinCondition();

    // AI
    std::vector<Move> GetAllValidMoves(int player) const;
    int Evaluate() const;
    int Minimax(int depth, bool isMaximizing, int alpha, int beta);
    Move GetBestMove(int depth);
};