# Test Report — University Request Management System (Circular Queue)

**Group:** 2500608 Ahmad Hussain · 2500594 Jozza Batool · 2500600 Raja wasif · **Term:** Spring 2026
**Compiler:** MSVC (Visual Studio 18, `/std:c++17 /W3`) — **0 warnings, 0 errors**
**Test runs:** 2 (functional suite + circular-queue stress) · **exit code 0, no crashes**

---

## 1. Requirement Verification — 9/9

| # | Requirement | Evidence from test run | Status |
|---|-------------|------------------------|--------|
| 1 | Student Login with their own account | `Welcome, Anas Ali (Student)` — wrong password first: `[ERROR] Invalid username/password. Try again.` | ✅ |
| 2 | Submit complaints (fees / registration / examinations / other) | Submitted under `Fees` and `Other`; categories enforced by menu range | ✅ |
| 3 | Every request gets a unique ID | Auto-generated `REQ-0005`, `REQ-0006` (seeds `REQ-0001`…`REQ-0004`) | ✅ |
| 4 | Circular queue, **FIFO — oldest first** | Queue showed `1 REQ-0001 09:15` … `5 REQ-0006` with `-> Next to process (oldest): REQ-0001`; 45 requests processed in strict submission order | ✅ |
| 5 | Admin login, separate from students | `Welcome, Help Desk Admin (Admin)` — distinct credential set and menu | ✅ |
| 6 | View pending, process next, assign operator if needed | `Next request in FIFO order` → optional operator pick → `REQ-0001 is now Processing (assigned to Kamran Sheikh)`; other runs skipped assignment (`REQ-0002 is now Processing.`) | ✅ |
| 7 | Search by request ID **or** registration number | `REQ-0001` found / `REQ-9999` not found; `STD-2026-03` → 1 request / `STD-9999` not found | ✅ |
| 8 | Update status to Processing, Resolved, Rejected | `REQ-0005 status -> Resolved.`, `REQ-0006 status -> Rejected.`; closed request protected: `already closed (Resolved) and cannot be changed` | ✅ |
| 9 | Statistics: pending, processing, resolved, rejected | `Pending: 0 · Processing: 3 · Resolved: 2 · Rejected: 1 · Total: 6` | ✅ |

---

## 2. Sample Output

### Requirement 4 — FIFO queue view (oldest first)
```
PENDING REQUESTS  (pending: 5/20)
Pos Request ID   Reg No        Category      Submitted            Description
---------------------------------------------------------------
  1 REQ-0001     STD-2026-01   Fees          2026-10-20 09:15:00  Scholarship waiver not app
  2 REQ-0002     STD-2026-02   Registration  2026-10-20 10:15:00  Course registration stuck
  3 REQ-0003     STD-2026-03   Examinations  2026-10-20 11:15:00  Roll number slip missing f
  4 REQ-0005     STD-2026-01   Fees          2026-10-24 14:44:11  Scholarship waiver not app
  5 REQ-0006     STD-2026-01   Other         2026-10-24 14:44:11  Cafeteria remain open till
-> Next to process (oldest): REQ-0001
```

### Requirement 6 — process next + operator assignment
```
[OK] REQ-0001 is now Processing (assigned to Kamran Sheikh).
[OK] REQ-0002 is now Processing.
[OK] REQ-0003 is now Processing.
```

### Requirement 7 — both search modes
```
Request ID: REQ-0001  ->  | Request REQ-0001   Processing |
Reg No: STD-2026-03   ->  REQ-0003  Examinations  Processing
Reg No: STD-9999      ->  [ERROR] No requests found for registration number 'STD-9999'.
```

### Requirement 9 — statistics
```
+--------------------------------------+
|       REQUEST STATISTICS             |
+--------------------------------------+
Pending    :    0   (in circular queue: 0/20)
Processing :    3
Resolved   :    2
Rejected   :    1
----------------------------------------
Total      :    6
```

---

## 3. Circular Queue Stress Test (wrap-around + capacity)

The critical property of a circular queue is that `front`/`rear` wrap back to
index 0 via modulo. The harness ran **25 submit→process cycles** (forcing both
indices past index 19 several times), then filled the queue to capacity, then
drained it completely.

| Check | Expected | Result |
|-------|----------|--------|
| Accepted submissions | 42 (25 wrap + 17 fill) | **42** ✅ |
| Rejections when full | 4 (3 fill + 1 overflow) | **4** ✅ |
| Invalid menu inputs during stress | 0 | **0** ✅ |
| Bad logins during stress | 0 | **0** ✅ |
| Processed in FIFO order | REQ-0001…REQ-0046, strictly sequential | **45/45 perfect** ✅ |
| Final counts | Pending 0, Processing 45, Resolved 1, Rejected 0, Total 46 | **exact** ✅ |

