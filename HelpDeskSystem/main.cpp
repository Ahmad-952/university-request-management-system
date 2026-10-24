/*==============================================================================
  University Help Desk Management System
  Assignment #02 - Data Structures & Algorithms (DSA)
  Author : 2500608 Ahmad Hussain | 2500594 Jozza Batool | 2500600 Raja wasif
  Term   : Spring 2026

  Built with the required data structures:
    (a)-(d,f) Priority-based Service Queue  -> sorted Singly Linked List (front = highest)
    (f)       Priority Index                -> Self-Balancing AVL Tree
    (e)       Staff Priority Ranking        -> Binary Max-Heap
    (g)       Knowledge Base                -> Binary Search Tree (3 traversals)
    (h)       Fast Lookup                   -> Hash Tables (separate chaining)
    (i)       Operation Log                 -> Doubly Linked List
    (k)       History Archive               -> Doubly Linked List
==============================================================================*/

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>

using namespace std;

/* =============================================================================
   SECTION 1 : UTILITY (Date & Time)
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
    /* Single comparable integer: YYYYMMDDHHMMSS */
    long key() const { return (((long)y * 100 + mo) * 100 + d) * 1000000L +
                              h * 10000L + mi * 100L + s; }
    static int compare(const DateTime& a, const DateTime& b) {
        if (a.key() < b.key()) return -1;
        if (a.key() > b.key()) return 1;
        return 0;
    }
    string str() const {
        ostringstream o;
        o << setfill('0') << setw(4) << y << "-" << setw(2) << mo << "-"
          << setw(2) << d << " " << setw(2) << h << ":" << setw(2) << mi
          << ":" << setw(2) << s;
        return o.str();
    }
};

/* ---- Input validation helpers -------------------------------------------- */
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
string pad(const string& s, int w) {
    if ((int)s.size() >= w) return s.substr(0, w - 1) + ".";
    return s + string(w - s.size(), ' ');
}

/* =============================================================================
   SECTION 2 : DOMAIN ENUMS
   ============================================================================= */
enum Category {
    CAT_ACADEMICS = 0, CAT_FINANCE, CAT_HOSTEL, CAT_TRANSPORT,
    CAT_IT_SUPPORT, CAT_HEALTH, CAT_LIBRARY, CAT_OTHERS, CAT_COUNT
};
static const char* CATEGORY_NAMES[CAT_COUNT] = {
    "Academics", "Finance", "Hostel", "Transport",
    "IT Support", "Health", "Library", "Others"
};
string catName(Category c) {
    if (c < 0 || c >= CAT_COUNT) return "Unknown";
    return CATEGORY_NAMES[c];
}

enum Priority { PRI_LOW = 1, PRI_MEDIUM = 2, PRI_HIGH = 3 };
string priName(Priority p) {
    if (p == PRI_HIGH) return "High";
    if (p == PRI_MEDIUM) return "Medium";
    return "Low";
}

enum Status { ST_OPEN = 0, ST_INPROGRESS, ST_RESOLVED, ST_CLOSED, ST_COUNT };
static const char* STATUS_NAMES[ST_COUNT] = {
    "Open", "In Progress", "Resolved", "Closed"
};
string statusName(Status s) {
    if (s < 0 || s >= ST_COUNT) return "Unknown";
    return STATUS_NAMES[s];
}

/* =============================================================================
   SECTION 3 : CORE ENTITIES
   ============================================================================= */
class Student {
public:
    string id, name, email, department, phone;
    Student() {}
    Student(const string& i, const string& n, const string& e,
            const string& dep, const string& ph)
        : id(i), name(n), email(e), department(dep), phone(ph) {}
    void display() const {
        cout << "  ID         : " << id << "\n  Name       : " << name
             << "\n  Email      : " << email
             << "\n  Department : " << department
             << "\n  Phone      : " << phone << "\n";
    }
};

class Staff {
public:
    string id, name, email, role, shift;
    bool skills[CAT_COUNT];
    int skillCount;
    int ticketsResolved;          // resolution counter  -> used by (e) heap
    int ratingsSum, ratingsCount; // satisfaction

    Staff() { reset(); }
    void reset() {
        skillCount = 0; ticketsResolved = 0; ratingsSum = 0; ratingsCount = 0;
        for (int i = 0; i < CAT_COUNT; i++) skills[i] = false;
    }
    Staff(const string& i, const string& n, const string& e,
          const string& r, const string& sh) : id(i), name(n), email(e),
          role(r), shift(sh) { reset(); }

    bool hasSkill(Category c) const { return skills[c]; }
    void addSkill(Category c) { if (!skills[c]) { skills[c] = true; skillCount++; } }

    double avgRating() const {
        return ratingsCount == 0 ? 0.0 : (double)ratingsSum / ratingsCount;
    }
    string skillList() const {
        string out;
        for (int i = 0; i < CAT_COUNT; i++)
            if (skills[i]) { if (!out.empty()) out += ", "; out += CATEGORY_NAMES[i]; }
        return out.empty() ? "(none)" : out;
    }
    void display() const {
        cout << "  " << pad(id, 10) << pad(name, 16) << pad(role, 14)
             << pad(shift, 10) << "Resolved: " << setw(3) << ticketsResolved
             << "  Rating: " << fixed << setprecision(1) << avgRating()
             << "  Skills: " << skillList() << "\n";
    }
};

class Ticket {
public:
    string id;            // (a) unique auto-generated
    string studentId;     // (a) validated
    Category category;    // (a) predefined categories
    string description;   // (a) free-text
    Priority priority;    // (a) 3 levels
    Status status;        // (b) 4 values
    DateTime created;     // (a)
    DateTime updated;     // (b) updated on every change
    string assignedStaff; // resolution details
    string resolutionNotes;
    string linkedArticles[8];
    int articleCount;
    int rating;           // 0 = not rated yet

    Ticket() : category(CAT_OTHERS), priority(PRI_LOW), status(ST_OPEN),
               articleCount(0), rating(0) {}

    bool linkArticle(const string& artId) {
        if (articleCount >= 8) return false;
        for (int i = 0; i < articleCount; i++)
            if (linkedArticles[i] == artId) return false; // already linked
        linkedArticles[articleCount++] = artId;
        return true;
    }
    void touch() { updated = DateTime::now(); }

    void displayBrief() const {
        cout << "  " << pad(id, 11) << pad(studentId, 15) << pad(catName(category), 12)
             << pad(priName(priority), 8) << pad(statusName(status), 12)
             << created.str() << "  " << pad(description, 34) << "\n";
    }
    void displayFull() const {
        cout << "  +----------------------------------------------------------+\n"
             << "  | Ticket " << pad(id, 12) << " Status: " << pad(statusName(status), 12) << "         |\n"
             << "  +----------------------------------------------------------+\n"
             << "  Student ID    : " << studentId << "\n"
             << "  Category      : " << catName(category) << "\n"
             << "  Priority      : " << priName(priority) << "\n"
             << "  Description   : " << description << "\n"
             << "  Created At    : " << created.str() << "\n"
             << "  Last Updated  : " << updated.str() << "\n";
        if (!assignedStaff.empty())
            cout << "  Assigned To   : " << assignedStaff << "\n";
        if (!resolutionNotes.empty())
            cout << "  Resolution    : " << resolutionNotes << "\n";
        if (rating > 0)
            cout << "  Satisfaction  : " << rating << "/5\n";
        cout << "  Linked KB Articles: ";
        if (articleCount == 0) cout << "(none)";
        for (int i = 0; i < articleCount; i++) {
            if (i) cout << ", ";
            cout << linkedArticles[i];
        }
        cout << "\n";
    }
};

