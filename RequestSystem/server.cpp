/*==============================================================================
  server.cpp  -  HTTP backend for the University Request Management System
  ---------------------------------------------------------------------------
  Serves the HTML/CSS/JS frontend (public/) and exposes the C++ core
  (requestcore.h - circular queue, FIFO, unique IDs, status rules, statistics)
  through a small JSON API.

    GET  /                 -> public/index.html
    GET  /style.css        -> public/style.css
    GET  /script.js        -> public/script.js

    POST /api/login        {username, password, role}      spec 1 / 5
    POST /api/logout       {}                              -
    GET  /api/me           (session)                       -
    POST /api/submit       {category, description}         spec 2 / 3
    GET  /api/queue        (admin)  FIFO queue, oldest first   spec 4 / 6
    GET  /api/next         (admin)  oldest pending request     spec 6
    POST /api/process      {action, operator, note}           spec 6
    GET  /api/requests     (student: own / admin: all)        spec 6
    GET  /api/search?id=   or   /api/search?reg=             spec 7
    POST /api/status       {id, status, note}                spec 8
    GET  /api/stats        (admin)                           spec 9
    GET  /api/operators    (admin)                           spec 6

  Build:
    cl /nologo /EHsc /W3 /std:c++17 /Fe:RequestServer.exe server.cpp /link ws2_32.lib shell32.lib
==============================================================================*/

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <ctime>

#include "requestcore.h"
#include "database.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shell32.lib")

using namespace std;

/* =============================================================================
   STATE
   ============================================================================= */
static RequestSystem        g_sys;      // shared business state (the DSA core)
static CRITICAL_SECTION     g_cs;       // guards g_sys + sessions
static map<string, Account*> g_sessions; // token -> logged-in account
static int                  g_sessionSeq = 0;
static string               g_publicDir;

/* =============================================================================
   SMALL HELPERS
   ============================================================================= */
static string exeDir() {
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    string p(buf);
    size_t s = p.find_last_of("\\/");
    return s == string::npos ? string(".") : p.substr(0, s);
}

static string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static string toLower(string s) {
    for (size_t i = 0; i < s.size(); i++) s[i] = (char)tolower((unsigned char)s[i]);
    return s;
}

static string urlDecode(const string& s) {
    string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '+') out += ' ';
        else if (s[i] == '%' && i + 2 < s.size()) {
            char hex[3] = { s[i + 1], s[i + 2], 0 };
            out += (char)strtol(hex, 0, 16);
            i += 2;
        } else out += s[i];
    }
    return out;
}

/* ---- JSON --------------------------------------------------------------- */
static string jsonEscape(const string& s) {
    string o;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if ((unsigned char)c < 0x20) {
                    char b[8];
                    sprintf(b, "\\u%04x", (unsigned char)c);
                    o += b;
                } else o += c;
        }
    }
    return o;
}

/* Pull one string value out of a flat JSON object: { "key": "value", ... } */
static bool jsonGetString(const string& body, const string& key, string& out) {
    string needle = "\"" + key + "\"";
    size_t p = body.find(needle);
    if (p == string::npos) return false;
    p = body.find(':', p + needle.size());
    if (p == string::npos) return false;
    p++;
    while (p < body.size() && isspace((unsigned char)body[p])) p++;
    if (p >= body.size() || body[p] != '"') return false;
    p++;
    string val;
    while (p < body.size()) {
        char c = body[p];
        if (c == '\\' && p + 1 < body.size()) {
            char n = body[p + 1];
            if (n == 'n') val += '\n';
            else if (n == 'r') val += '\r';
            else if (n == 't') val += '\t';
            else val += n;                       // \"  \\  \/
            p += 2;
            continue;
        }
        if (c == '"') break;
        val += c;
        p++;
    }
    out = val;
    return true;
}

