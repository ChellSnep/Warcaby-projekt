#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:wWinMainCRTStartup")
#include "Logika.h"
#include "Render.h"
#include "DatabaseManager.h"
#include <vector>
#include <algorithm>
#include <string>

// Globalne obiekty gry
CheckersGame g_game;
GameRenderer g_renderer;
DatabaseManager g_dbManager("checkers_data.db");

int g_selX = -1, g_selY = -1;

std::vector<PlayerRecord> g_ranking;
std::vector<GameHistoryEntry> g_history;

// Stan pomocniczy dla wpisywania imienia
std::wstring g_nameInput = L"";
bool         g_lastWasAI = false;

// --- ZAPIS WYNIKU DO NOWEJ KLASY DB ---

static void SaveResult(const std::wstring& name,
    int totalMoves, bool vsAI,
    int redCap, int blackCap, int winner) {
    if (name.empty()) return;

    bool isWin = !(vsAI && winner == BLACK);
    std::wstring historyWinnerName = isWin ? name : L"AI";

    bool found = false;
    for (auto& rec : g_ranking) {
        if (rec.name == name) {
            if (isWin) rec.wins++;
            else       rec.losses++;
            found = true;
            break;
        }
    }
    if (!found) {
        g_ranking.push_back({ name, isWin ? 1 : 0, isWin ? 0 : 1 });
    }

    std::sort(g_ranking.begin(), g_ranking.end(),
        [](const PlayerRecord& a, const PlayerRecord& b) {
            return a.wins > b.wins;
        });

    // Zapis z uzyciem Database Managera
    g_dbManager.SaveRanking(g_ranking);

    GameHistoryEntry newEntry = { historyWinnerName, totalMoves, vsAI, redCap, blackCap };
    g_history.push_back(newEntry);

    if (g_history.size() > 50) g_history.erase(g_history.begin());

    // Zapis pojedynczego wpisu do historii oraz update statystyk
    g_dbManager.AddHistoryEntry(newEntry);
    g_dbManager.UpdatePlayerStats(name, totalMoves);
}

