#include "DatabaseManager.h"
#include <iostream>

DatabaseManager::DatabaseManager(const std::string& dbPath) : m_dbPath(dbPath), m_db(nullptr) {}

DatabaseManager::~DatabaseManager() {
    if (m_db) {
        ClearAllTables();
        sqlite3_close(m_db);
    }
}

void DatabaseManager::ClearAllTables() {
    if (m_db) {
        ExecuteCommand("DELETR FROM Ranking;");
        ExecuteCommand("DELETR FROM History;");
        ExecuteCommand("DELETR FROM PlayerStats;");
    }
}

bool DatabaseManager::Initialize() {
    if (sqlite3_open(m_dbPath.c_str(), &m_db) != SQLITE_OK) return false;

    // Tworzymy trzy tabele
    const char* sqlRanking = "CREATE TABLE IF NOT EXISTS Ranking (Wins INTEGER, Losses INTEGER, Name TEXT);";
    ExecuteCommand(sqlRanking);

    const char* sqlHistory = "CREATE TABLE IF NOT EXISTS History (WinnerName TEXT, TotalMoves INTEGER, VsAI INTEGER, RedCaptures INTEGER, BlackCaptures INTEGER);";
    ExecuteCommand(sqlHistory);

    const char* sqlStats = "CREATE TABLE IF NOT EXISTS PlayerStats (Name TEXT UNIQUE, GamesPlayed INTEGER, TotalMovesMade INTEGER);";
    ExecuteCommand(sqlStats);

    ExecuteCommand("DELETE FROM Ranking;");
    ExecuteCommand("DELETE FROM History;");
    ExecuteCommand("DELETE FROM PlayerStats;");

    return true;
}

bool DatabaseManager::ExecuteCommand(const char* sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::vector<PlayerRecord> DatabaseManager::LoadRanking() {
    std::vector<PlayerRecord> loadedRanking;
    sqlite3_stmt* stmt;
    const char* query = "SELECT Wins, Losses, Name FROM Ranking ORDER BY Wins DESC LIMIT 10;";

    if (sqlite3_prepare_v2(m_db, query, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int wins = sqlite3_column_int(stmt, 0);
            int losses = sqlite3_column_int(stmt, 1);
            const void* text = sqlite3_column_text16(stmt, 2);
            std::wstring name = text ? static_cast<const wchar_t*>(text) : L"";
            if (!name.empty()) {
                loadedRanking.push_back({ name, wins, losses });
            }
        }
    }
    sqlite3_finalize(stmt);
    return loadedRanking;
}

void DatabaseManager::SaveRanking(const std::vector<PlayerRecord>& ranking) {
    ExecuteCommand("DELETE FROM Ranking;");
    sqlite3_stmt* stmt;
    const char* insertSQL = "INSERT INTO Ranking (Wins, Losses, Name) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(m_db, insertSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        for (const auto& rec : ranking) {
            sqlite3_bind_int(stmt, 1, rec.wins);
            sqlite3_bind_int(stmt, 2, rec.losses);
            sqlite3_bind_text16(stmt, 3, rec.name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
    }
    sqlite3_finalize(stmt);
}

std::vector<GameHistoryEntry> DatabaseManager::LoadHistory() {
    std::vector<GameHistoryEntry> loadedHistory;
    sqlite3_stmt* stmt;
    const char* query = "SELECT WinnerName, TotalMoves, VsAI, RedCaptures, BlackCaptures FROM History ORDER BY rowid ASC;";

    if (sqlite3_prepare_v2(m_db, query, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const void* text = sqlite3_column_text16(stmt, 0);
            std::wstring name = text ? static_cast<const wchar_t*>(text) : L"";
            int moves = sqlite3_column_int(stmt, 1);
            bool vsAI = sqlite3_column_int(stmt, 2) != 0;
            int redCap = sqlite3_column_int(stmt, 3);
            int blackCap = sqlite3_column_int(stmt, 4);

            loadedHistory.push_back({ name, moves, vsAI, redCap, blackCap });
        }
    }
    sqlite3_finalize(stmt);
    return loadedHistory;
}

void DatabaseManager::AddHistoryEntry(const GameHistoryEntry& entry) {
    sqlite3_stmt* stmt;
    const char* insertSQL = "INSERT INTO History (WinnerName, TotalMoves, VsAI, RedCaptures, BlackCaptures) VALUES (?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(m_db, insertSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text16(stmt, 1, entry.winnerName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, entry.totalMoves);
        sqlite3_bind_int(stmt, 3, entry.vsAI ? 1 : 0);
        sqlite3_bind_int(stmt, 4, entry.redCaptures);
        sqlite3_bind_int(stmt, 5, entry.blackCaptures);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void DatabaseManager::UpdatePlayerStats(const std::wstring& playerName, int moves) {
    sqlite3_stmt* stmt;
    // Sprawdzamy, czy gracz juz istnieje w statystykach
    const char* checkSQL = "SELECT GamesPlayed FROM PlayerStats WHERE Name = ?;";
    bool exists = false;

    if (sqlite3_prepare_v2(m_db, checkSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text16(stmt, 1, playerName.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) exists = true;
    }
    sqlite3_finalize(stmt);

    if (exists) {
        const char* updateSQL = "UPDATE PlayerStats SET GamesPlayed = GamesPlayed + 1, TotalMovesMade = TotalMovesMade + ? WHERE Name = ?;";
        if (sqlite3_prepare_v2(m_db, updateSQL, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, moves);
            sqlite3_bind_text16(stmt, 2, playerName.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
        }
    }
    else {
        const char* insertSQL = "INSERT INTO PlayerStats (Name, GamesPlayed, TotalMovesMade) VALUES (?, 1, ?);";
        if (sqlite3_prepare_v2(m_db, insertSQL, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text16(stmt, 1, playerName.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, moves);
            sqlite3_step(stmt);
        }
    }
    sqlite3_finalize(stmt);
}

GameStats DatabaseManager::GetGlobalStatistics() {
    GameStats stats = { 0, 0.0, 0, 0 };
    sqlite3_stmt* stmt;
    const char* query = "SELECT COUNT(*), AVG(TotalMoves), MAX(TotalMoves), MIN(TotalMoves) FROM History;";

    if (sqlite3_prepare_v2(m_db, query, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.totalGames = sqlite3_column_int(stmt, 0);
            stats.avgMoves = sqlite3_column_double(stmt, 1);
            stats.maxMoves = sqlite3_column_int(stmt, 2);
            stats.minMoves = sqlite3_column_int(stmt, 3);
        }
    }
    sqlite3_finalize(stmt);
    return stats;
}
