#include "Render.h"
#include <string>

RECT GameRenderer::Get2PBtnRect() { return { BOARD_PIXELS / 2 - 80, 160, BOARD_PIXELS / 2 + 80, 210 }; }
RECT GameRenderer::GetAIBtnRect() { return { BOARD_PIXELS / 2 - 80, 230, BOARD_PIXELS / 2 + 80, 280 }; }
RECT GameRenderer::GetRestartBtnRect() { return { BOARD_PIXELS / 2 - 50, BOARD_PIXELS / 2 + 20, BOARD_PIXELS / 2 + 50, BOARD_PIXELS / 2 + 50 }; }

void GameRenderer::Draw(HDC hdc, const CheckersGame& game, int selX, int selY) {
    SetBkMode(hdc, TRANSPARENT);
    if (game.m_state == MENU) { DrawMenu(hdc); return; }

    // Rysowanie planszy i bierek statycznych
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            HBRUSH br = CreateSolidBrush((x + y) % 2 == 0 ? RGB(235, 235, 200) : RGB(100, 50, 0));
            RECT r = { x * CELL_SIZE, y * CELL_SIZE, (x + 1) * CELL_SIZE, (y + 1) * CELL_SIZE };
            FillRect(hdc, &r, br);
            DeleteObject(br);

            // Zaznaczenie wybranej bierki
            if (x == selX && y == selY) {
                HPEN pen = CreatePen(PS_SOLID, 4, RGB(255, 255, 0));
                HGDIOBJ oldP = SelectObject(hdc, pen);
                HGDIOBJ oldB = SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Rectangle(hdc, r.left + 2, r.top + 2, r.right - 2, r.bottom - 2);
                SelectObject(hdc, oldP); SelectObject(hdc, oldB); DeleteObject(pen);
            }
            DrawPiece(hdc, game.GetPiece(x, y), x * CELL_SIZE, y * CELL_SIZE);
        }
    }

    // Rysowanie bierki w locie
    if (game.m_anim.active) {
        int px = (int)(game.m_anim.x1 * CELL_SIZE + (game.m_anim.x2 - game.m_anim.x1) * CELL_SIZE * game.m_anim.progress);
        int py = (int)(game.m_anim.y1 * CELL_SIZE + (game.m_anim.y2 - game.m_anim.y1) * CELL_SIZE * game.m_anim.progress);
        DrawPiece(hdc, game.m_anim.piece, px, py);
    }

    DrawSidebar(hdc, game);
    if (game.IsGameOver()) DrawWinScreen(hdc, game.GetWinner());
}

void GameRenderer::DrawPiece(HDC hdc, Piece p, int px, int py) {
    if (p == EMPTY) return;
    COLORREF color = (p == RED || p == RED_KING) ? RGB(220, 20, 20) : RGB(30, 30, 30);
    HBRUSH br = CreateSolidBrush(color); HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HGDIOBJ oldB = SelectObject(hdc, br); HGDIOBJ oldP = SelectObject(hdc, pen);

    Ellipse(hdc, px + 8, py + 8, px + CELL_SIZE - 8, py + CELL_SIZE - 8);

    if (p == RED_KING || p == BLACK_KING) {
        SetTextColor(hdc, RGB(255, 215, 0));
        TextOut(hdc, px + 24, py + 20, L"K", 1);
    }

    SelectObject(hdc, oldB); SelectObject(hdc, oldP); DeleteObject(br); DeleteObject(pen);
}

void GameRenderer::DrawMenu(HDC hdc) { 
    RECT bg = { 0, 0, BOARD_PIXELS + SIDEBAR_WIDTH, BOARD_PIXELS }; 
    FillRect(hdc, &bg, (HBRUSH)GetStockObject(WHITE_BRUSH)); 
    SetTextColor(hdc, RGB(0, 0, 0)); 
    TextOut(hdc, BOARD_PIXELS / 2 - 50, 100, L"WARCABY", 7); 
    RECT r2p = Get2PBtnRect(); 
    HBRUSH b1 = CreateSolidBrush(RGB(200, 255, 200)); 
    FillRect(hdc, &r2p, b1); 
    FrameRect(hdc, &r2p, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
    TextOut(hdc, r2p.left + 45, r2p.top + 15, L"2 GRACZY", 8); 
    DeleteObject(b1); 
    RECT rai = GetAIBtnRect(); 
    HBRUSH b2 = CreateSolidBrush(RGB(200, 200, 255)); 
    FillRect(hdc, &rai, b2); 
    FrameRect(hdc, &rai, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
    TextOut(hdc, rai.left + 40, rai.top + 15, L"VS AI", 5); 
    DeleteObject(b2); 
}

void GameRenderer::DrawSidebar(HDC hdc, const CheckersGame& game) { 
    RECT sb = { BOARD_PIXELS, 0, BOARD_PIXELS + SIDEBAR_WIDTH, BOARD_PIXELS }; 
    FillRect(hdc, &sb, (HBRUSH)GetStockObject(LTGRAY_BRUSH)); 
    SetTextColor(hdc, RGB(0, 0, 0)); 
    TextOut(hdc, BOARD_PIXELS + 20, 50, L"TURA:", 5);
    COLORREF turnColor = (game.GetTurn() == RED) ? RGB(200, 0, 0) : RGB(0, 0, 0); 
    SetTextColor(hdc, turnColor); 
    TextOut(hdc, BOARD_PIXELS + 20, 80, game.GetTurn() == RED ? L"CZERWONE" : L"CZARNE", 8); 
    if (game.GetMultiJumpPiece().x != -1) { 
        SetTextColor(hdc, RGB(255, 0, 0)); 
        TextOut(hdc, BOARD_PIXELS + 15, 130, L"MUSISZ BIÆ!", 11); 
    } 
}

void GameRenderer::DrawWinScreen(HDC hdc, int winner) { 
    RECT ov = { 50, 100, BOARD_PIXELS - 50, BOARD_PIXELS - 100 }; 
    FillRect(hdc, &ov, (HBRUSH)GetStockObject(WHITE_BRUSH)); 
    FrameRect(hdc, &ov, (HBRUSH)GetStockObject(BLACK_BRUSH)); 
    SetTextColor(hdc, RGB(0, 0, 0)); 
    std::wstring txt = (winner == RED) ? L"CZERWONE WYGRYWAJ¥!" : L"CZARNE WYGRYWAJ¥!"; 
    TextOut(hdc, BOARD_PIXELS / 2 - 80, BOARD_PIXELS / 2 - 40, txt.c_str(), (int)txt.length()); 
    RECT btn = GetRestartBtnRect(); 
    HBRUSH b = CreateSolidBrush(RGB(0, 200, 0)); 
    FillRect(hdc, &btn, b); 
    DeleteObject(b); 
    SetTextColor(hdc, RGB(255, 255, 255)); 
    TextOut(hdc, btn.left + 15, btn.top + 7, L"MENU", 4); 
}