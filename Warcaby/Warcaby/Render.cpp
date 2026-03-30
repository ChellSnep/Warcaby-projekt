#include "Render.h"
#include <string>
#include <algorithm>

// ============================================================
//  PROSTOKATY PRZYCISKOW
// ============================================================

RECT GameRenderer::Get2PBtnRect() { return { BOARD_PIXELS / 2 - 90, 160, BOARD_PIXELS / 2 + 90, 205 }; }
RECT GameRenderer::GetAIBtnRect() { return { BOARD_PIXELS / 2 - 90, 220, BOARD_PIXELS / 2 + 90, 265 }; }
RECT GameRenderer::GetRankingMenuBtnRect() { return { BOARD_PIXELS / 2 - 90, 280, BOARD_PIXELS / 2 + 90, 325 }; }

RECT GameRenderer::GetSaveScoreBtnRect() { return { 70, 295, 240, 325 }; }
RECT GameRenderer::GetMenuBtnRect() { return { 260, 295, 400, 325 }; }

RECT GameRenderer::GetConfirmNameBtnRect() { return { 100, 270, 230, 300 }; }
RECT GameRenderer::GetSkipNameBtnRect() { return { 250, 270, 380, 300 }; }

RECT GameRenderer::GetBackBtnRect() { return { (BOARD_PIXELS + SIDEBAR_WIDTH) / 2 - 90, 445, (BOARD_PIXELS + SIDEBAR_WIDTH) / 2 + 90, 472 }; }

void GameRenderer::DrawButton(HDC hdc, RECT r, COLORREF bg,
    const wchar_t* txt, int len) {
    HBRUSH br = CreateSolidBrush(bg);
    FillRect(hdc, &r, br);
    DeleteObject(br);
    FrameRect(hdc, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, txt, len, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void GameRenderer::Draw(HDC hdc,
    const CheckersGame& game,
    int selX, int selY,
    const std::vector<PlayerRecord>& ranking,
    const std::vector<GameHistoryEntry>& history,
    const std::wstring& nameInput,
    const GameStats& stats)
{
    SetBkMode(hdc, TRANSPARENT);

    if (game.m_state == MENU) { DrawMenu(hdc); return; }
    if (game.m_state == RANKING) { DrawRanking(hdc, ranking, history, stats); return; }

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            COLORREF cellColor = ((x + y) % 2 == 0) ? RGB(235, 235, 200) : RGB(100, 50, 0);
            HBRUSH br = CreateSolidBrush(cellColor);
            RECT r = { x * CELL_SIZE, y * CELL_SIZE,
                      (x + 1) * CELL_SIZE, (y + 1) * CELL_SIZE };
            FillRect(hdc, &r, br);
            DeleteObject(br);

            if (x == selX && y == selY) {
                HPEN pen = CreatePen(PS_SOLID, 4, RGB(255, 255, 0));
                HGDIOBJ oldP = SelectObject(hdc, pen);
                HGDIOBJ oldB = SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Rectangle(hdc, r.left + 2, r.top + 2, r.right - 2, r.bottom - 2);
                SelectObject(hdc, oldP); SelectObject(hdc, oldB);
                DeleteObject(pen);
            }
            DrawPiece(hdc, game.GetPiece(x, y), x * CELL_SIZE, y * CELL_SIZE);
        }
    }

    if (game.m_anim.active) {
        int px = (int)(game.m_anim.x1 * CELL_SIZE +
            (game.m_anim.x2 - game.m_anim.x1) * CELL_SIZE * game.m_anim.progress);
        int py = (int)(game.m_anim.y1 * CELL_SIZE +
            (game.m_anim.y2 - game.m_anim.y1) * CELL_SIZE * game.m_anim.progress);
        DrawPiece(hdc, game.m_anim.piece, px, py);
    }

    DrawSidebar(hdc, game);

    if (game.m_state == ENTER_NAME) {
        DrawNameEntry(hdc, game.GetWinner(), nameInput);
    }
    else if (game.IsGameOver()) {
        DrawWinScreen(hdc, game);
    }
}