class KBArticle {
public:
    string id, title, body;
    Category category;
    int views;
    KBArticle() : category(CAT_OTHERS), views(0) {}
    void display() const {
        cout << "  [" << id << "] " << title << "  (" << catName(category)
             << ")  views: " << views << "\n      " << body << "\n";
    }
};

/* =============================================================================
   SECTION 4 : DATA STRUCTURES
   ---------------------------------------------------------------------------*/

/* ---- (h) Hash Table with separate chaining (string key -> entity) --------- */
template <class V>
class HashTable {
    struct Node {
        string key; V* val; Node* next;
        Node(const string& k, V* v) : key(k), val(v), next(0) {}
    };
    vector<Node*> buckets;
    int cnt;

    size_t h(const string& k) const {           // FNV-1a hash
        size_t hash = 14695981039346656037ULL;
        for (size_t i = 0; i < k.size(); i++) {
            hash ^= (unsigned char)k[i];
            hash *= 1099511628211ULL;
        }
        return hash % buckets.size();
    }
public:
    HashTable(int n = 37) : buckets(n, 0), cnt(0) {}
    ~HashTable() { clear(); }

    void insert(const string& key, V* val) {
        size_t b = h(key);
        for (Node* p = buckets[b]; p; p = p->next)
            if (p->key == key) { p->val = val; return; } // update
        buckets[b] = new Node(key, val);
        cnt++;
    }
    V* find(const string& key) const {
        for (Node* p = buckets[h(key)]; p; p = p->next)
            if (p->key == key) return p->val;
        return 0;
    }
    bool remove(const string& key) {
        size_t b = h(key);
        Node* prev = 0;
        for (Node* p = buckets[b]; p; prev = p, p = p->next)
            if (p->key == key) {
                if (prev) prev->next = p->next;
                else buckets[b] = p->next;
                delete p; cnt--; return true;
            }
        return false;
    }
    void clear() {
        for (size_t i = 0; i < buckets.size(); i++) {
            Node* p = buckets[i];
            while (p) { Node* n = p->next; delete p; p = n; }
            buckets[i] = 0;
        }
        cnt = 0;
    }
    int size() const { return cnt; }
};

/* ---- (a,f) Priority-based Service Queue (sorted singly linked list) ------ */
/*  Ordered by: priority (high->low), then creation time (old->new).
    Front always holds the highest-priority ticket.                           */
class ServiceQueue {
    struct QNode {
        Ticket* t; QNode* next;
        QNode(Ticket* x) : t(x), next(0) {}
    };
    QNode* front;
    QNode* rear;
    int cnt;

    /* true if ticket a should be served BEFORE ticket b */
    static bool before(Ticket* a, Ticket* b) {
        if (a->priority != b->priority) return a->priority > b->priority;
        int c = DateTime::compare(a->created, b->created);
        if (c != 0) return c < 0;
        return a->id < b->id;
    }
public:
    ServiceQueue() : front(0), rear(0), cnt(0) {}
    ~ServiceQueue() { clear(); }

    /* (j) insert a new priority ticket at its correct position */
    void insertSorted(Ticket* t) {
        QNode* node = new QNode(t);
        if (!front || before(t, front->t)) {          // becomes new front
            node->next = front; front = node;
            if (!rear) rear = node;
            cnt++; return;
        }
        QNode* prev = front;
        while (prev->next && !before(t, prev->next->t)) prev = prev->next;
        node->next = prev->next; prev->next = node;
        if (!node->next) rear = node;   // appended at tail (kept for O(1))
        cnt++;
    }
    /* (d) retrieve highest-priority ticket (does not remove) */
    Ticket* peekHighest() const { return front ? front->t : 0; }
    /* (d) remove & return the K highest-priority tickets (one at a time) */
    bool removeHighest(Ticket*& out) {
        if (!front) return false;
        QNode* n = front; out = n->t; front = front->next;
        if (!front) rear = 0;
        delete n; cnt--; return true;
    }
    /* (k) remove a specific (resolved) ticket from the queue */
    bool removeById(const string& id) {
        QNode* prev = 0;
        for (QNode* p = front; p; prev = p, p = p->next)
            if (p->t->id == id) {
                if (prev) prev->next = p->next; else front = p->next;
                if (rear == p) rear = prev;
                delete p; cnt--; return true;
            }
        return false;
    }
    bool contains(const string& id) const {
        for (QNode* p = front; p; p = p->next) if (p->t->id == id) return true;
        return false;
    }
    void toVector(vector<Ticket*>& out) const {
        out.clear();
        for (QNode* p = front; p; p = p->next) out.push_back(p->t);
    }
    int size() const { return cnt; }
    void clear() {
        while (front) { QNode* n = front->next; delete front; front = n; }
        rear = 0; cnt = 0;
    }
    void display() const {
        if (!front) { cout << "  (service queue is empty)\n"; return; }
        cout << "  " << pad("Ticket", 11) << pad("Student", 15)
             << pad("Category", 12) << pad("Priority", 9)
             << pad("Status", 12) << "Created At\n";
        cout << "  ---------------------------------------------------------------\n";
        int i = 1;
        for (QNode* p = front; p; p = p->next, i++) {
            cout << "  #" << setw(2) << i << " ";
            p->t->displayBrief();
        }
    }
    /* Grouped view by category (used in staff views) */
    void displayByCategory() const {
        for (int c = 0; c < CAT_COUNT; c++) {
            bool hdr = false;
            for (QNode* p = front; p; p = p->next)
                if ((int)p->t->category == c) {
                    if (!hdr) { cout << "  -- " << CATEGORY_NAMES[c] << " --\n"; hdr = true; }
                    cout << "      " << pad(p->t->id, 11)
                         << pad(priName(p->t->priority), 9)
                         << pad(statusName(p->t->status), 12)
                         << p->t->description.substr(0, 40) << "\n";
                }
        }
    }
};

/* ---- (f) AVL Tree priority index ---------------------------------------- */
/*  BST key order : priority ASC, creation time ASC, id ASC
    => maximum node (rightmost) = highest priority / oldest ticket.          */
class PriorityAVL {
    struct Node {
        Ticket* t; Node* l; Node* r; int h;
        Node(Ticket* x) : t(x), l(0), r(0), h(1) {}
    };
    Node* root;
    int cnt;

    static bool lessThan(Ticket* a, Ticket* b) {
        if (a->priority != b->priority) return a->priority < b->priority;
        int c = DateTime::compare(a->created, b->created);
        if (c != 0) return c < 0;
        return a->id < b->id;
    }
    static int height(Node* n) { return n ? n->h : 0; }
    static void upd(Node* n) {
        int L = height(n->l), R = height(n->r);
        n->h = 1 + (L > R ? L : R);
    }
    static int bal(Node* n) { return n ? height(n->l) - height(n->r) : 0; }

