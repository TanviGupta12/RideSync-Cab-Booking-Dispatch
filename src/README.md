# RideSync — Graph-Based Smart Cab Booking and Dispatch System

**Team:** The Four Bytes · **Team ID:** DSCPP-III-2026-T151 · **Department of CSE, Graphic Era (Deemed to be University)**

RideSync models a city as a weighted graph, finds shortest routes with Dijkstra's algorithm, ranks available drivers with a priority queue, and manages bookings from request to completion. This repository contains **two front ends for the same idea**:

| Part | Technology | Storage | How to run |
|---|---|---|---|
| **Website** | Node.js 18+ (`server.js`) + HTML/CSS/JS (`public/`) | `data/ridesync.json` (+ optional Firebase Firestore) | `npm install`, then `npm start` |
| **Console app** | C++17 (STL) | `users.txt`, `bookings.txt` | `g++ …` / `make` (see below) |

> The website is a **parallel implementation** of the same workflow and algorithms. It does not call the C++ program and does not share its data files.

---

## 1. Run the website

**Requirements:** Node.js 18 or newer.

```bash
npm install      # first time only: installs firebase-admin
npm start        # starts http://localhost:3000
```

Open **http://localhost:3000**.

Use another port:
- Windows PowerShell: `$env:PORT=3001; npm start`
- macOS / Linux: `PORT=3001 npm start`

The website works **without any Firebase setup**. If Google credentials are not found, the server prints a warning and keeps all data in `data/ridesync.json`.

### Demo accounts
| Role | Phone | Password |
|---|---|---|
| Admin | `9999999999` | `admin123` |
| Driver | `9000000001` (also `…02`, `…03`, `…04`) | `driver123` |

Create a **Customer** account from the website to book rides.

---

## 2. Firebase Firestore (durable storage)

Without Firestore the website keeps everything in `data/ridesync.json`, which is erased whenever a free hosting tier restarts or redeploys. **With Firestore credentials, Firestore becomes the durable store**: on startup the server loads all users and bookings from the collections `users` and `bookings` (document id = record id, project `ridesync--cab-booking-system`), and every change is synced back within about half a second (and flushed on shutdown). The local JSON file is still written first and is used as the fallback if Firestore is unreachable; a Firestore problem is logged, retried every 30 s, and never blocks the site. If Firestore is empty at startup, existing local data is uploaded to it automatically. If Firestore already has data, **Firestore wins** over the local file.

Requirements: a Firestore database must be created in the Firebase project (Firebase console → Build → Firestore Database → Create database).

