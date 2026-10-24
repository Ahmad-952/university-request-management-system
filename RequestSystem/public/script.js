/* =============================================================================
   University Request Management System - Web frontend
   Talks to the C++ backend (server.cpp) over a small JSON API.
   The circular-queue / FIFO rules themselves live in C++ (requestcore.h);
   this file only renders state and collects input.
============================================================================= */
"use strict";

const API = {
  token: null,
  user:  null,

  async call(path, opts = {}) {
    const headers = { "Content-Type": "application/json", ...(opts.headers || {}) };
    if (this.token) headers["X-Session"] = this.token;
    const res = await fetch(path, { ...opts, headers });
    let data;
    try { data = await res.json(); }
    catch { throw new Error("Server error (" + res.status + ")"); }
    if (data && data.token) this.token = data.token;      // login response
    return data;
  },
  get(path)             { return this.call(path); },
  post(path, body)      { return this.call(path, { method: "POST", body: JSON.stringify(body || {}) }); },
  logout()              { return this.post("/api/logout"); }
};

/* ---------- tiny DOM helpers ---------- */
const $  = (id) => document.getElementById(id);
const show = (el) => { if (el) el.hidden = false; };
const hide = (el) => { if (el) el.hidden = true; };
const esc  = (s) => String(s ?? "")
  .replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;")
  .replace(/"/g,"&quot;");

let toastTimer = null;
function toast(msg, kind) {
  const t = $("toast");
  t.textContent = msg;
  t.className = "toast " + (kind || "");
  t.hidden = false;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { t.hidden = true; }, 3200);
}
function setErr(el, msg)   { el.textContent = msg || ""; el.hidden = !msg; }
function setOk(el, msg)    { el.textContent = msg || ""; el.hidden = !msg; }

/* =============================================================================
   VIEW SWITCHING
============================================================================= */
function showView(name) {
  hide($("viewLogin")); hide($("viewStudent")); hide($("viewAdmin"));
  if (name === "login")   show($("viewLogin"));
  if (name === "student") show($("viewStudent"));
  if (name === "admin")   show($("viewAdmin"));

  const box = $("whoBox");
  if (name === "login" || !API.user) { hide(box); return; }
  show(box);
  $("whoName").textContent = API.user.name;
  $("whoRole").textContent = API.user.role + (API.user.reg ? " · " + API.user.reg : "");
}

/* =============================================================================
   LOGIN  (spec 1 & 5)
============================================================================= */
let loginRole = "student";

$("roleTabs").addEventListener("click", (e) => {
  const btn = e.target.closest(".tab");
  if (!btn) return;
  loginRole = btn.dataset.role;
  [...$("roleTabs").children].forEach(c => c.classList.toggle("active", c === btn));
  $("loginUser").placeholder = loginRole === "admin" ? "e.g. admin" : "e.g. anas";
  setErr($("loginError"), "");
});

async function doLogin() {
  const u = $("loginUser").value.trim();
  const p = $("loginPass").value;

  if (!u) { setErr($("loginError"), "Please enter your username."); $("loginUser").focus(); return; }
  if (!p) { setErr($("loginError"), "Please enter your password."); $("loginPass").focus(); return; }
  setErr($("loginError"), "");

  try {
    const d = await API.post("/api/login", { username: u, password: p, role: loginRole });
    if (!d.ok) { setErr($("loginError"), d.error); return; }

    API.user = d;
    toast("Welcome, " + d.name + "!", "good");
    if (d.isAdmin) { showView("admin"); refreshAdmin(); }
    else           { showView("student"); refreshMyRequests(); }
  } catch (e) { setErr($("loginError"), e.message); }
}

$("btnLogin").addEventListener("click", doLogin);
$("loginPass").addEventListener("keydown", e => { if (e.key === "Enter") doLogin(); });
$("loginUser").addEventListener("keydown", e => { if (e.key === "Enter") $("loginPass").focus(); });

$("btnLogout").addEventListener("click", async () => {
  try { await API.logout(); } catch {}
  API.token = null; API.user = null;
  $("loginPass").value = "";
  setErr($("loginError"), "");
  setOk($("subOk"), ""); setErr($("subError"), "");
  hide($("myDetailCard")); hide($("queueCard")); hide($("processCard"));
  hide($("allCard")); hide($("statusCard"));
  $("myList").innerHTML = ""; $("searchList").innerHTML = "";
  showView("login");
  toast("Logged out.", "");
});

