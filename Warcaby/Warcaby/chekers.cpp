#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:wWinMainCRTStartup")
#include "Logika.h"
#include "Render.h"
#include <vector>
#include <algorithm>
#include <string>
#include "sqlite3.h"

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
//  ZAPIS I ODCZYT DO PLIKU (PERSYSTENCJA)
// ============================================================

void LoadRankingData() {
    sqlite3* db;
    // Otwiera plik bazy (lub tworzy ranking.db, jeśli nie istnieje)
    if (sqlite3_open("ranking.db", &db) != SQLITE_OK) return;

    // Tworzymy tabelę Ranking, jeśli to pierwsze uruchomienie
    const char* createTableSQL =
        "CREATE TABLE IF NOT EXISTS Ranking ("
        "Wins INTEGER, "
        "Losses INTEGER, "
        "Name TEXT);";
    sqlite3_exec(db, createTableSQL, nullptr, nullptr, nullptr);

    sqlite3_stmt* stmt;
    // Wyciągamy dane prosto z bazy, posortowane według wygranych (dla pewności)
    const char* query = "SELECT Wins, Losses, Name FROM Ranking ORDER BY Wins DESC LIMIT 4;";

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) == SQLITE_OK) {
        std::vector<PlayerRecord> loadedRanking;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int wins = sqlite3_column_int(stmt, 0);
            int losses = sqlite3_column_int(stmt, 1);
            // Wyciągamy tekst jako format 16-bitowy (zgodny z std::wstring w Win32)
            const void* text = sqlite3_column_text16(stmt, 2);

            std::wstring name = text ? static_cast<const wchar_t*>(text) : L"";
            if (!name.empty()) {
                loadedRanking.push_back({ name, wins, losses });
            }
        }

        // Jeśli coś wczytaliśmy, zastępujemy tym domyślny ranking
        if (!loadedRanking.empty()) {
            g_ranking = loadedRanking;
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void SaveRankingData() {
    sqlite3* db;
    if (sqlite3_open("ranking.db", &db) != SQLITE_OK) return;

    // Najprostsza logika aktualizacji dla małych baz: czyścimy tabelę i wpisujemy nową TOP 4
    sqlite3_exec(db, "DELETE FROM Ranking;", nullptr, nullptr, nullptr);

    sqlite3_stmt* stmt;
    const char* insertSQL = "INSERT INTO Ranking (Wins, Losses, Name) VALUES (?, ?, ?);";
    sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);

    // Zapisujemy tylko 4 najlepsze wyniki z bieżącej sesji
    int count = 0;
    for (const auto& rec : g_ranking) {
        sqlite3_bind_int(stmt, 1, rec.wins);
        sqlite3_bind_int(stmt, 2, rec.losses);
        // Bindujemy szerokie znaki (std::wstring)
        sqlite3_bind_text16(stmt, 3, rec.name.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_step(stmt);    // Wykonaj insert
        sqlite3_reset(stmt);   // Zresetuj zapytanie dla następnego gracza

        count++;
        if (count >= 4) break; // Po zapisaniu 4 wyników, ucinamy
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void LoadHistoryData() {
    sqlite3* db;
    // Tworzymy osobny plik na historię
    if (sqlite3_open("history.db", &db) != SQLITE_OK) return;

    // Tworzymy tabelę History
    const char* createTableSQL =
        "CREATE TABLE IF NOT EXISTS History ("
        "WinnerName TEXT, "
        "TotalMoves INTEGER, "
        "VsAI INTEGER, "
        "RedCaptures INTEGER, "
        "BlackCaptures INTEGER);";
    sqlite3_exec(db, createTableSQL, nullptr, nullptr, nullptr);

    sqlite3_stmt* stmt;
    const char* query = "SELECT WinnerName, TotalMoves, VsAI, RedCaptures, BlackCaptures FROM History;";

    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) == SQLITE_OK) {
        std::vector<GameHistoryEntry> loadedHistory;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const void* text = sqlite3_column_text16(stmt, 0);
            std::wstring name = text ? static_cast<const wchar_t*>(text) : L"";
            int moves = sqlite3_column_int(stmt, 1);
            bool vsAI = sqlite3_column_int(stmt, 2) != 0;
            int redCap = sqlite3_column_int(stmt, 3);
            int blackCap = sqlite3_column_int(stmt, 4);

            loadedHistory.push_back({ name, moves, vsAI, redCap, blackCap });
        }

        // Zastępujemy domyślną historię z kodu tylko wtedy, gdy baza nie była pusta
        if (!loadedHistory.empty()) {
            g_history = loadedHistory;
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void SaveHistoryData() {
    sqlite3* db;
    if (sqlite3_open("history.db", &db) != SQLITE_OK) return;

    // Czyścimy tabelę przed zapisaniem nowych danych
    sqlite3_exec(db, "DELETE FROM History;", nullptr, nullptr, nullptr);

    sqlite3_stmt* stmt;
    const char* insertSQL = "INSERT INTO History (WinnerName, TotalMoves, VsAI, RedCaptures, BlackCaptures) VALUES (?, ?, ?, ?, ?);";
    sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr);

    // Wyznaczamy indeks, od którego zaczynamy zapisywać, aby zawsze zapisać max 4 OSTATNIE wpisy
    size_t startIdx = (g_history.size() > 4) ? g_history.size() - 4 : 0;

    for (size_t i = startIdx; i < g_history.size(); ++i) {
        const auto& entry = g_history[i];

        sqlite3_bind_text16(stmt, 1, entry.winnerName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, entry.totalMoves);
        sqlite3_bind_int(stmt, 3, entry.vsAI ? 1 : 0);
        sqlite3_bind_int(stmt, 4, entry.redCaptures);
        sqlite3_bind_int(stmt, 5, entry.blackCaptures);

        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

static void SaveResult(const std::wstring& name,
    int totalMoves, bool vsAI,
    int redCap, int blackCap, int winner) {
    if (name.empty()) return;

    // Zakładamy, że człowiek gra Czerwonymi. Jeśli to tryb vs AI i wygrały Czarne, człowiek przegrał.
    bool isWin = !(vsAI && winner == BLACK);

    // Ustal imię zwycięzcy do historii gier
    std::wstring historyWinnerName = isWin ? name : L"AI";

    // Szukaj istniejacego gracza (case-sensitive)
    bool found = false;
    for (auto& rec : g_ranking) {
        if (rec.name == name) {
            if (isWin) rec.wins++;
            else       rec.losses++; // <--- Rejestracja porażki
            found = true;
            break;
        }
    }
    if (!found) {
        // Dodaj gracza z prawidłowym początkowym wynikiem
        g_ranking.push_back({ name, isWin ? 1 : 0, isWin ? 0 : 1 });
    }

    // Posortuj malejaco po wygranych
    std::sort(g_ranking.begin(), g_ranking.end(),
        [](const PlayerRecord& a, const PlayerRecord& b) {
            return a.wins > b.wins;
        });
    
    SaveRankingData();

    // Dodaj wpis do historii z prawidłową nazwą zwycięzcy
    g_history.push_back({ historyWinnerName, totalMoves, vsAI, redCap, blackCap });

    // Ogranicz history do 50 wpisow
    if (g_history.size() > 50)
        g_history.erase(g_history.begin());

    SaveHistoryData();
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
                    g_game.GetRedCaptures(), g_game.GetBlackCaptures(), g_game.GetWinner());
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
            }
            else if (wParam == VK_RETURN) {
                // Enter = zatwierdzenie (jak klikniecie ZAPISZ)
                SaveResult(g_nameInput,
                    g_game.GetTotalMoves(), g_lastWasAI,
                    g_game.GetRedCaptures(), g_game.GetBlackCaptures(), g_game.GetWinner());
                g_nameInput.clear();
                g_game.m_state = MENU;
            }
            else if (wParam == 27) {
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
    LoadRankingData();
    LoadHistoryData();
        
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