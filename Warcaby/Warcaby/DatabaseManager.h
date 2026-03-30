#pragma once
#include <vector>
#include <string>
#include "Logika.h"
#include "sqlite3.h"

class DatabaseManager {
public:
    DatabaseManager(const std::string& dbPath = "checkers_data.db");
    ~DatabaseManager();

    bool Initialize();

    void ClearAllTables();

    std::vector<PlayerRecord> LoadRanking();
    void SaveRanking(const std::vector<PlayerRecord>& ranking);

    std::vector<GameHistoryEntry> LoadHistory();
    void AddHistoryEntry(const GameHistoryEntry& entry);

    void UpdatePlayerStats(const std::wstring& playerName, int moves);
    GameStats GetGlobalStatistics();

private:
    sqlite3* m_db;
    std::string m_dbPath;

    bool ExecuteCommand(const char* sql);
};