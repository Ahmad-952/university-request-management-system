/*==============================================================================
  database.h  -  SQLite persistence layer for University Request Management
  ---------------------------------------------------------------------------
  Tables (PDF Section 8):
    * Users          - user_id, registration_no, name, password_hash, role
    * Requests       - request_id, user_id, category, description, status,
                       created_at, updated_at, assigned_to
    * RequestHistory - history_id, request_id, changed_by, old_status,
                       new_status, remarks, changed_at

  Functions:
    - initDatabase()       : create tables if not exist
    - seedUsers()          : insert default accounts (Ahmad, hina, usman, admin, ops)
    - saveRequest()        : INSERT new request, return DB-generated ID
    - loadPendingRequests(): load all pending requests into circular queue on startup
    - updateRequestStatus(): UPDATE request + INSERT history record
    - logHistory()         : INSERT into RequestHistory
==============================================================================*/
#ifndef DATABASE_H
#define DATABASE_H

#include "sqlite3.h"
#include "requestcore.h"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

using namespace std;

static sqlite3* g_db = nullptr;

/* ---- Execute SQL with no result rows --------------------------------------- */
static bool execSQL(const string& sql, string* errOut = nullptr) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(g_db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        if (errOut) *errOut = errMsg ? errMsg : "unknown error";
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }
    return true;
}

/* ---- Initialize database: create tables if not exist ----------------------- */
bool initDatabase(const string& dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &g_db);
    if (rc != SQLITE_OK) {
        cerr << "[DB] Failed to open database: " << sqlite3_errmsg(g_db) << "\n";
        return false;
    }

    // Enable foreign keys
    execSQL("PRAGMA foreign_keys = ON;");

    // Users table
    string sql = R"(
        CREATE TABLE IF NOT EXISTS Users (
            user_id         INTEGER PRIMARY KEY AUTOINCREMENT,
            registration_no TEXT UNIQUE NOT NULL,
            name            TEXT NOT NULL,
            password_hash   TEXT NOT NULL,
            role            TEXT NOT NULL
        );
    )";
    string err;
    if (!execSQL(sql, &err)) {
        cerr << "[DB] Failed to create Users table: " << err << "\n";
        return false;
    }

    // Requests table
    sql = R"(
        CREATE TABLE IF NOT EXISTS Requests (
            request_id   INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id      INTEGER NOT NULL,
            category     TEXT NOT NULL,
            description  TEXT NOT NULL,
            status       TEXT NOT NULL,
            created_at   INTEGER NOT NULL,
            updated_at   INTEGER NOT NULL,
            assigned_to  TEXT,
            FOREIGN KEY(user_id) REFERENCES Users(user_id)
        );
    )";
    if (!execSQL(sql, &err)) {
        cerr << "[DB] Failed to create Requests table: " << err << "\n";
        return false;
    }

    // RequestHistory table
    sql = R"(
        CREATE TABLE IF NOT EXISTS RequestHistory (
            history_id   INTEGER PRIMARY KEY AUTOINCREMENT,
            request_id   INTEGER NOT NULL,
            changed_by   TEXT NOT NULL,
            old_status   TEXT,
            new_status   TEXT NOT NULL,
            remarks      TEXT,
            changed_at   INTEGER NOT NULL,
            FOREIGN KEY(request_id) REFERENCES Requests(request_id)
        );
    )";
    if (!execSQL(sql, &err)) {
        cerr << "[DB] Failed to create RequestHistory table: " << err << "\n";
        return false;
    }

    cout << "[DB] Database initialized: " << dbPath << "\n";
    return true;
}

/* ---- Seed default user accounts (if Users table is empty) ------------------ */
void seedUsers() {
    // Check if Users table is empty
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT COUNT(*) FROM Users;";
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "[DB] Failed to prepare count query: " << sqlite3_errmsg(g_db) << "\n";
        return;
    }
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (count > 0) {
        cout << "[DB] Users table already populated (" << count << " users).\n";
        return;
    }

    // Insert default accounts (passwords are stored as plain text for simplicity)
    const char* insertSQL = R"(
        INSERT INTO Users (registration_no, name, password_hash, role) VALUES
            ('2500608', 'Ahmad Hussain', '1234', 'student'),
            ('2500594', 'Jozza Batool', '1234', 'student'),
            ('2500600', 'Raja wasif', '1234', 'student'),
            ('admin', 'Administrator', 'admin123', 'admin'),
            ('op1', 'Operator One', 'op1pass', 'operator'),
            ('op2', 'Operator Two', 'op2pass', 'operator');
    )";

    string err;
    if (!execSQL(insertSQL, &err)) {
        cerr << "[DB] Failed to seed users: " << err << "\n";
    } else {
        cout << "[DB] Seeded 6 default user accounts.\n";
    }
}

/* ---- Save a new request to DB, return DB-generated request_id -------------- */
int saveRequest(int userId, const string& category, const string& description, 
                const string& status, time_t created, time_t updated, const string& assignedTo) {
    const char* sql = R"(
        INSERT INTO Requests (user_id, category, description, status, created_at, updated_at, assigned_to)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "[DB] Failed to prepare INSERT: " << sqlite3_errmsg(g_db) << "\n";
        return -1;
    }

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, (sqlite3_int64)created);
    sqlite3_bind_int64(stmt, 6, (sqlite3_int64)updated);
    if (assignedTo.empty())
        sqlite3_bind_null(stmt, 7);
    else
        sqlite3_bind_text(stmt, 7, assignedTo.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        cerr << "[DB] Failed to insert request: " << sqlite3_errmsg(g_db) << "\n";
        return -1;
    }

    int requestId = (int)sqlite3_last_insert_rowid(g_db);
    cout << "[DB] Saved request " << requestId << " (user " << userId << ", " << category << ").\n";
    return requestId;
}

