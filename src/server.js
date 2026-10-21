const http = require("http");
const fs = require("fs");
const path = require("path");
const crypto = require("crypto");

const ROOT = __dirname;
const PUBLIC = path.join(ROOT, "public");
const DATA_DIR = path.join(ROOT, "data");
const DATA_FILE = path.join(DATA_DIR, "ridesync.json");
const PORT = Number(process.env.PORT || 3000);

const locations = [
  {id:1,name:"Graphic Era University"},
  {id:2,name:"ISBT Dehradun"},
  {id:3,name:"Clock Tower"},
  {id:4,name:"Railway Station"},
  {id:5,name:"Rajpur Road"},
  {id:6,name:"Ballupur Chowk"},
  {id:7,name:"Prem Nagar"},
  {id:8,name:"Rispana Bridge"},
  {id:9,name:"Sahastradhara Road"},
  {id:10,name:"Forest Research Institute"}
];
const roads = [[1,6,2],[1,7,5],[6,8,4],[6,10,6],[7,10,3],[7,2,8],[8,3,2],[8,4,4],[3,4,1],[3,5,3],[2,4,3],[5,9,7],[10,5,5]];
let state = {users:[],bookings:[],nextUserId:1,nextBookingId:1};
const sessions = new Map();

function hashPassword(password, salt) {
  return crypto.scryptSync(password, salt, 64).toString("hex");
}
function makeUser(role, name, phone, password, extra={}) {
  const salt = crypto.randomBytes(16).toString("hex");
  return {id:state.nextUserId++,role,name:safeText(name),phone,passwordSalt:salt,passwordHash:hashPassword(password,salt),...extra};
}
function safeText(s) { return String(s || "").replace(/[|\\r\\n]/g," ").trim(); }
function save() {
  fs.mkdirSync(DATA_DIR,{recursive:true});
  const temp = DATA_FILE + ".tmp";
  fs.writeFileSync(temp,JSON.stringify(state,null,2));
  fs.renameSync(temp,DATA_FILE);
}
function seed() {
  if (fs.existsSync(DATA_FILE)) {
    try { state = JSON.parse(fs.readFileSync(DATA_FILE,"utf8")); } catch { state = {users:[],bookings:[],nextUserId:1,nextBookingId:1}; }
  }
  if (!Array.isArray(state.users)) state.users=[];
  if (!Array.isArray(state.bookings)) state.bookings=[];
  state.nextUserId = Math.max(Number(state.nextUserId)||1,...state.users.map(u=>u.id+1),1);
  state.nextBookingId = Math.max(Number(state.nextBookingId)||1,...state.bookings.map(b=>b.id+1),1);
  if (!state.users.some(u=>u.role==="ADMIN")) state.users.push(makeUser("ADMIN","System Admin","9999999999","admin123"));
  const demo = [
    ["Rakesh Negi","9000000001","Swift UK07CA1001",3],
    ["Anil Rawat","9000000002","WagonR UK07CB2002",1],
    ["Sunil Thapa","9000000003","Dzire UK07CC3003",4],
    ["Mohit Bisht","9000000004","Ertiga UK07CD4004",10]
  ];
  for (const [name,phone,vehicle,location] of demo) {
    if (!state.users.some(u=>u.phone===phone)) state.users.push(makeUser("DRIVER",name,phone,"driver123",{vehicle,location,rating:4,ratingCount:0,available:true}));
  }
  state.nextUserId=Math.max(state.nextUserId,...state.users.map(u=>u.id+1));
  save();
}
function getUser(id) { return state.users.find(u=>u.id===Number(id)); }
function publicUser(u) {
  const {passwordHash,passwordSalt,...safe}=u;
  if (safe.role==="DRIVER") safe.activeRide=state.bookings.find(b=>b.driverId===u.id && b.status==="CONFIRMED") || null;
  return safe;
}
function graph() {
  const adj = new Map(locations.map(l=>[l.id,[]]));
  for(const [a,b,w] of roads) {adj.get(a).push([b,w]);adj.get(b).push([a,w]);}
  return adj;
}
function shortestPath(start,end) {
  start=Number(start);end=Number(end);
  const adj=graph();
  if(!adj.has(start)||!adj.has(end)) return null;
  const dist=new Map(locations.map(l=>[l.id,Infinity])),prev=new Map(),todo=new Set(locations.map(l=>l.id));
  dist.set(start,0);
  while(todo.size) {
    let u=null,best=Infinity;
    for(const id of todo) if(dist.get(id)<best){best=dist.get(id);u=id;}
    if(u===null) break;
    todo.delete(u);
    if(u===end) break;
    for(const [v,w] of adj.get(u)) if(todo.has(v)&&best+w<dist.get(v)){dist.set(v,best+w);prev.set(v,u);}
  }
  if(!Number.isFinite(dist.get(end))) return null;
  const ids=[];let cur=end;ids.unshift(cur);
  while(cur!==start){cur=prev.get(cur);if(cur===undefined)return null;ids.unshift(cur);}
  return {distanceKm:dist.get(end),path:ids,description:ids.map(id=>locations.find(l=>l.id===id).name).join(" → ")};
}
function driverDistance(driver, pickup) {
  const r=shortestPath(driver.location,pickup);
  return r ? r.distanceKm : Infinity;
}
function candidates(pickup) {
  return state.users.filter(u=>u.role==="DRIVER"&&u.available&&!state.bookings.some(b=>b.driverId===u.id&&b.status==="CONFIRMED"))
    .map(d=>({driver:d,distanceKm:driverDistance(d,Number(pickup))}))
    .filter(x=>Number.isFinite(x.distanceKm))
    .map(x=>({...x,score:0.6*(1/(1+x.distanceKm))+0.4*((Number(x.driver.rating)||4)/5)}))
    .sort((a,b)=>b.score-a.score||a.driver.id-b.driver.id);
}
function fare(km, surge=1) { return Math.round(Math.max(60,40+12*km)*surge*100)/100; }
function assign(booking) {
  const c=candidates(booking.pickup)[0];
  if(!c) return false;
  c.driver.available=false;
  booking.driverId=c.driver.id;
  booking.pickupDistKm=c.distanceKm;
  booking.status="CONFIRMED";
  return true;
}
function processPending() {
  let assigned=0;
  for(const b of state.bookings.filter(x=>x.status==="PENDING").sort((a,b)=>a.id-b.id)) {
    if(!assign(b)) break;
    assigned++;
  }
  return assigned;
}
function send(res,status,data,headers={}) {
  const body=JSON.stringify(data);
  res.writeHead(status,{"Content-Type":"application/json; charset=utf-8","Content-Length":Buffer.byteLength(body),"Access-Control-Allow-Origin":"*","Access-Control-Allow-Headers":"Content-Type, Authorization","Access-Control-Allow-Methods":"GET,POST,PUT,DELETE,OPTIONS",...headers});
  res.end(body);
}
function readBody(req) {
  return new Promise((resolve,reject)=>{
    let data="";
    req.on("data",chunk=>{data+=chunk;if(data.length>1e6){reject(new Error("Request too large"));req.destroy();}});
    req.on("end",()=>{try{resolve(data?JSON.parse(data):{});}catch{reject(new Error("Invalid JSON body"));}});
    req.on("error",reject);
  });
}
function auth(req) {
  const token=(req.headers.authorization||"").replace(/^Bearer\s+/i,"");
  const id=sessions.get(token);
  return id ? getUser(id) : null;
}
function requireAuth(req,res,roles) {
  const u=auth(req);
  if(!u){send(res,401,{error:"Please log in first."});return null;}
  if(roles&&!roles.includes(u.role)){send(res,403,{error:"You do not have permission for this action."});return null;}
  return u;
}
function error(res,status,message){send(res,status,{error:message});}
async function api(req,res,url) {
  if(req.method==="OPTIONS"){res.writeHead(204,{"Access-Control-Allow-Origin":"*","Access-Control-Allow-Headers":"Content-Type, Authorization","Access-Control-Allow-Methods":"GET,POST,PUT,DELETE,OPTIONS"});return res.end();}
  const p=url.pathname;
  if(req.method==="GET"&&p==="/api/health") return send(res,200,{ok:true,service:"RideSync API"});
  if(req.method==="GET"&&p==="/api/locations") return send(res,200,{locations,roads});
  if(req.method==="POST"&&p==="/api/register") {
    const b=await readBody(req),role=String(b.role||"CUSTOMER").toUpperCase(),name=safeText(b.name),phone=String(b.phone||"").trim(),password=String(b.password||""),location=Number(b.location);
    if(!["CUSTOMER","DRIVER"].includes(role)) return error(res,400,"Choose customer or driver registration.");
    if(!name) return error(res,400,"Name is required.");
    if(!/^\d{10}$/.test(phone)) return error(res,400,"Phone number must be exactly 10 digits.");
    if(state.users.some(u=>u.phone===phone)) return error(res,409,"This phone number is already registered.");
    if(password.length<4) return error(res,400,"Password must contain at least 4 characters.");
    if(!locations.some(l=>l.id===location)) return error(res,400,"Choose a valid location.");
    if(role==="DRIVER"&&!safeText(b.vehicle)) return error(res,400,"Vehicle details are required.");
    const u=makeUser(role,name,phone,password,role==="CUSTOMER"?{homeLocation:location}:{location,vehicle:safeText(b.vehicle),rating:4,ratingCount:0,available:true});
    state.users.push(u);save();
    return send(res,201,{user:publicUser(u)});
  }
  if(req.method==="POST"&&p==="/api/login") {
    const b=await readBody(req),phone=String(b.phone||"").trim(),password=String(b.password||"");
    const u=state.users.find(x=>x.phone===phone);
    if(!u||hashPassword(password,u.passwordSalt)!==u.passwordHash) return error(res,401,"Invalid phone number or password.");
    const token=crypto.randomBytes(32).toString("hex");sessions.set(token,u.id);
    return send(res,200,{token,user:publicUser(u)});
  }
  if(req.method==="POST"&&p==="/api/logout") {
    const token=(req.headers.authorization||"").replace(/^Bearer\s+/i,"");sessions.delete(token);
    return send(res,200,{ok:true});
  }
  if(req.method==="GET"&&p==="/api/me") {
    const u=requireAuth(req,res);if(!u)return;
    return send(res,200,{user:publicUser(u)});
  }
  if(req.method==="GET"&&p==="/api/summary") {
    const u=requireAuth(req,res,["ADMIN"]);if(!u)return;
    return send(res,200,{users:state.users.length,customers:state.users.filter(x=>x.role==="CUSTOMER").length,drivers:state.users.filter(x=>x.role==="DRIVER").length,availableDrivers:state.users.filter(x=>x.role==="DRIVER"&&x.available&&!state.bookings.some(b=>b.driverId===x.id&&b.status==="CONFIRMED")).length,totalBookings:state.bookings.length,pending:state.bookings.filter(b=>b.status==="PENDING").length,completed:state.bookings.filter(b=>b.status==="COMPLETED").length,revenue:state.bookings.filter(b=>b.status==="COMPLETED").reduce((n,b)=>n+b.fare,0)});
  }
  if(req.method==="GET"&&p==="/api/users") {
    const u=requireAuth(req,res,["ADMIN"]);if(!u)return;
    return send(res,200,{users:state.users.map(publicUser)});
  }
  if(req.method==="GET"&&p==="/api/bookings") {
    const u=requireAuth(req,res);if(!u)return;
    let items=state.bookings;
    if(u.role==="CUSTOMER")items=items.filter(b=>b.customerId===u.id);
    if(u.role==="DRIVER")items=items.filter(b=>b.driverId===u.id);
    return send(res,200,{bookings:items.slice().sort((a,b)=>b.id-a.id).map(b=>bookingView(b))});
  }
  if(req.method==="GET"&&p==="/api/dispatch") {
    const u=requireAuth(req,res,["ADMIN"]);if(!u)return;
    const pickup=Number(url.searchParams.get("pickup"));
    if(!locations.some(l=>l.id===pickup))return error(res,400,"Choose a valid pickup location.");
    return send(res,200,{candidates:candidates(pickup).map(c=>({driver:publicUser(c.driver),distanceKm:c.distanceKm,rating:c.driver.rating,score:Number(c.score.toFixed(3))}))});
  }
  if(req.method==="GET"&&p==="/api/route") {
    const u=requireAuth(req,res);if(!u)return;
    const route=shortestPath(url.searchParams.get("from"),url.searchParams.get("to"));
    if(!route)return error(res,400,"No route exists between those locations.");
    return send(res,200,{...route,fare:fare(route.distanceKm)});
  }
  if(req.method==="POST"&&p==="/api/bookings") {
    const u=requireAuth(req,res,["CUSTOMER"]);if(!u)return;
    const b=await readBody(req),pickup=Number(b.pickup),drop=Number(b.drop);
    if(!locations.some(l=>l.id===pickup)||!locations.some(l=>l.id===drop))return error(res,400,"Choose valid pickup and drop locations.");
    if(pickup===drop)return error(res,400,"Pickup and drop locations must be different.");
    const route=shortestPath(pickup,drop);if(!route)return error(res,400,"No route found.");
    const surge=state.bookings.filter(x=>x.status==="PENDING").length>=3?1.25:1;
    const booking={id:state.nextBookingId++,customerId:u.id,driverId:0,pickup,drop,distanceKm:route.distanceKm,pickupDistKm:0,fare:fare(route.distanceKm,surge),status:"PENDING",rating:0,timestamp:new Date().toISOString()};
    if(!assign(booking)) booking.status="PENDING";
    state.bookings.push(booking);save();
    return send(res,201,{booking:bookingView(booking),route});
  }
  const match=p.match(/^\/api\/bookings\/(\d+)\/(cancel|rate)$/);
  if(req.method==="POST"&&match) {
    const u=requireAuth(req,res,["CUSTOMER"]);if(!u)return;
    const id=Number(match[1]),action=match[2],booking=state.bookings.find(x=>x.id===id&&x.customerId===u.id);
    if(!booking)return error(res,404,"Booking not found.");
    if(action==="cancel") {
      if(!["PENDING","CONFIRMED"].includes(booking.status))return error(res,400,"Only pending or confirmed bookings can be cancelled.");
      if(booking.status==="CONFIRMED"){const d=getUser(booking.driverId);if(d){d.available=true;}}
      booking.status="CANCELLED";processPending();save();return send(res,200,{booking:bookingView(booking)});
    }
    const b=await readBody(req),stars=Number(b.stars);
    if(booking.status!=="COMPLETED")return error(res,400,"Only completed rides can be rated.");
    if(booking.rating>0)return error(res,400,"This ride has already been rated.");
    if(!Number.isFinite(stars)||stars<1||stars>5)return error(res,400,"Rating must be between 1 and 5.");
    booking.rating=stars;const d=getUser(booking.driverId);
    if(d){d.rating=((d.rating||4)*(d.ratingCount||0)+stars)/((d.ratingCount||0)+1);d.ratingCount=(d.ratingCount||0)+1;}
    save();return send(res,200,{booking:bookingView(booking)});
  }
  if(req.method==="POST"&&p==="/api/driver/availability") {
    const u=requireAuth(req,res,["DRIVER"]);if(!u)return;
    if(state.bookings.some(b=>b.driverId===u.id&&b.status==="CONFIRMED"))return error(res,400,"Complete your current ride before changing availability.");
    const b=await readBody(req);u.available=Boolean(b.available);if(u.available)processPending();save();
    return send(res,200,{user:publicUser(u)});
  }
  if(req.method==="POST"&&p==="/api/driver/complete") {
    const u=requireAuth(req,res,["DRIVER"]);if(!u)return;
    const b=state.bookings.find(x=>x.driverId===u.id&&x.status==="CONFIRMED");
    if(!b)return error(res,400,"You have no active ride.");
    b.status="COMPLETED";u.location=b.drop;u.available=true;processPending();save();
    return send(res,200,{booking:bookingView(b),user:publicUser(u)});
  }
  if(req.method==="POST"&&p==="/api/admin/process-pending") {
    const u=requireAuth(req,res,["ADMIN"]);if(!u)return;
    const assigned=processPending();save();return send(res,200,{assigned,pending:state.bookings.filter(b=>b.status==="PENDING").length});
  }
  return error(res,404,"API endpoint not found.");
}
function bookingView(b) {
  const customer=getUser(b.customerId),driver=getUser(b.driverId);
  return {...b,pickupName:locations.find(l=>l.id===b.pickup)?.name||"Unknown",dropName:locations.find(l=>l.id===b.drop)?.name||"Unknown",customerName:customer?.name||"Unknown",driverName:driver?.name||"Waiting for driver",vehicle:driver?.vehicle||""};
}
const server=http.createServer(async(req,res)=>{
  const url=new URL(req.url,`http://${req.headers.host||"localhost"}`);
  try {
    if(url.pathname.startsWith("/api/")) return await api(req,res,url);
    let file=url.pathname==="/"?"index.html":decodeURIComponent(url.pathname.slice(1));
    file=path.normalize(file);
    if(file.startsWith("..")||path.isAbsolute(file))return send(res,400,{error:"Invalid path"});
    const full=path.join(PUBLIC,file);
    if(!full.startsWith(PUBLIC+path.sep)&&full!==path.join(PUBLIC,"index.html"))return send(res,403,{error:"Forbidden"});
    fs.readFile(full,(err,data)=>{
      if(err)return send(res,404,{error:"File not found"});
      const ext=path.extname(full);
      const types={".html":"text/html; charset=utf-8",".css":"text/css; charset=utf-8",".js":"text/javascript; charset=utf-8",".svg":"image/svg+xml"};
      res.writeHead(200,{"Content-Type":types[ext]||"application/octet-stream","X-Content-Type-Options":"nosniff"});
      res.end(data);
    });
  } catch(e) { send(res,400,{error:e.message||"Request failed"}); }
});
seed();
server.listen(PORT,()=>console.log(`RideSync website running at http://localhost:${PORT}`));
