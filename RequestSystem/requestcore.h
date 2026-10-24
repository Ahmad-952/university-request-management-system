/*==============================================================================
  requestcore.h  -  Shared, I/O-free core of the University Request Management
  System (Circular Queue based).

  Used by BOTH:
    * main.cpp       -> console application
    * gui.cpp        -> native Win32 GUI application

  Nothing in here prompts or prints; callers supply input and render output.
  That keeps every business rule (FIFO order, unique IDs, status transitions,
  statistics) defined in exactly one place.

  Requirements covered:
    1. Student login      2. Submit requests    3. Unique request ID
    4. Circular queue (FIFO)                    5. Admin login
    6. Process requests (assign operator)       7. Search requests
    8. Update status                            9. Statistics
==============================================================================*/
#ifndef REQUEST_CORE_H
#define REQUEST_CORE_H

#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>

using namespace std;

/* =============================================================================
   DATE & TIME
   ============================================================================= */
struct DateTime {
    int y, mo, d, h, mi, s;
    DateTime() : y(1970), mo(1), d(1), h(0), mi(0), s(0) {}
    DateTime(int Y, int Mo, int D, int H, int Mi, int S)
        : y(Y), mo(Mo), d(D), h(H), mi(Mi), s(S) {}
    static DateTime now() {
        time_t t = time(0);
        tm* lt = localtime(&t);
        return DateTime(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                        lt->tm_hour, lt->tm_min, lt->tm_sec);
    }
    static DateTime fromTimeT(time_t t) {
        tm* lt = localtime(&t);
        return DateTime(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                        lt->tm_hour, lt->tm_min, lt->tm_sec);
    }
    time_t toTimeT() const {
        tm t = {};
        t.tm_year = y - 1900;
        t.tm_mon = mo - 1;
        t.tm_mday = d;
        t.tm_hour = h;
        t.tm_min = mi;
        t.tm_sec = s;
        t.tm_isdst = -1;
        return mktime(&t);
    }
    string str() const {
        ostringstream o;
        o << setfill('0') << setw(4) << y << "-" << setw(2) << mo << "-"
          << setw(2) << d << " " << setw(2) << h << ":" << setw(2) << mi
          << ":" << setw(2) << s;
        return o.str();
    }
};

/* left-align / truncate to width w (used by the text renderers) */
inline string pad(const string& s, int w) {
    if ((int)s.size() >= w) return s.substr(0, w - 1) + ".";
    return s + string(w - s.size(), ' ');
}

/* =============================================================================
   ENUMS
   ============================================================================= */
enum Category { CAT_FEES = 0, CAT_REGISTRATION, CAT_EXAMINATIONS, CAT_OTHERS, CAT_COUNT };
enum Status   { ST_PENDING = 0, ST_PROCESSING, ST_RESOLVED, ST_REJECTED, STATUS_COUNT };

inline const char* const CATEGORY_NAMES[CAT_COUNT] = {
    "Fees", "Registration", "Examinations", "Other"
};
inline const char* const STATUS_NAMES[STATUS_COUNT] = {
    "Pending", "Processing", "Resolved", "Rejected"
};
inline string statusName(Status s) { return STATUS_NAMES[s]; }

/* =============================================================================
   ACCOUNT
   ============================================================================= */
struct Account {
    string username, password;
    string fullName;
    string registrationNo;      // student registration number ("" for admins)
    bool isAdmin;               // false = student, true = admin / operator
    string role;                // "Student" / "Admin" / "Operator"

    Account() : isAdmin(false) {}
    Account(const string& u, const string& p, const string& n,
            const string& r, bool a, const string& rl)
        : username(u), password(p), fullName(n), registrationNo(r), isAdmin(a), role(rl) {}
};

/* =============================================================================
   REQUEST
   ============================================================================= */
class Request {
public:
    int id;               // spec 3: unique auto-generated request ID (from DB)
    string studentReg;    // spec 7: searchable by registration number
    string studentName;
    Category category;    // spec 2: fees / registration / examinations / other
    string description;   // free text complaint
    Status status;        // spec 8: Pending / Processing / Resolved / Rejected
    DateTime submitted;
    DateTime updated;
    string assignedTo;    // spec 6: operator the request was assigned to
    string adminNote;

    Request() : id(0), category(CAT_OTHERS), status(ST_PENDING) {}

    void touch() { updated = DateTime::now(); }