    static Node* rotR(Node* y) {
        Node* x = y->l; Node* T2 = x->r;
        x->r = y; y->l = T2;
        upd(y); upd(x);
        return x;
    }
    static Node* rotL(Node* x) {
        Node* y = x->r; Node* T2 = y->l;
        y->l = x; x->r = T2;
        upd(x); upd(y);
        return y;
    }
    static Node* insert(Node* n, Ticket* t) {
        if (!n) return new Node(t);
        if (lessThan(t, n->t)) n->l = insert(n->l, t);
        else n->r = insert(n->r, t);
        upd(n);
        int b = bal(n);
        if (b > 1 && lessThan(t, n->l->t)) return rotR(n);      // LL
        if (b < -1 && !lessThan(t, n->r->t)) return rotL(n);    // RR
        if (b > 1 && !lessThan(t, n->l->t)) {                   // LR
            n->l = rotL(n->l); return rotR(n);
        }
        if (b < -1 && lessThan(t, n->r->t)) {                   // RL
            n->r = rotR(n->r); return rotL(n);
        }
        return n;
    }
    static Node* minNode(Node* n) { while (n->l) n = n->l; return n; }
    static Node* removeNode(Node* n, Ticket* t) {
        if (!n) return 0;
        if (lessThan(t, n->t)) n->l = removeNode(n->l, t);
        else if (lessThan(n->t, t)) n->r = removeNode(n->r, t);
        else {
            if (!n->l || !n->r) {
                Node* dead = n;
                n = n->l ? n->l : n->r;
                delete dead;
            } else {
                Node* s = minNode(n->r);
                n->t = s->t;
                n->r = removeNode(n->r, s->t);
            }
        }
        if (!n) return 0;
        upd(n);
        int b = bal(n);
        if (b > 1 && bal(n->l) >= 0) return rotR(n);
        if (b > 1 && bal(n->l) < 0) { n->l = rotL(n->l); return rotR(n); }
        if (b < -1 && bal(n->r) <= 0) return rotL(n);
        if (b < -1 && bal(n->r) > 0) { n->r = rotR(n->r); return rotL(n); }
        return n;
    }
    static void inorder(Node* n, vector<Ticket*>& out) {
        if (!n) return;
        inorder(n->l, out); out.push_back(n->t); inorder(n->r, out);
    }
    static void destroy(Node* n) {
        if (!n) return; destroy(n->l); destroy(n->r); delete n;
    }
public:
    PriorityAVL() : root(0), cnt(0) {}
    ~PriorityAVL() { destroy(root); }

    void insert(Ticket* t) { root = insert(root, t); cnt++; }
    bool remove(Ticket* t) {
        if (!root) return false;
        root = removeNode(root, t); cnt--; return true;
    }
    /* (d,f) extract the single highest-priority ticket */
    bool extractHighest(Ticket*& out) {
        if (!root) return false;
        Node* n = root;
        while (n->r) n = n->r;
        out = n->t;
        root = removeNode(root, n->t);
        cnt--;
        return true;
    }
    /* (f) fast retrieval of all tickets in a given category (sorted by priority) */
    vector<Ticket*> retrieveByCategory(Category c) const {
        vector<Ticket*> all, res;
        inorder(root, all);
        for (size_t i = 0; i < all.size(); i++)
            if (all[i]->category == c) res.push_back(all[i]);
        return res;
    }
    vector<Ticket*> allSorted() const {
        vector<Ticket*> out; inorder(root, out); return out;
    }
    int size() const { return cnt; }
    int height() const { return height(root); }
};

/* ---- (e) Binary Max-Heap : staff priority ranking ----------------------- */
/*  Key = number of tickets resolved (then average rating, then name).       */
class StaffRankingHeap {
    vector<Staff*> a;
    int n;

    static bool higher(Staff* x, Staff* y) {
        if (x->ticketsResolved != y->ticketsResolved)
            return x->ticketsResolved > y->ticketsResolved;
        if (x->avgRating() != y->avgRating()) return x->avgRating() > y->avgRating();
        return x->id < y->id;
    }
    void siftUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (!higher(a[i], a[p])) break;
            Staff* tmp = a[i]; a[i] = a[p]; a[p] = tmp;
            i = p;
        }
    }
    void siftDown(int i) {
        while (true) {
            int l = 2 * i + 1, r = 2 * i + 2, big = i;
            if (l < n && higher(a[l], a[big])) big = l;
            if (r < n && higher(a[r], a[big])) big = r;
            if (big == i) break;
            Staff* tmp = a[i]; a[i] = a[big]; a[big] = tmp;
            i = big;
        }
    }
public:
    /* (l) heap re-ordered after each batch of operations */
    void rebuild(const vector<Staff*>& staff) {
        a = staff;
        n = (int)a.size();
        for (int i = n / 2 - 1; i >= 0; i--) siftDown(i);   // O(n) build-heap
    }
    void display() const {
        if (n == 0) { cout << "  (no staff registered)\n"; return; }
        vector<Staff*> sorted = a;                 // heap-sort copy -> ranking
        int m = (int)sorted.size();
        for (int i = m - 1; i > 0; i--) {
            Staff* tmp = sorted[0]; sorted[0] = sorted[i]; sorted[i] = tmp;
            // sift down on the reduced range
            int idx = 0;
            while (true) {
                int l = 2 * idx + 1, r = 2 * idx + 2, big = idx;
                if (l < i && higher(sorted[l], sorted[big])) big = l;
                if (r < i && higher(sorted[r], sorted[big])) big = r;
                if (big == idx) break;
                Staff* t2 = sorted[idx]; sorted[idx] = sorted[big]; sorted[big] = t2;
                idx = big;
            }
        }
        cout << "  Rank " << pad("Staff ID", 10) << pad("Name", 16)
             << pad("Resolved", 10) << pad("Avg Rating", 12) << "Skills\n";
        cout << "  ------------------------------------------------------------\n";
        /* Heapsort leaves the array ASCENDING w.r.t. `higher` (max moved to the
           end each pass), so the top-ranked staff member sits at index m-1.
           Print from the end backwards to show rank 1 = best. */
        for (int i = m - 1; i >= 0; i--)
            cout << "  " << setw(4) << (m - i) << " " << pad(sorted[i]->id, 10)
                 << pad(sorted[i]->name, 16) << pad(toStr(sorted[i]->ticketsResolved), 10)
                 << pad(fixed2(sorted[i]->avgRating()), 12)
                 << sorted[i]->skillList() << "\n";
    }
    static string toStr(int v) { ostringstream o; o << v; return o.str(); }
    static string fixed2(double v) { ostringstream o; o << fixed << setprecision(2) << v; return o.str(); }
};

/* ---- (g) Binary Search Tree : Knowledge Base (key: category, then id) ---- */
class KnowledgeBaseBST {
    struct Node {
        KBArticle art; Node* l; Node* r;
        Node(const KBArticle& a) : art(a), l(0), r(0) {}
    };
    Node* root;
    int cnt;

    static bool keyLess(const KBArticle& a, const KBArticle& b) {
        if (a.category != b.category) return a.category < b.category;
        return a.id < b.id;
    }
    static Node* insert(Node* n, const KBArticle& a) {
        if (!n) return new Node(a);
        if (keyLess(a, n->art)) n->l = insert(n->l, a);
        else if (keyLess(n->art, a)) n->r = insert(n->r, a);
        return n;
    }
    static KBArticle* find(Node* n, const string& id) {
        if (!n) return 0;
        if (n->art.id == id) return &n->art;
        KBArticle* f = find(n->l, id);
        return f ? f : find(n->r, id);
    }
    static void traversePre(Node* n, vector<KBArticle*>& out) {
        if (!n) return; out.push_back(&n->art);
        traversePre(n->l, out); traversePre(n->r, out);
    }
    static void traverseIn(Node* n, vector<KBArticle*>& out) {
        if (!n) return; traverseIn(n->l, out); out.push_back(&n->art); traverseIn(n->r, out);
    }
    static void traversePost(Node* n, vector<KBArticle*>& out) {
        if (!n) return; traversePost(n->l, out); traversePost(n->r, out); out.push_back(&n->art);
    }
    static void searchCategory(Node* n, Category c, vector<KBArticle*>& out) {
        if (!n) return;
        if ((int)c <= (int)n->art.category) searchCategory(n->l, c, out);
        if (n->art.category == c) out.push_back(&n->art);
        if ((int)c >= (int)n->art.category) searchCategory(n->r, c, out);
    }
    static void destroy(Node* n) { if (!n) return; destroy(n->l); destroy(n->r); delete n; }
public:
    KnowledgeBaseBST() : root(0), cnt(0) {}
    ~KnowledgeBaseBST() { destroy(root); }