void GameRenderer::DrawPiece(HDC hdc, Piece p, int px, int py) {
    if (p == EMPTY) return;
    COLORREF color = (p == RED || p == RED_KING) ? RGB(220, 20, 20) : RGB(30, 30, 30);
    HBRUSH br = CreateSolidBrush(color);
    HPEN   pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HGDIOBJ oldB = SelectObject(hdc, br);
    HGDIOBJ oldP = SelectObject(hdc, pen);
    Ellipse(hdc, px + 8, py + 8, px + CELL_SIZE - 8, py + CELL_SIZE - 8);
    if (p == RED_KING || p == BLACK_KING) {
        SetTextColor(hdc, RGB(255, 215, 0));
        TextOut(hdc, px + 24, py + 20, L"K", 1);
    }
    SelectObject(hdc, oldB); SelectObject(hdc, oldP);
    DeleteObject(br); DeleteObject(pen);
}

void GameRenderer::DrawMenu(HDC hdc) {
    RECT bg = { 0, 0, BOARD_PIXELS + SIDEBAR_WIDTH, BOARD_PIXELS };
    HBRUSH bgBr = CreateSolidBrush(RGB(245, 235, 210));
    FillRect(hdc, &bg, bgBr);
    DeleteObject(bgBr);

    HFONT bigFont = CreateFont(42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        EASTEUROPE_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, L"Arial");
    HGDIOBJ oldFont = SelectObject(hdc, bigFont);
    SetTextColor(hdc, RGB(80, 30, 0));
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, BOARD_PIXELS / 2 - 90, 70, L"WARCABY", 7);
    SelectObject(hdc, oldFont);
    DeleteObject(bigFont);

    SetTextColor(hdc, RGB(120, 60, 0));
    TextOut(hdc, BOARD_PIXELS / 2 - 75, 120, L"Wybierz tryb gry:", 17);

    DrawButton(hdc, Get2PBtnRect(), RGB(180, 255, 180), L"2 GRACZY", 8);
    DrawButton(hdc, GetAIBtnRect(), RGB(180, 200, 255), L"VS AI", 5);
    DrawButton(hdc, GetRankingMenuBtnRect(), RGB(255, 230, 150), L"RANKING", 7);

    SetTextColor(hdc, RGB(100, 100, 100));
    TextOut(hdc, BOARD_PIXELS / 2 - 110, 355, L"Baza danych jest aktualizowana na biezaco.", 42);
}

