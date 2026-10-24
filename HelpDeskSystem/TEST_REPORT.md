# DSA Assignment #02 — Test Report

**System:** University Help Desk Management System
**Group:** 2500608 Ahmad Hussain · 2500594 Jozza Batool · 2500600 Raja wasif · **Term:** Spring 2026
**Compiler:** MSVC (Visual Studio 18, `/std:c++17 /W3`) — **0 warnings, 0 errors**
**Test run:** 88 scripted inputs covering every menu item · exit code **0** (no crashes)

---

## 1. Requirements Verification (against the PDF)

### Data Structures — 6/6 implemented

| # | Required structure | Implementation | Status |
|---|--------------------|----------------|--------|
| 1 | Priority-based Service Queue (linked list, high priority first) | `ServiceQueue` — singly linked list sorted by priority desc, then creation time asc; front = highest priority | ✅ |
| 2 | Self-balancing tree (AVL/Red-Black) for fast priority retrieval of tickets **within a category** | `PriorityAVL` — AVL with 4 rotation cases, `retrieveByCategory()` | ✅ |
| 3 | Binary max-heap for staff priority ranking based on resolutions | `StaffRankingHeap` — max-heap, O(n) build-heap | ✅ |
| 4 | Binary search tree for knowledge base: search by category + in/pre/post-order traversals | `KnowledgeBaseBST` — 3 traversal modes + category search | ✅ |
| 5 | Hash tables: tickets by student, staff by ID, students by ID | `HashTable` (separate chaining, FNV-1a) × **4 tables**: `studentTable`, `staffTable`, `ticketTable`, `ticketsByStudent` | ✅ |
| 6 | Doubly linked list storing history of **all** operations, searchable | `OperationLog` — forward/backward traversal + keyword search | ✅ |

### Core Requirements — 12/12 (a–l) verified with output

| Req | Description | Evidence from test run | Status |
|-----|-------------|------------------------|--------|
| **(a)** | Create ticket: unique auto-ID, validated student ID, category enum, free-text desc, 3 priority levels, created + updated timestamps | `TCK-1007` auto-generated; invalid ID rejected; invalid priority `9` rejected; `Created At: 2026-10-24` | ✅ |
| **(b)** | Update: 4-value status enum, modify priority, resolution notes, update timestamp, link KB article | 4× `[OK] Ticket TCK-1007 updated.` — status→In Progress, priority→Low, note added, `KB-LINKED TCK-1007 -> KB-002` | ✅ |
| **(c)** | Student view by ID, tickets **sorted by priority then creation time** | TCK-1001 (High) before TCK-1007 (Low); `Total: 2 \| Active: 1 \| Completed: 1` | ✅ |
| **(d)** | Staff retrieve **K highest-priority** tickets | K=3 peek → TCK-1001, TCK-1002, TCK-1003 (High, High, Medium); K=2 extract → staging batch | ✅ |
| **(e)** | Resolution stats per staff + re-ordered in heap after each batch | `1 STF-02 Kamran Sheikh 1 5.00` ranked first (was rank 6 before fix) | ✅ |
| **(f)** | AVL for fast priority-based retrieval within a category | `AVL retrieval -> 1 ticket(s) in 'IT Support' (priority order)` | ✅ |
| **(g)** | Tickets link to KB articles; BST search by category + 3 traversals | `KB-LINKED`; search Finance → 2 articles; Pre/In/Post-order all printed (9 articles) | ✅ |
| **(h)** | Hash tables for tickets-by-student, staff-by-ID, students-by-ID | `Hash tables: students=9 staff=6 tickets=7`; student view now O(1) | ✅ |
| **(i)** | Doubly linked list of all operations, searchable | 30 entries, newest→oldest, oldest→newest, search `TCK-1007` → `8 matching operation(s)` | ✅ |
| **(j)** | Insert new priority ticket at **correct position** in queue | TCK-1007 (Low) placed last: after High×2, Medium×3, Low×1 | ✅ |
| **(k)** | Remove resolved ticket from queue, keep in history archive | Queue 7→6 after resolve; archive shows `TCK-1007 ... 5/5` | ✅ |
| **(l)** | Re-order priority ranking heap after each **batch** of operations | `HEAP-REBALANCE Staff ranking heap re-ordered after batch of 5 operations` (auto-triggered ×2) | ✅ |

