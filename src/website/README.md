# RideSync Cab Booking System — Website

This folder adds a browser-based interface for the RideSync C++ cab-booking project. It is a dependency-free Node.js web app and mirrors the project's core workflow: customer/driver registration and login, shortest-route calculation over the Dehradun graph, fare estimation, driver ranking, booking and cancellation, FIFO pending requests, ride completion, customer ratings, admin views, and persistent records.

## Requirements
- Node.js 18 or newer
- No npm package installation is needed

## Run
Open a terminal in this folder and run:

```bash
npm start
```

Then visit `http://localhost:3000`.

To use another port:
- Windows PowerShell: `$env:PORT=3001; npm start`
- macOS/Linux: `PORT=3001 npm start`

## Demo accounts
- Admin: `9999999999` / `admin123`
- Driver: `9000000001` / `driver123`
- Other seeded drivers: `9000000002`, `9000000003`, `9000000004` / `driver123`

Create a customer account from the website to book rides.

## Project structure
- `server.js` — API, graph routing, dispatch, booking lifecycle, authentication, persistence
- `public/index.html` — pages and forms
- `public/style.css` — responsive dashboard styling
- `public/app.js` — browser-side workflow and API calls
- `data/ridesync.json` — created automatically on first run; persistent web data

## Workflow
1. Register or log in.
2. Customers calculate a route/fare, book a ride, view history, cancel eligible rides and rate completed rides.
3. Drivers toggle availability, view assigned trips and complete active rides.
4. Admins view users/bookings/metrics, rank drivers, process the pending FIFO queue and query shortest routes.

## Important implementation notes
- The map and road distances are the illustrative graph included in the C++ source; this is not live GPS or a live map.
- The website stores its own records in `data/ridesync.json`. It does not directly call the existing C++ CLI process or share the C++ `users.txt` and `bookings.txt` files. The website backend is a parallel web implementation of the same documented workflow and algorithms.
- Passwords in the website are stored as salted scrypt hashes. This is stronger than the educational hash in the current C++ CLI, but this project is still a demonstration, not a production cab service.
- Do not expose the server directly to the public internet without additional security work (HTTPS, rate limiting, CSRF/security review, robust session storage and operational monitoring).