/* =============================================================================
   SHARED RENDERERS
============================================================================= */
function detailHtml(r) {
  if (!r) return '<div class="empty">No request to show.</div>';
  return '<div class="kv">' +
    '<b>Request ID</b><span>' + esc(r.id) + '</span>' +
    '<b>Status</b><span><span class="pill p-' + esc(r.status) + '">' + esc(r.status) + '</span></span>' +
    '<b>Student</b><span>' + esc(r.reg) + ' (' + esc(r.name) + ')</span>' +
    '<b>Category</b><span>' + esc(r.category) + '</span>' +
    '<b>Description</b><span>' + esc(r.description) + '</span>' +
    '<b>Submitted</b><span>' + esc(r.submitted) + '</span>' +
    '<b>Last Updated</b><span>' + esc(r.updated) + '</span>' +
    (r.operator  ? '<b>Assigned To</b><span>' + esc(r.operator) + '</span>' : '') +
    (r.note      ? '<b>Admin Note</b><span>' + esc(r.note) + '</span>' : '') +
    '</div>';
}

function itemHtml(r, opts = {}) {
  const pill  = '<span class="pill p-' + esc(r.status) + '">' + esc(r.status) + '</span>';
  const next  = opts.isNext ? '<span class="nexttag">NEXT</span>' : '';
  const meta  = esc(r.reg) + ' · ' + esc(r.category) + ' · ' + esc(r.submitted);
  const pos   = opts.pos ? ' pos" data-pos="' + opts.pos : '';
  return '<div class="item' + pos + '" data-id="' + esc(r.id) + '">' +
           '<div class="main"><div class="t1">' + esc(r.id) + ' ' + pill + ' ' + next + '</div>' +
           '<div class="t2">' + meta + '</div></div></div>';
}

function listHtml(items, emptyMsg, opts = {}) {
  if (!items || items.length === 0)
    return '<div class="empty">' + esc(emptyMsg) + '</div>';
  return items.map((r, i) => itemHtml(r, {
    pos:    opts.numbered ? (i + 1) : null,
    isNext: opts.isNext === r.id
  })).join("");
}

/* =============================================================================
   STUDENT  (spec 2 & 3)
============================================================================= */
$("subDesc").addEventListener("input", () => {
  $("descCount").textContent = $("subDesc").value.length;
});

async function refreshMyRequests() {
  try {
    const d = await API.get("/api/requests");
    if (!d.ok) { setErr($("subError"), d.error); return; }
    $("myList").innerHTML = listHtml(d.items, "You have not submitted any requests yet.");
    $("myList").querySelectorAll(".item").forEach(el =>
      el.addEventListener("click", () => showMyDetail(el.dataset.id)));
  } catch (e) { toast(e.message, "bad"); }
}

async function showMyDetail(id) {
  try {
    const d = await API.get("/api/requests");
    const r  = (d.items || []).find(x => x.id === id);
    $("myDetail").innerHTML = detailHtml(r);
    show($("myDetailCard"));
    $("myDetailCard").scrollIntoView({ behavior: "smooth", block: "nearest" });
  } catch (e) { toast(e.message, "bad"); }
}

$("btnMyRefresh").addEventListener("click", refreshMyRequests);

async function submitRequest() {
  const cat  = $("subCat").value;
  const desc = $("subDesc").value.trim();

  if (!cat)  { setErr($("subError"), "Please choose a category."); $("subCat").focus(); return; }
  if (!desc) { setErr($("subError"), "Please describe the complaint."); $("subDesc").focus(); return; }
  if (desc.length < 10) { setErr($("subError"), "Description is too short (min 10 characters)."); $("subDesc").focus(); return; }
  setErr($("subError"), ""); setOk($("subOk"), "");

  try {
    const d = await API.post("/api/submit", { category: cat, description: desc });
    if (!d.ok) { setErr($("subError"), d.error); return; }
    setOk($("subOk"), d.message + "  (position " + d.queueSize + " of " + d.queueMax + ")");
    toast(d.message, "good");
    $("subDesc").value = ""; $("descCount").textContent = "0";
    refreshMyRequests();
  } catch (e) { setErr($("subError"), e.message); }
}
$("btnSubmit").addEventListener("click", submitRequest);