    bool add(const KBArticle& a) {
        if (find(root, a.id)) return false;      // unique article ID
        root = insert(root, a); cnt++; return true;
    }
    KBArticle* findById(const string& id) { return find(root, id); }
    vector<KBArticle*> searchByCategory(Category c) {
        vector<KBArticle*> out; searchCategory(root, c, out); return out;
    }
    void traversal(int mode, vector<KBArticle*>& out) {   // 0=pre,1=in,2=post
        out.clear();
        if (mode == 0) traversePre(root, out);
        else if (mode == 1) traverseIn(root, out);
        else traversePost(root, out);
    }
    int size() const { return cnt; }
};

/* ---- (i) Doubly Linked List : operation log ----------------------------- */
class OperationLog {
    struct LogNode {
        int opId; string action, details; DateTime when;
        LogNode *prev, *next;
        LogNode(int id, const string& a, const string& d)
            : opId(id), action(a), details(d), when(DateTime::now()), prev(0), next(0) {}
    };
    LogNode* head;
    LogNode* tail;
    int cnt;
public:
    OperationLog() : head(0), tail(0), cnt(0) {}
    ~OperationLog() {
        while (head) { LogNode* n = head->next; delete head; head = n; }
    }
    void record(const string& action, const string& details) {
        LogNode* n = new LogNode(++cnt, action, details);
        n->next = head;
        if (head) head->prev = n;
        head = n;
        if (!tail) tail = n;
    }
    void traverseForward() const {          // newest -> oldest
        if (!head) { cout << "  (no operations logged)\n"; return; }
        for (LogNode* p = head; p; p = p->next)
            cout << "  #" << setw(3) << left << p->opId << right << " "
                 << p->when.str() << "  " << pad(p->action, 22) << p->details << "\n";
    }
    void traverseBackward() const {         // oldest -> newest
        if (!tail) { cout << "  (no operations logged)\n"; return; }
        for (LogNode* p = tail; p; p = p->prev)
            cout << "  #" << setw(3) << left << p->opId << right << " "
                 << p->when.str() << "  " << pad(p->action, 22) << p->details << "\n";
    }
    int search(const string& keyword) const {
        int found = 0;
        for (LogNode* p = head; p; p = p->next)
            if (p->details.find(keyword) != string::npos ||
                p->action.find(keyword) != string::npos) {
                cout << "  #" << p->opId << " " << p->when.str() << "  "
                     << pad(p->action, 22) << p->details << "\n";
                found++;
            }
        return found;
    }
    int size() const { return cnt; }
};

/* ---- (k) Doubly Linked List : history archive --------------------------- */
class ArchiveList {
    struct AN { Ticket* t; AN *prev, *next; AN(Ticket* x) : t(x), prev(0), next(0) {} };
    AN* head;
    AN* tail;
    int cnt;
public:
    ArchiveList() : head(0), tail(0), cnt(0) {}
    ~ArchiveList() { while (head) { AN* n = head->next; delete head; head = n; } }
    void add(Ticket* t) {                     // insert at front (most recent first)
        AN* n = new AN(t);
        n->next = head;
        if (head) head->prev = n;
        head = n;
        if (!tail) tail = n;
        cnt++;
    }
    void display() const {
        if (!head) { cout << "  (archive is empty)\n"; return; }
        cout << "  " << pad("Ticket", 11) << pad("Student", 15)
             << pad("Category", 12) << pad("Staff", 10)
             << pad("Rating", 8) << "Resolved At\n";
        cout << "  ---------------------------------------------------------------\n";
        for (AN* p = head; p; p = p->next)
            cout << "  " << pad(p->t->id, 11) << pad(p->t->studentId, 15)
                 << pad(catName(p->t->category), 12) << pad(p->t->assignedStaff, 10)
                 << pad(p->t->rating ? to_string(p->t->rating) + "/5" : "-", 8)
                 << p->t->updated.str() << "\n";
    }
    int size() const { return cnt; }
};

/* =============================================================================
   SECTION 5 : SYSTEM (ties every structure together)
   ============================================================================= */
class HelpDeskSystem {
public:
    HashTable<Student> studentTable;      // student ID -> Student
    HashTable<Staff>   staffTable;        // staff ID   -> Staff
    HashTable<Ticket>  ticketTable;       // ticket ID  -> Ticket

    /* (h) Hash table: fast retrieval of a student's tickets by student ID.
       Each bucket owns a vector so lookup is O(1) instead of a full scan. */
    struct TicketBucket { vector<Ticket*> v; };
    HashTable<TicketBucket> ticketsByStudent;   // student ID -> that student's tickets
    ServiceQueue       queue;             // (a,b,c,d,j,k) service queue
    PriorityAVL        avl;               // (f) priority index
    KnowledgeBaseBST   kb;                // (g) knowledge base
    OperationLog       opLog;             // (i) operation log
    ArchiveList        archive;           // (k) history archive
    StaffRankingHeap   ranking;           // (e,l) staff ranking heap

    vector<Student*> students;
    vector<Staff*>   staff;
    vector<Ticket*>  tickets;             // owns all tickets
    vector<Ticket*>  batch;               // (d) staging batch of extracted tickets

    int nextTicket, nextArticle, opsSinceRebuild;
    static const int BATCH_SIZE = 5;      // (l) rebuild heap after each batch

    HelpDeskSystem() : nextTicket(1001), nextArticle(1), opsSinceRebuild(0) {}
    ~HelpDeskSystem() {
        /* hash table frees its nodes but not the values, so buckets must go */
        for (size_t i = 0; i < students.size(); i++) {
            TicketBucket* b = ticketsByStudent.find(students[i]->id);
            if (b) delete b;
        }
        for (size_t i = 0; i < tickets.size(); i++) delete tickets[i];
        for (size_t i = 0; i < students.size(); i++) delete students[i];
        for (size_t i = 0; i < staff.size(); i++) delete staff[i];
    }

    /* ---- helpers -------------------------------------------------------- */
    Ticket* findTicket(const string& id) { return ticketTable.find(id); }
    Staff*  findStaff(const string& id)  { return staffTable.find(id); }

    /* (h) index a ticket under its student's hash bucket (O(1) retrieval) */
    void indexTicketForStudent(Ticket* t) {
        TicketBucket* b = ticketsByStudent.find(t->studentId);
        if (!b) { b = new TicketBucket(); ticketsByStudent.insert(t->studentId, b); }
        b->v.push_back(t);
    }
    /* (h) create an (empty) bucket when a student is registered */
    void ensureStudentBucket(const string& sid) {
        if (!ticketsByStudent.find(sid)) {
            TicketBucket* b = new TicketBucket();
            ticketsByStudent.insert(sid, b);
        }
    }

    void afterOperation() {                 // (l) batch processing of the heap
        opsSinceRebuild++;
        if (opsSinceRebuild >= BATCH_SIZE) {
            ranking.rebuild(staff);
            opsSinceRebuild = 0;
            opLog.record("HEAP-REBALANCE", "Staff ranking heap re-ordered after batch of " +
                         to_string(BATCH_SIZE) + " operations");
        }
    }

