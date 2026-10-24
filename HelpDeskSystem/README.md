# University Help Desk Management System

**DSA Assignment #02 — 2500608 Ahmad Hussain · 2500594 Jozza Batool · 2500600 Raja wasif, Spring 2026**

A complete C++ console application implementing every requirement of the
assignment, built on hand-implemented data structures (no STL containers used
for the core structures).

## Build & Run

Using MSVC (Visual Studio Developer Command Prompt):

```bat
cl /nologo /EHsc /std:c++17 /Fe:HelpDeskSystem.exe main.cpp
HelpDeskSystem.exe
```

Using g++:

```bash
g++ -std=c++17 -O2 -o helpdesk main.cpp
./helpdesk
```

Sample data (8 students, 6 staff, 6 tickets, 8 KB articles) loads automatically
at startup.

## Requirements Coverage

| Req | Feature | Implementation |
|-----|---------|----------------|
| (a) | Add ticket: unique auto ID, validated student ID, category, description, 3 priority levels, created/updated timestamps | `HelpDeskSystem::addTicket`, `Ticket` |
| (b) | Update ticket: 4-value status, priority change, resolution notes, timestamp refresh, KB article linking | `HelpDeskSystem::updateTicket` |
| (c) | View all tickets of a student, sorted by priority then creation time | `HelpDeskSystem::viewStudentTickets` |
| (d) | Service queue: retrieve K highest-priority tickets (peek or extract to staging batch) | `ServiceQueue` (sorted singly linked list) |
| (e) | Resolution statistics per staff member + re-ordered ranking | `StaffRankingHeap` (binary max-heap) |
| (f) | AVL tree for fast priority-based retrieval of tickets in a category | `PriorityAVL` (self-balancing, with rotations) |
| (g) | Knowledge base: BST by category with pre/in/post-order traversals, linked to tickets | `KnowledgeBaseBST` |
| (h) | Hash tables for tickets-by-student, staff-by-ID, students-by-ID | `HashTable` (separate chaining, FNV-1a) |
| (i) | Doubly linked list of all operations, searchable | `OperationLog` |
| (j) | Insert new priority ticket at correct queue position | `ServiceQueue::insertSorted` |
| (k) | Remove resolved ticket from queue, keep in history archive | `HelpDeskSystem::resolveTicketById` + `ArchiveList` |
| (l) | Re-order the staff priority ranking heap after each batch of operations | `StaffRankingHeap::rebuild`, auto-triggered every 5 ops |

## Menu Map

- **1–6** Ticket lifecycle: create, view, update, assign (workload-matched to
  specialized staff), resolve → archive, rate satisfaction
- **7** Student: view tickets (priority/time sorted) or register a new student
- **8–11** Service queue: full view, retrieve top-K, view by category,
  staging batch view/return
- **12–15** Reports: staff ranking (heap), AVL category query, statistics,
  history archive
- **16–18** Knowledge base: add article, search by category, tree traversals
- **19–20** Operation log: view (both directions), keyword search

## Data Structures Summary

- **ServiceQueue** — singly linked list kept sorted by priority (desc), then
  creation time (asc); front is always the highest-priority ticket
- **PriorityAVL** — AVL tree keyed by (priority, creation time, ID); supports
  O(log n) insert/remove and highest-priority extraction
- **StaffRankingHeap** — binary max-heap keyed by tickets resolved (then avg
  rating); rebuilt after every batch of 5 operations
- **KnowledgeBaseBST** — BST keyed by (category, article ID); category range
  search plus all three traversal orders
- **HashTable** — separate chaining with FNV-1a hashing (students, staff, tickets)
- **OperationLog / ArchiveList** — doubly linked lists with forward/backward
  traversal and keyword search