/* ---- object serialisation ------------------------------------------------ */
static string requestJson(const Request* r) {
    ostringstream o;
    o << "{"
      << "\"id\":"          << r->id << ","
      << "\"reg\":\""       << jsonEscape(r->studentReg) << "\","
      << "\"name\":\""      << jsonEscape(r->studentName) << "\","
      << "\"category\":\""  << jsonEscape(CATEGORY_NAMES[r->category]) << "\","
      << "\"catIndex\":"    << (int)r->category << ","
      << "\"description\":\"" << jsonEscape(r->description) << "\","
      << "\"status\":\""    << jsonEscape(statusName(r->status)) << "\","
      << "\"statusCode\":"  << (int)r->status << ","
      << "\"submitted\":\"" << jsonEscape(r->submitted.str()) << "\","
      << "\"updated\":\""   << jsonEscape(r->updated.str()) << "\","
      << "\"operator\":\""  << jsonEscape(r->assignedTo) << "\","
      << "\"note\":\""      << jsonEscape(r->adminNote) << "\""
      << "}";
    return o.str();
}

static string requestArray(const vector<Request*>& v) {
    ostringstream o;
    for (size_t i = 0; i < v.size(); i++) {
        if (i) o << ",";
        o << requestJson(v[i]);
    }
    return o.str();
}

static string errJson(const string& msg) {
    return "{\"ok\":false,\"error\":\"" + jsonEscape(msg) + "\"}";
}
static string okJson() { return "{\"ok\":true}"; }

/* =============================================================================
   HTTP PLUMBING
   ============================================================================= */
static string httpReply(int code, const char* status, const string& type,
                        const string& body) {
    ostringstream o;
    o << "HTTP/1.1 " << code << " " << status << "\r\n"
      << "Content-Type: " << type << "; charset=utf-8\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Connection: close\r\n"
      << "Cache-Control: no-store\r\n"
      << "\r\n" << body;
    return o.str();
}

static bool sendAll(SOCKET s, const string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        int n = send(s, data.data() + sent, (int)(data.size() - sent), 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

static bool recvRequest(SOCKET s, string& raw) {
    char buf[4096];
    size_t headerEnd;
    while ((headerEnd = raw.find("\r\n\r\n")) == string::npos) {
        int n = recv(s, buf, sizeof(buf), 0);
        if (n <= 0) return false;
        raw.append(buf, n);
        if (raw.size() > 512 * 1024) return false;
    }
    string headers = raw.substr(0, headerEnd);
    string lower = toLower(headers);
    int contentLength = 0;
    size_t cl = lower.find("content-length:");
    if (cl != string::npos)
        contentLength = atoi(lower.c_str() + cl + 15);
    if (contentLength < 0) contentLength = 0;
    if (contentLength > 512 * 1024) return false;

    size_t have = raw.size() - (headerEnd + 4);
    while (have < (size_t)contentLength) {
        int n = recv(s, buf, sizeof(buf), 0);
        if (n <= 0) return false;
        raw.append(buf, n);
        have += n;
    }
    return true;
}

static string mimeFor(const string& path) {
    size_t d = path.find_last_of('.');
    string ext = d == string::npos ? "" : toLower(path.substr(d));
    if (ext == ".html") return "text/html";
    if (ext == ".css")  return "text/css";
    if (ext == ".js")   return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".png")  return "image/png";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".ico")  return "image/x-icon";
    return "application/octet-stream";
}

