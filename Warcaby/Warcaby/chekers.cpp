#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:wWinMainCRTStartup")
#include "Logika.h"
#include "Render.h"
#include <vector>
#include <algorithm>
#include <string>

// ============================================================
//  GLOBALNE OBIEKTY GRY I RENDERERA
// ============================================================

CheckersGame g_game;
GameRenderer g_renderer;
int g_selX = -1, g_selY = -1;

// ============================================================
//  GLOBALNA "BAZA DANYCH" – RANKING I HISTORIA
//  (dane przykladowe ladowane przy starcie programu)
// ============================================================

std::vector<PlayerRecord> g_ranking = {
    { L"Kasparow",    8, 2 },
    { L"Anna K.",     5, 3 },
    { L"Piotr W.",    3, 5 },
    { L"Marek S.",    1, 4 },
};

std::vector<GameHistoryEntry> g_history = {
    { L"Kasparow", 24, false, 3, 5 },
    { L"AI",       18, true,  6, 2 },
    { L"Anna K.",  31, false, 4, 7 },
};

// ---- Stan pomocniczy dla ENTER_NAME ----
std::wstring g_nameInput  = L"";
bool         g_lastWasAI  = false;   // tryb ostatniej partii

// ============================================================
//  POMOCNIK – ZAPIS WYNIKU DO RANKINGU I HISTORII
// ============================================================

static void SaveResult(const std::wstring& name,
                        int totalMoves, bool vsAI,
                        int redCap, int blackCap) {
    if (name.empty()) return;

    // Szukaj istniejacego gracza (case-sensitive)
    bool found = false;
    for (auto& rec : g_ranking) {
        if (rec.name == name) {
            rec.wins++;
            found = true;
            break;
        }
    }
    if (!found) {
        g_ranking.push_back({ name, 1, 0 });
    }

    // Posortuj malejaco po wygranych
    std::sort(g_ranking.begin(), g_ranking.end(),
              [](const PlayerRecord& a, const PlayerRecord& b) {
                  return a.wins > b.wins;
              });

    // Dodaj wpis do historii
    g_history.push_back({ name, totalMoves, vsAI, redCap, blackCap });

    // Ogranicz history do 50 wpisow
    if (g_history.size() > 50)
        g_history.erase(g_history.begin());
}

