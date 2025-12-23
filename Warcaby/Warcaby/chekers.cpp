#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:wWinMainCRTStartup")
#include "Logika.h"
#include "Render.h"

CheckersGame g_game;
GameRenderer g_renderer;
int g_selX = -1, g_selY = -1;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        SetTimer(hwnd, 1, 16, NULL); 
        return 0;

    case WM_TIMER:
        // Aktualizacja stanu animacji lub wykonanie ruchu bota
        if (g_game.m_anim.active) {
            g_game.UpdateAnimation(0.08f); // Szybkoœæ animacji
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (g_game.m_state == PLAYING_AI && g_game.GetTurn() == BLACK && !g_game.IsGameOver()) {
            g_game.ExecuteBotMove();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_LBUTTONDOWN: {
        int mx = LOWORD(lParam), my = HIWORD(lParam); POINT pt = { mx, my };
        if (g_game.m_anim.active) return 0; // Blokada wejœcia podczas ruchu

        if (g_game.m_state == MENU) {
            RECT r2p = g_renderer.Get2PBtnRect(), rai = g_renderer.GetAIBtnRect();
            if (PtInRect(&r2p, pt)) { g_game.m_state = PLAYING_2P; g_game.Initialize(); }
            else if (PtInRect(&rai, pt)) { g_game.m_state = PLAYING_AI; g_game.Initialize(); }
        }
        else if (g_game.IsGameOver()) {
            RECT rb = g_renderer.GetRestartBtnRect();
            if (PtInRect(&rb, pt)) g_game.m_state = MENU;
        }
        else {
            int x = mx / GameRenderer::CELL_SIZE, y = my / GameRenderer::CELL_SIZE;
            if (x < 8 && y < 8) {
                if (g_selX == -1) {
                    Point mj = g_game.GetMultiJumpPiece();
                    if (mj.x != -1) {
                        if (x == mj.x && y == mj.y) { g_selX = x; g_selY = y; }
                    }
                    else if (g_game.GetPiece(x, y) != EMPTY && (g_game.GetPiece(x, y) % 2 == g_game.GetTurn() % 2)) {
                        g_selX = x; g_selY = y;
                    }
                }
                else {
                    if (g_game.MovePiece(g_selX, g_selY, x, y)) { g_selX = -1; g_selY = -1; }
                    else { g_selX = -1; g_selY = -1; }
                }
            }
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_PAINT: {
        // Double Buffering
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
        HDC mDC = CreateCompatibleDC(hdc);
        HBITMAP mBM = CreateCompatibleBitmap(hdc, 640, 480);
        SelectObject(mDC, mBM);

        g_renderer.Draw(mDC, g_game, g_selX, g_selY);

        BitBlt(hdc, 0, 0, 640, 480, mDC, 0, 0, SRCCOPY);
        DeleteObject(mBM); DeleteDC(mDC); EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE h, HINSTANCE, PWSTR, int n) {
    WNDCLASS wc = { 0 }; wc.lpfnWndProc = WindowProc; wc.hInstance = h; wc.lpszClassName = L"Checkers";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(0, L"Checkers", L"Warcaby AI", WS_OVERLAPPEDWINDOW, 100, 100, 660, 520, NULL, NULL, h, NULL);
    ShowWindow(hwnd, n); MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}