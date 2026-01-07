#ifndef INDEX_H
#define INDEX_H

const char webpageIndex[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="style.css">
    <title>ESP32 OS</title>
</head>
<body>
    <div id="system-cursor" class="cursor"></div>

    <section id="hub-view" class="view active">
        
        <div class="top-bar">
            <div class="top-left">
                <div class="icon-box">
                    <div class="avatar"><img src="https://ui-avatars.com/api/?name=User&background=0070d1&color=fff" alt=""></div>
                    <span class="user-name">Player 1</span>
                </div>
            </div>
            <div class="top-right">
                <div class="icon-box"><span id="clock">12:00 PM</span></div>
            </div>
        </div>

        <div class="game-ribbon">
            
            <div class="tile interactable" data-game="ball-game">
                <div class="tile-img" style="background: linear-gradient(45deg, #ff0055, #5500ff);"></div>
                
                <div class="tile-info">
                    <div class="game-title">Gyro Ball</div>
                    <div class="start-prompt">
                        <div class="x-btn">X</div>
                        <span>Start</span>
                    </div>
                </div>
            </div>

            <div class="tile interactable" data-game="none">
                <div class="tile-img" style="background: #333;"></div>
                <div class="tile-info">
                    <div class="game-title">Locked</div>
                </div>
            </div>

            <div class="tile interactable" data-game="none">
                <div class="tile-img" style="background: #222;"></div>
            </div>

        </div>
    </section>

    <section id="game-view" class="view">
        <div class="window">
            <div class="game-ui">SCORE: <span id="score">0</span></div>
            <div class="exit-hint">Press Back (or Hold Button) to Quit</div>
            
            <div class="target"></div>
            </div>
    </section>

    <script src="script.js"></script>
</body>
</html>
)rawliteral";

#endif // INDEX_H