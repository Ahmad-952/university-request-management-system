/*==============================================================================
  University Request Management System  -  Circular Queue based  (CONSOLE)
  ---------------------------------------------------------------------------
  Requirements implemented (from spec):
    1. Student Login        : students log in with their own accounts
    2. Submit Requests      : complaints about Fees, Registration, Examinations, Other
    3. Unique Request ID    : every request gets a unique auto-generated ID
    4. Circular Queue (C++) : FIFO - the OLDEST request must be processed first
    5. Admin Login          : admins / help desk operators log in separately
    6. Process Requests     : admin views pending, processes NEXT request,
                              and assigns it to an operator if needed
    7. Search Requests      : by request ID or by student registration number
    8. Update Status        : Processing, Resolved, Rejected
    9. View Statistics      : counts of pending / processing / resolved / rejected

  This file is only the CONSOLE interface. All data structures and business
  rules live in requestcore.h, shared with the web version (server.cpp), so the
  FIFO order, unique-ID and status rules are defined in exactly one place.
==============================================================================*/

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

#include "requestcore.h"

using namespace std;

/* =============================================================================
   SECTION 1 : CONSOLE INPUT  (validated - rejects bad input with a message)
   ============================================================================= */
int readInt(const string& prompt, int lo, int hi) {
    int v;
    while (true) {
        cout << prompt;
        if (cin >> v) {
            cin.ignore(100000, '\n');
            if (v >= lo && v <= hi) return v;
        } else {
            cin.clear();
            cin.ignore(100000, '\n');
        }
        cout << "  [ERROR] Invalid input! Enter a number between "
             << lo << " and " << hi << ".\n";
    }
}
string readLine(const string& prompt) {
    string s;
    cout << prompt;
    getline(cin, s);
    while (s.empty()) {
        cout << "  [ERROR] Value cannot be empty.\n" << prompt;
        getline(cin, s);
    }
    return s;
}

/* =============================================================================
   SECTION 2 : CONSOLE WRAPPERS  (prompt -> core -> print)
   ---------------------------------------------------------------------------
   Each wrapper keeps the friendly console prompts but delegates the actual
   rules to requestcore.h.
   ============================================================================= */

/* ---- spec 1 / 5 : login (3 attempts) ------------------------------------- */
Account* consoleLogin(RequestSystem& sys, bool wantAdmin) {
    string kind = wantAdmin ? "Admin / Operator" : "Student";
    for (int attempt = 1; attempt <= 3; attempt++) {
        string u = readLine("  " + kind + " username: ");
        string p = readLine("  Password: ");
        Account* a = sys.authenticate(u, p, wantAdmin);
        if (a) {
            cout << "  [OK] Welcome, " << a->fullName
                 << " (" << a->role << ")\n";
            return a;
        }
        cout << "  [ERROR] Invalid username/password"
             << (attempt < 3 ? ". Try again." : ". Account locked for this session.") << "\n";
    }
    return 0;
}

/* ---- spec 2 / 3 : submit a request --------------------------------------- */
Request* consoleSubmit(RequestSystem& sys, const Account* student) {
    cout << "  Categories:\n";
    for (int i = 0; i < CAT_COUNT; i++)
        cout << "    " << (i + 1) << ". " << CATEGORY_NAMES[i] << "\n";
    int ci = readInt("  Choose category: ", 1, CAT_COUNT);
    string desc = readLine("  Describe the complaint: ");

    string msg;
    Request* r = sys.submit(student, ci - 1, desc, msg);
    if (!r) { cout << "  [ERROR] " << msg << "\n"; return 0; }
    cout << "  [OK] " << msg << "\n";
    return r;
}

/* ---- spec 6 : process the NEXT (oldest) request -------------------------- */
void consoleProcessNext(RequestSystem& sys) {
    if (sys.queue.isEmpty()) {
        cout << "  [ERROR] No pending requests to process.\n";
        return;
    }
    Request* next = sys.queue.peek();            // FIFO: oldest first
    cout << "  Next request in FIFO order:\n";
    next->displayFull();

    cout << "  What do you want to do with " << next->id << "?\n"
         << "    1. Start processing - assign to an operator\n"
         << "    2. Start processing - no operator\n"
         << "    3. Resolve it now  (mark Resolved)\n"
         << "    4. Reject it now   (mark Rejected)\n"
         << "    5. Leave it in the queue (cancel)\n";
    int ch = readInt("  Choice: ", 1, 5);

    ProcessAction act;
    string opName, note;

    if (ch == 5) {                                  // cancel
        string msg;
        sys.process(next, PA_CANCEL, "", "", msg);
        cout << "  [OK] " << msg << "\n";
        return;
    } else if (ch == 1) {                           // assign to an operator
        act = PA_PROCESS_WITH_OP;
        vector<Account*> ops = sys.operators();
        if (ops.empty())
            cout << "  [WARN] No operators registered - continuing without one.\n";
        else {
            cout << "  Operators:\n";
            for (size_t i = 0; i < ops.size(); i++)
                cout << "    " << (i + 1) << ". " << ops[i]->fullName
                     << " (" << ops[i]->username << ")\n";
            int pick = readInt("  Choose operator: ", 1, (int)ops.size());
            opName = ops[pick - 1]->fullName;
        }
        if (opName.empty()) act = PA_PROCESS_NO_OP; // no operators registered
    } else if (ch == 2) {                           // processing, no operator
        act = PA_PROCESS_NO_OP;
    } else {                                        // 3 = Resolved, 4 = Rejected
        act = (ch == 3) ? PA_RESOLVE : PA_REJECT;
        note = readLine("  Admin note: ");
    }

    string msg;
    if (!sys.process(next, act, opName, note, msg)) {
        cout << "  [ERROR] " << msg << "\n";
        return;
    }
    cout << "  [OK] " << msg << "\n";
    if (sys.queue.size() > 0)
        cout << "  [i] Next in queue is now: " << sys.queue.peek()->id << "\n";
    else
        cout << "  [i] Queue is now empty.\n";
}

