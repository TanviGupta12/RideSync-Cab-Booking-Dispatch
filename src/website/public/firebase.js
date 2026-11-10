import { initializeApp } from "https://www.gstatic.com/firebasejs/12.19.0/firebase-app.js";

const firebaseConfig = {
apiKey: "AIzaSyCrMLvp6WCcXfgJPoNEQaOe6qAVZKTVp1o",
authDomain: "ridesync--cab-booking-system.firebaseapp.com",
projectId: "ridesync--cab-booking-system",
storageBucket: "ridesync--cab-booking-system.firebasestorage.app",
messagingSenderId: "170109929554",
appId: "1:170109929554:web:f3aaa4ee90361b0944378b",
measurementId: "G-52BERL0VVJ"
};

const app = initializeApp(firebaseConfig);

console.log("Firebase initialized successfully!");

export { app };
