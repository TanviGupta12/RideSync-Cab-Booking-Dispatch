const fs = require("fs");
const path = require("path");

const admin = require("firebase-admin");
const { getFirestore } = require("firebase-admin/firestore");

admin.initializeApp({
    credential: admin.applicationDefault(),
    projectId: "ridesync--cab-booking-system"
});

const db = getFirestore();

async function migrateUsers() {
    try {
        const filePath = path.join(__dirname, "data", "ridesync.json");

        // Read existing JSON data
        const state = JSON.parse(fs.readFileSync(filePath, "utf8"));
        const users = state.users || [];

        if (users.length === 0) {
            console.log("No existing users found.");
            return;
        }

        console.log(`Found ${users.length} users to migrate.`);

        // Firestore batches support up to 500 writes
        for (let i = 0; i < users.length; i += 500) {
            const batch = db.batch();
            const chunk = users.slice(i, i + 500);

            for (const user of chunk) {
                const userRef = db.collection("users")
                    .doc(String(user.id));

                batch.set(userRef, user);
            }

            await batch.commit();
        }

        console.log("User migration completed successfully!");

    } catch (error) {
        console.error("Migration failed:", error.message);
    }
}

migrateUsers();