/* ---- spec 7 : search ------------------------------------------------------ */
void consoleSearchById(RequestSystem& sys) {
    string id = readLine("  Request ID (e.g. REQ-0001): ");
    Request* r = sys.findRequest(id);
    if (!r) { cout << "  [ERROR] No request found with ID '" << id << "'.\n"; return; }
    r->displayFull();
}
void consoleSearchByReg(RequestSystem& sys) {
    string reg = readLine("  Student registration number: ");
    vector<Request*> hits = sys.byRegistration(reg);
    if (hits.empty()) {
        cout << "  [ERROR] No requests found for registration number '"
             << reg << "'.\n";
        return;
    }
    cout << "  Requests for " << reg << ":\n";
    for (size_t i = 0; i < hits.size(); i++)
        cout << "    " << pad(hits[i]->id, 11)
             << pad(CATEGORY_NAMES[hits[i]->category], 14)
             << pad(statusName(hits[i]->status), 12)
             << hits[i]->description.substr(0, 32) << "\n";
    cout << "  " << hits.size() << " request(s) found.\n";
}

/* ---- spec 8 : update status ----------------------------------------------- */
void consoleUpdateStatus(RequestSystem& sys) {
    string id = readLine("  Request ID: ");
    Request* r = sys.findRequest(id);
    if (!r) { cout << "  [ERROR] No request found with ID '" << id << "'.\n"; return; }
    r->displayFull();
    if (r->isClosed()) {
        cout << "  [ERROR] This request is already closed (" << statusName(r->status)
             << ") and cannot be changed.\n";
        return;
    }
    cout << "  New status: 1. Processing   2. Resolved   3. Rejected\n";
    int ch = readInt("  Choice: ", 1, 3);
    Status ns = (Status)ch;             // 1 -> Processing, 2 -> Resolved, 3 -> Rejected
    string note;
    if (ch >= 2) note = readLine("  Admin note: ");

    string msg;
    if (!sys.changeStatus(id, ns, note, msg)) { cout << "  [ERROR] " << msg << "\n"; return; }
    cout << "  [OK] " << msg << "\n";
}

/* ---- spec 9 : statistics --------------------------------------------------- */
void consoleStatistics(const RequestSystem& sys) {
    int c[STATUS_COUNT];
    sys.statistics(c);
    cout << "  +--------------------------------------+\n"
         << "  |       REQUEST STATISTICS             |\n"
         << "  +--------------------------------------+\n"
         << "  Pending    : " << setw(4) << c[ST_PENDING]
         << "   (in circular queue: " << sys.queue.size() << "/" << CircularQueue::MAX << ")\n"
         << "  Processing : " << setw(4) << c[ST_PROCESSING] << "\n"
         << "  Resolved   : " << setw(4) << c[ST_RESOLVED] << "\n"
         << "  Rejected   : " << setw(4) << c[ST_REJECTED] << "\n"
         << "  ----------------------------------------\n"
         << "  Total      : " << setw(4) << (int)sys.allRequests.size() << "\n";
}

