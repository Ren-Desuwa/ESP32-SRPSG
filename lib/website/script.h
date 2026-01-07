#ifndef SCRIPT_H
#define SCRIPT_H

const char webpageScript[] = R"rawliteral(

// --- GLOBAL ELEMENTS ---
const cursor = document.getElementById('system-cursor');
const windowWidth = window.innerWidth;
const windowHeight = window.innerHeight;

// --- STATE MANAGEMENT ---
let appState = "HUB"; // "HUB" or "GAME"
let selectedGameId = null;

// --- INPUT VARIABLES ---
let inputX = 0;
let inputY = 0;

// The "Virtual" coordinates
let cursorX = windowWidth / 2;
let cursorY = windowHeight / 2;

let lastButtonState = false;

// --- MOUSE INTEGRATION ---
document.addEventListener('mousemove', (e) => {
    if(appState === "HUB") {
        cursorX = e.clientX;
        cursorY = e.clientY;
    }
});

document.addEventListener('click', () => {
    if (appState === "HUB" && selectedGameId) {
        launchGame();
    }
});

// --- MAIN LOOP ---
// CONFIGURATION:
// Range: 90 means -45 (left) to +45 (right)
const TOTAL_ANGLE_X = 90; 

// Vertical: 55 means approx -27 to +27
// We use a smaller angle here so vertical movement feels as fast as horizontal
const TOTAL_ANGLE_Y = 55; 

function update() {
    // 1. Calculate Scale Factor (Pixels per Degree)
    let pixelsPerDegreeX = window.innerWidth / TOTAL_ANGLE_X;
    let pixelsPerDegreeY = window.innerHeight / TOTAL_ANGLE_Y;

    // 2. Absolute Mapping
    let targetX = (window.innerWidth / 2) + (inputX * pixelsPerDegreeX);
    let targetY = (window.innerHeight / 2) + (inputY * pixelsPerDegreeY);

    // 3. SMOOTHING 
    // 0.5 = Snappy and responsive
    let smoothing = 0.5;
    
    cursorX += (targetX - cursorX) * smoothing;
    cursorY += (targetY - cursorY) * smoothing;

    // 4. Clamp to Screen Edges
    if (cursorX < 0) cursorX = 0;
    if (cursorX > window.innerWidth) cursorX = window.innerWidth;
    if (cursorY < 0) cursorY = 0;
    if (cursorY > window.innerHeight) cursorY = window.innerHeight;

    // 5. Apply Visual Position
    cursor.style.left = cursorX + 'px';
    cursor.style.top = cursorY + 'px';

    // 6. Handle Interaction
    if (appState === "HUB") {
        handleHubInteractions();
    }

    requestAnimationFrame(update);
}
update(); 

// --- HUB INTERACTION LOGIC ---
function handleHubInteractions() {
    let elements = document.querySelectorAll('.interactable');
    let foundHover = false;
    let cRect = cursor.getBoundingClientRect();

    elements.forEach(el => {
        let eRect = el.getBoundingClientRect();
        if (
            cRect.right > eRect.left &&
            cRect.left < eRect.right &&
            cRect.bottom > eRect.top &&
            cRect.top < eRect.bottom
        ) {
            if (!el.classList.contains('hovered')) {
                // Play sound effect if needed
            }
            el.classList.add('hovered');
            selectedGameId = el.getAttribute('data-game');
            foundHover = true;
        } else {
            el.classList.remove('hovered');
        }
    });

    if (!foundHover) selectedGameId = null;
}

// --- APP CONTROL ---
function launchGame() {
    if (!selectedGameId || selectedGameId === 'none') return;

    if (selectedGameId === 'ball-game') {
        appState = "GAME";
        document.getElementById('hub-view').classList.remove('active');
        document.getElementById('game-view').classList.add('active');
        cursor.classList.add('in-game');
        
        moveTargetRandomly();
        score = 0;
        document.getElementById('score').innerText = "0";
    }
}

function exitGame() {
    appState = "HUB";
    document.getElementById('game-view').classList.remove('active');
    document.getElementById('hub-view').classList.add('active');
    cursor.classList.remove('in-game');
}

// --- GAME LOGIC ---
const target = document.querySelector('.target');
let score = 0;

function moveTargetRandomly() {
    // 1. Define Margins (10% on each side)
    const marginX = window.innerWidth * 0.10; 
    const marginY = window.innerHeight * 0.10;
    
    // 2. Define Playable Area (80% Width/Height)
    // We assume target size is ~80px (from CSS), so we subtract it 
    // to ensure the target stays fully INSIDE the 80% box.
    const safeWidth = (window.innerWidth * 0.80) - 80;
    const safeHeight = (window.innerHeight * 0.80) - 80;

    // 3. Randomize within the safe area + margin offset
    // Math.max(0, ...) prevents issues on extremely small screens
    const randomX = marginX + Math.random() * Math.max(0, safeWidth);
    const randomY = marginY + Math.random() * Math.max(0, safeHeight);

    target.style.left = randomX + 'px';
    target.style.top = randomY + 'px';
}

function checkGameCollision() {
    let cRect = cursor.getBoundingClientRect();
    let tRect = target.getBoundingClientRect();
    
    let cX = cRect.left + cRect.width/2;
    let cY = cRect.top + cRect.height/2;
    let tX = tRect.left + tRect.width/2;
    let tY = tRect.top + tRect.height/2;
    
    let dist = Math.hypot(cX - tX, cY - tY);
    return dist < 40 + (cRect.width/2); 
}

// --- UTILITIES ---
function updateClock() {
    const now = new Date();
    const timeString = now.toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'});
    document.getElementById('clock').innerText = timeString;
}
setInterval(updateClock, 1000);
updateClock();

// --- WEBSOCKET ---
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

function initWebSocket() {
    console.log('Trying WebSocket: ' + gateway);
    websocket = new WebSocket(gateway);
    websocket.onopen = () => console.log('Connected');
    websocket.onclose = () => setTimeout(initWebSocket, 2000);
    
    websocket.onmessage = (event) => {
        var data = JSON.parse(event.data);
        
        if (data.gyroX !== undefined) {
             inputX = data.gyroX; 
             inputY = data.gyroZ; 
        }

        if (data.btn !== undefined) {
            let currentBtn = data.btn;
            if (currentBtn === true && lastButtonState === false) {
                if (appState === "HUB") {
                    if (selectedGameId) launchGame();
                } 
                else if (appState === "GAME") {
                    if (checkGameCollision()) {
                        score++;
                        document.getElementById('score').innerText = score;
                        moveTargetRandomly();
                        cursor.style.transform = "translate(-50%, -50%) scale(1.5)";
                        setTimeout(() => cursor.style.transform = "translate(-50%, -50%) scale(1)", 100);
                    }
                }
            }
            lastButtonState = currentBtn;
        }
    };
}

window.addEventListener('load', initWebSocket);

)rawliteral";

#endif // SCRIPT_H