    bool isClosed() const { return status == ST_RESOLVED || status == ST_REJECTED; }

    void displayFull() const {
        ostringstream idStr;
        idStr << "#" << id;
        cout << "  +--------------------------------------------------+\n"
             << "  | Request " << pad(idStr.str(), 12) << " " << pad(statusName(status), 12) << "   |\n"
             << "  +--------------------------------------------------+\n"
             << "  Student Reg   : " << studentReg << " (" << studentName << ")\n"
             << "  Category      : " << CATEGORY_NAMES[category] << "\n"
             << "  Description   : " << description << "\n"
             << "  Submitted At  : " << submitted.str() << "\n"
             << "  Last Updated  : " << updated.str() << "\n";
        if (!assignedTo.empty()) cout << "  Assigned To   : " << assignedTo << "\n";
        if (!adminNote.empty())   cout << "  Admin Note    : " << adminNote << "\n";
    }
};

/* =============================================================================
   CIRCULAR QUEUE  (spec 4 - FIFO, oldest request first)
   front = OLDEST pending request, rear = newest, wrap-around via modulo.
   ============================================================================= */
class CircularQueue {
public:
    static const int MAX = 20;

private:
    Request* arr[MAX];
    int front;
    int rear;
    int count;

public:
    CircularQueue() : front(0), rear(-1), count(0) {}

    bool isEmpty() const { return count == 0; }
    bool isFull()  const { return count == MAX; }
    int  size()    const { return count; }

    /* enqueue at the rear (new request waits behind older ones) */
    bool enqueue(Request* r) {
        if (isFull()) return false;
        rear = (rear + 1) % MAX;
        arr[rear] = r;
        count++;
        return true;
    }
    /* dequeue from the front -> FIFO: OLDEST comes out first */
    bool dequeue(Request*& out) {
        if (isEmpty()) return false;
        out = arr[front];
        front = (front + 1) % MAX;
        count--;
        return true;
    }
    Request* peek() const { return isEmpty() ? 0 : arr[front]; }

    bool contains(int id) const {
        int idx = front;
        for (int i = 0; i < count; i++, idx = (idx + 1) % MAX)
            if (arr[idx]->id == id) return true;
        return false;
    }
    /* used when a still-queued (Pending) request is closed by hand, so the
       FIFO never keeps offering an already-closed item */
    bool removeById(int id) {
        if (isEmpty()) return false;
        int idx = front;
        for (int i = 0; i < count; i++, idx = (idx + 1) % MAX) {
            if (arr[idx]->id == id) {
                int write = idx;
                int moves = count - i - 1;
                for (int k = 0; k < moves; k++) {
                    int nxt = (write + 1) % MAX;
                    arr[write] = arr[nxt];
                    write = nxt;
                }
                rear = (rear - 1 + MAX) % MAX;
                count--;
                return true;
            }
        }
        return false;
    }
    /* FIFO snapshot, oldest first */
    vector<Request*> snapshot() const {
        vector<Request*> out;
        int idx = front;
        for (int i = 0; i < count; i++, idx = (idx + 1) % MAX) out.push_back(arr[idx]);
        return out;
    }
    void clearAll() { front = 0; rear = -1; count = 0; }

    /* console rendering (spec 6) */
    void display(const string& title) const {
        cout << "  " << title << "  (pending: " << count << "/" << MAX << ")\n";
        if (isEmpty()) { cout << "  (queue is empty - nothing pending)\n"; return; }
        cout << "  Pos " << pad("Request ID", 13) << pad("Reg No", 14)
             << pad("Category", 14) << pad("Submitted", 21) << "Description\n";
        cout << "  ---------------------------------------------------------------\n";
        int idx = front;
        for (int i = 0; i < count; i++, idx = (idx + 1) % MAX) {
            ostringstream idStr;
            idStr << "#" << arr[idx]->id;
            cout << "  " << setw(3) << (i + 1) << " " << pad(idStr.str(), 13)
                 << pad(arr[idx]->studentReg, 14)
                 << pad(CATEGORY_NAMES[arr[idx]->category], 14)
                 << pad(arr[idx]->submitted.str(), 21)
                 << arr[idx]->description.substr(0, 26) << "\n";
        }
        ostringstream frontIdStr;
        frontIdStr << "#" << arr[front]->id;
        cout << "  -> Next to process (oldest): " << frontIdStr.str()
             << "\n     Use menu 2 to assign / resolve / reject it.\n";
    }
};