/* ---- Update request status in DB ------------------------------------------- */
bool updateRequestStatus(int requestId, const string& newStatus, time_t updated, const string& assignedTo) {
    const char* sql = "UPDATE Requests SET status = ?, updated_at = ?, assigned_to = ? WHERE request_id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "[DB] Failed to prepare UPDATE: " << sqlite3_errmsg(g_db) << "\n";
        return false;
    }

    sqlite3_bind_text(stmt, 1, newStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)updated);
    if (assignedTo.empty())
        sqlite3_bind_null(stmt, 3);
    else
        sqlite3_bind_text(stmt, 3, assignedTo.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, requestId);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        cerr << "[DB] Failed to update request " << requestId << ": " << sqlite3_errmsg(g_db) << "\n";
        return false;
    }

    return true;
}

/* ---- Log status change to RequestHistory ----------------------------------- */
void logHistory(int requestId, const string& changedBy, const string& oldStatus, 
                const string& newStatus, const string& remarks, time_t changedAt) {
    const char* sql = R"(
        INSERT INTO RequestHistory (request_id, changed_by, old_status, new_status, remarks, changed_at)
        VALUES (?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "[DB] Failed to prepare history INSERT: " << sqlite3_errmsg(g_db) << "\n";
        return;
    }

    sqlite3_bind_int(stmt, 1, requestId);
    sqlite3_bind_text(stmt, 2, changedBy.c_str(), -1, SQLITE_TRANSIENT);
    if (oldStatus.empty())
        sqlite3_bind_null(stmt, 3);
    else
        sqlite3_bind_text(stmt, 3, oldStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, newStatus.c_str(), -1, SQLITE_TRANSIENT);
    if (remarks.empty())
        sqlite3_bind_null(stmt, 5);
    else
        sqlite3_bind_text(stmt, 5, remarks.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, (sqlite3_int64)changedAt);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        cerr << "[DB] Failed to log history: " << sqlite3_errmsg(g_db) << "\n";
    }
}

/* ---- Load pending requests from DB into circular queue on startup ---------- */
int loadPendingRequests(RequestSystem* sys) {
    const char* sql = R"(
        SELECT r.request_id, u.registration_no, r.category, r.description, r.status, 
               r.created_at, r.updated_at, r.assigned_to
        FROM Requests r
        JOIN Users u ON r.user_id = u.user_id
        WHERE r.status = 'Pending' OR r.status = 'In Progress'
        ORDER BY r.created_at ASC;
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "[DB] Failed to prepare SELECT: " << sqlite3_errmsg(g_db) << "\n";
        return 0;
    }

    int loaded = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int reqId = sqlite3_column_int(stmt, 0);
        string regNo = (const char*)sqlite3_column_text(stmt, 1);
        string category = (const char*)sqlite3_column_text(stmt, 2);
        string description = (const char*)sqlite3_column_text(stmt, 3);
        string status = (const char*)sqlite3_column_text(stmt, 4);
        time_t created = (time_t)sqlite3_column_int64(stmt, 5);
        time_t updated = (time_t)sqlite3_column_int64(stmt, 6);
        const char* assigned = (const char*)sqlite3_column_text(stmt, 7);
        string assignedTo = assigned ? assigned : "";

        // Find the account by regNo
        Account* acc = nullptr;
        for (size_t i = 0; i < sys->accounts.size(); i++) {
            if (sys->accounts[i]->registrationNo == regNo) {
                acc = sys->accounts[i];
                break;
            }
        }

        if (!acc) {
            cerr << "[DB] Warning: request " << reqId << " references unknown registration_no " << regNo << "\n";
            continue;
        }

        // Parse category
        Category cat = CAT_OTHERS;
        for (int c = 0; c < CAT_COUNT; c++) {
            if (category == CATEGORY_NAMES[c]) {
                cat = (Category)c;
                break;
            }
        }

        // Parse status
        Status stat = ST_PENDING;
        for (int s = 0; s < STATUS_COUNT; s++) {
            if (status == STATUS_NAMES[s]) {
                stat = (Status)s;
                break;
            }
        }

        // Create Request object and restore into system
        Request* r = new Request;
        r->id = reqId;
        r->studentReg = regNo;
        r->studentName = acc->fullName;
        r->category = cat;
        r->description = description;
        r->status = stat;
        r->submitted = DateTime::fromTimeT(created);
        r->updated = DateTime::fromTimeT(updated);
        r->assignedTo = assignedTo;

        // Manually add to queue (bypass normal enqueue to preserve DB IDs)
        if (!sys->addRequestFromDB(r)) {
            cerr << "[DB] Warning: failed to add request " << reqId << " to queue (full?).\n";
            delete r;
            continue;
        }

        loaded++;
    }

    sqlite3_finalize(stmt);
    cout << "[DB] Loaded " << loaded << " pending requests from database.\n";
    return loaded;
}

/* ---- Get user_id by registration_no ---------------------------------------- */
int getUserIdByRegNo(const string& regNo) {
    const char* sql = "SELECT user_id FROM Users WHERE registration_no = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "[DB] Failed to prepare user lookup: " << sqlite3_errmsg(g_db) << "\n";
        return -1;
    }

    sqlite3_bind_text(stmt, 1, regNo.c_str(), -1, SQLITE_TRANSIENT);
    int userId = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return userId;
}

/* ---- Close database -------------------------------------------------------- */
void closeDatabase() {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = nullptr;
        cout << "[DB] Database closed.\n";
    }
}

#endif // DATABASE_H
