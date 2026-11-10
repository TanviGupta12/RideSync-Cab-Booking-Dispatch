# RideSync — Phase 2 GitHub Upload Plan

Repo: https://github.com/TanviGupta12/RideSync-Cab-Booking-Dispatch

## Read this first: what is verified and what is not

| Status | Item |
|---|---|
| ✅ Verified in this session (Ubuntu, g++ 13.3, `-std=c++17 -Wall -Wextra`) | All 5 `.cpp` files compile with no warnings; each module compiles in the dependency order used below; scripted CLI session worked (register → login → book → complete → rate → admin views → files saved); `tests/test_core.cpp` (new, below) passes **61/61**; the Makefile `test` target below works; the FNV-1a hash fix and the `bookRide` customer check below compile and pass |
| ❌ NOT verified | **Windows** builds (MinGW or MSVC); **the contents of your GitHub repo** (access was not granted, so existing files are unknown to me); anything marked *proposed* below until you run it |
| ⚠️ Labelled "proposed" | The Phase 2 / Phase 3 split. I do not have the official phase requirements, so the allocation is my recommendation, not a rule |

**Preserve-the-repo rule:** nothing here deletes or overwrites anything. Step G2 makes each of you list what is already in the repo before adding a file. If a file I name already exists there, **stop and ask Tanvi** — don't overwrite it.

---

## A. Actual source code audit

Total: ~1,280 lines in 9 source files + Makefile (all from this conversation). Dependency order (a file only needs the ones above it):

`graph` → `auth` → `dispatch` (needs graph, auth) → `system` (needs all three) → `main.cpp` (needs all)

| File | Classes / functions (actual) | Status |
|---|---|---|
| `graph.hpp/.cpp` | `CityGraph`: `addLocation`, `addRoad`, `nameOf`, `nodeIds`, `neighbours`, `edgeCount`, `loadDefaultDehradun` (10 places, 13 roads), `print`. `RoutePlanner`: `shortestPath` (Dijkstra, min-heap), `distancesFrom` (single-source), `describe` | Complete, tested |
| `auth.hpp/.cpp` | `User`, `Customer`, `Driver` (availability flag, rating, `addRating`), `Admin`, `AuthenticationManager` (`registerCustomer/Driver/Admin`, `login`, `updateProfile`, `hashPassword`, `sanitize`, `ensureDefaultAdmin`, `addLoadedUser`) | Complete, tested. **Known weakness:** `hashPassword` uses `std::hash`, whose value is not guaranteed to be the same on different compilers → see commit 10 |
| `dispatch.hpp/.cpp` | `DriverPriorityQueue` (max-heap, tie → smaller id), `DriverDispatcher`: `compositeScore` (0.6·1/(1+km) + 0.4·rating/5), `buildQueue`, `findBestDriver`, `topCandidates` | Complete, tested |
| `system.hpp/.cpp` | `Booking` struct, `FareCalculator` (`calculate`), `BookingManager`: `bookRide`, `completeRide`, `cancelBooking`, `cancelLastBooking` (stack), `rateRide`, `processPending` (FIFO queue), `historyOf`, `activeBookingOfDriver`, `load`. `FileStorageManager`: `saveUsers/Bookings`, `loadUsers/Bookings` (in the same file, lines ~171–254 of `system.cpp`) | Complete, tested. **Known gap:** `bookRide` does not check that `customerId` is a real customer → see commit 11 |
| `main.cpp` | CLI: `customerMenu`, `driverMenu`, `adminMenu`, `registerFlow`, `loginFlow`, `profileMenu`, `seedDemoDrivers`, `App` wiring | Complete, manually tested |
| `Makefile` | `ridesync` and `clean` targets only | Complete; no test target yet |
| *(none)* | **There were no automated tests.** | Gap → `tests/test_core.cpp` (new, justified in C) |

Not implemented anywhere (do not claim these): GUI, real GPS/maps, payments, name-based location search, automatic re-dispatch when a driver cancels, secure password hashing, unit-test framework, MSVC-verified build.

---

## B. Phase 2 vs Phase 3 allocation (**proposed — not official**)

Because the code already exists as whole files, a Phase 2 upload cannot hold back "half a file". So the split is about **what you demonstrate and have tested/reviewed in Phase 2** versus **what you harden and extend in Phase 3**.

