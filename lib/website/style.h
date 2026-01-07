#ifndef STYLE_H
#define STYLE_H

const char webpageStyle[] = R"rawliteral(

/* --- GLOBAL & RESET --- */
:root {
    --ps-blue-light: #0070d1;
    --ps-blue-dark: #003791;
    --text-white: #ffffff;
    --active-glow: 0 0 40px rgba(255, 255, 255, 0.4);
}

body {
    margin: 0;
    padding: 0;
    font-family: "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    background: linear-gradient(135deg, var(--ps-blue-light), var(--ps-blue-dark));
    color: var(--text-white);
    height: 100vh;
    overflow: hidden;
    cursor: none; /* HIDE THE REAL SYSTEM MOUSE so only ours shows */
}

/* --- VIRTUAL CURSOR --- */
.cursor {
    width: 20px;
    height: 20px;
    background-color: rgba(255, 255, 255, 0.9);
    border-radius: 50%;
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    box-shadow: 0 0 10px rgba(255, 255, 255, 0.5);
    pointer-events: none; /* Important: Lets clicks pass through to elements */
    z-index: 9999;
    transition: transform 0.1s, background-color 0.2s;
}

/* When in Game Mode, look like the player ball */
.cursor.in-game {
    background-color: #ff0055;
    border: 2px solid white;
    width: 25px;
    height: 25px;
}

/* --- VIEW MANAGEMENT --- */
.view {
    display: none;
    width: 100%;
    height: 100%;
}
.view.active { display: block; }

/* --- HUB UI --- */
.top-bar {
    display: flex;
    justify-content: space-between;
    padding: 3rem 4rem;
    font-size: 1.2rem;
    font-weight: 300;
}

.icon-box {
    display: flex;
    align-items: center;
    gap: 15px;
}

.avatar {
    width: 40px;
    height: 40px;
    background-color: #eee;
    border-radius: 50%;
    overflow: hidden;
    border: 2px solid rgba(255,255,255,0.2);
}
.avatar img { width: 100%; height: 100%; object-fit: cover; }

/* --- GAME RIBBON --- */
.game-ribbon {
    display: flex;
    align-items: center;
    gap: 20px;
    padding-left: 4rem;
    margin-top: 2rem;
    height: 400px;
}

.tile {
    width: 220px;
    height: 220px;
    background-color: #1a1a1a;
    position: relative;
    transition: all 0.25s cubic-bezier(0.25, 0.46, 0.45, 0.94);
    opacity: 0.6;
    border-radius: 4px;
}

.tile-img {
    width: 100%;
    height: 100%;
    border-radius: 4px;
    background-size: cover;
}

/* HOVER STATE (Triggered by Mouse OR Gyro) */
.tile.hovered {
    width: 340px;
    height: 340px;
    opacity: 1;
    box-shadow: var(--active-glow);
    border: 4px solid white;
    transform: translateY(-10px);
    z-index: 100;
}

.tile-info {
    position: absolute;
    bottom: -90px;
    left: 0;
    width: 100%;
    opacity: 0;
    transition: opacity 0.2s;
}

.tile.hovered .tile-info { opacity: 1; }

.game-title {
    font-size: 2rem;
    font-weight: 300;
    margin-bottom: 10px;
    text-transform: uppercase;
    letter-spacing: 2px;
}

.start-prompt {
    font-size: 1rem;
    display: flex;
    align-items: center;
    gap: 10px;
    opacity: 0.8;
}

.x-btn {
    width: 24px;
    height: 24px;
    background: #222;
    border-radius: 50%;
    display: flex;
    justify-content: center;
    align-items: center;
    font-weight: bold;
    font-size: 0.8rem;
    color: #00c3ff;
    border: 2px solid #555;
}

/* --- GAME UI --- */
.window {
    width: 100vw;
    height: 100vh;
    background: #f0f0f0;
    position: relative;
}

.target {
    width: 80px;
    height: 80px;
    border: 5px solid #00ff88;
    border-radius: 50%;
    position: absolute;
    box-shadow: 0 0 20px rgba(0, 255, 136, 0.4);
}

.game-ui {
    position: absolute;
    top: 30px;
    left: 40px;
    color: #333;
    font-size: 2rem;
    font-weight: bold;
    z-index: 10;
}

.exit-hint {
    position: absolute;
    bottom: 30px;
    left: 40px;
    color: #888;
    z-index: 10;
}

)rawliteral";

#endif // STYLE_H