1. Firebase console → **Project settings → Service accounts → Generate new private key**.
2. Save the downloaded file inside the `keys/` folder (already ignored by Git). **Never commit it or share it.**
3. Point the server at it (or, on a hosting platform, paste the file's JSON into the `FIREBASE_SERVICE_ACCOUNT` variable), then start:
   - Windows PowerShell: `$env:GOOGLE_APPLICATION_CREDENTIALS="keys\your-key-file.json"; npm start`
   - macOS / Linux: `GOOGLE_APPLICATION_CREDENTIALS=keys/your-key-file.json npm start`
4. On startup you should see: `Firebase connected: new accounts are also saved to Firestore.`

Helper scripts (need the same credentials):
- `node firebase-test.js` — checks that Firestore can be reached.
- `node migrate-users.js` — copies every user from `data/ridesync.json` to Firestore (overwrites documents with the same id).

`public/firebase.js` only initialises the Firebase *web* SDK in the browser; the website's data is read and written through `server.js`.

---

## 3. Project structure

```
server.js              Web API: routing (Dijkstra), dispatch scoring, bookings, auth, persistence
public/index.html      Pages and forms
public/style.css       Responsive styling
public/app.js          Browser-side logic and API calls
public/firebase.js     Firebase web SDK initialisation
firebase-test.js       Firestore connectivity check
migrate-users.js       One-off copy of local users to Firestore
data/ridesync.json     Created on first run (ignored by Git)
package.json           Node dependencies and `npm start`

graph.hpp / graph.cpp          CityGraph (adjacency list), RoutePlanner (Dijkstra)
auth.hpp / auth.cpp            User, Customer, Driver, Admin, AuthenticationManager
dispatch.hpp / dispatch.cpp    DriverPriorityQueue, DriverDispatcher
system.hpp / system.cpp        FareCalculator, BookingManager, FileStorageManager
main.cpp                       Console menus
test_core.cpp                  Framework-free automated tests
Makefile                       Build, test and run shortcuts
```

---

## 4. Features and workflow

- **Everyone:** register, log in, profile.
- **Customer:** estimate route and fare, book a ride, view history, cancel a pending/confirmed booking, rate a completed ride.
- **Driver:** go online/offline, see the assigned ride, complete it (location moves to the drop point).
- **Admin:** metrics, all users and bookings, rank drivers for any pickup, process the waiting queue, route query.

**Rules**
- Route = shortest path on the 10-location Dehradun graph (distances are illustrative, not live GPS).
- Fare = `max(60, 40 + 12 × km)`, × 1.25 when 3 or more requests are waiting.
- Driver score = `0.6 × 1/(1 + km) + 0.4 × rating/5`; the highest score gets the ride, ties go to the lower id.
- With no free driver a booking stays `PENDING` and is served first-come-first-served when a driver becomes free.
- Passwords: 4+ characters; phone: exactly 10 digits.
- Names and vehicle text: `|`, carriage return and new line are replaced by a space; all other characters are kept.

---

## 5. Console app (C++17)

Needs a C++17 compiler (`g++`; on Windows use MinGW-w64 / MSYS2).

```bash
g++ -std=c++17 -Wall -Wextra -O2 -o ridesync main.cpp auth.cpp graph.cpp dispatch.cpp system.cpp
./ridesync                 # Windows: .\ridesync.exe
```

Run the tests:
```bash
g++ -std=c++17 -Wall -Wextra -I. -o test_core test_core.cpp auth.cpp graph.cpp dispatch.cpp system.cpp
./test_core                # last line: Passed: 64  Failed: 0
```

With `make` installed: `make`, `make test`, `make clean`. (`make web` runs `node server.js`.)

The console app stores data in `users.txt` and `bookings.txt` next to the program; the demo accounts above are created on first run.

---

## 6. Troubleshooting

| Problem | Cause / fix |
|---|---|
| Page shows "Could not connect to the RideSync server" | The server is not running. Run `npm start` and keep that terminal open. |
| `Cannot find module 'firebase-admin'` | Run `npm install` once. |
| Console warning "Firebase credentials not found" | Not an error. The site runs on `data/ridesync.json`. Follow section 2 to enable Firestore. |
| `Could not load the default credentials` in the log | `GOOGLE_APPLICATION_CREDENTIALS` is not set in the same terminal you started the server from. |
| `EADDRINUSE` / port already in use | Another program uses port 3000. Start with another `PORT` (section 1). |
| Letters missing from names such as "Ta vi" | Old data saved before the `safeText()` fix in `server.js`. Correct the names in `data/ridesync.json` (and re-run `node migrate-users.js` if you use Firestore). |
| Everyone is logged out after a restart | Sessions are kept in memory. Log in again. |
| `undefined reference to AuthenticationManager::…` when building C++ | `auth.cpp` must contain the implementation, not a copy of `auth.hpp`. |

---

## 7. Security notes

- Website passwords are stored as salted `scrypt` hashes (the salt is stored with the user record, including in Firestore).
- Login tokens are held in server memory (expire after 7 days) and in the browser's `localStorage`.
- Login is limited to 30 attempts per 15 minutes per IP, registration to 20 per hour. Security headers are sent on every response (HSTS in production).
- CORS is **off** by default because the bundled front end is same-origin. Set `CORS_ORIGIN` only if another website must call the API.
- In production (`NODE_ENV=production`) the admin and demo-driver passwords come from `ADMIN_PASSWORD` / `DEMO_DRIVER_PASSWORD`; if unset, random ones are generated (the admin one is printed once in the log).
- The C++ console app uses a simple educational hash.
- This is still a coursework project: no email/phone verification, no password reset, 4-character minimum passwords, and sessions are lost on restart. Treat it as a demo, not a production ride-hailing service.

---

## 8. Planned for Phase 3

Name-based location search and UI polish · automatic re-dispatch when a driver cancels · ride-in-progress status and ETA · admin editing of map and drivers · configurable fare and surge rules · connecting the website to a single shared data store · stress and edge-case testing.

---

## 9. Deploying online

The website is a single Node process (no build step). Any host that runs `npm start` works.

### Environment variables

| Variable | Required | Purpose |
|---|---|---|
| `PORT` | set by the host | Port to listen on (default 3000) |
| `NODE_ENV` | recommended: `production` | Enables HSTS, random seed passwords, proxy-aware rate limiting |
| `ADMIN_PASSWORD` | **yes** | Password for the admin account (phone `9999999999`), used when the account is first created |
| `DEMO_DRIVER_PASSWORD` | if you want to log in as demo drivers | Password for the 4 seeded drivers |
| `SEED_DEMO_DRIVERS` | no | `false` = do not create demo drivers |
| `DATA_DIR` | only if using a persistent disk instead of Firestore | Folder for `ridesync.json` |
| `FIREBASE_SERVICE_ACCOUNT` | **yes, to keep data on free hosts** | The whole service-account key JSON as one value (alternative to a key file) |
| `GOOGLE_APPLICATION_CREDENTIALS` | optional | Path to a service-account key file |
| `CORS_ORIGIN` | no | Allowed origin if another site calls the API |
| `TRUST_PROXY` | no | Defaults to `true` in production; read the client IP from `X-Forwarded-For` |

See `.env.example`. **Never commit real values.**

### Render (free tier)

1. Push this folder to a GitHub repository (`node_modules/`, `data/` and `keys/` are ignored by `.gitignore`).
2. On render.com: **New → Blueprint**, choose the repository. `render.yaml` configures everything; enter `ADMIN_PASSWORD` (and optionally `DEMO_DRIVER_PASSWORD`) when asked.
3. Also add `FIREBASE_SERVICE_ACCOUNT` (the key file's full JSON) - this is what keeps your data across restarts.
4. After the first deploy, open `https://<your-service>.onrender.com/api/health` - it should return `{"ok":true,...}`.

### Data persistence (Firestore)

Free hosting tiers erase the disk on every restart and redeploy. To keep your data, give the server the Firebase service-account key (section 2) as the `FIREBASE_SERVICE_ACCOUNT` environment variable. The startup log then shows `durable storage: Firestore` and `Loaded N users and M bookings from Firestore.`; if it says `local file only`, credentials were not found and data will be lost on restart. Check this line after your first deploy.

Alternative without Firestore: use a host with a persistent disk/volume and set `DATA_DIR` to its path (e.g. Render paid plan + Disk, Railway Volume, Fly.io Volume).

Login sessions are still held in memory, so users must log in again after a restart; their accounts and bookings are kept.

### Before you go public

- Use a strong `ADMIN_PASSWORD`.
- In the Firebase console, make sure **Firestore rules deny all client access** (the server uses the Admin SDK, which bypasses rules). Stored user documents contain password hashes.
- Optionally restrict the Firebase web API key in `public/firebase.js` to your site's domain in Google Cloud Console.