| Feature | Status now | Phase | Reason | Must work in the Phase 2 demo | Pending for Phase 3 |
|---|---|---|---|---|---|
| City graph (adjacency list) | Done | **2** | Foundation for everything | Show map (Admin → City map): 10 locations, 13 roads | Admin add/remove roads at run time |
| Dijkstra shortest path | Done | **2** | Core subject concept | Admin → Shortest route query: 1→9 = 18 km via 5 stops | Larger/disconnected graph stress tests |
| Register / login / roles | Done | **2** | Needed to use the app | Register customer + driver; wrong password rejected | Salted hash (bcrypt/Argon2) |
| Profile management, driver availability | Done | **2** | Required by project scope | Driver toggles AVAILABLE/OFFLINE | Input hardening |
| Priority-queue dispatch | Done | **2** | Core subject concept | Admin → Dispatcher ranking shows ordered drivers; booking picks the top one | Tunable weights, re-dispatch on cancel |
| Fare calculation | Done | **2** | Needed for a booking | 8 km trip = Rs. 136.00 | Configurable rates |
| Booking: book, complete, cancel, rate | Done | **2** | Core flow | Book → driver completes → customer rates; driver rating updates | Ride-in-progress status, ETA |
| Waiting queue (FIFO) | Done | **2** (light) | Demonstrates queue concept | Book with all drivers busy → PENDING → auto-assigned when a ride completes | Priority/timeout for waiting requests |
| Cancel-last-booking (stack) | Done | **2** (light) | Demonstrates stack concept | Cancel last → newest active booking cancelled | Full undo history |
| Surge multiplier (≥3 waiting → ×1.25) | Done | **3 emphasis** | Not needed for core demo; covered by a unit test | Optional | Make it configurable, document the rule |
| File persistence | Done | **2** | Required (`fstream`) | Quit, restart: users/bookings still there | Backup/recovery, a separate `storage.hpp/.cpp` |
| Automated tests | New | **2** | "Technical progress" evidence | `make test` → 61 passed | GoogleTest/Catch2, performance tests |
| Name-based location search, nicer CLI | Not built | **3** | Polish | — | Yes |
| Windows MSVC verification | Not verified | **3** | Needs your machines | MinGW g++ only in Phase 2 | Yes |

---

## C. File organization

Keep the flat layout you already have (all source at repo root, next to the existing reports/docs). **Do not move or rename any existing repo file.**

New files, and why each is genuinely needed:

| New file | Purpose |
|---|---|
| `.gitignore` | Stops compiled binaries and runtime `users.txt` / `bookings.txt` from being committed (they would cause constant merge conflicts, and the hashes inside differ per machine). *If a `.gitignore` already exists, append these lines instead of replacing it.* |
| `tests/test_core.cpp` | The only test file. New folder `tests/` keeps its own `main()` away from `main.cpp` |

Everything else (`README.md`, report, `main.tex`) is **edited or placed beside what already exists** — see G2 for the check. Do **not** commit: `ridesync`, `ridesync.exe`, `test_core(.exe)`, `*.o`, `users.txt`, `bookings.txt`.

---

## D. Individual responsibilities

| Member | Owns (files) | Functions to implement/explain | Tests they write/own |
|---|---|---|---|
| **Tanvi Gupta** (Lead, Booking) | `.gitignore`, `Makefile`, `auth.hpp/.cpp`, `main.cpp`; reviews and merges all PRs | Booking-creation side: `FareCalculator::calculate`, `BookingManager::bookRide`, `cancelBooking`, `cancelLastBooking`; `AuthenticationManager` register/login/updateProfile; CLI `customerMenu`, `registerFlow`, `loginFlow` | `testAuth`, fare checks, unknown-customer check (commit 11) |
| **Nishtha Tyagi** (Booking) | `system.hpp/.cpp` (first commit) | Booking-lifecycle side: `completeRide`, `rateRide`, `processPending`, `historyOf`, `activeBookingOfDriver`, `load`; `Driver`/`Admin` classes; CLI `driverMenu`, `adminMenu` | FIFO waiting-queue test, rating/cancel-pending checks, manual demo walkthrough |
| **Jahnavi** (Graph, Docs) | `graph.hpp/.cpp`, `README.md` Phase 2 section, report source | `CityGraph::*`, `RoutePlanner::shortestPath`, `distancesFrom`, `describe` | Graph tests incl. extra checks (commit 14); README + report |
| **Dhairya Sharma** (Priority queue, File handling, Testing) | `dispatch.hpp/.cpp`, `tests/test_core.cpp`, Makefile `test` target | `DriverPriorityQueue`, `DriverDispatcher::*`; owns **review and testing of `FileStorageManager`** and the password-hash fix | `testDispatch`, `testPersistence`, corrupt-file test |