// ============================================================
//  PROCEDURA OKNA
// ============================================================

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {

    // ---- Timer – animacje i ruch AI ----
    case WM_CREATE:
        SetTimer(hwnd, 1, 16, NULL);
        return 0;

    case WM_TIMER:
        if (g_game.m_anim.active) {
            g_game.UpdateAnimation(0.08f);
            InvalidateRect(hwnd, NULL, FALSE);
        } else if ((g_game.m_state == PLAYING_AI) &&
                   g_game.GetTurn() == BLACK &&
                   !g_game.IsGameOver()) {
            g_game.ExecuteBotMove();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    // ---- Klikniecia mysza ----
    case WM_LBUTTONDOWN: {
        int mx = LOWORD(lParam), my = HIWORD(lParam);
        POINT pt = { mx, my };

        // Blokada wejscia podczas animacji
        if (g_game.m_anim.active) return 0;

        switch (g_game.m_state) {

        // ---- MENU ----
        case MENU: {
            RECT r2p  = g_renderer.Get2PBtnRect();
            RECT rai  = g_renderer.GetAIBtnRect();
            RECT rank = g_renderer.GetRankingMenuBtnRect();

            if (PtInRect(&r2p, pt)) {
                g_game.m_state = PLAYING_2P;
                g_game.Initialize();
                g_lastWasAI = false;
                g_selX = g_selY = -1;
            } else if (PtInRect(&rai, pt)) {
                g_game.m_state = PLAYING_AI;
                g_game.Initialize();
                g_lastWasAI = true;
                g_selX = g_selY = -1;
            } else if (PtInRect(&rank, pt)) {
                g_game.m_state = RANKING;
            }
            break;
        }

        // ---- RANKING ----
        case RANKING: {
            RECT rb = g_renderer.GetBackBtnRect();
            if (PtInRect(&rb, pt)) g_game.m_state = MENU;
            break;
        }

        // ---- ENTER_NAME – wprowadzanie imienia ----
        case ENTER_NAME: {
            RECT rConfirm = g_renderer.GetConfirmNameBtnRect();
            RECT rSkip    = g_renderer.GetSkipNameBtnRect();

            if (PtInRect(&rConfirm, pt)) {
                // Zapisz wynik i wróc do menu
                SaveResult(g_nameInput,
                           g_game.GetTotalMoves(), g_lastWasAI,
                           g_game.GetRedCaptures(), g_game.GetBlackCaptures());
                g_nameInput.clear();
                g_game.m_state = MENU;
            } else if (PtInRect(&rSkip, pt)) {
                // Anuluj bez zapisu
                g_nameInput.clear();
                g_game.m_state = MENU;
            }
            break;
        }

        // ---- PLAYING – rozgrywka ----
        case PLAYING_2P:
        case PLAYING_AI: {
            if (g_game.IsGameOver()) {
                // Przyciski na ekranie konca gry
                RECT rSave = g_renderer.GetSaveScoreBtnRect();
                RECT rMenu = g_renderer.GetMenuBtnRect();

                if (PtInRect(&rSave, pt)) {
                    // Przejdz do ekranu wpisywania imienia
                    g_nameInput.clear();
                    g_game.m_state = ENTER_NAME;
                } else if (PtInRect(&rMenu, pt)) {
                    g_game.m_state = MENU;
                }
            } else {
                // Normalna rozgrywka – zaznaczanie i ruch
                int x = mx / GameRenderer::CELL_SIZE;
                int y = my / GameRenderer::CELL_SIZE;

                if (x < 8 && y < 8) {
                    if (g_selX == -1) {
                        Point mj = g_game.GetMultiJumpPiece();
                        if (mj.x != -1) {
                            if (x == mj.x && y == mj.y) {
                                g_selX = x; g_selY = y;
                            }
                        } else if (g_game.GetPiece(x, y) != EMPTY &&
                                   (g_game.GetPiece(x, y) % 2 == g_game.GetTurn() % 2)) {
                            g_selX = x; g_selY = y;
                        }
                    } else {
                        g_game.MovePiece(g_selX, g_selY, x, y);
                        g_selX = g_selY = -1;
                    }
                }
            }
            break;
        }

        default: break;
        }

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    // ---- Klawiatura – wpisywanie imienia ----
    case WM_CHAR:
        if (g_game.m_state == ENTER_NAME) {
            if (wParam == VK_BACK) {
                if (!g_nameInput.empty()) g_nameInput.pop_back();
            } else if (wParam == VK_RETURN) {
                // Enter = zatwierdzenie (jak klikniecie ZAPISZ)
                SaveResult(g_nameInput,
                           g_game.GetTotalMoves(), g_lastWasAI,
                           g_game.GetRedCaptures(), g_game.GetBlackCaptures());
                g_nameInput.clear();
                g_game.m_state = MENU;
            } else if (wParam == 27) {
                // Escape = anulowanie
                g_nameInput.clear();
                g_game.m_state = MENU;
            } else if (g_nameInput.size() < 20 && wParam >= 32) {
                g_nameInput += (wchar_t)wParam;
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    // ---- Rysowanie (podwojne buforowanie) ----
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HDC     mDC = CreateCompatibleDC(hdc);
        HBITMAP mBM = CreateCompatibleBitmap(hdc, 640, 480);
        SelectObject(mDC, mBM);

        g_renderer.Draw(mDC, g_game,
                        g_selX, g_selY,
                        g_ranking, g_history, g_nameInput);

        BitBlt(hdc, 0, 0, 640, 480, mDC, 0, 0, SRCCOPY);
        DeleteObject(mBM);
        DeleteDC(mDC);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ============================================================
//  PUNKT WEJSCIA
// ============================================================

int WINAPI wWinMain(HINSTANCE h, HINSTANCE, PWSTR, int n) {
    WNDCLASS wc   = { 0 };
    wc.lpfnWndProc  = WindowProc;
    wc.hInstance    = h;
    wc.lpszClassName = L"Checkers";
    wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, L"Checkers", L"Warcaby AI",
                               WS_OVERLAPPEDWINDOW,
                               100, 100, 660, 520,
                               NULL, NULL, h, NULL);
    ShowWindow(hwnd, n);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}