/* =============================================================================
   ADMIN  (spec 4, 6, 7, 8, 9)
============================================================================= */
async function refreshStats() {
  try {
    const d = await API.get("/api/stats");
    if (!d.ok) return;
    $("sPending").textContent    = d.pending;
    $("sProcessing").textContent = d.processing;
    $("sResolved").textContent   = d.resolved;
    $("sRejected").textContent   = d.rejected;
    $("sTotal").textContent      = d.total;
  } catch {}
}
async function loadOperators() {
  try {
    const d = await API.get("/api/operators");
    const sel = $("procOperator");
    sel.innerHTML = '<option value="">-- choose operator --</option>';
    (d.items || []).forEach(o =>
      sel.insertAdjacentHTML("beforeend",
        '<option value="' + esc(o.username) + '">' + esc(o.name) + ' (' + esc(o.username) + ')</option>'));
  } catch {}
}
function refreshAdmin() {
  refreshStats(); loadOperators();
}

$("btnQueue").addEventListener("click", async () => {
  hide($("processCard")); hide($("allCard")); hide($("statusCard"));
  try {
    const d = await API.get("/api/queue");
    if (!d.ok) { toast(d.error, "bad"); return; }
    $("queueCount").textContent = d.size + " / " + d.max;
    $("queueList").innerHTML = listHtml(d.items, "Queue is empty - nothing pending.", {
      numbered: true, isNext: d.next
    });
    show($("queueCard"));
  } catch (e) { toast(e.message, "bad"); }
});

/* ---- process the NEXT request (spec 6) ---- */
$("btnProcess").addEventListener("click", async () => {
  hide($("queueCard")); hide($("allCard")); hide($("statusCard"));
  setErr($("procError"), ""); setOk($("procOk"), "");
  $("procNote").value = "";
  [...$("procChoices").querySelectorAll("input")].forEach(i => i.checked = false);
  hide($("opField")); hide($("noteField"));

  try {
    const d = await API.get("/api/next");
    if (!d.ok) { toast(d.error, "bad"); return; }
    if (d.empty) {
      $("processDetail").innerHTML = '<div class="empty">The queue is empty - nothing to process.</div>';
      show($("processCard"));
      return;
    }
    $("processDetail").innerHTML = detailHtml(d.request);
    $("btnProcGo").dataset.id = d.request.id;
    show($("processCard"));
  } catch (e) { toast(e.message, "bad"); }
});

$("procChoices").addEventListener("change", (e) => {
  const v = e.target.value;
  /* the operator dropdown belongs to option 1 ONLY - choosing "no operator"
     (or any other option) removes it from the form */
  const needOp   = (v === "process_op");
  const needNote = (v === "resolve" || v === "reject");

  if (needOp) show($("opField"));
  else { hide($("opField")); $("procOperator").value = ""; }

  if (needNote) show($("noteField"));
  else { hide($("noteField")); $("procNote").value = ""; }
});

$("btnProcClose").addEventListener("click", () => hide($("processCard")));

$("btnProcGo").addEventListener("click", async () => {
  const picked = $("procChoices").querySelector("input:checked");
  if (!picked) { setErr($("procError"), "Please choose what to do with this request."); return; }

  const action   = picked.value;
  const operator = $("procOperator").value;
  const note     = $("procNote").value.trim();

  if (action === "process_op" && !operator) {
    setErr($("procError"), "Select an operator to assign this request to."); return;
  }
  if ((action === "resolve" || action === "reject") && !note) {
    setErr($("procError"), "An admin note is required to resolve or reject a request."); return;
  }
  setErr($("procError"), ""); setOk($("procOk"), "");

  try {
    const d = await API.post("/api/process", { action, operator, note });
    if (!d.ok) { setErr($("procError"), d.error); return; }
    setOk($("procOk"), d.message);
    toast(d.message, "good");
    refreshStats();
    hide($("processCard"));
    if (d.next) $("btnQueue").click();        // show the new "next"
  } catch (e) { setErr($("procError"), e.message); }
});