```
[ERROR] Queue is full (20 pending requests). Please try again later.

FIFO ORDER PERFECT: processed REQ-0001..REQ-0046 in strict submission
order (REQ-0004 skipped, seeded Resolved)
First 6: 1, 2, 3, 5, 6, 7      Last 6: 41, 42, 43, 44, 45, 46
```

---

## 4. Error Handling — 11/11 passed

| Case | Output |
|------|--------|
| Wrong password | `[ERROR] Invalid username/password. Try again.` |
| 3 failed logins | `[ERROR] Invalid username/password. Account locked for this session.` |
| Queue full (capacity 20) | `[ERROR] Queue is full (20 pending requests). Please try again later.` |
| Process empty queue | `[ERROR] No pending requests to process.` |
| Search unknown request ID | `[ERROR] No request found with ID 'REQ-9999'.` |
| Search unknown registration | `[ERROR] No requests found for registration number 'STD-9999'.` |
| Update a closed request | `[ERROR] This request is already closed (Resolved) and cannot be changed.` |
| Student viewing another student's request | `[ERROR] That request belongs to another student.` |
| Non-numeric menu input | `[ERROR] Invalid input! Enter a number between …` |
| Out-of-range menu input | `[ERROR] Invalid input! Enter a number between …` |
| Empty text field | `[ERROR] Value cannot be empty.` |

---

## 5. Notes on bugs found during testing

| # | Issue | Where | Resolution |
|---|-------|-------|------------|
| 1 | A pending request closed by hand would stay in the queue, so FIFO would keep offering an already-Resolved item | `updateStatus()` | Added `queue.removeById()` compaction before changing status out of *Pending* |
| 2 | Harness bug (not a program bug): stray leading admin login fed menu bytes to the wrong menu, causing 3 false `Invalid input` errors and a miscount of rejections | test script | Fixed the harness; re-run produced exact expected counts (42/4/0/0) |

---

## 6. Web (HTML/CSS/JS) Version — API tests

The same C++ core (`requestcore.h`) is served over HTTP by `server.cpp`; the UI
is plain HTML/CSS/JS in the browser. Server built with **0 warnings** and driven
end-to-end through its JSON API.

| Check | Request | Result |
|-------|---------|--------|
| Static frontend served | `GET /`, `/style.css`, `/script.js` | **200 OK** for all three ✅ |
| Anonymous blocked from admin data | `GET /api/stats` (no session) | `{"ok":false,"error":"Admin access required."}` ✅ |
| Admin login (spec 1/5) | `POST /api/login` admin/admin123 | `{"ok":true,"name":"Help Desk Admin","isAdmin":true}` ✅ |
| Bad login rejected | `POST /api/login` wrong password | `Invalid username/password for student login.` ✅ |
| FIFO queue, oldest first (spec 4/6) | `GET /api/queue` | `size:3`, `"next":"REQ-0001"`, items in submit order ✅ |
| Resolve without note blocked (spec 6) | `POST /api/process` resolve, empty note | `An admin note is required to resolve or reject a request.` ✅ |
| Assign to operator (spec 6) | `POST /api/process` `process_op`/`op1` | `REQ-0001 -> Processing (assigned to Kamran Sheikh).` ✅ |
| Queue advances after processing | `GET /api/next` | now returns `REQ-0002` ✅ |
| Search by registration (spec 7) | `GET /api/search?reg=STD-2026-01` | 1 hit, status `Processing` ✅ |
| Student submit (spec 2/3) | `POST /api/submit` Examinations | `REQ-0005 submitted. Position in queue: 3` ✅ |
| Student blocked from admin API | `GET /api/queue` as student | `Admin access required.` ✅ |
| Statistics (spec 9) | `GET /api/stats` | `pending:2 processing:1 resolved:1 total:4` ✅ |

Frontend consistency was also checked mechanically: **all 61 element IDs that
`script.js` references exist in `index.html`** (0 missing).

---

## 7. Verdict

| Category | Result |
|----------|--------|
| Specification items (1–9) | **9/9** ✅ |
| Circular-queue stress (wrap + capacity + FIFO) | **perfect** ✅ |
| Error-handling cases | **11/11** ✅ |
| Web API checks | **12/12** ✅ |
| Compile warnings (console + server) | **0** ✅ |
| Crashes across ~500 scripted inputs | **0** ✅ |

**The system fulfills all requirements of the circular-queue specification,
in both the console and the web (HTML/CSS/JS) interface.**

### Files
- `requestcore.h` — shared C++ core: data structures + all business rules (no I/O)
- `main.cpp` — console interface (requirements 1–9)
- `server.cpp` — HTTP backend for the web interface (Winsock + JSON API)
- `public/` — web frontend: `index.html`, `style.css`, `script.js`
- `RequestSystem.exe`, `RequestServer.exe` — compiled executables
- `README.md` — build instructions, logins, design notes
- `TEST_REPORT.md` — this report