void GameRenderer::DrawSidebar(HDC hdc, const CheckersGame& game) {
    RECT sb = { BOARD_PIXELS, 0, BOARD_PIXELS + SIDEBAR_WIDTH, BOARD_PIXELS };
    FillRect(hdc, &sb, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    SetBkMode(hdc, TRANSPARENT);

    SetTextColor(hdc, RGB(0, 0, 0));
    TextOut(hdc, BOARD_PIXELS + 15, 20, L"TURA:", 5);

    COLORREF tc = (game.GetTurn() == RED) ? RGB(200, 0, 0) : RGB(0, 0, 0);
    SetTextColor(hdc, tc);
    TextOut(hdc, BOARD_PIXELS + 15, 45,
        game.GetTurn() == RED ? L"CZERWONE" : L"CZARNE", 8);

    SetTextColor(hdc, RGB(50, 50, 50));
    TextOut(hdc, BOARD_PIXELS + 15, 100, L"Ruchy:", 6);
    std::wstring moves = std::to_wstring(game.GetTotalMoves());
    TextOut(hdc, BOARD_PIXELS + 15, 120, moves.c_str(), (int)moves.size());

    TextOut(hdc, BOARD_PIXELS + 15, 155, L"Bicia:", 6);
    TextOut(hdc, BOARD_PIXELS + 15, 175, L"Czerw.:", 7);
    std::wstring rc = std::to_wstring(game.GetRedCaptures());
    TextOut(hdc, BOARD_PIXELS + 95, 175, rc.c_str(), (int)rc.size());
    TextOut(hdc, BOARD_PIXELS + 15, 195, L"Czarne:", 7);
    std::wstring bc = std::to_wstring(game.GetBlackCaptures());
    TextOut(hdc, BOARD_PIXELS + 95, 195, bc.c_str(), (int)bc.size());

    if (game.GetMultiJumpPiece().x != -1) {
        SetTextColor(hdc, RGB(200, 0, 0));
        TextOut(hdc, BOARD_PIXELS + 10, 240, L"MUSISZ BIC!", 11);
    }
}

void GameRenderer::DrawWinScreen(HDC hdc, const CheckersGame& game) {
    RECT ov = { 40, 90, BOARD_PIXELS - 40, 340 };
    HBRUSH ovBr = CreateSolidBrush(RGB(250, 250, 240));
    FillRect(hdc, &ov, ovBr);
    DeleteObject(ovBr);
    FrameRect(hdc, &ov, (HBRUSH)GetStockObject(BLACK_BRUSH));

    SetBkMode(hdc, TRANSPARENT);
    int winner = game.GetWinner();
    COLORREF wc = (winner == RED) ? RGB(200, 0, 0) : RGB(30, 30, 30);
    SetTextColor(hdc, wc);
    std::wstring txt = (winner == RED) ? L"CZERWONE WYGRYWAJA!" : L"CZARNE WYGRYWAJA!";
    TextOut(hdc, BOARD_PIXELS / 2 - 90, 110, txt.c_str(), (int)txt.size());

    SetTextColor(hdc, RGB(60, 60, 60));
    std::wstring info1 = L"Liczba ruchow: " + std::to_wstring(game.GetTotalMoves());
    std::wstring info2 = L"Bicia czerwonych: " + std::to_wstring(game.GetRedCaptures());
    std::wstring info3 = L"Bicia czarnych:   " + std::to_wstring(game.GetBlackCaptures());
    TextOut(hdc, BOARD_PIXELS / 2 - 90, 155, info1.c_str(), (int)info1.size());
    TextOut(hdc, BOARD_PIXELS / 2 - 90, 185, info2.c_str(), (int)info2.size());
    TextOut(hdc, BOARD_PIXELS / 2 - 90, 210, info3.c_str(), (int)info3.size());

    SetTextColor(hdc, RGB(80, 80, 80));
    TextOut(hdc, BOARD_PIXELS / 2 - 90, 255, L"Chcesz zapisac wynik do rankingu?", 33);

    DrawButton(hdc, GetSaveScoreBtnRect(), RGB(150, 220, 150), L"ZAPISZ WYNIK", 12);
    DrawButton(hdc, GetMenuBtnRect(), RGB(220, 150, 150), L"MENU", 4);
}

void GameRenderer::DrawNameEntry(HDC hdc, int winner, const std::wstring& nameInput) {
    RECT box = { 60, 120, BOARD_PIXELS - 60, 350 };
    HBRUSH boxBr = CreateSolidBrush(RGB(250, 248, 235));
    FillRect(hdc, &box, boxBr);
    DeleteObject(boxBr);
    HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(80, 50, 0));
    HGDIOBJ oldP = SelectObject(hdc, borderPen);
    HGDIOBJ oldB = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, box.left, box.top, box.right, box.bottom);
    SelectObject(hdc, oldP); SelectObject(hdc, oldB);
    DeleteObject(borderPen);

    SetBkMode(hdc, TRANSPARENT);
    COLORREF wc = (winner == RED) ? RGB(180, 0, 0) : RGB(30, 30, 30);
    SetTextColor(hdc, wc);
    std::wstring hdr = (winner == RED) ? L"Zwyciezca: CZERWONE" : L"Zwyciezca: CZARNE";
    TextOut(hdc, BOARD_PIXELS / 2 - 80, 140, hdr.c_str(), (int)hdr.size());

    SetTextColor(hdc, RGB(60, 60, 60));
    TextOut(hdc, BOARD_PIXELS / 2 - 125, 175, L"Podaj imie gracza (max 20 znakow):", 34);

    RECT inputBox = { 100, 210, BOARD_PIXELS - 100, 245 };
    HBRUSH inputBr = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &inputBox, inputBr);
    DeleteObject(inputBr);
    FrameRect(hdc, &inputBox, (HBRUSH)GetStockObject(BLACK_BRUSH));

    SetTextColor(hdc, RGB(0, 0, 0));
    std::wstring display = nameInput + L"|";
    TextOut(hdc, inputBox.left + 5, inputBox.top + 8, display.c_str(), (int)display.size());

    DrawButton(hdc, GetConfirmNameBtnRect(), RGB(150, 220, 150), L"ZAPISZ", 6);
    DrawButton(hdc, GetSkipNameBtnRect(), RGB(220, 220, 220), L"POMIN", 5);

    SetTextColor(hdc, RGB(120, 120, 120));
    TextOut(hdc, BOARD_PIXELS / 2 - 120, 320, L"Nacisnij Enter lub kliknij ZAPISZ", 33);
}