### Sample Data — minimums met/exceeded

| Entity | Required | Provided |
|--------|----------|----------|
| Students | 8 | **8** (STD-2026-01…08) ✅ |
| Staff | 5 | **6** (STF-01…06) ✅ |
| Tickets | 6 | **6** (TCK-1001…1006) ✅ |
| Knowledge base articles | 4 | **8** (KB-001…008) ✅ |

### Validation & Error Handling — 7/7 passed

| Case | Output |
|------|--------|
| Invalid student ID | `[ERROR] Student ID 'STD-2026-99' not found. Register the student first (Menu 7 -> 2).` |
| Invalid priority (9) | `[ERROR] Invalid input! Enter a number between 1 and 3.` |
| Re-resolving a resolved ticket | `[ERROR] Already resolved.` |
| Rating an unresolved ticket | `[ERROR] Only resolved/closed tickets can be rated.` |
| Non-existent ticket | `[ERROR] Ticket not found.` |
| Invalid menu choice (99) | `[ERROR] Invalid input! Enter a number between 0 and 20.` |
| Non-numeric menu input (abc) | `[ERROR] Invalid input! Enter a number between 0 and 20.` |

---

## 2. Sample Output

### (a) Ticket creation
```
[OK] Ticket TCK-1007 created and queued.

  | Ticket TCK-1007     Status: In Progress          |
  +----------------------------------------------------------+
  Student ID    : STD-2026-01
  Category      : Finance
  Priority      : Low
  Description   : Tuition fee refund not processed for spring semester
  Created At    : 2026-10-24 14:36:20
  Last Updated  : 2026-10-24 14:36:20
  Resolution    : Refund initiated via finance office
  Linked KB Articles: KB-002
```

### (j) Service queue — correct priority positions
```
# 1 TCK-1001 STD-2026-01 IT Support High   Open 2026-10-20 09:15:00
# 2 TCK-1002 STD-2026-02 Finance    High   Open 2026-10-20 10:15:00
# 3 TCK-1003 STD-2026-03 Hostel     Medium Open 2026-10-20 11:15:00
# 4 TCK-1004 STD-2026-04 Academics  Medium Open 2026-10-20 12:15:00
# 5 TCK-1006 STD-2026-06 Library    Medium Open 2026-10-20 14:15:00
# 6 TCK-1005 STD-2026-05 Transport  Low    Open 2026-10-20 13:15:00
# 7 TCK-1007 STD-2026-01 Finance    Low    In Progress 2026-10-24 14:36:20
```
TCK-1007 was created **High**, then lowered to **Low** — it correctly moved to position 7 (requirement j).

### (d) K highest-priority retrieval
```
How many (K) highest-priority tickets? 3
Top 3 highest-priority tickets:
TCK-1001  IT Support  High    Open   Cannot access student portal after pas
TCK-1002  Finance     High    Open   Tuition fee payment not reflected in L
TCK-1003  Hostel      Medium  Open   Room allotment letter not received yet
```

### (c) Student tickets sorted by priority, then creation time
```
Tickets for Anas Ali (sorted by priority, then time):
TCK-1001  IT Support  High  Open     2026-10-20 09:15:00
TCK-1007  Finance     Low   Resolved 2026-10-24 14:36:20
Total: 2  |  Active: 1  |  Completed: 1
```

### (e) Staff ranking — binary max-heap
```
Rank Staff ID  Name            Resolved  Avg Rating  Skills
     1 STF-02    Kamran Sheikh   1         5.00        Finance
     2 STF-01    Sadia Raza      0         0.00        Academics, IT Support
     3 STF-03    Nadia Aslam     0         0.00        Hostel, Transport
     4 STF-04    Imran Javed     0         0.00        IT Support
     5 STF-05    Rabia Sultan    0         0.00        Academics, Library
     6 STF-06    Tariq Mehmood   0         0.00        Health
```