/* =============================================================================
   PROCESS ACTION  (spec 6 - what the admin decides for the "next" request)
   ============================================================================= */
enum ProcessAction {
    PA_CANCEL = 0,          // leave it in the queue (stays Pending)
    PA_PROCESS_WITH_OP,     // dequeue, status Processing, operator assigned
    PA_PROCESS_NO_OP,       // dequeue, status Processing, no operator
    PA_RESOLVE,             // dequeue, status Resolved  (note required)
    PA_REJECT               // dequeue, status Rejected  (note required)
};

/* =============================================================================
   SYSTEM CORE  (all business rules, no I/O)
   ============================================================================= */
class RequestSystem {
public:
    vector<Account*> accounts;
    vector<Request*> allRequests;   // every request ever created
    CircularQueue queue;            // spec 4: pending requests, FIFO
    int nextRequestId;

    RequestSystem() : nextRequestId(1) {}
    ~RequestSystem() {
        for (size_t i = 0; i < allRequests.size(); i++) delete allRequests[i];
        for (size_t i = 0; i < accounts.size(); i++) delete accounts[i];
    }

    /* ---- spec 1 / 5 : credential check (caller handles retries/prompts) --- */
    Account* authenticate(const string& u, const string& p, bool wantAdmin) const {
        for (size_t i = 0; i < accounts.size(); i++) {
            Account* a = accounts[i];
            if (a->username == u && a->password == p && a->isAdmin == wantAdmin)
                return a;
        }
        return 0;
    }
    Account* findAccount(const string& u) const {
        for (size_t i = 0; i < accounts.size(); i++)
            if (accounts[i]->username == u) return accounts[i];
        return 0;
    }
    vector<Account*> operators() const {
        vector<Account*> out;
        for (size_t i = 0; i < accounts.size(); i++)
            if (accounts[i]->isAdmin && accounts[i]->role == "Operator")
                out.push_back(accounts[i]);
        return out;
    }

    /* ---- spec 2 / 3 : submit a request with a unique ID ------------------- */
    Request* submit(const Account* student, int categoryIdx,
                    const string& desc, string& msg) {
        if (!student) { msg = "No student signed in."; return 0; }
        if (categoryIdx < 0 || categoryIdx >= CAT_COUNT) {
            msg = "Invalid category."; return 0;
        }
        if (desc.empty()) { msg = "Description cannot be empty."; return 0; }
        if (queue.isFull()) {
            ostringstream o;
            o << "Queue is full (" << CircularQueue::MAX
              << " pending requests). Please try again later.";
            msg = o.str();
            return 0;
        }
        Request* r = new Request();
        // Note: ID will be assigned by database (set to 0 initially)
        r->id = 0;  // DB will set this via saveRequest()
        r->studentReg = student->registrationNo;
        r->studentName = student->fullName;
        r->category = (Category)categoryIdx;
        r->description = desc;
        r->status = ST_PENDING;
        r->submitted = r->updated = DateTime::now();

        allRequests.push_back(r);
        queue.enqueue(r);                          // goes to the BACK (FIFO)

        ostringstream m;
        m << "Request submitted. Position in queue: " << queue.size();
        msg = m.str();
        return r;
    }

    /* ---- spec 6 : act on the OLDEST request ------------------------------- */
    /* Returns true on success; `msg` always describes the outcome. */
    bool process(Request* r, ProcessAction action, const string& operatorName,
                 const string& note, string& msg) {
        if (!r) { msg = "No request selected."; return false; }
        if (r->status != ST_PENDING) {
            ostringstream o;
            o << "Request #" << r->id << " is already " << statusName(r->status) << ".";
            msg = o.str();
            return false;
        }
        if (action == PA_CANCEL) {
            ostringstream o;
            o << "Request #" << r->id << " left in the queue (still Pending).";
            msg = o.str();
            return true;
        }
        /* action must have an admin note when closing the request */
        if ((action == PA_RESOLVE || action == PA_REJECT) && note.empty()) {
            msg = "An admin note is required to resolve or reject a request.";
            return false;
        }
        /* always remove it: front normally, but fall back to a safe removal
           in case the queue moved on while the UI was open */
        Request* frontNow = queue.peek();
        if (frontNow && frontNow->id == r->id) {
            Request* gone = 0;
            queue.dequeue(gone);
        } else {
            queue.removeById(r->id);
        }

        switch (action) {
            case PA_PROCESS_WITH_OP:
                r->assignedTo = operatorName;
                r->status = ST_PROCESSING;
                break;
            case PA_PROCESS_NO_OP:
                r->status = ST_PROCESSING;
                break;
            case PA_RESOLVE:
                r->status = ST_RESOLVED;
                r->adminNote = note;
                break;
            case PA_REJECT:
                r->status = ST_REJECTED;
                r->adminNote = note;
                break;
            default: break;
        }
        r->touch();

        ostringstream o;
        o << "Request #" << r->id << " -> " << statusName(r->status);
        if (!r->assignedTo.empty()) o << " (assigned to " << r->assignedTo << ")";
        o << ".";
        msg = o.str();
        return true;
    }