void GameRenderer::DrawRanking(HDC hdc,
    const std::vector<PlayerRecord>& ranking,
    const std::vector<GameHistoryEntry>& history,
    const GameStats& stats)
{
    RECT bg = { 0, 0, BOARD_PIXELS + SIDEBAR_WIDTH, BOARD_PIXELS };
    HBRUSH bgBr = CreateSolidBrush(RGB(240, 235, 215));
    FillRect(hdc, &bg, bgBr);
    DeleteObject(bgBr);

    SetBkMode(hdc, TRANSPARENT);

    HFONT titleFont = CreateFont(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        EASTEUROPE_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH, L"Arial");
    HGDIOBJ oldFont = SelectObject(hdc, titleFont);
    SetTextColor(hdc, RGB(80, 30, 0));
    TextOut(hdc, BOARD_PIXELS / 2 - 55, 8, L"RANKING", 7);
    SelectObject(hdc, oldFont);
    DeleteObject(titleFont);

    // ============================================================
    //  LEWA POLOWA
    // ============================================================
    RECT secL = { 5, 48, 308, 68 };
    HBRUSH secBr = CreateSolidBrush(RGB(180, 140, 80));
    FillRect(hdc, &secL, secBr);
    DeleteObject(secBr);
    SetTextColor(hdc, RGB(255, 255, 255));
    TextOut(hdc, 10, 51, L"NAJLEPSI GRACZE", 15);

    SetTextColor(hdc, RGB(60, 30, 0));
    TextOut(hdc, 10, 74, L"#", 1);
    TextOut(hdc, 30, 74, L"Gracz", 5);
    TextOut(hdc, 200, 74, L"Wygr.", 5);
    TextOut(hdc, 255, 74, L"Przegr.", 7);

    HPEN linePen = CreatePen(PS_SOLID, 1, RGB(150, 100, 50));
    HGDIOBJ oldP = SelectObject(hdc, linePen);
    MoveToEx(hdc, 5, 90, NULL); LineTo(hdc, 308, 90);
    SelectObject(hdc, oldP); DeleteObject(linePen);

    std::vector<PlayerRecord> sorted = ranking;
    std::sort(sorted.begin(), sorted.end(),
        [](const PlayerRecord& a, const PlayerRecord& b) {
            return a.wins > b.wins;
        });

    int maxShow = (std::min)((int)sorted.size(), 10);
    for (int i = 0; i < maxShow; i++) {
        int rowY = 96 + i * 32;

        if (i % 2 == 0) {
            RECT rowR = { 5, rowY, 308, rowY + 32 };
            HBRUSH rowBr = CreateSolidBrush(RGB(255, 250, 235));
            FillRect(hdc, &rowR, rowBr);
            DeleteObject(rowBr);
        }

        if (i == 0)      SetTextColor(hdc, RGB(180, 130, 0));
        else if (i == 1) SetTextColor(hdc, RGB(130, 130, 130));
        else if (i == 2) SetTextColor(hdc, RGB(150, 80, 30));
        else             SetTextColor(hdc, RGB(50, 50, 50));

        std::wstring rank = std::to_wstring(i + 1);
        std::wstring wins = std::to_wstring(sorted[i].wins);
        std::wstring loss = std::to_wstring(sorted[i].losses);

        std::wstring name = sorted[i].name;
        if (name.size() > 16) name = name.substr(0, 16) + L"..";

        TextOut(hdc, 10, rowY + 8, rank.c_str(), (int)rank.size());
        TextOut(hdc, 30, rowY + 8, name.c_str(), (int)name.size());
        TextOut(hdc, 200, rowY + 8, wins.c_str(), (int)wins.size());
        TextOut(hdc, 255, rowY + 8, loss.c_str(), (int)loss.size());
    }

    if (sorted.empty()) {
        SetTextColor(hdc, RGB(150, 150, 150));
        TextOut(hdc, 30, 106, L"Brak danych", 11);
    }

    // ============================================================
    //  PRAWA POLOWA
    // ============================================================
    RECT secR = { 318, 48, 635, 68 };
    HBRUSH secRBr = CreateSolidBrush(RGB(80, 110, 170));
    FillRect(hdc, &secR, secRBr);
    DeleteObject(secRBr);
    SetTextColor(hdc, RGB(255, 255, 255));
    TextOut(hdc, 323, 51, L"HISTORIA GIER", 13);

    SetTextColor(hdc, RGB(30, 30, 80));
    TextOut(hdc, 323, 74, L"#", 1);
    TextOut(hdc, 343, 74, L"Zwyciezca", 9);
    TextOut(hdc, 513, 74, L"Ruchy", 5);
    TextOut(hdc, 563, 74, L"Tryb", 4);

    HPEN linePen2 = CreatePen(PS_SOLID, 1, RGB(80, 110, 170));
    HGDIOBJ oldP2 = SelectObject(hdc, linePen2);
    MoveToEx(hdc, 318, 90, NULL); LineTo(hdc, 635, 90);
    SelectObject(hdc, oldP2); DeleteObject(linePen2);

    int hSize = (int)history.size();
    int hStart = (std::max)(0, hSize - 10);
    int hCount = hSize - hStart;

    for (int i = 0; i < hCount; i++) {
        int rowY = 96 + i * 32;
        const GameHistoryEntry& e = history[hStart + i];

        if (i % 2 == 0) {
            RECT rowR = { 318, rowY, 635, rowY + 32 };
            HBRUSH rowBr = CreateSolidBrush(RGB(235, 240, 255));
            FillRect(hdc, &rowR, rowBr);
            DeleteObject(rowBr);
        }

        SetTextColor(hdc, RGB(40, 40, 80));
        std::wstring idx = std::to_wstring(hStart + i + 1);
        std::wstring moves = std::to_wstring(e.totalMoves);
        std::wstring mode = e.vsAI ? L"AI" : L"2P";

        std::wstring wname = e.winnerName;
        if (wname.size() > 13) wname = wname.substr(0, 13) + L"..";

        TextOut(hdc, 323, rowY + 8, idx.c_str(), (int)idx.size());
        TextOut(hdc, 343, rowY + 8, wname.c_str(), (int)wname.size());
        TextOut(hdc, 513, rowY + 8, moves.c_str(), (int)moves.size());
        TextOut(hdc, 563, rowY + 8, mode.c_str(), (int)mode.size());
    }

    if (history.empty()) {
        SetTextColor(hdc, RGB(150, 150, 150));
        TextOut(hdc, 343, 106, L"Brak historii", 13);
    }

    // Linia pionowa, skrocona by nie nachodzic na pasek
    HPEN divPen = CreatePen(PS_SOLID, 2, RGB(120, 90, 40));
    HGDIOBJ oldPDiv = SelectObject(hdc, divPen);
    MoveToEx(hdc, 312, 45, NULL); LineTo(hdc, 312, 385);
    SelectObject(hdc, oldPDiv); DeleteObject(divPen);

    // ============================================================
    //  NOWOSC: PASEK STATYSTYK GLOBALNYCH
    // ============================================================
    RECT statsBg = { 10, 395, 630, 435 };
    HBRUSH statsBr = CreateSolidBrush(RGB(220, 230, 210));
    FillRect(hdc, &statsBg, statsBr);
    DeleteObject(statsBr);

    HPEN statsPen = CreatePen(PS_SOLID, 1, RGB(150, 170, 140));
    HGDIOBJ oldStatsPen = SelectObject(hdc, statsPen);
    HGDIOBJ oldStatsBr = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, statsBg.left, statsBg.top, statsBg.right, statsBg.bottom);
    SelectObject(hdc, oldStatsPen); SelectObject(hdc, oldStatsBr);
    DeleteObject(statsPen);

    SetTextColor(hdc, RGB(50, 70, 40));

    wchar_t buffer[256];
    swprintf_s(buffer, 256, L"STATYSTYKI: Gier: %d | Sr. ruchow: %.1f | Min ruchow: %d | Max ruchow: %d",
        stats.totalGames, stats.avgMoves, stats.minMoves, stats.maxMoves);

    std::wstring statsText = buffer;
    DrawText(hdc, statsText.c_str(), (int)statsText.size(), &statsBg, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    DrawButton(hdc, GetBackBtnRect(), RGB(220, 180, 100), L"POWROT DO MENU", 14);
}