    /* (a) CREATE --------------------------------------------------------- */
    Ticket* addTicket() {
        string sid = readLine("  Student ID (e.g. STD-2026-01): ");
        Student* st = studentTable.find(sid);
        if (!st) {
            cout << "  [ERROR] Student ID '" << sid << "' not found. "
                 << "Register the student first (Menu 7 -> 2).\n";
            return 0;
        }
        cout << "  Categories:\n";
        for (int i = 0; i < CAT_COUNT; i++)
            cout << "    " << (i + 1) << ". " << CATEGORY_NAMES[i] << "\n";
        int ci = readInt("  Choose category: ", 1, CAT_COUNT);
        Category cat = (Category)(ci - 1);
        string desc = readLine("  Problem description: ");
        cout << "  Priority: 1. Low   2. Medium   3. High\n";
        int pi = readInt("  Choose priority: ", 1, 3);
        Priority pri = (Priority)pi;

        Ticket* t = new Ticket();
        ostringstream id; id << "TCK-" << nextTicket++;
        t->id = id.str();
        t->studentId = sid; t->category = cat; t->description = desc;
        t->priority = pri; t->status = ST_OPEN;
        t->created = t->updated = DateTime::now();

        tickets.push_back(t);
        ticketTable.insert(t->id, t);
        indexTicketForStudent(t);            // (h) hash index: tickets by student
        queue.insertSorted(t);              // (j) correct priority position
        avl.insert(t);                      // (f) AVL priority index
        opLog.record("TICKET-CREATED", t->id + " [" + catName(cat) + "/" +
                     priName(pri) + "] by " + sid);
        afterOperation();
        cout << "  [OK] Ticket " << t->id << " created and queued.\n";
        return t;
    }

    /* (b) UPDATE --------------------------------------------------------- */
    void updateTicket() {
        string id = readLine("  Ticket ID: ");
        Ticket* t = findTicket(id);
        if (!t) { cout << "  [ERROR] Ticket not found.\n"; return; }
        t->displayFull();
        cout << "  1. Change status (Open/In Progress/Resolved/Closed)\n"
             << "  2. Change priority\n"
             << "  3. Add resolution note\n"
             << "  4. Link a Knowledge Base article\n"
             << "  5. Cancel\n";
        int ch = readInt("  Choice: ", 1, 5);
        if (ch == 5) return;

        if (ch == 1) {
            cout << "  1. Open   2. In Progress   3. Resolved   4. Closed\n";
            int s = readInt("  New status: ", 1, 4);
            if (s - 1 == ST_RESOLVED) { resolveTicketById(id); return; }
            t->status = (Status)(s - 1);
            opLog.record("STATUS-UPDATED", id + " -> " + statusName(t->status));
        } else if (ch == 2) {
            cout << "  1. Low   2. Medium   3. High\n";
            int p = readInt("  New priority: ", 1, 3);
            t->priority = (Priority)p;
            /* (j) Re-position the ticket: both the queue and the AVL index are
               keyed by priority, so a priority change must re-insert the ticket
               into its correct position. */
            if (queue.contains(id)) {
                queue.removeById(id);
                queue.insertSorted(t);
            }
            avl.remove(t);
            avl.insert(t);
            opLog.record("PRIORITY-UPDATED", id + " -> " + priName(t->priority) +
                         " (re-positioned in queue & AVL)");
        } else if (ch == 3) {
            t->resolutionNotes = readLine("  Resolution notes: ");
            opLog.record("NOTES-ADDED", id + " : " + t->resolutionNotes);
        } else if (ch == 4) {
            vector<KBArticle*> arts = kb.searchByCategory(t->category);
            if (arts.empty()) cout << "  (no articles in category '" << catName(t->category) << "')\n";
            else {
                cout << "  Articles for this category:\n";
                for (size_t i = 0; i < arts.size(); i++)
                    cout << "    " << (i + 1) << ". [" << arts[i]->id << "] "
                         << arts[i]->title << "\n";
                int pick = readInt("  Choose article: ", 1, (int)arts.size());
                if (t->linkArticle(arts[pick - 1]->id)) {
                    arts[pick - 1]->views++;
                    opLog.record("KB-LINKED", id + " -> " + arts[pick - 1]->id);
                } else cout << "  [ERROR] Already linked or link limit reached.\n";
            }
        }
        t->touch();                          // updated timestamp
        afterOperation();
        cout << "  [OK] Ticket " << id << " updated.\n";
    }

    /* (k) RESOLVE : remove from queue, keep in archive ------------------- */
    bool resolveTicketById(const string& id) {
        Ticket* t = findTicket(id);
        if (!t) { cout << "  [ERROR] Ticket not found.\n"; return false; }
        if (t->status == ST_RESOLVED) { cout << "  [ERROR] Already resolved.\n"; return false; }
        t->status = ST_RESOLVED;
        if (t->resolutionNotes.empty())
            t->resolutionNotes = "Resolved by help desk staff.";
        t->touch();
        queue.removeById(id);                // (k) removed from service queue
        for (size_t i = 0; i < batch.size(); i++)
            if (batch[i]->id == id) {        // never re-queue a resolved ticket
                batch.erase(batch.begin() + i);
                break;
            }
        archive.add(t);                      // (k) kept in history archive
        if (!t->assignedStaff.empty()) {
            Staff* s = findStaff(t->assignedStaff);
            if (s) s->ticketsResolved++;     // (e) update ranking data
        }
        opLog.record("TICKET-RESOLVED", id + " archived");
        afterOperation();
        cout << "  [OK] Ticket " << id << " resolved & moved to archive.\n";
        return true;
    }
    void resolveTicket() { resolveTicketById(readLine("  Ticket ID: ")); }

    /* ASSIGN : (e) workload-based assignment to matching staff ----------- */
    void assignTicket() {
        string id = readLine("  Ticket ID: ");
        Ticket* t = findTicket(id);
        if (!t) { cout << "  [ERROR] Ticket not found.\n"; return; }
        if (t->status == ST_RESOLVED) { cout << "  [ERROR] Ticket already resolved.\n"; return; }

        Staff* best = 0; int bestLoad = -1;
        cout << "  Staff with matching specialization:\n";
        for (size_t i = 0; i < staff.size(); i++)
            if (staff[i]->hasSkill(t->category)) {
                int load = countOpenAssigned(staff[i]->id);
                cout << "    - " << staff[i]->id << " " << staff[i]->name
                     << " (current load: " << load << ")\n";
                if (best == 0 || load < bestLoad) { best = staff[i]; bestLoad = load; }
            }
        if (!best) {
            cout << "  [ERROR] No staff member is specialized in '"
                 << catName(t->category) << "'.\n";
            return;
        }
        t->assignedStaff = best->id;
        if (t->status == ST_OPEN) t->status = ST_INPROGRESS;
        t->touch();
        opLog.record("TICKET-ASSIGNED", id + " -> " + best->id + " (" + best->name + ")");
        afterOperation();
        cout << "  [OK] Ticket " << id << " assigned to " << best->name
             << " (minimum workload).\n";
    }
    int countOpenAssigned(const string& staffId) {
        int c = 0;
        for (size_t i = 0; i < tickets.size(); i++)
            if (tickets[i]->assignedStaff == staffId &&
                tickets[i]->status != ST_RESOLVED && tickets[i]->status != ST_CLOSED)
                c++;
        return c;
    }

