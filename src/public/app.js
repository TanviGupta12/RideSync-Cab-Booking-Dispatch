const $ = s => document.querySelector(s);
const $$ = s => [...document.querySelectorAll(s)];
let token = localStorage.getItem("ridesync_token") || "";
let me = null, locations = [], estimatedRoute = null, authMode = "login";
const locName = id => locations.find(x=>x.id===Number(id))?.name || "Unknown";
async function api(path, options={}) {
  const headers={"Content-Type":"application/json",...(options.headers||{})};
  if(token) headers.Authorization=`Bearer ${token}`;
  const res=await fetch(path,{...options,headers});
  const data=await res.json().catch(()=>({error:"Invalid server response"}));
  if(!res.ok) throw new Error(data.error||"Request failed.");
  return data;
}
function notify(message,type="success") {
  const el=$("#notice");el.textContent=message;el.className=`notice ${type}`;
  window.scrollTo({top:0,behavior:"smooth"});
  clearTimeout(notify.timer);notify.timer=setTimeout(()=>el.classList.add("hidden"),5000);
}
function escapeHtml(value) {return String(value??"").replace(/[&<>"']/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]));}
function setView(view) {
  $$(".view").forEach(x=>x.classList.add("hidden"));$(`#view-${view}`).classList.remove("hidden");
  $$(".nav-item").forEach(x=>x.classList.toggle("active",x.dataset.view===view));
  const titles={home:"Your ride, simplified.",book:"Book your next ride.",rides:"Your booking history.",driver:"Driver workspace.",admin:"Network control center."};
  $("#page-title").textContent=titles[view]||"RideSync";
  if(view==="rides")loadRides();if(view==="driver")loadDriver();if(view==="admin")loadAdmin();
}
function showAuth(mode) {
  authMode=mode;$("#auth-modal").classList.remove("hidden");
  $$(".auth-tab").forEach(x=>x.classList.toggle("active",x.dataset.auth===mode));
  $("#modal-title").textContent=mode==="login"?"Welcome back":"Create your account";
  $("#auth-submit").innerHTML=mode==="login"?"Log in <span>→</span>":"Create account <span>→</span>";
  ["name-row","role-row","location-row"].forEach(id=>$("#"+id).classList.toggle("hidden",mode!=="register"));
  $("#vehicle-row").classList.add("hidden");
  $("#auth-form").reset();
}
function closeAuth(){$("#auth-modal").classList.add("hidden");}
function renderUser() {
  const logged=Boolean(me);
  $("#login-open").classList.toggle("hidden",logged);$("#register-open").classList.toggle("hidden",logged);$("#logout").classList.toggle("hidden",!logged);
  $("#user-chip").classList.toggle("hidden",!logged);
  if(logged)$("#user-chip").textContent=`${me.name} · ${me.role.toLowerCase()}`;
  $$(".nav-item[data-view='book'],.nav-item[data-view='rides']").forEach(x=>x.classList.toggle("hidden",!logged||me.role!=="CUSTOMER"));
  $$(".nav-item[data-view='driver']").forEach(x=>x.classList.toggle("hidden",!logged||me.role!=="DRIVER"));
  $$(".nav-item[data-view='admin']").forEach(x=>x.classList.toggle("hidden",!logged||me.role!=="ADMIN"));
  if(!logged)setView("home");
}
function fillSelect(el,includeBlank=false) {
  el.innerHTML=(includeBlank?'<option value="">Choose a location</option>':"")+locations.map(l=>`<option value="${l.id}">${l.id}. ${escapeHtml(l.name)}</option>`).join("");
}
function statusPill(status){return `<span class="status ${escapeHtml(status)}">${escapeHtml(status)}</span>`;}
function empty(message="No records found yet.") {return `<div class="empty-state"><b>Nothing to show yet</b>${escapeHtml(message)}</div>`;}
function table(headers,rows) {
  if(!rows)return empty();
  return `<div class="table-scroll"><table><thead><tr>${headers.map(h=>`<th>${h}</th>`).join("")}</tr></thead><tbody>${rows}</tbody></table></div>`;
}
function bookingRows(bookings,admin=false,driver=false) {
  return bookings.map(b=>`<tr><td>#${b.id}</td>${admin?`<td>${escapeHtml(b.customerName)}</td>`:""}<td>${escapeHtml(b.pickupName)} → ${escapeHtml(b.dropName)}<small style="display:block;color:#939aab;margin-top:4px">${b.distanceKm} km</small></td>${admin||!driver?`<td>${escapeHtml(b.driverName||"Waiting")}</td>`:""}<td>₹${Number(b.fare).toFixed(2)}</td><td>${statusPill(b.status)}</td><td>${b.status==="COMPLETED"?(b.rating?`★ ${b.rating}`:(!admin&&!driver?`<button class="table-action" data-rate="${b.id}">Rate</button>`:"—")):(!admin&&!driver&&["PENDING","CONFIRMED"].includes(b.status)?`<button class="table-action" data-cancel="${b.id}">Cancel</button>`:"—")}</td></tr>`).join("");
}
async function loadRides() {
  if(!me)return;
  try {const {bookings}=await api("/api/bookings");$("#rides-list").innerHTML=table(["Booking","Route","Driver","Fare","Status","Review / action"],bookingRows(bookings));}
  catch(e){notify(e.message,"error");}
}
async function loadDriver() {
  if(!me||me.role!=="DRIVER")return;
  try {
    const [meData,records]=await Promise.all([api("/api/me"),api("/api/bookings")]);me=meData.user;
    const active=records.bookings.find(b=>b.status==="CONFIRMED");
    $("#driver-status").textContent=me.available?"Available for trips":"Offline";
    $("#toggle-availability").textContent=me.available?"Go offline":"Go online";
    $("#driver-profile").innerHTML=`<b>${escapeHtml(me.vehicle)}</b><br>Driver ID: #${me.id}<br>Rating: ★ ${Number(me.rating||4).toFixed(2)} (${me.ratingCount||0} ratings)<br>Current location: ${escapeHtml(locName(me.location))}`;
    $("#active-ride").innerHTML=active?`<div class="active-ride-card"><h3>Booking #${active.id}</h3><p><b>${escapeHtml(active.pickupName)}</b><br>↓ ${active.distanceKm} km<br><b>${escapeHtml(active.dropName)}</b></p><p>Customer: ${escapeHtml(active.customerName)}<br>Fare: ₹${Number(active.fare).toFixed(2)}</p></div>`:empty("No active ride assigned.");
    $("#complete-ride").classList.toggle("hidden",!active);
    $("#driver-rides").innerHTML=table(["Booking","Route","Fare","Status"],bookingRows(records.bookings,false,true));
  } catch(e){notify(e.message,"error");}
}
async function loadAdmin() {
  if(!me||me.role!=="ADMIN")return;
  try {
    const [summary,records,users]=await Promise.all([api("/api/summary"),api("/api/bookings"),api("/api/users")]);
    const stats=[["Total bookings",summary.totalBookings,"purple"],["Available drivers",summary.availableDrivers,"green"],["Waiting requests",summary.pending,"amber"],["Completed revenue",`₹${summary.revenue.toFixed(0)}`,"blue"]];
    $("#admin-stats").innerHTML=stats.map(s=>`<article class="stat-card"><div class="stat-icon ${s[2]}">◈</div><span>NETWORK METRIC</span><strong>${s[1]}</strong><small>${s[0]}</small></article>`).join("");
    $("#admin-bookings").innerHTML=table(["Booking","Customer","Route","Driver","Fare","Status","Rating"],bookingRows(records.bookings,true));
    $("#admin-users").innerHTML=table(["ID","Name","Phone","Role","Details"],users.users.map(u=>`<tr><td>#${u.id}</td><td>${escapeHtml(u.name)}</td><td>${escapeHtml(u.phone)}</td><td>${escapeHtml(u.role)}</td><td>${u.role==="DRIVER"?`${escapeHtml(u.vehicle)} · ${u.available?"Available":"Offline"} · ★ ${Number(u.rating||4).toFixed(1)}`:u.role==="CUSTOMER"?escapeHtml(locName(u.homeLocation)):"Administrator"}</td></tr>`));
  } catch(e){notify(e.message,"error");}
}
async function refreshMe() {
  if(!token)return renderUser();
  try {me=(await api("/api/me")).user;}catch{token="";me=null;localStorage.removeItem("ridesync_token");}
  renderUser();
}
function updateVehicleField(){const driver=$("#auth-role").value==="DRIVER"&&authMode==="register";$("#vehicle-row").classList.toggle("hidden",!driver);$("#location-row").classList.toggle("hidden",authMode!=="register");}
async function init() {
  try {
    const data=await api("/api/locations");locations=data.locations;
    [$("#pickup"),$("#drop"),$("#auth-location"),$("#admin-pickup"),$("#route-from"),$("#route-to")].forEach(el=>fillSelect(el));
    $("#pickup").value="1";$("#drop").value="10";$("#auth-location").value="1";$("#admin-pickup").value="3";$("#route-from").value="1";$("#route-to").value="9";
    await refreshMe();
  } catch(e){notify("Could not connect to the RideSync server. Start it with npm start.", "error");}
}
$("#login-open").addEventListener("click",()=>showAuth("login"));
$("#register-open").addEventListener("click",()=>showAuth("register"));
$("#hero-book").addEventListener("click",()=>{if(!me)showAuth("login");else if(me.role==="CUSTOMER")setView("book");else notify("Ride booking is available to customer accounts.","error");});
$$("[data-close]").forEach(x=>x.addEventListener("click",closeAuth));
$$(".auth-tab").forEach(x=>x.addEventListener("click",()=>showAuth(x.dataset.auth)));
$("#auth-role").addEventListener("change",updateVehicleField);
$("#auth-form").addEventListener("submit",async e=>{
  e.preventDefault();
  try {
    const body={phone:$("#auth-phone").value.trim(),password:$("#auth-password").value};
    const endpoint=authMode==="login"?"/api/login":"/api/register";
    if(authMode==="register"){body.name=$("#auth-name").value;body.role=$("#auth-role").value;body.location=Number($("#auth-location").value);body.vehicle=$("#auth-vehicle").value;}
    const data=await api(endpoint,{method:"POST",body:JSON.stringify(body)});
    if(authMode==="login"){token=data.token;localStorage.setItem("ridesync_token",token);me=data.user;}else{notify("Account created. Log in with your new credentials.");}
    closeAuth();renderUser();
    if(authMode==="login"){setView(me.role==="CUSTOMER"?"book":me.role==="DRIVER"?"driver":"admin");notify(`Welcome, ${me.name}!`);}
  }catch(e){notify(e.message,"error");}
});
$("#logout").addEventListener("click",async()=>{try{await api("/api/logout",{method:"POST"});}catch{}token="";me=null;localStorage.removeItem("ridesync_token");renderUser();notify("You have logged out.");});
$$(".nav-item").forEach(x=>x.addEventListener("click",()=>{if(!me&&["book","rides","driver","admin"].includes(x.dataset.view)){showAuth("login");return;}setView(x.dataset.view);}));
$("#estimate-btn").addEventListener("click",async()=>{
  try{
    const pickup=Number($("#pickup").value),drop=Number($("#drop").value);
    if(pickup===drop)throw new Error("Pickup and drop locations must be different.");
    estimatedRoute=await api(`/api/route?from=${pickup}&to=${drop}`);
    $("#estimate").innerHTML=`<div class="estimate-top"><div><span class="eyebrow">ESTIMATED FARE</span><small>${estimatedRoute.distanceKm} km · shortest route</small></div><strong>₹${estimatedRoute.fare.toFixed(2)}</strong></div><div class="estimate-route">${escapeHtml(estimatedRoute.description)}</div>`;
    $("#estimate").classList.remove("hidden");$("#confirm-booking").classList.remove("hidden");
  }catch(e){notify(e.message,"error");}
});
$("#pickup").addEventListener("change",()=>{$("#estimate").classList.add("hidden");$("#confirm-booking").classList.add("hidden");});
$("#drop").addEventListener("change",()=>{$("#estimate").classList.add("hidden");$("#confirm-booking").classList.add("hidden");});
$("#booking-form").addEventListener("submit",async e=>{
  e.preventDefault();if(!me){showAuth("login");return;}
  try{const data=await api("/api/bookings",{method:"POST",body:JSON.stringify({pickup:Number($("#pickup").value),drop:Number($("#drop").value)})});notify(`Booking #${data.booking.id} created: ${data.booking.status}.`);$("#estimate").classList.add("hidden");$("#confirm-booking").classList.add("hidden");setView("rides");}
  catch(e){notify(e.message,"error");}
});
document.addEventListener("click",async e=>{
  const cancel=e.target.closest("[data-cancel]"),rate=e.target.closest("[data-rate]");
  if(cancel&&confirm(`Cancel booking #${cancel.dataset.cancel}?`)){
    try{await api(`/api/bookings/${cancel.dataset.cancel}/cancel`,{method:"POST",body:"{}"});notify("Booking cancelled.");await loadRides();}catch(err){notify(err.message,"error");}
  }
  if(rate){
    const value=prompt("Rate your completed ride from 1 to 5 stars:");
    if(value===null)return;
    try{await api(`/api/bookings/${rate.dataset.rate}/rate`,{method:"POST",body:JSON.stringify({stars:Number(value)})});notify("Thank you for rating your driver.");await loadRides();}
    catch(err){notify(err.message,"error");}
  }
});
$("#refresh-rides").addEventListener("click",loadRides);
$("#toggle-availability").addEventListener("click",async()=>{
  try{const d=await api("/api/driver/availability",{method:"POST",body:JSON.stringify({available:!me.available})});me=d.user;notify(`You are now ${me.available?"available":"offline"}.`);await loadDriver();}
  catch(e){notify(e.message,"error");}
});
$("#complete-ride").addEventListener("click",async()=>{try{const d=await api("/api/driver/complete",{method:"POST",body:"{}"});me=d.user;notify("Ride completed. The next waiting request, if any, has been assigned.");await loadDriver();}catch(e){notify(e.message,"error");}});
$("#admin-refresh").addEventListener("click",loadAdmin);
$("#rank-drivers").addEventListener("click",async()=>{
  try{const data=await api(`/api/dispatch?pickup=${Number($("#admin-pickup").value)}`);$("#dispatch-results").innerHTML=data.candidates.length?data.candidates.map((c,i)=>`<div class="rank-row"><span class="rank-no">${i+1}</span><div class="rank-info"><b>${escapeHtml(c.driver.name)}</b><small>${escapeHtml(c.driver.vehicle)} · ${c.distanceKm} km away · ★ ${Number(c.rating).toFixed(2)}</small></div><span class="rank-score">${c.score.toFixed(3)}</span></div>`).join(""):empty("No available drivers can reach this pickup.");}
  catch(e){notify(e.message,"error");}
});
$("#process-pending").addEventListener("click",async()=>{try{const d=await api("/api/admin/process-pending",{method:"POST",body:"{}"});$("#pending-result").textContent=`Assigned ${d.assigned} request(s). ${d.pending} still waiting.`;notify("Waiting queue processed.");await loadAdmin();}catch(e){notify(e.message,"error");}});
$("#admin-route").addEventListener("click",async()=>{
  try{const d=await api(`/api/route?from=${Number($("#route-from").value)}&to=${Number($("#route-to").value)}`);$("#admin-route-result").innerHTML=`<div class="route-result"><b>${d.distanceKm} km · ₹${d.fare.toFixed(2)}</b><br>${escapeHtml(d.description)}</div>`;}
  catch(e){notify(e.message,"error");}
});
init();