static bool readFileBytes(const string& path, string& out) {
    ifstream f(path.c_str(), ios::binary);
    if (!f) return false;
    ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

static map<string, string> parseQuery(const string& q) {
    map<string, string> m;
    size_t start = 0;
    while (start < q.size()) {
        size_t amp = q.find('&', start);
        string part = q.substr(start, amp == string::npos ? string::npos : amp - start);
        size_t eq = part.find('=');
        if (eq != string::npos)
            m[urlDecode(part.substr(0, eq))] = urlDecode(part.substr(eq + 1));
        else if (!part.empty())
            m[urlDecode(part)] = "";
        if (amp == string::npos) break;
        start = amp + 1;
    }
    return m;
}

/* =============================================================================
   API HANDLERS  (all called with g_cs held)
   ============================================================================= */
static Account* sessionFor(const map<string, string>& headers) {
    map<string, string>::const_iterator it = headers.find("x-session");
    if (it == headers.end()) return 0;
    map<string, Account*>::iterator s = g_sessions.find(it->second);
    return s == g_sessions.end() ? 0 : s->second;
}

static bool isAdmin(Account* a) { return a && a->isAdmin; }

/* spec 1 / 5 : login */
static string apiLogin(const string& body, map<string, string>& setCookie) {
    string u, p, role;
    jsonGetString(body, "username", u);
    jsonGetString(body, "password", p);
    jsonGetString(body, "role", role);
    u = trim(u);
    role = toLower(trim(role));

    if (u.empty() || p.empty())
        return errJson("Username and password are required.");
    if (role != "student" && role != "admin")
        return errJson("Choose Student or Admin login.");

    bool wantAdmin = (role == "admin");
    Account* a = g_sys.authenticate(u, p, wantAdmin);
    if (!a)
        return errJson("Invalid username/password for " +
                       string(wantAdmin ? "admin" : "student") + " login.");

    ostringstream tok;
    tok << "S" << (++g_sessionSeq) << "-" << time(0);
    g_sessions[tok.str()] = a;
    setCookie["X-Session"] = tok.str();

    ostringstream o;
    o << "{\"ok\":true,\"token\":\"" << tok.str() << "\","
      << "\"name\":\"" << jsonEscape(a->fullName) << "\","
      << "\"reg\":\""  << jsonEscape(a->registrationNo)    << "\","
      << "\"role\":\"" << jsonEscape(a->role)     << "\","
      << "\"isAdmin\":" << (a->isAdmin ? "true" : "false") << "}";
    return o.str();
}

/* spec 2 / 3 : submit a request */
static string apiSubmit(Account* me, const string& body) {
    if (!me)              return errJson("Please log in first.");
    if (me->isAdmin)      return errJson("Only students can submit requests.");

    string catStr, desc;
    jsonGetString(body, "category", catStr);
    jsonGetString(body, "description", desc);
    desc = trim(desc);

    if (desc.empty())
        return errJson("Please describe your complaint before submitting.");
    if (desc.size() > 400)
        return errJson("Description is too long (max 400 characters).");

    int ci = -1;
    for (int i = 0; i < CAT_COUNT; i++)
        if (toLower(catStr) == toLower(CATEGORY_NAMES[i])) { ci = i; break; }
    if (ci < 0 && !catStr.empty()) {
        int v = atoi(catStr.c_str());
        if (v >= 1 && v <= CAT_COUNT) ci = v - 1;
        else if (v >= 0 && v < CAT_COUNT) ci = v;
    }
    if (ci < 0) return errJson("Please choose a valid category.");

    string msg;
    Request* r = g_sys.submit(me, ci, desc, msg);
    if (!r) return errJson(msg);

    // Save to database and get the DB-generated ID
    int userId = getUserIdByRegNo(me->registrationNo);
    if (userId == -1) {
        return errJson("User account not found in database.");
    }
    
    int dbId = saveRequest(userId, CATEGORY_NAMES[r->category], r->description,
                           statusName(r->status), r->submitted.toTimeT(), 
                           r->updated.toTimeT(), r->assignedTo);
    if (dbId == -1) {
        return errJson("Failed to save request to database.");
    }
    
    // Update the request with the DB-generated ID
    r->id = dbId;
    
    // Log history
    logHistory(dbId, me->username, "", statusName(r->status), "Request submitted", r->submitted.toTimeT());

    ostringstream o;
    o << "{\"ok\":true,\"message\":\"" << jsonEscape(msg) << " (ID #" << dbId << ")\","
      << "\"request\":" << requestJson(r) << ","
      << "\"queueSize\":" << g_sys.queue.size() << ","
      << "\"queueMax\":"  << CircularQueue::MAX << "}";
    return o.str();
}

/* spec 4 / 6 : FIFO queue, oldest first */
static string apiQueue() {
    vector<Request*> items = g_sys.queue.snapshot();
    ostringstream o;
    o << "{\"ok\":true,\"size\":" << g_sys.queue.size()
      << ",\"max\":" << CircularQueue::MAX
      << ",\"empty\":" << (items.empty() ? "true" : "false")
      << ",\"next\":" << (items.empty() ? "null" : to_string(items[0]->id)) << ","
      << "\"items\":[" << requestArray(items) << "]}";
    return o.str();
}

/* spec 6 : the OLDEST pending request */
static string apiNext() {
    Request* n = g_sys.queue.peek();
    ostringstream o;
    o << "{\"ok\":true,\"empty\":" << (n ? "false" : "true")
      << ",\"request\":" << (n ? requestJson(n) : string("null")) << "}";
    return o.str();
}

/* spec 6 : assign / resolve / reject the next request */
static string apiProcess(Account* me, const string& body) {
    if (!me)         return errJson("Please log in first.");
    if (!isAdmin(me)) return errJson("Admin access required to process requests.");

    string action, opUser, note;
    jsonGetString(body, "action", action);
    jsonGetString(body, "operator", opUser);
    jsonGetString(body, "note", note);
    action = toLower(trim(action));
    opUser = trim(opUser);
    note   = trim(note);

    Request* next = g_sys.queue.peek();
    if (!next) return errJson("The queue is empty - there is nothing to process.");

    ProcessAction act;
    if      (action == "cancel")     act = PA_CANCEL;
    else if (action == "process_op") act = PA_PROCESS_WITH_OP;
    else if (action == "process")    act = PA_PROCESS_NO_OP;
    else if (action == "resolve")    act = PA_RESOLVE;
    else if (action == "reject")     act = PA_REJECT;
    else {
        ostringstream err;
        err << "Choose what to do with request #" << next->id << ".";
        return errJson(err.str());
    }

    /* resolve / reject always need a written reason */
    if ((act == PA_RESOLVE || act == PA_REJECT) && note.empty())
        return errJson("An admin note is required to resolve or reject a request.");

    string opName;
    if (act == PA_PROCESS_WITH_OP) {
        Account* op = g_sys.findAccount(opUser);
        if (!op || !op->isAdmin)
            return errJson("Select an operator to assign this request to.");
        opName = op->fullName;
    }

    // Store old status for history
    string oldStatus = statusName(next->status);
    int requestId = next->id;
    
    string msg;
    if (!g_sys.process(next, act, opName, note, msg)) return errJson(msg);

    // Update database
    if (act != PA_CANCEL) {
        updateRequestStatus(requestId, statusName(next->status), next->updated.toTimeT(), next->assignedTo);
        logHistory(requestId, me->username, oldStatus, statusName(next->status), note, next->updated.toTimeT());
    }

    Request* nowNext = g_sys.queue.peek();
    ostringstream o;
    o << "{\"ok\":true,\"message\":\"" << jsonEscape(msg) << "\","
      << "\"queueSize\":" << g_sys.queue.size()
      << ",\"next\":" << (nowNext ? to_string(nowNext->id) : string("null")) << ","
      << "\"processed\":" << requestJson(next) << "}";
    return o.str();
}

/* spec 6 : student sees own requests, admin sees everything */
static string apiRequests(Account* me) {
    if (!me) return errJson("Please log in first.");
    vector<Request*> v = me->isAdmin ? g_sys.allRequests : g_sys.byStudent(me);
    ostringstream o;
    o << "{\"ok\":true,\"count\":" << v.size()
      << ",\"items\":[" << requestArray(v) << "]}";
    return o.str();
}

/* spec 7 : search by request ID or registration number */
static string apiSearch(const map<string, string>& q) {
    map<string, string>::const_iterator iq = q.find("id");
    map<string, string>::const_iterator rq = q.find("reg");

    ostringstream o;
    if (iq != q.end() && !iq->second.empty()) {
        string idStr = trim(iq->second);
        int id = atoi(idStr.c_str());
        Request* r = g_sys.findRequest(id);
        o << "{\"ok\":true,\"mode\":\"id\",\"count\":" << (r ? 1 : 0)
          << ",\"items\":[" << (r ? requestJson(r) : string("")) << "]}";
        return o.str();
    }
    if (rq != q.end() && !rq->second.empty()) {
        string reg = trim(rq->second);
        vector<Request*> v = g_sys.byRegistration(reg);
        o << "{\"ok\":true,\"mode\":\"reg\",\"count\":" << v.size()
          << ",\"items\":[" << requestArray(v) << "]}";
        return o.str();
    }
    return errJson("Enter a request ID or a student registration number.");
}

/* spec 8 : update status */
static string apiStatus(Account* me, const string& body) {
    if (!me)         return errJson("Please log in first.");
    if (!isAdmin(me)) return errJson("Admin access required to change status.");

    string idStr, status, note;
    jsonGetString(body, "id", idStr);
    jsonGetString(body, "status", status);
    jsonGetString(body, "note", note);
    idStr = trim(idStr);
    status = toLower(trim(status));
    note = trim(note);

    if (idStr.empty())   return errJson("Request ID is required.");
    if (status.empty()) return errJson("Choose a new status.");

    int id = atoi(idStr.c_str());
    if (id <= 0) return errJson("Invalid request ID.");

    Status ns;
    if      (status == "processing") ns = ST_PROCESSING;
    else if (status == "resolved")   ns = ST_RESOLVED;
    else if (status == "rejected")   ns = ST_REJECTED;
    else if (status == "pending")    ns = ST_PENDING;
    else return errJson("Status must be Processing, Resolved or Rejected.");

    if ((ns == ST_RESOLVED || ns == ST_REJECTED) && note.empty())
        return errJson("An admin note is required to resolve or reject a request.");

    // Get old status for history
    Request* r = g_sys.findRequest(id);
    if (!r) return errJson("Request not found.");
    string oldStatus = statusName(r->status);

    string msg;
    if (!g_sys.changeStatus(id, ns, note, msg)) return errJson(msg);

    // Update database
    updateRequestStatus(id, statusName(ns), r->updated.toTimeT(), r->assignedTo);
    logHistory(id, me->username, oldStatus, statusName(ns), note, r->updated.toTimeT());

    ostringstream o;
    o << "{\"ok\":true,\"message\":\"" << jsonEscape(msg) << "\","
      << "\"request\":" << requestJson(r) << ","
      << "\"queueSize\":" << g_sys.queue.size() << "}";
    return o.str();
}

/* spec 9 : statistics */
static string apiStats() {
    int c[STATUS_COUNT];
    g_sys.statistics(c);
    ostringstream o;
    o << "{\"ok\":true,"
      << "\"pending\":"    << c[ST_PENDING]    << ","
      << "\"processing\":" << c[ST_PROCESSING] << ","
      << "\"resolved\":"   << c[ST_RESOLVED]   << ","
      << "\"rejected\":"   << c[ST_REJECTED]   << ","
      << "\"total\":"      << (int)g_sys.allRequests.size() << ","
      << "\"queueSize\":"  << g_sys.queue.size() << ","
      << "\"queueMax\":"   << CircularQueue::MAX << "}";
    return o.str();
}

/* spec 6 : operator list for assignment */
static string apiOperators() {
    vector<Account*> ops = g_sys.operators();
    ostringstream o;
    o << "{\"ok\":true,\"items\":[";
    for (size_t i = 0; i < ops.size(); i++) {
        if (i) o << ",";
        o << "{\"username\":\"" << jsonEscape(ops[i]->username)
          << "\",\"name\":\""   << jsonEscape(ops[i]->fullName) << "\"}";
    }
    o << "]}";
    return o.str();
}

static string apiMe(Account* me) {
    if (!me) return errJson("Not signed in.");
    ostringstream o;
    o << "{\"ok\":true,\"name\":\"" << jsonEscape(me->fullName)
      << "\",\"reg\":\""  << jsonEscape(me->registrationNo)
      << "\",\"role\":\"" << jsonEscape(me->role)
      << "\",\"isAdmin\":" << (me->isAdmin ? "true" : "false") << "}";
    return o.str();
}

/* =============================================================================
   DISPATCH
   ============================================================================= */
struct HttpReq {
    string method, path;
    map<string, string> query, headers;
    string body;
};

static void handle(SOCKET s) {
    string raw;
    if (!recvRequest(s, raw)) { closesocket(s); return; }

    size_t he = raw.find("\r\n\r\n");
    if (he == string::npos) { closesocket(s); return; }

    HttpReq req;
    req.body = raw.substr(he + 4);

    string head = raw.substr(0, he);
    istringstream ls(head);
    string requestLine;
    getline(ls, requestLine);
    requestLine = trim(requestLine);

    istringstream rl(requestLine);
    string target, version;
    rl >> req.method >> target >> version;
    if (req.method.empty() || target.empty()) {
        sendAll(s, httpReply(400, "Bad Request", "text/plain", "Bad request"));
        closesocket(s);
        return;
    }

    string line;
    while (getline(ls, line)) {
        size_t c = line.find(':');
        if (c == string::npos) continue;
        req.headers[toLower(trim(line.substr(0, c)))] = trim(line.substr(c + 1));
    }

    size_t qm = target.find('?');
    if (qm != string::npos) {
        req.query = parseQuery(target.substr(qm + 1));
        target = target.substr(0, qm);
    }
    req.path = urlDecode(target);

    string reply;
    string type = "application/json";
    int code = 200;
    const char* status = "OK";

    EnterCriticalSection(&g_cs);

    bool isApi = req.path.compare(0, 5, "/api/") == 0;
    if (isApi) {
        Account* me = sessionFor(req.headers);
        string body;

        if      (req.path == "/api/login")     { map<string,string> sc; body = apiLogin(req.body, sc);
                                                 map<string,string>::iterator i = sc.find("X-Session");
                                                 if (i != sc.end()) req.headers["x-session-out"] = i->second; }
        else if (req.path == "/api/logout")    { map<string,string>::iterator i = req.headers.find("x-session");
                                                 if (i != req.headers.end()) g_sessions.erase(i->second);
                                                 body = okJson(); }
        else if (req.path == "/api/me")        body = apiMe(me);
        else if (req.path == "/api/submit")    body = apiSubmit(me, req.body);
        else if (req.path == "/api/queue")     body = isAdmin(me) ? apiQueue() : errJson("Admin access required.");
        else if (req.path == "/api/next")      body = isAdmin(me) ? apiNext()  : errJson("Admin access required.");
        else if (req.path == "/api/process")   body = apiProcess(me, req.body);
        else if (req.path == "/api/requests")  body = apiRequests(me);
        else if (req.path == "/api/search")    body = apiSearch(req.query);
        else if (req.path == "/api/status")    body = apiStatus(me, req.body);
        else if (req.path == "/api/stats")     body = isAdmin(me) ? apiStats()    : errJson("Admin access required.");
        else if (req.path == "/api/operators") body = isAdmin(me) ? apiOperators(): errJson("Admin access required.");
        else body = errJson("Unknown API endpoint.");

        reply = httpReply(200, "OK", type, body);
        printf("%-5s %-22s %s\n", req.method.c_str(), req.path.c_str(),
               body.compare(0, 11, "{\"ok\":false") == 0 ? "ERR" : "ok");
    } else {
        /* static frontend files */
        string file = req.path;
        if (file == "/" || file.empty()) file = "/index.html";
        if (file.find("..") != string::npos || file.find('\\') != string::npos) {
            code = 403; status = "Forbidden";
            reply = httpReply(code, status, "text/plain", "Forbidden");
        } else {
            string bytes;
            string full = g_publicDir + file;
            if (readFileBytes(full, bytes)) {
                type = mimeFor(full);
                reply = httpReply(200, "OK", type, bytes);
            } else {
                code = 404; status = "Not Found";
                reply = httpReply(code, status, "text/plain", "File not found");
            }
        }
        printf("%-5s %-22s %d\n", req.method.c_str(), req.path.c_str(), code);
    }

    LeaveCriticalSection(&g_cs);

    /* session token travels back as a cookie AND a header */
    map<string, string>::iterator tok = req.headers.find("x-session-out");
    if (tok != req.headers.end()) {
        size_t p = reply.find("\r\n");
        string setc = "\r\nSet-Cookie: sid=" + tok->second +
                      "; Path=/; HttpOnly; SameSite=Strict";
        reply.insert(p, setc);
    }

    sendAll(s, reply);
    closesocket(s);
}

static DWORD WINAPI clientThread(LPVOID param) {
    handle((SOCKET)param);
    return 0;
}

/* =============================================================================
   MAIN
   ============================================================================= */
int main() {
    SetConsoleOutputCP(CP_UTF8);
    InitializeCriticalSection(&g_cs);

    g_publicDir = exeDir() + "\\public";
    
    // Initialize SQLite database
    string dbPath = exeDir() + "\\helpdesk.db";
    if (!initDatabase(dbPath)) {
        printf("  [ERROR] Failed to initialize database.\n");
        return 1;
    }
    
    // Seed user accounts if database is empty
    seedUsers();
    
    // Load sample data (accounts only, no sample requests)
    g_sys.loadSampleData();
    
    // Load pending requests from database
    loadPendingRequests(&g_sys);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("  [ERROR] WSAStartup failed.\n");
        closeDatabase();
        return 1;
    }

    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) {
        printf("  [ERROR] Could not create socket.\n");
        WSACleanup();
        return 1;
    }

    int port = 0;
    for (int p = 8080; p <= 8085; p++) {
        sockaddr_in addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        addr.sin_port = htons((u_short)p);
        if (bind(listener, (sockaddr*)&addr, sizeof(addr)) == 0) { port = p; break; }
    }
    if (port == 0) {
        printf("  [ERROR] Could not bind to ports 8080-8085.\n");
        closesocket(listener);
        WSACleanup();
        closeDatabase();
        return 1;
    }

    if (listen(listener, 16) == 0) {
        char url[64];
        sprintf(url, "http://localhost:%d", port);
        printf("\n");
        printf("  +==================================================+\n");
        printf("  | UNIVERSITY REQUEST MANAGEMENT SYSTEM - WEB UI    |\n");
        printf("  | Circular Queue (FIFO) backend running            |\n");
        printf("  | 2500608 Ahmad Hussain  2500594 Jozza Batool      |\n");
        printf("  | 2500600 Raja wasif                               |\n");
        printf("  +==================================================+\n");
        printf("  Open : %s\n", url);
        printf("  Login:\n");
        printf("    Admin    : admin (admin123)\n");
        printf("    Operators: op1 (op1pass), op2 (op2pass)\n");
        printf("    Username : Ahmad  ·  Password: 1234\n");
        printf("  Press Ctrl+C to stop the server.\n\n");
        fflush(stdout);
        ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
    } else {
        printf("  [ERROR] listen() failed.\n");
        closesocket(listener);
        WSACleanup();
        closeDatabase();
        return 1;
    }

    for (;;) {
        SOCKET client = accept(listener, 0, 0);
        if (client == INVALID_SOCKET) break;
        HANDLE h = CreateThread(0, 0, clientThread, (LPVOID)client, 0, 0);
        if (h) CloseHandle(h);
        else closesocket(client);
    }

    closesocket(listener);
    WSACleanup();
    closeDatabase();
    DeleteCriticalSection(&g_cs);
    return 0;
}