// -------------------------------------------------------------

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {

    case WM_CREATE:
        SetTimer(hwnd, 1, 16, NULL);
        return 0;

    case WM_SIZE:
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_TIMER:
        if (g_game.m_anim.active) {
            g_game.UpdateAnimation(0.08f);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        else if ((g_game.m_state == PLAYING_AI) &&
            g_game.GetTurn() == BLACK &&
            !g_game.IsGameOver()) {
            g_game.ExecuteBotMove();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_LBUTTONDOWN: {
        int rawX = LOWORD(lParam), rawY = HIWORD(lParam);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int clientWidth = clientRect.right - clientRect.left;
        int clientHeight = clientRect.bottom - clientRect.top;

        if (clientWidth == 0 || clientHeight == 0) return 0;

        int mx = rawX * 640 / clientWidth;
        int my = rawY * 480 / clientHeight;
        POINT pt = { mx, my };

        if (g_game.m_anim.active) return 0;

        switch (g_game.m_state) {

        case MENU: {
            RECT r2p = g_renderer.Get2PBtnRect();
            RECT rai = g_renderer.GetAIBtnRect();
            RECT rank = g_renderer.GetRankingMenuBtnRect();

            if (PtInRect(&r2p, pt)) {
                g_game.m_state = PLAYING_2P;
                g_game.Initialize();
                g_lastWasAI = false;
                g_selX = g_selY = -1;
            }
            else if (PtInRect(&rai, pt)) {
                g_game.m_state = PLAYING_AI;
                g_game.Initialize();
                g_lastWasAI = true;
                g_selX = g_selY = -1;
            }
            else if (PtInRect(&rank, pt)) {
                g_game.m_state = RANKING;
            }
            break;
        }

        case RANKING: {
            RECT rb = g_renderer.GetBackBtnRect();
            if (PtInRect(&rb, pt)) g_game.m_state = MENU;
            break;
        }

        case ENTER_NAME: {
            RECT rConfirm = g_renderer.GetConfirmNameBtnRect();
            RECT rSkip = g_renderer.GetSkipNameBtnRect();

            if (PtInRect(&rConfirm, pt)) {
                SaveResult(g_nameInput,
                    g_game.GetTotalMoves(), g_lastWasAI,
                    g_game.GetRedCaptures(), g_game.GetBlackCaptures(), g_game.GetWinner());
                g_nameInput.clear();
                g_game.m_state = MENU;
            }
            else if (PtInRect(&rSkip, pt)) {
                g_nameInput.clear();
                g_game.m_state = MENU;
            }
            break;
        }

        case PLAYING_2P:
        case PLAYING_AI: {
            if (g_game.IsGameOver()) {
                RECT rSave = g_renderer.GetSaveScoreBtnRect();
                RECT rMenu = g_renderer.GetMenuBtnRect();

                if (PtInRect(&rSave, pt)) {
                    g_nameInput.clear();
                    g_game.m_state = ENTER_NAME;
                }
                else if (PtInRect(&rMenu, pt)) {
                    g_game.m_state = MENU;
                }
            }
            else {
                int x = mx / GameRenderer::CELL_SIZE;
                int y = my / GameRenderer::CELL_SIZE;

                if (x < 8 && y < 8) {
                    if (g_selX == -1) {
                        Point mj = g_game.GetMultiJumpPiece();
                        if (mj.x != -1) {
                            if (x == mj.x && y == mj.y) {
                                g_selX = x; g_selY = y;
                            }
                        }
                        else if (g_game.GetPiece(x, y) != EMPTY &&
                            (g_game.GetPiece(x, y) % 2 == g_game.GetTurn() % 2)) {
                            g_selX = x; g_selY = y;
                        }
                    }
                    else {
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

    case WM_CHAR:
        if (g_game.m_state == ENTER_NAME) {
            if (wParam == VK_BACK) {
                if (!g_nameInput.empty()) g_nameInput.pop_back();
            }
            else if (wParam == VK_RETURN) {
                SaveResult(g_nameInput,
                    g_game.GetTotalMoves(), g_lastWasAI,
                    g_game.GetRedCaptures(), g_game.GetBlackCaptures(), g_game.GetWinner());
                g_nameInput.clear();
                g_game.m_state = MENU;
            }
            else if (wParam == 27) {
                g_nameInput.clear();
                g_game.m_state = MENU;
            }
            else if (g_nameInput.size() < 20 && wParam >= 32) {
                g_nameInput += (wchar_t)wParam;
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int clientWidth = clientRect.right - clientRect.left;
        int clientHeight = clientRect.bottom - clientRect.top;

        HDC     mDC = CreateCompatibleDC(hdc);

        HBITMAP mBM = CreateCompatibleBitmap(hdc, 640, 480);
        HGDIOBJ oldBM = SelectObject(mDC, mBM);

        GameStats globalStats = g_dbManager.GetGlobalStatistics();

        g_renderer.Draw(mDC, g_game,
            g_selX, g_selY,
            g_ranking, g_history, g_nameInput, globalStats);

        SetStretchBltMode(hdc, HALFTONE);
        SetBrushOrgEx(hdc, 0, 0, NULL);

        StretchBlt(hdc, 0, 0, clientWidth, clientHeight, mDC, 0, 0, 640, 480, SRCCOPY);

        SelectObject(mDC, oldBM);
        DeleteObject(mBM);
        DeleteDC(mDC);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        g_dbManager.ClearAllTables();

        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// -----------------------------------------------------

int WINAPI wWinMain(HINSTANCE h, HINSTANCE, PWSTR, int n) {
    // Inicjalizacja bazy i wczytanie danych
    g_dbManager.Initialize();
    g_ranking = g_dbManager.LoadRanking();
    g_history = g_dbManager.LoadHistory();

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = h;
    wc.lpszClassName = L"Checkers";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
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