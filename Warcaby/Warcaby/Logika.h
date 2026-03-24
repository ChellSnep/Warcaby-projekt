#pragma once
#include <vector>
#include <string>

enum Piece      { EMPTY = 0, RED = 1, BLACK = 2, RED_KING = 3, BLACK_KING = 4 };
enum GameState  { MENU, PLAYING_2P, PLAYING_AI, ENTER_NAME, RANKING };

struct Point { int x, y; };
struct Move  { int x1, y1, x2, y2; bool isJump; };

struct AnimationData {
    bool  active   = false;
    int   x1, y1, x2, y2;
    float progress = 0.0f;
    Piece piece    = EMPTY;
    bool  isCapture = false;
};

// ---- Struktury bazy danych ----

struct PlayerRecord {
    std::wstring name;
    int wins   = 0;
    int losses = 0;
};

struct GameHistoryEntry {
    std::wstring winnerName;
    int  totalMoves;
    bool vsAI;
    int  redCaptures;
    int  blackCaptures;
};

// ---- Glowna klasa gry ----

class CheckersGame {
public:
    GameState     m_state = MENU;
    static const int BOARD_SIZE = 8;
    AnimationData m_anim;

    CheckersGame();
    void Initialize();
    void ExecuteBotMove();
    bool MovePiece(int x1, int y1, int x2, int y2);
    void UpdateAnimation(float step);

    Piece GetPiece(int x, int y)      const;
    int   GetTurn()                   const;
    bool  IsGameOver()                const;
    int   GetWinner()                 const;
    Point GetMultiJumpPiece()         const;
    int   GetTotalMoves()             const;
    int   GetRedCaptures()            const;
    int   GetBlackCaptures()          const;

private:
    Piece m_board[BOARD_SIZE][BOARD_SIZE];
    int   m_turn;
    bool  m_gameOver;
    int   m_winner;
    Point m_multiJumpPiece;

    // Statystyki partii
    int   m_totalMoves;
    int   m_redCaptures;
    int   m_blackCaptures;

    bool IsPlayerPiece(int x, int y, int player) const;
    bool IsValidSlide(int x1, int y1, int x2, int y2) const;
    bool IsValidJump(int x1, int y1, int x2, int y2)  const;
    bool CanPieceJump(int x, int y)  const;
    bool CanPieceSlide(int x, int y) const;
    bool CanPlayerMove(int player)   const;
    bool IsJumpAvailable()           const;

    void ExecuteMove(int x1, int y1, int x2, int y2, bool isCapture);
    void EndTurn();
    void CheckWinCondition();

    std::vector<Move> GetAllValidMoves(int player) const;
    int  Evaluate()   const;
    int  Minimax(int depth, bool isMaximizing, int alpha, int beta);
    Move GetBestMove(int depth);
};