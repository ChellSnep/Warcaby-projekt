#pragma once
#include <windows.h>
#include <vector>
#include "Logika.h"

class GameRenderer {
public:
    static const int CELL_SIZE     = 60;
    static const int SIDEBAR_WIDTH = 160;
    static const int BOARD_PIXELS  = 8 * CELL_SIZE;   // 480

    // ---- Prostokaty przyciskow ----

    // Menu glowne
    RECT Get2PBtnRect();
    RECT GetAIBtnRect();
    RECT GetRankingMenuBtnRect();

    // Ekran konca gry
    RECT GetSaveScoreBtnRect();   // [ZAPISZ WYNIK]
    RECT GetMenuBtnRect();         // [MENU]

    // Wprowadzanie imienia
    RECT GetConfirmNameBtnRect(); // [ZAPISZ]
    RECT GetSkipNameBtnRect();    // [POMIN]

    // Ekran rankingu
    RECT GetBackBtnRect();        // [POWROT]

    // ---- Glowna funkcja rysowania ----
    void Draw(HDC hdc,
              const CheckersGame&              game,
              int                              selX,
              int                              selY,
              const std::vector<PlayerRecord>& ranking,
              const std::vector<GameHistoryEntry>& history,
              const std::wstring&              nameInput);

private:
    void DrawMenu(HDC hdc);
    void DrawPiece(HDC hdc, Piece p, int pixelX, int pixelY);
    void DrawSidebar(HDC hdc, const CheckersGame& game);
    void DrawWinScreen(HDC hdc, const CheckersGame& game);
    void DrawNameEntry(HDC hdc, int winner, const std::wstring& nameInput);
    void DrawRanking(HDC hdc,
                     const std::vector<PlayerRecord>& ranking,
                     const std::vector<GameHistoryEntry>& history);

    // Pomocnik - przycisk z tekstem
    void DrawButton(HDC hdc, RECT r, COLORREF bg, const wchar_t* txt, int len);
};