Tanvi/Nishtha split inside the Booking Module: **Tanvi = creating and cancelling bookings + fares; Nishtha = what happens after a booking exists** (complete, rate, queue, history). `FileStorageManager` physically sits in `system.cpp`, so Dhairya's changes there must wait for Nishtha's PR to merge (Phase 3 option: split it into its own `storage.*` file).

Note: a file's first commit shows a single author; that's normal. Everyone's individual contribution shows through the later feature/fix/test/doc commits and through who can explain which functions. **Each person must commit from their own GitHub account — never commit as someone else.**

---

## E. Commit-by-commit plan (18 commits)

Format: `type(scope): message`. Merge each PR (branch) before the next one that depends on it.

| # | Member | Exact commit message | Files | What it adds | Test before committing | Depends on / result |
|---|---|---|---|---|---|---|
| 1 | Tanvi | `chore: add .gitignore for binaries and runtime data files` | `.gitignore` (create, or append) | Ignore rules | Build, then `git status` shows no `.exe`/`users.txt` | None. Clean status |
| 2 | Tanvi | `build: add Makefile for ridesync target` | `Makefile` | Build recipe | `make` (Linux/WSL) or the direct g++ command (H) | #1 |
| 3 | Jahnavi | `feat(graph): add CityGraph adjacency list and Dijkstra RoutePlanner` | `graph.hpp`, `graph.cpp` | Graph + shortest path | `g++ -std=c++17 -Wall -Wextra -c graph.cpp` (verified standalone) | #2. Module compiles alone |
| 4 | Tanvi | `feat(auth): add User, Customer, Driver, Admin and AuthenticationManager` | `auth.hpp`, `auth.cpp` | Accounts, login, profiles, availability flag | `g++ … -c auth.cpp` (verified standalone) | #2. **Parallel with #3** |
| 5 | Dhairya | `feat(dispatch): add DriverPriorityQueue and proximity-rating DriverDispatcher` | `dispatch.hpp`, `dispatch.cpp` | Driver ranking | `g++ … -c dispatch.cpp` (needs #3, #4 merged — verified) | #3, #4 |
| 6 | Nishtha | `feat(booking): add FareCalculator, BookingManager and FileStorageManager` | `system.hpp`, `system.cpp` | Booking lifecycle, queue/stack, file persistence | `g++ … -c system.cpp` | #3, #4, #5 |
| 7 | Tanvi | `feat(cli): add console menus and wire modules together in main` | `main.cpp` | **First runnable integrated app** | Full build + manual flow (I) | #3–#6. `ridesync` runs |
| 8 | Dhairya | `test: add framework-free core tests for graph, auth, dispatch, booking, storage` | `tests/test_core.cpp` | 61 checks | Build per H3 → `Passed: 61  Failed: 0` (verified) | #7 |
| 9 | Dhairya | `build: add test target to Makefile` | `Makefile` | `make test` | `make test` (verified) | #8. Tell Tanvi (she owns Makefile) |
| 10 | Dhairya | `fix(auth): use deterministic FNV-1a password hash instead of std::hash` | `auth.cpp` | Same hash on every compiler/OS | Tests pass 61/61; `hashPassword("admin123")` must print `18036240723408470531` on every teammate's PC (verified on Linux only). **Delete any old `users.txt` afterwards** | #8 |
| 11 | Tanvi | `fix(booking): reject bookings from unknown or non-customer accounts` | `system.cpp`, `tests/test_core.cpp` | `bookRide` checks `auth.getUser(customerId)` is a Customer | Add test lines (G below); verified manually: unknown id and a driver id both return "Unknown customer account." | #8 |
| 12 | Nishtha | `test(booking): add FIFO waiting-queue test` | `tests/test_core.cpp` | Verifies oldest PENDING booking gets the freed driver | Expected: booking 1 COMPLETED, 2 CONFIRMED, 3 PENDING (verified) | #8 |
| 13 | Nishtha | `test(booking): add rating-average and cancel-pending checks` | `tests/test_core.cpp` | *Proposed:* two ratings → average; cancel a PENDING booking → skipped by queue | Run `make test` (not yet verified) | #12 |
| 14 | Jahnavi | `test(graph): add distance symmetry and path-validity checks` | `tests/test_core.cpp` | *Proposed:* dist(a,b)==dist(b,a) for all node pairs; each step of a path is a real road | Run `make test` (not yet verified) | #8 |
| 15 | Dhairya | `test(storage): verify loader skips corrupt lines without crashing` | `tests/test_core.cpp` | *Proposed:* write a file with a bad line; loaders skip it | Run `make test` (not yet verified) | #8 |
| 16 | Jahnavi | `docs: add Phase 2 build, run and module overview to README` | `README.md` (**append only**) | Setup + module table | Follow your own steps on a fresh clone | #9 |
| 17 | Nishtha | `docs: add demo walkthrough and manual test steps to README` | `README.md` (append) | Demo logins and flow (section I) | Follow the steps | #16 merged first (practice conflict resolution) |
| 18 | Jahnavi | `docs: add Phase 2 report source` | report file (non-clashing name) | LaTeX report | Compile on Overleaf | After G2 check |

Tanvi creates the tag `phase2-submission` after all PRs merge (a tag, not a commit).

**Code for commit 10** — replace the body of `hashPassword` in `auth.cpp`:
```cpp
std::string AuthenticationManager::hashPassword(const std::string& plain) {
    // FNV-1a 64-bit: same output on every compiler/OS (std::hash is not portable),
    // so users.txt stays valid across teammates' machines. Educational only -
    // a real system would use bcrypt/argon2.
    unsigned long long h = 1469598103934665603ULL;
    for (unsigned char c : "ridesync$" + plain) { h ^= c; h *= 1099511628211ULL; }
    return std::to_string(h);
}
```

**Code for commit 11** — at the start of `BookingManager::bookRide` in `system.cpp`:
```cpp
    auto customer = auth.getUser(customerId);
    if (!customer || customer->getRole() != Role::CUSTOMER) { err = "Unknown customer account."; return false; }
```
and in `testBooking()`:
```cpp
    CHECK(!e.bm.bookRide(999, 1, 10, tmp, e.err));          // unknown customer id
    CHECK(!e.bm.bookRide(1, 1, 10, tmp, e.err));            // id 1 is a driver here
```

**Code for commit 12** — new function in `tests/test_core.cpp`, call it from `main()`:
```cpp
static void testWaitingQueueFifo() {      // Nishtha
    std::cout << "[queue]\n";
    CityGraph city; city.loadDefaultDehradun(); RoutePlanner pl(city); AuthenticationManager au;
    DriverDispatcher dp(au, pl); BookingManager bm(au, pl, dp); std::string err;
    au.registerDriver("D1", "9000000001", "pass", "c", 3, err);
    int c1 = au.registerCustomer("C1", "8000000001", "pass", 1, err)->getId();
    int c2 = au.registerCustomer("C2", "8000000002", "pass", 1, err)->getId();
    Booking a, b, d, t;
    bm.bookRide(c1, 3, 9, a, err);   // takes the only driver
    bm.bookRide(c1, 1, 10, b, err);  // PENDING, older
    bm.bookRide(c2, 4, 2, d, err);   // PENDING, newer
    CHECK(bm.pendingCount() == 2);
    bm.completeRide(a.driverId, t, err);
    CHECK(bm.all()[1].status == "CONFIRMED");   // older request served first
    CHECK(bm.all()[2].status == "PENDING");
}
```

---

## F. Stage-by-stage upload plan

| Stage | Commits | Who | Parallel? | Gate before moving on |
|---|---|---|---|---|
| 0 Setup | 1–2 | Tanvi | — | Everyone has cloned (G) |
| 1 Foundations | 3, 4 | Jahnavi ‖ Tanvi | **Parallel** (no shared files) | Both PRs merged |
| 2 Dispatch | 5 | Dhairya | Needs stage 1 | Merged |
| 3 Booking | 6 | Nishtha | Needs stages 1–2 | Merged |
| 4 Integration | 7 | Tanvi | Needs 1–3 | `ridesync` builds and the demo flow works (I) |
| 5 Tests | 8, 9 | Dhairya | Needs 4 | `make test` passes on Dhairya's PC |
| 6 Fixes + extra tests | 10, 11, 12, 13, 14, 15 | Dhairya, Tanvi, Nishtha, Jahnavi | **Parallel branches**, but all touch `tests/test_core.cpp` (10 and 11 touch `auth.cpp`/`system.cpp`). Merge one at a time; the next person runs `git pull` first | Tests still pass after each merge |
| 7 Docs | 16 → 17, 18 | Jahnavi, then Nishtha; Jahnavi | 16 before 17 (same file); 18 independent | README renders correctly on GitHub |
| 8 Release | tag | Tanvi | After everything | Fresh clone builds and passes tests |

While stages 2–3 wait on stage 1, Dhairya and Nishtha can read the merged `graph`/`auth` headers and prepare their branches locally. They can't *compile* their files until those PRs are merged.

---

## G. GitHub workflow (Windows, PowerShell, VS Code)

**Strategy:** `main` is the stable branch. Each person makes **one short-lived feature branch per commit group** (`feature/<name>-<topic>`), opens a Pull Request, Tanvi reviews and merges. No `develop` branch — with four people and a short phase it only adds confusion.

### G1. One-time setup (each member, own PC)
```powershell
git --version                                   # install Git for Windows if missing
git config --global user.name  "Your Real Name"
git config --global user.email "email-used-on-your-github-account"
cd $HOME\Documents                              # any folder you like
git clone https://github.com/TanviGupta12/RideSync-Cab-Booking-Dispatch.git
cd RideSync-Cab-Booking-Dispatch
code .                                          # opens VS Code here
```
Tanvi must add the other three as collaborators (GitHub → Settings → Collaborators) or they can't push; otherwise they fork and open PRs from forks.

### G2. Preserve the existing repo (everyone, before the first commit)
```powershell
git branch --show-current       # note the default branch: main or master (use that name below)
git ls-files                    # everything already in the repo
git log --oneline -10
```
Compare `git ls-files` with the files named in section E. If a name exists already (e.g. `README.md`, `Makefile`, `.gitignore`, `main.tex`), do **not** overwrite: append to it, or choose another name, or ask Tanvi. Never run `git rm`, `git push --force`, or `git reset --hard origin/...`.

### G3. Daily loop for each commit
```powershell
git checkout main                 # use master if that is your default
git pull
git checkout -b feature/jahnavi-graph
# copy your files into the repo folder, then:
git status                        # READ this. Only your intended files should be listed
git add graph.hpp graph.cpp       # add files by name; avoid "git add ."
git commit -m "feat(graph): add CityGraph adjacency list and Dijkstra RoutePlanner"
git push -u origin feature/jahnavi-graph
```
Then on GitHub: **Compare & pull request** → write what you changed and how you tested → assign Tanvi as reviewer.

**Merge option:** use **"Create a merge commit"** or **"Rebase and merge"** — **not "Squash"**, because squash collapses your individual commits into one and hides the commit messages in section E.

After a merge: `git checkout main; git pull; git branch -d feature/jahnavi-graph`.

### G4. Merge conflicts (most likely in `Makefile`, `README.md`, `tests/test_core.cpp`)
```powershell
git checkout feature/your-branch
git pull origin main              # reports CONFLICT in some file
```
In VS Code the file shows `<<<<<<<`, `=======`, `>>>>>>>` with buttons **Accept Current / Accept Incoming / Accept Both**. Pick the correct result (for tests or README it is usually **Accept Both**), save, then:
```powershell
g++ -std=c++17 -Wall -Wextra -I. -o test_core.exe tests/test_core.cpp auth.cpp graph.cpp dispatch.cpp system.cpp ; .\test_core.exe
git add <the file>
git commit                        # accept the default merge message
git push
```
If you get lost: `git merge --abort` returns to the state before the pull. Ask Tanvi before using anything stronger.

### G5. Windows notes
- Git may print `LF will be replaced by CRLF` warnings — harmless. Don't commit `.exe` files (see `.gitignore`).
- Install a compiler: MinGW-w64 / MSYS2 (`g++`). `make` often isn't installed on Windows — use the direct g++ commands in H, or use WSL.

---

## H. Compilation and testing

### H1. Build the app
Linux / WSL / MSYS2 (**verified on Linux**):
```bash
g++ -std=c++17 -Wall -Wextra -O2 -o ridesync main.cpp auth.cpp graph.cpp dispatch.cpp system.cpp
```
Windows PowerShell with MinGW g++ (**not verified here** — same command, different output name):
```powershell
g++ -std=c++17 -Wall -Wextra -O2 -o ridesync.exe main.cpp auth.cpp graph.cpp dispatch.cpp system.cpp
.\ridesync.exe
```
MSVC (`cl`) is **not verified**; `std::localtime` in `system.cpp` may need `/D_CRT_SECURE_NO_WARNINGS`. Use MinGW for Phase 2.

### H2. Per-module compile checks (use at each stage)
```powershell
g++ -std=c++17 -Wall -Wextra -c graph.cpp       # stage 1
g++ -std=c++17 -Wall -Wextra -c auth.cpp        # stage 1
g++ -std=c++17 -Wall -Wextra -c dispatch.cpp    # needs graph.hpp + auth.hpp present
g++ -std=c++17 -Wall -Wextra -c system.cpp      # needs the three above
```
No output = success. Delete the `.o` files afterwards (they're git-ignored).

### H3. Automated tests (after commit 8)
```powershell
g++ -std=c++17 -Wall -Wextra -I. -o test_core.exe tests/test_core.cpp auth.cpp graph.cpp dispatch.cpp system.cpp
.\test_core.exe
```
Expected last line: `Passed: 61  Failed: 0` (verified on Linux; the count rises as commits 11–15 add checks). After commit 9, Linux/WSL users can run `make test`.

---

## I. Phase 2 demonstration checklist

**Before the demo:** delete old `users.txt` / `bookings.txt` in your run folder for clean seed data. Seeded logins: admin `9999999999` / `admin123`; drivers `9000000001` … `9000000004` / `driver123` (Rakesh at Clock Tower, Anil at Graphic Era, Sunil at Railway Station, Mohit at FRI).

| # | Step | Expected result (verified on Linux) |
|---|---|---|
| 1 | Main menu → Register → Customer (10-digit phone, password ≥4, home location 1) | "Registration successful" |
| 2 | Register again with the same phone, or a 3-digit phone | Rejected with a message |
| 3 | Login with a wrong password | "Invalid phone or password." |
| 4 | Customer → Book ride, pickup **1**, drop **10** | Route *Graphic Era University → Ballupur Chowk → Forest Research Institute*, 8 km, **Rs. 136.00**, driver **Anil Rawat**, "0 km from your pickup" |
| 5 | Customer → Booking history | Shows booking #1 `CONFIRMED` |
| 6 | Log in as driver `9000000002` → Current ride → Complete ride | "Ride completed. Fare collected: Rs. 136.00"; driver moves to FRI |
| 7 | Customer → Rate ride #1 with 5 | "Thanks for rating!"; rating again is refused |
| 8 | Admin → Shortest route query 1 → 9 | 5 stops via Ballupur → Rispana → Clock Tower → Rajpur Rd, **18 km** |
| 9 | Admin → Dispatcher ranking for a pickup | Drivers listed best score first |
| 10 | Quit with `0`, restart the program, log in again | Users and booking still present (file persistence) |
| 11 | Book while all drivers are busy | `PENDING`; after a ride completes it becomes `CONFIRMED` |
| 12 | Customer → Cancel last booking | Newest active booking → `CANCELLED` |
| 13 | `.\test_core.exe` | `Failed: 0` |

**Final Phase 2 checklist**
- [ ] All 18 commits merged to `main` via PRs, each from the member's own account
- [ ] Fresh clone builds with the command in H1 and no warnings
- [ ] Tests pass on at least two members' PCs (so the hash fix is proven portable)
- [ ] `git ls-files` still lists every file that was in the repo at the start
- [ ] No `.exe`, `.o`, `users.txt`, `bookings.txt` committed
- [ ] README has build/run + demo steps; report source added
- [ ] Report Section 4 contribution table matches **section D above** (the earlier `main.tex` used a different split: it gave Dhairya only dispatch, and put file handling under Tanvi/Nishtha) — ask me to regenerate it if you want
- [ ] Each member can explain their own functions to the mentor
- [ ] Tag `phase2-submission` created by Tanvi

**Reserved for Phase 3 (proposed):** surge-rule configuration and documentation; split `FileStorageManager` into `storage.hpp/.cpp`; salted/strong password hashing; name-based location search and nicer CLI; admin add/remove roads and drivers; automatic re-dispatch on driver cancel; ride-in-progress status and ETA; booking-id hash-map index (currently a linear scan); GoogleTest/Catch2 and performance tests on large graphs; MSVC/Windows-wide verification; backup/recovery for data files.