    /* SATISFACTION RATING ------------------------------------------------- */
    void rateTicket() {
        string id = readLine("  Ticket ID: ");
        Ticket* t = findTicket(id);
        if (!t) { cout << "  [ERROR] Ticket not found.\n"; return; }
        if (t->status != ST_RESOLVED && t->status != ST_CLOSED) {
            cout << "  [ERROR] Only resolved/closed tickets can be rated.\n"; return;
        }
        int r = readInt("  Rating (1-5): ", 1, 5);
        t->rating = r; t->touch();
        Staff* s = findStaff(t->assignedStaff);
        if (s) { s->ratingsSum += r; s->ratingsCount++; }
        opLog.record("TICKET-RATED", id + " rated " + to_string(r) + "/5");
        afterOperation();
        cout << "  [OK] Thank you for your feedback!\n";
    }

    /* (c) STUDENT VIEW : sorted by priority, then creation time ---------- */
    void registerStudent() {
        string sid = readLine("  Student ID (e.g. STD-2026-09): ");
        if (studentTable.find(sid)) {
            cout << "  [ERROR] Student ID '" << sid << "' already exists.\n";
            return;
        }
        string name = readLine("  Full name: ");
        string email = readLine("  Email: ");
        string dep = readLine("  Department: ");
        string ph = readLine("  Phone: ");
        Student* s = new Student(sid, name, email, dep, ph);
        students.push_back(s);
        studentTable.insert(sid, s);
        ensureStudentBucket(sid);          // (h) empty ticket bucket for lookup
        opLog.record("STUDENT-ADDED", sid + " " + name);
        afterOperation();
        cout << "  [OK] Student " << sid << " registered.\n";
    }
    void viewStudentTickets() {
        string sid = readLine("  Student ID: ");
        Student* st = studentTable.find(sid);
        if (!st) { cout << "  [ERROR] Student ID not found.\n"; return; }
        st->display();
        /* (h) O(1) retrieval via the tickets-by-student hash table
              (instead of scanning every ticket) */
        vector<Ticket*> mine;
        TicketBucket* b = ticketsByStudent.find(sid);
        if (b) mine = b->v;
        if (mine.empty()) { cout << "  (this student has no tickets)\n"; return; }

        // sort: priority DESC, then creation time ASC  (requirement c)
        sort(mine.begin(), mine.end(), TicketOrder());
        cout << "\n  Tickets for " << st->name << " (sorted by priority, then time):\n";
        cout << "  " << pad("Ticket", 11) << pad("Category", 12)
             << pad("Priority", 9) << pad("Status", 12) << "Created At\n";
        cout << "  ---------------------------------------------------------------\n";
        for (size_t i = 0; i < mine.size(); i++) {
            cout << "  " << pad(mine[i]->id, 11) << pad(catName(mine[i]->category), 12)
                 << pad(priName(mine[i]->priority), 9)
                 << pad(statusName(mine[i]->status), 12) << mine[i]->created.str() << "\n";
        }
        int openCnt = 0;
        for (size_t i = 0; i < mine.size(); i++)
            if (mine[i]->status != ST_RESOLVED && mine[i]->status != ST_CLOSED) openCnt++;
        cout << "  Total: " << mine.size() << "  |  Active: " << openCnt
             << "  |  Completed: " << (mine.size() - openCnt) << "\n";
    }
    struct TicketOrder {
        bool operator()(Ticket* a, Ticket* b) const {
            if (a->priority != b->priority) return a->priority > b->priority;
            return DateTime::compare(a->created, b->created) < 0;
        }
    };

    /* (d) RETRIEVE K HIGHEST-PRIORITY TICKETS ---------------------------- */
    void retrieveTopK() {
        if (queue.size() == 0) { cout << "  (service queue is empty)\n"; return; }
        cout << "  Queue size: " << queue.size() << "\n";
        int k = readInt("  How many (K) highest-priority tickets? ", 1, queue.size());
        cout << "  1. Peek only (view top-K, keep them in queue)\n"
             << "  2. Extract (remove top-K into a staging batch)\n";
        int mode = readInt("  Choice: ", 1, 2);
        vector<Ticket*> top;
        queue.toVector(top);
        cout << "\n  Top " << k << " highest-priority tickets:\n";
        cout << "  " << pad("Ticket", 11) << pad("Category", 12) << pad("Priority", 9)
             << pad("Status", 12) << "Description\n";
        cout << "  ---------------------------------------------------------------\n";
        for (int i = 0; i < k; i++)
            cout << "  " << pad(top[i]->id, 11) << pad(catName(top[i]->category), 12)
                 << pad(priName(top[i]->priority), 9)
                 << pad(statusName(top[i]->status), 12)
                 << top[i]->description.substr(0, 38) << "\n";
        if (mode == 2) {
            for (int i = 0; i < k; i++) {
                Ticket* t = 0;
                if (queue.removeHighest(t)) batch.push_back(t);
            }
            opLog.record("TOP-K-EXTRACTED", to_string(k) + " tickets moved to staging batch");
            cout << "  [OK] " << k << " tickets extracted into staging batch "
                 << "(use Menu 3.4 to return them).\n";
        } else {
            opLog.record("TOP-K-PEEKED", "viewed top " + to_string(k) + " tickets");
        }
        afterOperation();
    }

    /* STAGING BATCH ------------------------------------------------------ */
    void showBatch() {
        if (batch.empty()) { cout << "  (staging batch is empty)\n"; return; }
        cout << "  Staging batch (" << batch.size() << " tickets):\n";
        for (size_t i = 0; i < batch.size(); i++) {
            cout << "    " << pad(batch[i]->id, 11) << pad(priName(batch[i]->priority), 9)
                 << batch[i]->description.substr(0, 40) << "\n";
        }
    }
    void returnBatch() {
        if (batch.empty()) { cout << "  (nothing to return)\n"; return; }
        int n = (int)batch.size();
        for (size_t i = 0; i < batch.size(); i++)
            if (queue.contains(batch[i]->id)) {
                cout << "  [WARN] " << batch[i]->id << " already in queue - skipped.\n";
                continue;
            } else queue.insertSorted(batch[i]);   // (j) correct position again
        batch.clear();
        opLog.record("BATCH-RETURNED", to_string(n) + " tickets re-queued in priority order");
        afterOperation();
        cout << "  [OK] All " << n << " tickets returned to the service queue.\n";
    }

