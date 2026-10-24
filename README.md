# University Request Management System

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![SQLite](https://img.shields.io/badge/SQLite-3-green.svg)](https://www.sqlite.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)](https://www.microsoft.com/windows)

**A complete help desk management system with circular queue implementation, SQLite persistence, and web UI.**

📚 **Data Structures Assignment - Air University Islamabad**  
👥 **Group Members:**
- **Ahmad Hussain** (2500608)
- **Jozza Batool** (2500594)
- **Raja Wasif** (2500600)

📅 **Submission Date:** October 24, 2026  
🎓 **Course:** ADCS 3A - Data Structures

---

## 🌟 Features

### Core Data Structure
- **Circular Queue (FIFO)** - Capacity: 20
- Custom implementation with modulo arithmetic
- O(1) enqueue/dequeue operations
- Advanced operations: `removeById()`, `snapshot()`, `contains()`

### Backend
- **Native Winsock HTTP Server** (zero external dependencies)
- 12 RESTful JSON API endpoints
- Thread-safe concurrent request handling
- Session management with cookies

### Database (SQLite)
- **3-table schema**: Users, Requests, RequestHistory
- Complete audit trail of all status changes
- Foreign key constraints for data integrity
- Automatic ID generation (survives server restarts)

### Frontend
- Single-Page Application (SPA)
- Role-based views (Student/Admin)
- Responsive design with CSS gradients
- Real-time queue updates

---

## 📋 Requirements Met

✅ **1. Student Login** - Credential authentication  
✅ **2. Submit Requests** - Students can submit complaints  
✅ **3. Unique Request ID** - DB-generated integer IDs  
✅ **4. Circular Queue (FIFO)** - Oldest request processed first  
✅ **5. Admin Login** - Separate admin access  
✅ **6. Process Requests** - Assign to operators or resolve/reject  
✅ **7. Search Requests** - By ID or registration number  
✅ **8. Update Status** - Pending → Processing → Resolved/Rejected  
✅ **9. View Statistics** - Count by status  

**Extra Features:**
- ✨ Complete persistence (data survives restarts)
- ✨ Request history tracking
- ✨ Operator assignment system
- ✨ Direct status change (bypass queue)

---

## 🗂️ Project Structure

```
DSA/
├── RequestSystem/
│   ├── requestcore.h        (442 lines) - Core business logic + Circular Queue
│   ├── database.h           (350 lines) - SQLite persistence layer
│   ├── server.cpp           (780 lines) - HTTP server + API endpoints
│   ├── main.cpp             (testing)   - Console version
│   ├── sqlite3.h            - SQLite header
│   ├── public/
│   │   ├── index.html       (380 lines) - Frontend UI
│   │   ├── script.js        (450 lines) - Client-side logic
│   │   └── style.css        (280 lines) - Styling
│   └── RequestServer.exe    - Compiled server (runtime)
│
├── HelpDeskSystem/
│   ├── main.cpp             - DSA Assignment #02 (separate project)
│   └── README.md
│
├── COMPARISON_REPORT.md     - Comparison with reference implementation
└── README.md                - This file
```

---

## 🚀 Quick Start

### Prerequisites
- **Windows 10/11**
- **Microsoft Visual Studio 2022** (MSVC compiler)
- **SQLite** (included as amalgamation)

### Build

```bash
# Method 1: Build script (recommended)
cd RequestSystem
build.bat

# Method 2: Manual compilation
cd RequestSystem
cl /nologo /EHsc /W3 /std:c++17 /Fe:RequestServer.exe server.cpp sqlite3.c /link ws2_32.lib shell32.lib
```

### Run

```bash
cd RequestSystem
RequestServer.exe
```

Server starts at: **http://localhost:8080**  
Browser opens automatically.

---

## 👤 Login Credentials

### Students
| Name | Username | Password | Reg No |
|------|----------|----------|--------|
| Ahmad Hussain | `Ahmad` | `1234` | 2500608 |
| Jozza Batool | `hina` | `1234` | 2500594 |
| Raja Wasif | `usman` | `1234` | 2500600 |

### Admin & Operators
| Role | Username | Password |
|------|----------|----------|
| Admin | `admin` | `admin123` |
| Operator 1 | `op1` | `op1pass` |
| Operator 2 | `op2` | `op2pass` |

---

## 📊 Database Schema

### Users Table
```sql
CREATE TABLE Users (
    user_id         INTEGER PRIMARY KEY AUTOINCREMENT,
    registration_no TEXT UNIQUE NOT NULL,
    name            TEXT NOT NULL,
    password_hash   TEXT NOT NULL,
    role            TEXT NOT NULL
);
```

### Requests Table
```sql
CREATE TABLE Requests (
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
```

### RequestHistory Table
```sql
CREATE TABLE RequestHistory (
    history_id   INTEGER PRIMARY KEY AUTOINCREMENT,
    request_id   INTEGER NOT NULL,
    changed_by   TEXT NOT NULL,
    old_status   TEXT,
    new_status   TEXT NOT NULL,
    remarks      TEXT,
    changed_at   INTEGER NOT NULL,
    FOREIGN KEY(request_id) REFERENCES Requests(request_id)
);
```

---

## 🔌 API Endpoints

| Method | Endpoint | Description | Auth |
|--------|----------|-------------|------|
| POST | `/api/login` | Authenticate user | - |
| POST | `/api/logout` | End session | Session |
| GET | `/api/me` | Get current user | Session |
| POST | `/api/submit` | Submit new request | Student |
| GET | `/api/queue` | Get pending queue (FIFO) | Admin |
| GET | `/api/next` | Get oldest pending request | Admin |
| POST | `/api/process` | Process oldest request | Admin |
| GET | `/api/requests` | Get all/own requests | Session |
| GET | `/api/search` | Search by ID or reg no | Session |
| POST | `/api/status` | Update request status | Admin |
| GET | `/api/stats` | Get statistics | Admin |
| GET | `/api/operators` | Get operator list | Admin |

---

## 🧪 Testing Guide

### Test 1: Student Workflow
1. Login as `Ahmad` / `1234`
2. Submit request: Category = "Fees", Description = "Scholarship issue"
3. View "My Requests" - verify request appears with ID #1
4. Logout

### Test 2: Admin Workflow
1. Login as `admin` / `admin123`
2. View "Pending Queue" - see Ahmad's request (oldest first)
3. Click "Process Next Request"
4. Select "Assign to operator" → Choose `op1`
5. Click "Execute"
6. Verify request status changed to "Processing"

### Test 3: Persistence
1. Submit 3 requests as student
2. Note the request IDs
3. Stop server (Ctrl+C)
4. Restart server
5. Refresh browser → **All requests still present!**

---

## 🏗️ Architecture

### Circular Queue Implementation

```cpp
class CircularQueue {
public:
    static const int MAX = 20;
private:
    Request* arr[MAX];
    int front, rear, count;
public:
    bool enqueue(Request* r) {
        if (isFull()) return false;
        rear = (rear + 1) % MAX;  // Circular wrap
        arr[rear] = r;
        count++;
        return true;
    }
    
    bool dequeue(Request*& out) {
        if (isEmpty()) return false;
        out = arr[front];
        front = (front + 1) % MAX;  // Circular wrap
        count--;
        return true;
    }
};
```

### Request Flow

```
Student Submits Request
         ↓
   Save to Database (get ID)
         ↓
   Enqueue (add to rear)
         ↓
   Admin Views Queue (FIFO)
         ↓
   Process (dequeue from front)
         ↓
   Update Status in DB
         ↓
   Log to RequestHistory
```

---

## 🔧 Technical Highlights

### Zero External Dependencies
- No Crow framework
- No Boost libraries
- Pure C++ standard library + Winsock + SQLite amalgamation

### Thread Safety
```cpp
static CRITICAL_SECTION g_cs;
EnterCriticalSection(&g_cs);
// ... access shared state
LeaveCriticalSection(&g_cs);
```

### Modular Design
- **`requestcore.h`**: I/O-free business logic (testable, reusable)
- **`database.h`**: Isolated SQL operations
- **`server.cpp`**: HTTP/network layer only
- **Frontend**: Separate HTML/CSS/JS files

---

## 📈 Performance

| Metric | Value |
|--------|-------|
| Queue Operations | O(1) enqueue/dequeue |
| Database Queries | Indexed by primary keys |
| Concurrent Requests | Thread-safe with mutex |
| Memory Footprint | ~1.6 MB (compiled server) |
| Startup Time | < 1 second |

---

## 🆚 Comparison with Reference Implementation

See [COMPARISON_REPORT.md](COMPARISON_REPORT.md) for detailed analysis.

**Key Advantages:**
- ✅ 4x larger queue capacity (20 vs 5)
- ✅ Zero external dependencies (no Crow)
- ✅ Complete persistence with history tracking
- ✅ Advanced queue operations
- ✅ Professional frontend design

---

## 📝 Code Documentation

### Key Classes

**`CircularQueue`** - Core data structure
- FIFO order enforcement
- Modulo arithmetic for wrap-around
- Manual removal support

**`RequestSystem`** - Business logic container
- User authentication
- Request submission
- Queue processing
- Status management
- Search operations
- Statistics calculation

**`Request`** - Entity model
- Integer ID (DB primary key)
- Student info
- Category (Fees/Registration/Examinations/Other)
- Status (Pending/Processing/Resolved/Rejected)
- Timestamps (submitted, updated)
- Operator assignment

---

## 🐛 Known Limitations

- Windows-only (uses Winsock)
- Plain text passwords (educational project)
- No HTTPS (local development only)
- Fixed queue capacity (compile-time constant)

---

## 📚 References

- **Assignment Spec**: DSA Assignment #02.pdf
- **SQLite Documentation**: https://www.sqlite.org/docs.html
- **Winsock Programming**: Microsoft Docs
- **Circular Queue Theory**: Data Structures course notes

---

## 📄 License

This is an academic project for Air University Islamabad.  
**For educational purposes only.**

---

## 👥 Contributors

| Member | Roll No | Contribution |
|--------|---------|--------------|
| Ahmad Hussain | 2500608 | Core implementation, database design |
| Jozza Batool | 2500594 | Frontend UI, testing |
| Raja Wasif | 2500600 | API design, documentation |

---

## 📞 Contact

**Air University Islamabad**  
ADCS Batch 2025  
Data Structures Course  

---

**Built with ❤️ and lots of ☕**
