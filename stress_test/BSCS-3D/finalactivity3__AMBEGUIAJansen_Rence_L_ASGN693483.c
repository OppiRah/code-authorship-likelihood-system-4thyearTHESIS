// JavaScript (Node.js) version
const fs = require("fs");

try {
    const contents = fs.readFileSync("student_data.txt", "utf8");
    console.log("--- File Contents ---");
    process.stdout.write(contents);
} catch (error) {
    console.log("Error opening file.");
}