    /* ---- spec 7 : search -------------------------------------------------- */
    Request* findRequest(int id) {
        for (size_t i = 0; i < allRequests.size(); i++)
            if (allRequests[i]->id == id) return allRequests[i];
        return 0;
    }
    vector<Request*> byRegistration(const string& reg) const {
        vector<Request*> out;
        for (size_t i = 0; i < allRequests.size(); i++)
            if (allRequests[i]->studentReg == reg) out.push_back(allRequests[i]);
        return out;
    }
    vector<Request*> byStudent(const Account* st) const {
        vector<Request*> out;
        if (!st) return out;
        for (size_t i = 0; i < allRequests.size(); i++)
            if (allRequests[i]->studentReg == st->registrationNo) out.push_back(allRequests[i]);
        return out;
    }

    /* ---- spec 8 : update status ------------------------------------------ */
    bool changeStatus(int id, Status ns, const string& note, string& msg) {
        Request* r = findRequest(id);
        if (!r) { 
            ostringstream o;
            o << "No request found with ID #" << id << ".";
            msg = o.str();
            return false; 
        }
        if (r->isClosed()) {
            ostringstream o;
            o << "Request #" << r->id << " is already closed (" 
              << statusName(r->status) << ") and cannot be changed.";
            msg = o.str();
            return false;
        }
        /* a still-queued (Pending) request must leave the circular queue */
        if (r->status == ST_PENDING && ns != ST_PENDING) queue.removeById(r->id);

        r->status = ns;
        if (!note.empty()) r->adminNote = note;
        r->touch();
        
        ostringstream o;
        o << "Request #" << r->id << " status -> " << statusName(ns) << ".";
        msg = o.str();
        return true;
    }

    /* ---- spec 9 : statistics --------------------------------------------- */
    void statistics(int out[STATUS_COUNT]) const {
        for (int i = 0; i < STATUS_COUNT; i++) out[i] = 0;
        for (size_t i = 0; i < allRequests.size(); i++) out[allRequests[i]->status]++;
    }

    /* ---- sample data ------------------------------------------------------ */
    void loadSampleData() {
        if (!accounts.empty()) return;
        /* students: the three group members (roll number = registration number) */
        accounts.push_back(new Account("Ahmad", "1234", "Ahmad Hussain", "2500608", false, "Student"));
        accounts.push_back(new Account("hina",  "1234", "Jozza Batool",  "2500594", false, "Student"));
        accounts.push_back(new Account("usman", "1234", "Raja wasif",    "2500600", false, "Student"));
        accounts.push_back(new Account("admin",  "admin123", "Help Desk Admin", "", true, "Admin"));
        accounts.push_back(new Account("op1",    "op1pass",  "Kamran Sheikh",   "", true, "Operator"));
        accounts.push_back(new Account("op2",    "op2pass",  "Sadia Raza",      "", true, "Operator"));

        /* Note: Sample requests are now loaded from the database instead of hardcoded here.
           This ensures request IDs are persistent and survive server restarts. */
    }

    /* ---- add request from database (for persistence reload) --------------- */
    bool addRequestFromDB(Request* r) {
        // Add to allRequests
        allRequests.push_back(r);
        
        // Update nextRequestId to avoid collisions
        if (r->id >= nextRequestId) {
            nextRequestId = r->id + 1;
        }
        
        // If pending/processing, add to queue
        if (r->status == ST_PENDING || r->status == ST_PROCESSING) {
            if (queue.isFull()) return false;
            queue.enqueue(r);
        }
        
        return true;
    }
};

#endif // REQUEST_CORE_H
