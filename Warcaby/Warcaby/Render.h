#pragma once
#include <windows.h>
#include <vector>
#include "Logika.h"

class GameRenderer {
public:
    static const int CELL_SIZE = 60;
    static const int SIDEBAR_WIDTH = 160;
    static const int BOARD_PIXELS = 8 * CELL_SIZE;   // 480

    RECT Get2PBtnRect();
    RECT GetAIBtnRect();
    RECT GetRankingMenuBtnRect();
    RECT GetSaveScoreBtnRect();
    RECT GetMenuBtnRect();
    RECT GetConfirmNameBtnRect();
    RECT GetSkipNameBtnRect();
    RECT GetBackBtnRect();

    void Draw(HDC hdc,
        const CheckersGame& game,
        int                              selX,
        int                              selY,
        const std::vector<PlayerRecord>& ranking,
        const std::vector<GameHistoryEntry>& history,
        const std::wstring& nameInput,
        const GameStats& stats);

private:
    void DrawMenu(HDC hdc);
    void DrawPiece(HDC hdc, Piece p, int pixelX, int pixelY);
    void DrawSidebar(HDC hdc, const CheckersGame& game);
    void DrawWinScreen(HDC hdc, const CheckersGame& game);
    void DrawNameEntry(HDC hdc, int winner, const std::wstring& nameInput);
    void DrawRanking(HDC hdc,
        const std::vector<PlayerRecord>& ranking,
        const std::vector<GameHistoryEntry>& history,
        const GameStats& stats);

    void DrawButton(HDC hdc, RECT r, COLORREF bg, const wchar_t* txt, int len);
};