    /* REPORTS ------------------------------------------------------------ */
    void staffReport() {
        ranking.rebuild(staff);               // always fresh for display
        cout << "  Staff priority ranking (binary max-heap):\n";
        ranking.display();
    }
    void queueView() {
        cout << "  === SERVICE QUEUE (" << queue.size() << " active tickets) ===\n";
        queue.display();
    }
    void queueByCategory() {
        cout << "  === SERVICE QUEUE BY CATEGORY ===\n";
        queue.displayByCategory();
    }
    void avlCategoryQuery() {
        cout << "  Categories:\n";
        for (int i = 0; i < CAT_COUNT; i++)
            cout << "    " << (i + 1) << ". " << CATEGORY_NAMES[i] << "\n";
        int ci = readInt("  Choose category: ", 1, CAT_COUNT);
        vector<Ticket*> res = avl.retrieveByCategory((Category)(ci - 1));
        cout << "  AVL retrieval -> " << res.size() << " ticket(s) in '"
             << CATEGORY_NAMES[ci - 1] << "' (priority order):\n";
        for (size_t i = 0; i < res.size(); i++) {
            cout << "    " << pad(res[i]->id, 11) << pad(priName(res[i]->priority), 9)
                 << pad(statusName(res[i]->status), 12) << res[i]->description.substr(0, 38) << "\n";
        }
    }
    void statisticsReport() {
        int catCount[CAT_COUNT]; int stCount[ST_COUNT];
        for (int i = 0; i < CAT_COUNT; i++) catCount[i] = 0;
        for (int i = 0; i < ST_COUNT; i++) stCount[i] = 0;
        for (size_t i = 0; i < tickets.size(); i++) {
            catCount[tickets[i]->category]++;
            stCount[tickets[i]->status]++;
        }
        cout << "  TOTALS: tickets=" << tickets.size() << "  in-queue=" << queue.size()
             << "  archived=" << archive.size() << "  students=" << students.size()
             << "  staff=" << staff.size() << "  KB-articles=" << kb.size() << "\n";
        cout << "\n  Category distribution:\n";
        for (int i = 0; i < CAT_COUNT; i++)
            if (catCount[i])
                cout << "    " << pad(CATEGORY_NAMES[i], 14) << catCount[i] << "\n";
        cout << "\n  Status distribution:\n";
        for (int i = 0; i < ST_COUNT; i++)
            cout << "    " << pad(STATUS_NAMES[i], 14) << stCount[i] << "\n";
        cout << "\n  AVL index: nodes=" << avl.size() << "  height=" << avl.height()
             << "  (balanced: O(log n) priority retrieval)\n";
        cout << "  Hash tables: students=" << studentTable.size()
             << "  staff=" << staffTable.size() << "  tickets=" << ticketTable.size() << "\n";
        cout << "  Operation log entries: " << opLog.size() << "\n";
    }

    /* KNOWLEDGE BASE ------------------------------------------------------ */
    void kbAdd() {
        KBArticle a;
        ostringstream id; id << "KB-" << setw(3) << setfill('0') << nextArticle++;
        a.id = id.str();
        a.title = readLine("  Article title: ");
        cout << "  Categories:\n";
        for (int i = 0; i < CAT_COUNT; i++)
            cout << "    " << (i + 1) << ". " << CATEGORY_NAMES[i] << "\n";
        int ci = readInt("  Choose category: ", 1, CAT_COUNT);
        a.category = (Category)(ci - 1);
        a.body = readLine("  Article content: ");
        a.views = 0;
        if (kb.add(a)) {
            opLog.record("KB-ADDED", a.id + " [" + catName(a.category) + "] " + a.title);
            afterOperation();
            cout << "  [OK] Article " << a.id << " added to the knowledge base.\n";
        } else cout << "  [ERROR] Article ID already exists.\n";
    }
    void kbSearch() {
        cout << "  Categories:\n";
        for (int i = 0; i < CAT_COUNT; i++)
            cout << "    " << (i + 1) << ". " << CATEGORY_NAMES[i] << "\n";
        int ci = readInt("  Choose category: ", 1, CAT_COUNT);
        vector<KBArticle*> res = kb.searchByCategory((Category)(ci - 1));
        cout << "  " << res.size() << " article(s) found:\n";
        for (size_t i = 0; i < res.size(); i++) { res[i]->display(); res[i]->views++; }
        opLog.record("KB-SEARCHED", "category " + string(CATEGORY_NAMES[ci - 1]) +
                     " (" + to_string(res.size()) + " hits)");
    }
    void kbTraversals() {
        cout << "  1. Pre-order   2. In-order   3. Post-order\n";
        int m = readInt("  Traversal: ", 1, 3);
        vector<KBArticle*> out;
        kb.traversal(m - 1, out);
        cout << "  " << (m == 1 ? "Pre-order" : (m == 2 ? "In-order" : "Post-order"))
             << " traversal (" << out.size() << " articles):\n";
        for (size_t i = 0; i < out.size(); i++)
            cout << "    " << pad(out[i]->id, 7) << pad(catName(out[i]->category), 12)
                 << out[i]->title << "\n";
    }

    /* SAMPLE DATA --------------------------------------------------------- */
    void loadSampleData() {
        if (!students.empty()) { cout << "  (sample data already loaded)\n"; return; }
        struct S { const char* a, *b, *c, *d, *e; };
        S sd[8] = {
            {"STD-2026-01", "Anas Ali",      "anas@univ.edu",    "Computer Science", "0300-1111111"},
            {"STD-2026-02", "Hina Malik",    "hina@univ.edu",    "Electrical Eng.",  "0300-2222222"},
            {"STD-2026-03", "Usman Tariq",   "usman@univ.edu",   "Business Admin.",  "0300-3333333"},
            {"STD-2026-04", "Fatima Zahra",  "fatima@univ.edu",  "Physics",          "0300-4444444"},
            {"STD-2026-05", "Bilal Khan",    "bilal@univ.edu",   "Computer Science", "0300-5555555"},
            {"STD-2026-06", "Sana Iqbal",    "sana@univ.edu",    "Mathematics",      "0300-6666666"},
            {"STD-2026-07", "Zain Abbas",    "zain@univ.edu",    "Mechanical Eng.",  "0300-7777777"},
            {"STD-2026-08", "Ayesha Noor",   "ayesha@univ.edu",  "English",          "0300-8888888"}
        };
        for (int i = 0; i < 8; i++) {
            Student* s = new Student(sd[i].a, sd[i].b, sd[i].c, sd[i].d, sd[i].e);
            students.push_back(s);
            studentTable.insert(s->id, s);
            ensureStudentBucket(s->id);
        }
        struct F { const char* a, *b, *c, *d, *e; int f[3]; int nf; };
        F fd[6] = {
            {"STF-01", "Sadia Raza",   "sadia@univ.edu",  "Senior Officer", "Morning", {CAT_IT_SUPPORT, CAT_ACADEMICS}, 2},
            {"STF-02", "Kamran Sheikh","kamran@univ.edu", "Officer",        "Evening", {CAT_FINANCE}, 1},
            {"STF-03", "Nadia Aslam",  "nadia@univ.edu",  "Officer",        "Morning", {CAT_HOSTEL, CAT_TRANSPORT}, 2},
            {"STF-04", "Imran Javed",  "imran@univ.edu",  "Technician",     "Morning", {CAT_IT_SUPPORT}, 1},
            {"STF-05", "Rabia Sultan", "rabia@univ.edu",  "Librarian",      "Morning", {CAT_LIBRARY, CAT_ACADEMICS}, 2},
            {"STF-06", "Tariq Mehmood","tariq@univ.edu",  "Medical Officer","Evening", {CAT_HEALTH}, 1}
        };
        for (int i = 0; i < 6; i++) {
            Staff* s = new Staff(fd[i].a, fd[i].b, fd[i].c, fd[i].d, fd[i].e);
            for (int j = 0; j < fd[i].nf; j++) s->addSkill((Category)fd[i].f[j]);
            staff.push_back(s);
            staffTable.insert(s->id, s);
        }
        struct T { const char* sid; Category c; const char* d; Priority p; };
        T td[6] = {
            {"STD-2026-01", CAT_IT_SUPPORT, "Cannot access student portal after password reset", PRI_HIGH},
            {"STD-2026-02", CAT_FINANCE,    "Tuition fee payment not reflected in LMS",          PRI_HIGH},
            {"STD-2026-03", CAT_HOSTEL,     "Room allotment letter not received yet",            PRI_MEDIUM},
            {"STD-2026-04", CAT_ACADEMICS,  "Need transcript for scholarship application",       PRI_MEDIUM},
            {"STD-2026-05", CAT_TRANSPORT,  "Bus route 4 timetable changed without notice",      PRI_LOW},
            {"STD-2026-06", CAT_LIBRARY,    "Journal access denied from off-campus network",     PRI_MEDIUM}
        };
        for (int i = 0; i < 6; i++) {
            Ticket* t = new Ticket();
            ostringstream id; id << "TCK-" << nextTicket++;
            t->id = id.str();
            t->studentId = td[i].sid; t->category = td[i].c;
            t->description = td[i].d; t->priority = td[i].p; t->status = ST_OPEN;
            t->created = DateTime(2026, 10, 20, 9 + i, 15, 0);
            t->updated = t->created;
            tickets.push_back(t);
            ticketTable.insert(t->id, t);
            indexTicketForStudent(t);
            queue.insertSorted(t);
            avl.insert(t);
            opLog.record("TICKET-CREATED", t->id + " [" + catName(t->category) + "/" + priName(td[i].p) + "]");
        }
        const char* art[8][3] = {
            {"Resetting your student portal password", "IT Support",
             "Visit Account > Reset Password, use your university email OTP, then re-login."},
            {"Understanding your fee challan", "Finance",
             "Challans are generated every semester; pay before the due date to avoid fine."},
            {"Hostel room allotment process", "Hostel",
             "Apply online in the first month of semester; allotment is merit + distance based."},
            {"Bus routes and timings", "Transport",
             "Route maps and timings are published on the transport office notice board."},
            {"Transcript request procedure", "Academics",
             "Submit request online; transcripts are issued within 5 working days."},
            {"Library off-campus access", "Library",
             "Use the university VPN client to access journals from home."},
            {"Health center services", "Health",
             "Free checkups 9am-2pm; emergency ambulance available 24/7."},
            {"Wi-Fi connectivity guide", "IT Support",
             "Connect to 'UniNet', accept the portal, use your login credentials."}
        };
        Category artCat[8] = {CAT_IT_SUPPORT, CAT_FINANCE, CAT_HOSTEL, CAT_TRANSPORT,
                              CAT_ACADEMICS, CAT_LIBRARY, CAT_HEALTH, CAT_IT_SUPPORT};
        for (int i = 0; i < 8; i++) {
            KBArticle a;
            ostringstream id; id << "KB-" << setw(3) << setfill('0') << nextArticle++;
            a.id = id.str(); a.title = art[i][0]; a.category = artCat[i];
            a.body = art[i][2]; a.views = 0;
            kb.add(a);
            opLog.record("KB-ADDED", a.id + " " + a.title);
        }
        ranking.rebuild(staff);
        cout << "  [OK] Sample data loaded: " << students.size() << " students, "
             << staff.size() << " staff, " << tickets.size() << " tickets, "
             << kb.size() << " KB articles.\n";
    }
};