/* ---- views ---------------------------------------------------------------- */
void consoleViewAll(const RequestSystem& sys) {
    if (sys.allRequests.empty()) { cout << "  (no requests yet)\n"; return; }
    cout << "  " << pad("Request ID", 13) << pad("Reg No", 14) << pad("Category", 14)
         << pad("Status", 12) << pad("Submitted", 21) << "Operator\n";
    cout << "  ---------------------------------------------------------------\n";
    for (size_t i = 0; i < sys.allRequests.size(); i++) {
        Request* r = sys.allRequests[i];
        cout << "  " << pad(r->id, 13) << pad(r->studentReg, 14)
             << pad(CATEGORY_NAMES[r->category], 14)
             << pad(statusName(r->status), 12)
             << pad(r->submitted.str(), 21)
             << (r->assignedTo.empty() ? "-" : r->assignedTo) << "\n";
    }
}
void consoleViewMyRequests(const RequestSystem& sys, const Account* student) {
    vector<Request*> mine = sys.byStudent(student);
    if (mine.empty()) { cout << "  (you have not submitted any requests yet)\n"; return; }
    cout << "  " << pad("Request ID", 13) << pad("Category", 14)
         << pad("Status", 12) << pad("Submitted", 21) << "Description\n"
         << "  ---------------------------------------------------------------\n";
    for (size_t i = 0; i < mine.size(); i++)
        cout << "  " << pad(mine[i]->id, 13) << pad(CATEGORY_NAMES[mine[i]->category], 14)
             << pad(statusName(mine[i]->status), 12) << pad(mine[i]->submitted.str(), 21)
             << mine[i]->description.substr(0, 28) << "\n";
}

/* =============================================================================
   SECTION 3 : MENUS
   ============================================================================= */
/* pad a line to width W so the banner box always lines up */
static string bw(const string& s, int W) {
    string t = (s.size() > (size_t)(W - 1)) ? s.substr(0, W - 1) : s;
    return "| " + t + string((size_t)(W - 1) - t.size(), ' ') + "|";
}
static void banner() {
    const int W = 62;
    cout << "\n  +" << string(W, '=') << "+\n"
         << "  " << bw("UNIVERSITY REQUEST MANAGEMENT SYSTEM", W) << "\n"
         << "  " << bw("Circular Queue (FIFO)", W) << "\n"
         << "  " << bw("2500608 Ahmad Hussain   2500594 Jozza Batool", W) << "\n"
         << "  " << bw("2500600 Raja wasif", W) << "\n"
         << "  +" << string(W, '=') << "+\n";
}

int main() {
    RequestSystem sys;
    sys.loadSampleData();
    cout << "  [OK] Sample data loaded: 6 students, 1 admin, 2 operators, 4 requests.\n";

    bool running = true;
    while (running) {
        banner();
        cout << "  1. Student Login\n"
             << "  2. Admin / Help Desk Login\n"
             << "  0. Exit\n"
             << "  ------------------------------------------------------\n";
        int ch = readInt("  Enter choice: ", 0, 2);

        if (ch == 0) { running = false; continue; }

        Account* user = consoleLogin(sys, ch == 2);
        if (!user) continue;

        if (!user->isAdmin) {
            /* ---------------- STUDENT MENU (spec 1 & 2) ---------------- */
            bool done = false;
            while (!done) {
                cout << "\n  --- Student Menu [" << user->fullName
                     << " / " << user->regId << "] ----------------------\n"
                     << "  1. Submit a request\n"
                     << "  2. View my requests\n"
                     << "  3. View request details by ID\n"
                     << "  0. Logout\n";
                int m = readInt("  Choice: ", 0, 3);
                if (m == 1) consoleSubmit(sys, user);
                else if (m == 2) consoleViewMyRequests(sys, user);
                else if (m == 3) {
                    string id = readLine("  Request ID: ");
                    Request* r = sys.findRequest(id);
                    if (!r) cout << "  [ERROR] No request found with ID '" << id << "'.\n";
                    else if (r->studentReg != user->regId)
                        cout << "  [ERROR] That request belongs to another student.\n";
                    else r->displayFull();
                }
                else done = true;
            }
        } else {
            /* ---------------- ADMIN / OPERATOR MENU (5,6,7,8,9) -------- */
            bool done = false;
            while (!done) {
                cout << "\n  --- Admin Menu [" << user->fullName
                     << " / " << user->role << "] ----------------------\n"
                     << "  1. View pending requests (queue, oldest first)\n"
                     << "  2. Process next request -> assign / resolve / reject\n"
                     << "  3. Search by request ID\n"
                     << "  4. Search by student registration number\n"
                     << "  5. Update status of an already-processing request\n"
                     << "  6. View statistics\n"
                     << "  7. View all requests\n"
                     << "  0. Logout\n"
                     << "  (hint: option 2 closes the request shown as 'next')\n";
                int m = readInt("  Choice: ", 0, 7);
                if (m == 1) sys.queue.display("PENDING REQUESTS");
                else if (m == 2) consoleProcessNext(sys);
                else if (m == 3) consoleSearchById(sys);
                else if (m == 4) consoleSearchByReg(sys);
                else if (m == 5) consoleUpdateStatus(sys);
                else if (m == 6) consoleStatistics(sys);
                else if (m == 7) consoleViewAll(sys);
                else done = true;
            }
        }
        cout << "  [OK] Logged out.\n";
    }
    cout << "  Thank you for using the University Request Management System. Goodbye!\n";
    return 0;
}