### (g) Knowledge base — BST search by category
```
Choose category: Finance
2 article(s) found:
[KB-002] Understanding your fee challan  (Finance)  views: 1
[KB-009] Fee refund processing timeline  (Finance)  views: 0
```

### (g) BST traversals (all three orders)
```
Pre-order  (9): KB-001, KB-002, KB-005, KB-003, KB-009, KB-004, KB-006, KB-007, KB-008
In-order   (9): KB-005, KB-002, KB-009, KB-003, KB-004, KB-001, KB-008, KB-007, KB-006
Post-order (9): KB-005, KB-009, KB-004, KB-003, KB-002, KB-008, KB-007, KB-006, KB-001
```

### (i) Doubly linked list — operation log, newest → oldest
```
#30  2026-10-24  KB-SEARCHED      category Finance (2 hits)
#29  2026-10-24  KB-ADDED         KB-009 [Finance] Fee refund processing timeline
#28  2026-10-24  BATCH-RETURNED   2 tickets re-queued in priority order
#27  2026-10-24  TOP-K-EXTRACTED  2 tickets moved to staging batch
#26  2026-10-24  HEAP-REBALANCE   Staff ranking heap re-ordered after batch of 5 operations
...
#15  2026-10-24  TICKET-CREATED   TCK-1007 [Finance/High] by STD-2026-01
```
Keyword search `TCK-1007` → **8 matching operation(s)**.

### (k)+(l) Resolve → archive, heap auto-rebalance
```
[OK] Ticket TCK-1007 resolved & moved to archive.

=== HISTORY ARCHIVE (1) ===
TCK-1007  STD-2026-01  Finance  STF-02  5/5  2026-10-24 14:36:20

HEAP-REBALANCE  Staff ranking heap re-ordered after batch of 5 operations
```

### (h) Statistics report
```
TOTALS: tickets=7  in-queue=6  archived=1  students=9  staff=6  KB-articles=9
AVL index: nodes=7  height=4  (balanced: O(log n) priority retrieval)
Hash tables: students=9  staff=6  tickets=7
Operation log entries: 30
```

---

## 3. Bugs Found & Fixed During Testing

| # | Bug | Impact | Fix |
|---|-----|--------|-----|
| 1 | **Heap ranking displayed in reverse** — heapsort leaves the array ascending w.r.t. the comparator, but I printed front-to-back | The staff member with the *most* resolutions ranked **last** (violated req. e/l) | Print from index `m-1` down to `0` so rank 1 = best |
| 2 | **Priority change didn't reposition the ticket** in the queue or AVL (both keyed by priority) | A ticket lowered from High→Low stayed at the queue front; AVL order corrupted (violated req. j/f) | On priority change: remove + re-insert into both queue and AVL |
| 3 | **Resolved ticket could be re-queued** from the staging batch | An archived ticket reappeared as active (violated req. k) | Remove from batch during resolve; skip resolved tickets on batch return |
| 4 | **Queue `rear` pointer never updated** when appending at the tail | Tail O(1) append broken | Test `node->next == 0` instead of the stale `prev->next` |
| 5 | **Tickets-by-student was a linear scan**, not a hash table | Violated requirement (h) | Added 4th hash table `ticketsByStudent` with per-student buckets |
| 6 | Invalid `Student ID` message pointed to a non-existent menu | UX confusion | Added student registration (Menu 7 → 2) |

---

## 4. Verdict

| Category | Result |
|----------|--------|
| Data structures (6 required) | **6/6** ✅ |
| Core requirements (a–l) | **12/12** ✅ |
| Sample data minimums | **4/4 met or exceeded** ✅ |
| Input validation cases | **7/7 passed** ✅ |
| Compile warnings | **0** ✅ |
| Test runs (crashes) | **0 crashes** ✅ |

**The system fulfills all requirements of DSA Assignment #02.**

### Files
- `HelpDeskSystem/main.cpp` — full source (~1,500 lines, single file, commented against requirements a–l)
- `HelpDeskSystem/HelpDeskSystem.exe` — compiled executable
- `HelpDeskSystem/README.md` — build instructions + requirement coverage map
- `HelpDeskSystem/TEST_REPORT.md` — this report
