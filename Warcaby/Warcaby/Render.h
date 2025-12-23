#pragma once
#include <windows.h>
#include "Logika.h"

class GameRenderer {
public:
    static const int CELL_SIZE = 60;
    static const int SIDEBAR_WIDTH = 160;
    static const int BOARD_PIXELS = 8 * CELL_SIZE;

    RECT Get2PBtnRect();
    RECT GetAIBtnRect();
    RECT GetRestartBtnRect();

    void Draw(HDC hdc, const CheckersGame& game, int selX, int selY);

private:
    void DrawMenu(HDC hdc);
    // DrawPiece teraz operuje na wspó³rzêdnych ekranu (pikselach)
    void DrawPiece(HDC hdc, Piece p, int pixelX, int pixelY);
    void DrawSidebar(HDC hdc, const CheckersGame& game);
    void DrawWinScreen(HDC hdc, int winner);
};