/* ---- view all ---- */
$("btnAll").addEventListener("click", async () => {
  hide($("queueCard")); hide($("processCard")); hide($("statusCard"));
  try {
    const d = await API.get("/api/requests");
    if (!d.ok) { toast(d.error, "bad"); return; }
    $("allList").innerHTML = listHtml(d.items, "No requests yet.");
    $("allList").querySelectorAll(".item").forEach(el =>
      el.addEventListener("click", () => openStatus(el.dataset.id)));
    show($("allCard"));
  } catch (e) { toast(e.message, "bad"); }
});

/* ---- update status (spec 8) ---- */
async function openStatus(id) {
  try {
    const d = await API.get("/api/requests");
    const r  = (d.items || []).find(x => x.id === id);
    if (!r) { toast("Request not found.", "bad"); return; }
    if (r.status === "Resolved" || r.status === "Rejected") {
      toast(r.id + " is already closed (" + r.status + ").", "bad");
      return;
    }
    $("statusDetail").innerHTML = detailHtml(r);
    $("btnStGo").dataset.id = r.id;
    $("stSel").value = "";
    $("stNote").value = "";
    hide($("stNoteField"));
    setErr($("stError"), ""); setOk($("stOk"), "");
    show($("statusCard"));
    $("statusCard").scrollIntoView({ behavior: "smooth", block: "nearest" });
  } catch (e) { toast(e.message, "bad"); }
}
$("stSel").addEventListener("change", () => {
  const v = $("stSel").value;
  if (v === "Resolved" || v === "Rejected") show($("stNoteField")); else hide($("stNoteField"));
});
$("btnStClose").addEventListener("click", () => hide($("statusCard")));
$("btnStGo").addEventListener("click", async () => {
  const id     = $("btnStGo").dataset.id;
  const status = $("stSel").value;
  const note   = $("stNote").value.trim();

  if (!status) { setErr($("stError"), "Choose a new status."); return; }
  if ((status === "Resolved" || status === "Rejected") && !note) {
    setErr($("stError"), "An admin note is required to resolve or reject a request."); return;
  }
  setErr($("stError"), ""); setOk($("stOk"), "");

  try {
    const d = await API.post("/api/status", { id, status, note });
    if (!d.ok) { setErr($("stError"), d.error); return; }
    setOk($("stOk"), d.message);
    toast(d.message, "good");
    refreshStats();
    hide($("statusCard"));
    $("btnAll").click();
  } catch (e) { setErr($("stError"), e.message); }
});

/* ---- search (spec 7) ---- */
$("searchMode").addEventListener("change", () => {
  $("searchInput").placeholder = $("searchMode").value === "id"
    ? "REQ-0001" : "STD-2026-01";
  $("searchInput").focus();
});

$("btnSearch").addEventListener("click", async () => {
  const mode = $("searchMode").value;
  const val  = $("searchInput").value.trim();
  if (!val) {
    setErr($("searchError"), mode === "id"
      ? "Enter a request ID (e.g. REQ-0001)."
      : "Enter a registration number (e.g. STD-2026-01).");
    $("searchList").innerHTML = "";
    $("searchInput").focus();
    return;
  }
  if (mode === "id" && !/^REQ-\d{1,4}$/i.test(val)) {
    setErr($("searchError"), "Request IDs look like REQ-0001.");
    $("searchList").innerHTML = ""; $("searchInput").focus(); return;
  }
  setErr($("searchError"), "");

  try {
    const q = mode === "id" ? "id=" + encodeURIComponent(val)
                            : "reg=" + encodeURIComponent(val);
    const d = await API.get("/api/search?" + q);
    if (!d.ok) { setErr($("searchError"), d.error); $("searchList").innerHTML = ""; return; }
    if (d.count === 0) {
      $("searchList").innerHTML = '<div class="empty">No requests found for "' + esc(val) + '".</div>';
      return;
    }
    $("searchList").innerHTML = listHtml(d.items, "No results.");
    $("searchList").querySelectorAll(".item").forEach(el =>
      el.addEventListener("click", () => openStatus(el.dataset.id)));
  } catch (e) { setErr($("searchError"), e.message); }
});
$("searchInput").addEventListener("keydown", e => { if (e.key === "Enter") $("btnSearch").click(); });

/* =============================================================================
   BOOT
============================================================================= */
showView("login");
refreshStats();
