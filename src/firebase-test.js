const admin = require("firebase-admin");
const { getFirestore } = require("firebase-admin/firestore");

admin.initializeApp({
    credential: admin.applicationDefault()
});

const db = getFirestore();

async function testFirebase() {
    try {
        await db.collection("users").limit(1).get();

        console.log("Firebase Admin SDK connected successfully!");
    } catch (error) {
        console.error("Firebase connection failed:", error);
    }
}

testFirebase();