/* =============================================================================
   SECTION 6 : MAIN MENU (interactive validation on every choice)
   ============================================================================= */
/* pad a line to width W so the banner box always lines up */
static string bw(const string& s, int W) {
    string t = (s.size() > (size_t)(W - 1)) ? s.substr(0, W - 1) : s;
    return "| " + t + string((size_t)(W - 1) - t.size(), ' ') + "|";
}
static void banner() {
    const int W = 62;
    cout << "\n  +" << string(W, '=') << "+\n"
         << "  " << bw("UNIVERSITY HELP DESK MANAGEMENT SYSTEM", W) << "\n"
         << "  " << bw("DSA Assignment #02", W) << "\n"
         << "  " << bw("2500608 Ahmad Hussain   2500594 Jozza Batool", W) << "\n"
         << "  " << bw("2500600 Raja wasif", W) << "\n"
         << "  +" << string(W, '=') << "+\n";
}

int main() {
    HelpDeskSystem sys;
    cout << "  Welcome! Loading sample data...\n";
    sys.loadSampleData();

    bool running = true;
    while (running) {
        banner();
        cout << "   TICKETS\n"
             << "     1. Add new ticket                     (a)\n"
             << "     2. View ticket by ID                  (b)\n"
             << "     3. Update ticket                      (b)\n"
             << "     4. Assign ticket to staff             (e)\n"
             << "     5. Resolve ticket -> archive          (k)\n"
             << "     6. Rate ticket satisfaction           (h)\n"
             << "   STUDENTS / QUEUE\n"
             << "     7. Student: view tickets / register   (c)\n"
             << "     8. View service queue                 (d)\n"
             << "     9. Retrieve K highest-priority tickets(d)\n"
             << "    10. View queue grouped by category\n"
             << "    11. Staging batch: view / return       (d)\n"
             << "   REPORTS / STRUCTURES\n"
             << "    12. Staff ranking (heap)               (e)\n"
             << "    13. AVL: query tickets by category     (f)\n"
             << "    14. Statistics report                  (h)\n"
             << "    15. View history archive               (k)\n"
             << "   KNOWLEDGE BASE\n"
             << "    16. Add KB article                     (g)\n"
             << "    17. Search KB by category              (g)\n"
             << "    18. KB tree traversals                 (g)\n"
             << "   OTHERS\n"
             << "    19. View operation log                 (i)\n"
             << "    20. Search operation log               (i)\n"
             << "     0. Exit\n"
             << "  --------------------------------------------------------------\n";
        int ch = readInt("  Enter choice: ", 0, 20);
        switch (ch) {
            case 1:  sys.addTicket(); break;
            case 2: {
                string id = readLine("  Ticket ID: ");
                Ticket* t = sys.findTicket(id);
                if (t) t->displayFull();
                else cout << "  [ERROR] Ticket not found.\n";
                break;
            }
            case 3:  sys.updateTicket(); break;
            case 4:  sys.assignTicket(); break;
            case 5:  sys.resolveTicket(); break;
            case 6:  sys.rateTicket(); break;
            case 7: {
                cout << "  1. View a student's tickets\n"
                     << "  2. Register a new student\n";
                int s = readInt("  Choice: ", 1, 2);
                if (s == 1) sys.viewStudentTickets(); else sys.registerStudent();
                break;
            }
            case 8:  sys.queueView(); break;
            case 9:  sys.retrieveTopK(); break;
            case 10: sys.queueByCategory(); break;
            case 11: {
                cout << "  1. View staging batch   2. Return batch to queue\n";
                int s = readInt("  Choice: ", 1, 2);
                if (s == 1) sys.showBatch(); else sys.returnBatch();
                break;
            }
            case 12: sys.staffReport(); break;
            case 13: sys.avlCategoryQuery(); break;
            case 14: sys.statisticsReport(); break;
            case 15:
                cout << "  === HISTORY ARCHIVE (" << sys.archive.size() << ") ===\n";
                sys.archive.display();
                break;
            case 16: sys.kbAdd(); break;
            case 17: sys.kbSearch(); break;
            case 18: sys.kbTraversals(); break;
            case 19:
                cout << "  1. Newest -> oldest   2. Oldest -> newest\n";
                if (readInt("  Choice: ", 1, 2) == 1) sys.opLog.traverseForward();
                else sys.opLog.traverseBackward();
                break;
            case 20: {
                string kw = readLine("  Keyword: ");
                int f = sys.opLog.search(kw);
                if (!f) cout << "  (no matching operation found)\n";
                else cout << "  " << f << " matching operation(s).\n";
                break;
            }
            case 0: running = false; break;
        }
        cout << "\n  --------------------------------------------------------------\n";
    }
    cout << "  Thank you for using the University Help Desk Management System. Goodbye!\n";
    return 0;
}
