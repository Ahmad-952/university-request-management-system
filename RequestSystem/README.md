# University Request Management System — Circular Queue (FIFO)

**Companion project to DSA Assignment #02 — 2500608 Ahmad Hussain · 2500594 Jozza Batool · 2500600 Raja wasif, Spring 2026**

This is a **separate** program from `HelpDeskSystem` (the priority-based DSA
assignment). It implements the circular-queue request workflow described in the
second specification, and does not modify that project in any way.

## Build & Run

### Console version

MSVC (Developer Command Prompt):

```bat
cl /nologo /EHsc /W3 /std:c++17 /Fe:RequestSystem.exe main.cpp
RequestSystem.exe
```

g++:

```bash
g++ -std=c++17 -O2 -o requestsys main.cpp
./requestsys
```

### Web (HTML/CSS/JS) version

The same C++ core (`requestcore.h`) is also exposed as a small HTTP backend, so
the user interface is plain **HTML + CSS + JavaScript** running in the browser.
The circular-queue / FIFO rules still live in C++ — the web layer only renders
state and collects input.

```bat
cl /nologo /EHsc /W3 /std:c++17 /Fe:RequestServer.exe server.cpp /link ws2_32.lib shell32.lib
RequestServer.exe
```

The server binds `http://localhost:8080` (falls back to 8085 if busy) and opens
your browser automatically. Stop it with `Ctrl+C` in its console window.

| File | Purpose |
|------|---------|
| `requestcore.h` | Shared C++ core — data structures + all business rules (no I/O) |
| `server.cpp` | Winsock HTTP server + JSON API (built with `ws2_32.lib`) |
| `public/index.html` | Page structure (login, student, admin panels) |
| `public/style.css` | Styling, status colour-coding, responsive layout |
| `public/script.js` | Fetches the JSON API, renders lists, validates all input |

**JSON API** (session token required except login):

```
POST /api/login      {username, password, role}     spec 1 / 5
POST /api/submit     {category, description}        spec 2 / 3
GET  /api/queue      FIFO queue, oldest first        spec 4 / 6
GET  /api/next       oldest pending request          spec 6
POST /api/process    {action, operator, note}        spec 6
GET  /api/requests   student: own · admin: all
GET  /api/search?id=  or  ?reg=                      spec 7
POST /api/status     {id, status, note}              spec 8
GET  /api/stats      per-status counts               spec 9
```

Admin-only endpoints reject student sessions, and resolve/reject actions are
rejected without an admin note — the same rules the console version enforces.

Sample data loads automatically: **6 students, 1 admin, 2 operators, 4 requests**
(3 pending in the queue, 1 already resolved).

## Logins

| Role | Username | Password |
|------|----------|----------|
| Student | `Ahmad` | `1234` |
| Admin | `admin` | `admin123` |
| Operator | `op1`, `op2` | `op1pass`, `op2pass` |

## Specification Coverage

| # | Requirement | Implementation |
|---|-------------|----------------|
| 1 | **Student Login** — students log in using their accounts | `login(false)`, 3 attempts, student-only credentials |
| 2 | **Submit Requests** — complaints about fees, registration, examinations, or other | `submitRequest()`, categories: Fees / Registration / Examinations / Other |
| 3 | **Unique Request ID** — every request gets a unique ID | auto-generated `REQ-0001`, `REQ-0002`, … (monotonic counter) |
| 4 | **Circular Queue in C++** — FIFO, oldest request processed first | `CircularQueue`: fixed `arr[20]`, `front`/`rear`/`count`, modulo wrap-around |
| 5 | **Admin Login** — admins/operators log in separately | `login(true)`, distinct credential set, separate menu |
| 6 | **Process Requests** — view pending, process next, assign to operator if needed | `processNext()`: `peek()` the oldest → optional operator assignment → `dequeue()` → status *Processing* |
| 7 | **Search Requests** — by request ID or student registration number | `searchById()`, `searchByReg()` |
| 8 | **Update Status** — Processing, Resolved, Rejected | `updateStatus()`, also removes still-queued items from the FIFO |
| 9 | **View Statistics** — pending, processing, resolved, rejected counts | `statistics()` with per-status counts + queue occupancy |

## Circular Queue Design

```
enqueue()  -> rear = (rear + 1) % MAX;  arr[rear] = request;  count++
dequeue()  -> out = arr[front];  front = (front + 1) % MAX;   count--
```

- `front` always points at the **oldest** pending request → strict FIFO
- modulo arithmetic wraps indices back to index 0, reusing freed slots
- `count` distinguishes *empty* from *full* (both would otherwise look identical)
- `removeById()` compacts the remaining elements when a pending request is
  closed by hand, so FIFO order is never corrupted

## Menus

**Student:** submit a request · view my requests · view request details by ID · logout

**Admin/Operator:** view pending queue (oldest first) · process next request (FIFO) ·
search by request ID · search by registration number · update status ·
view statistics · view all requests · logout

## Files

- `requestcore.h` — shared C++ core (data structures + business rules, no I/O)
- `main.cpp` — console version, commented against requirements 1–9
- `server.cpp` — HTTP backend for the web version (Winsock + JSON API)
- `public/` — web frontend: `index.html`, `style.css`, `script.js`
- `RequestSystem.exe`, `RequestServer.exe` — compiled executables
- `TEST_REPORT.md` — test results
