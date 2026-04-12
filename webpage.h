#pragma once
#include <pgmspace.h>

const char WEBPAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes">
<title>ESP32 GameBoy</title>
<style>

  * {
    box-sizing: border-box;
    -webkit-tap-highlight-color: transparent;
    user-select: none;
  }

  html, body {
    margin: 0;
    padding: 0;
    width: 100%;
    min-height: 100%;
    background: #1a1a2e;
    font-family: sans-serif;
  }

  /* Game Selection Screen */
  .game-selector {
    padding: 20px;
    max-width: 800px;
    margin: 0 auto;
  }

  h1 {
    text-align: center;
    color: #0ff;
    margin-bottom: 30px;
  }

  .game-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
    gap: 20px;
    margin-bottom: 30px;
  }

  .game-card {
    background: linear-gradient(135deg, #2a2a4a, #1a1a2e);
    border: 2px solid #0ff;
    border-radius: 15px;
    padding: 30px 20px;
    text-align: center;
    cursor: pointer;
    transition: transform 0.2s, box-shadow 0.2s;
  }

  .game-card:hover {
    transform: scale(1.05);
    box-shadow: 0 0 20px rgba(0,255,255,0.3);
  }

  .game-card h2 {
    color: #0ff;
    margin: 0 0 10px 0;
    font-size: 1.5em;
  }

  .game-card p {
    color: #aaa;
    margin: 0;
    font-size: 0.9em;
  }

  .game-card.selected {
    background: linear-gradient(135deg, #0ff, #0aa);
    border-color: #fff;
  }

  .game-card.selected h2,
  .game-card.selected p {
    color: #000;
  }

  /* Controller Screen (hidden initially) */
  .controller {
    display: none;
    width: 100vw;
    height: 100vh;
    background: linear-gradient(160deg, #2a2a4a 0%, #1a1a2e 100%);
    position: fixed;
    top: 0;
    left: 0;
  }

  .shell {
    width: 100%;
    height: 100%;
    display: flex;
    flex-direction: row;
    align-items: center;
    justify-content: space-between;
    padding: 0 20px;
    position: relative;
  }

  .screen-area {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 10px;
  }

  .screen-bezel {
    background: #111;
    border-radius: 8px;
    padding: 8px;
    border: 2px solid #333;
    box-shadow: 0 0 20px rgba(0,255,255,0.15);
  }

  .screen-label {
    font-family: sans-serif;
    font-size: 11px;
    color: #444;
    letter-spacing: 3px;
    text-transform: uppercase;
  }

  .sys-row {
    display: flex;
    gap: 16px;
    margin-top: 6px;
  }

  .btn-sys {
    width: 64px;
    height: 22px;
    border-radius: 11px;
    background: #2d2d50;
    border: 1px solid #444;
    color: #aaa;
    font-size: 10px;
    font-family: sans-serif;
    font-weight: bold;
    letter-spacing: 1px;
    cursor: pointer;
    box-shadow: 0 3px 0 #111;
    transition: background 0.08s, transform 0.08s, box-shadow 0.08s;
  }

  .btn-sys.held, .btn-sys:active {
    background: #0ff;
    color: #000;
    transform: translateY(2px);
    box-shadow: 0 1px 0 #111;
  }

  .left-side {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 0;
  }

  .dpad {
    display: grid;
    grid-template-columns: 52px 52px 52px;
    grid-template-rows: 52px 52px 52px;
    gap: 3px;
  }

  .btn-dpad {
    width: 52px;
    height: 52px;
    background: #2a2a4a;
    border: none;
    border-radius: 6px;
    color: #ccc;
    font-size: 20px;
    cursor: pointer;
    box-shadow: 0 4px 0 #0a0a1a;
    transition: background 0.08s, transform 0.08s, box-shadow 0.08s;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .btn-dpad.held, .btn-dpad:active {
    background: #0ff;
    color: #000;
    transform: translateY(3px);
    box-shadow: 0 1px 0 #0a0a1a;
  }

  .dpad-center {
    background: #1e1e38 !important;
    pointer-events: none;
    box-shadow: none !important;
  }

  .right-side {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
  }

  .ab-grid {
    display: grid;
    grid-template-columns: 64px 64px;
    grid-template-rows: 64px 64px;
    gap: 6px;
  }

  .btn-ab {
    width: 64px;
    height: 64px;
    border-radius: 50%;
    border: none;
    font-size: 18px;
    font-weight: bold;
    font-family: sans-serif;
    cursor: pointer;
    box-shadow: 0 5px 0 #000;
    transition: background 0.08s, transform 0.08s, box-shadow 0.08s;
    color: #fff;
  }

  .btn-ab.held, .btn-ab:active {
    transform: translateY(4px);
    box-shadow: 0 1px 0 #000;
    filter: brightness(1.3);
  }

  .btn-a {
    background: radial-gradient(circle at 35% 35%, #e83030, #900);
    grid-column: 2;
    grid-row: 1;
  }

  .btn-b {
    background: radial-gradient(circle at 35% 35%, #e8a030, #960);
    grid-column: 1;
    grid-row: 2;
  }

  .ab-labels {
    display: grid;
    grid-template-columns: 64px 64px;
    gap: 6px;
    margin-top: 4px;
  }

  .ab-label {
    text-align: center;
    font-size: 11px;
    color: #555;
    font-family: sans-serif;
  }

  .btn-menu {
    background: #900;
    color: #fff;
    width: 80px;
    height: 30px;
    border-radius: 15px;
    border: none;
    font-size: 12px;
    font-weight: bold;
    cursor: pointer;
    box-shadow: 0 3px 0 #300;
    transition: background 0.08s, transform 0.08s, box-shadow 0.08s;
  }

  .btn-menu:active {
    transform: translateY(2px);
    box-shadow: 0 1px 0 #300;
  }

  #status {
    position: fixed;
    bottom: 8px;
    left: 50%;
    transform: translateX(-50%);
    background: rgba(0,0,0,0.5);
    color: #555;
    font-size: 11px;
    font-family: sans-serif;
    padding: 3px 12px;
    border-radius: 10px;
    pointer-events: none;
  }

  @media(max-width: 600px) {
    .game-grid {
      grid-template-columns: 1fr;
    }
    .dpad {
      grid-template-columns: 40px 40px 40px;
      grid-template-rows: 40px 40px 40px;
      gap: 2px;
    }
    .btn-dpad {
      width: 40px;
      height: 40px;
      font-size: 16px;
    }
    .btn-ab {
      width: 50px;
      height: 50px;
      font-size: 14px;
    }
    .ab-grid {
      grid-template-columns: 50px 50px;
      grid-template-rows: 50px 50px;
      gap: 4px;
    }
    .btn-sys, .btn-menu {
      width: 55px;
      height: 25px;
      font-size: 9px;
    }
  }
</style>
</head>
<body>

<!-- Game Selection Screen -->
<div id="gameSelector" class="game-selector">
  <h1>🎮 ESP32 GameBoy</h1>
  <div class="game-grid" id="gameGrid">
    <!-- Games will be loaded here -->
    <div style="text-align:center; padding:40px;">Loading games...</div>
  </div>
</div>

<!-- Controller Screen -->
<div id="controller" class="controller">
  <div class="shell">
    <div class="left-side">
      <div class="dpad">
        <div></div>
        <button id="up" class="btn-dpad"
          ontouchstart="press('/up',this)" ontouchend="release(this)"
          onmousedown="press('/up',this)" onmouseup="release(this)">▲</button>
        <div></div>
        <button id="left" class="btn-dpad"
          ontouchstart="press('/left',this)" ontouchend="release(this)"
          onmousedown="press('/left',this)" onmouseup="release(this)">◀</button>
        <div class="btn-dpad dpad-center"></div>
        <button id="right" class="btn-dpad"
          ontouchstart="press('/right',this)" ontouchend="release(this)"
          onmousedown="press('/right',this)" onmouseup="release(this)">▶</button>
        <div></div>
        <button id="down" class="btn-dpad"
          ontouchstart="press('/down',this)" ontouchend="release(this)"
          onmousedown="press('/down',this)" onmouseup="release(this)">▼</button>
        <div></div>
      </div>
    </div>

    <div class="screen-area">
      <div class="screen-bezel">
        <div class="screen-label">■&nbsp;&nbsp;Nintendo&nbsp;&nbsp;■</div>
      </div>
      <div class="sys-row">
        <button id="select" class="btn-sys"
          ontouchstart="press('/select',this)" ontouchend="release(this)"
          onmousedown="press('/select',this)" onmouseup="release(this)">SELECT</button>
        <button id="start" class="btn-sys"
          ontouchstart="press('/start',this)" ontouchend="release(this)"
          onmousedown="press('/start',this)" onmouseup="release(this)">START</button>
        <button id="menu" class="btn-menu" onclick="exitToMenu()">MENU</button>
      </div>
    </div>

    <div class="right-side">
      <div class="ab-grid">
        <button id="a" class="btn-ab btn-a"
          ontouchstart="press('/a',this)" ontouchend="release(this)"
          onmousedown="press('/a',this)" onmouseup="release(this)">A</button>
        <button id="b" class="btn-ab btn-b"
          ontouchstart="press('/b',this)" ontouchend="release(this)"
          onmousedown="press('/b',this)" onmouseup="release(this)">B</button>
      </div>
      <div class="ab-labels">
        <div class="ab-label">B</div>
        <div class="ab-label">A</div>
      </div>
    </div>
  </div>
  <div id="status">Ready</div>
</div>

<script>
  let held = null;

  function press(cmd, el) {
    if (held && held !== el) release(held);
    held = el;
    el.classList.add('held');
    fetch(cmd).catch(() => {});
    document.getElementById('status').textContent = cmd.replace('/', '').toUpperCase();
  }

  function release(el) {
    if (!el) return;
    el.classList.remove('held');
    held = null;
    fetch('/release').catch(() => {});
    document.getElementById('status').textContent = 'Ready';
  }

  function exitToMenu() {
    fetch('/exit').then(() => {
      location.reload();
    }).catch(() => {
      location.reload();
    });
  }

  function selectGame(gameId, gameName) {
    // Show loading indicator
    const gameGrid = document.getElementById('gameGrid');
    gameGrid.innerHTML = '<div style="text-align:center; padding:40px;">Loading ' + gameName + '...</div>';
    
    // Send selection to ESP32
    fetch('/selectgame?game=' + gameId)
      .then(() => {
        // Hide game selector, show controller
        document.getElementById('gameSelector').style.display = 'none';
        document.getElementById('controller').style.display = 'block';
      })
      .catch(err => {
        console.error('Error:', err);
        alert('Failed to load game. Please try again.');
        location.reload();
      });
  }

  // Load available games from ESP32
  function loadGames() {
    fetch('/games')
      .then(response => response.json())
      .then(games => {
        const gameGrid = document.getElementById('gameGrid');
        gameGrid.innerHTML = '';
        
        games.forEach((game, index) => {
          const card = document.createElement('div');
          card.className = 'game-card';
          card.onclick = () => selectGame(index, game.name);
          card.innerHTML = `
            <h2>${game.name}</h2>
            <p>Click to play</p>
          `;
          gameGrid.appendChild(card);
        });
      })
      .catch(err => {
        console.error('Error loading games:', err);
        document.getElementById('gameGrid').innerHTML = 
          '<div style="text-align:center; padding:40px;">Error loading games. Make sure you are connected to the ESP32.</div>';
      });
  }

  // Keyboard support for controller
  document.addEventListener('contextmenu', e => e.preventDefault());

  const keyMap = {
    ArrowUp:    '/up',
    ArrowDown:  '/down',
    ArrowLeft:  '/left',
    ArrowRight: '/right',
    z:          '/a',
    Z:          '/a',
    x:          '/b',
    X:          '/b',
    Enter:      '/start',
    Shift:      '/select',
    Escape:     '/exit',
  };

  const heldKeys = new Set();

  document.addEventListener('keydown', e => {
    if (heldKeys.has(e.key) || !keyMap[e.key]) return;
    e.preventDefault();
    heldKeys.add(e.key);
    const cmd = keyMap[e.key];
    const btnId = cmd.replace('/', '');
    const el = document.getElementById(btnId);
    if (el && cmd !== '/exit') {
      el.classList.add('held');
    }
    fetch(cmd).catch(() => {});
    if (cmd === '/exit') {
      location.reload();
    }
    document.getElementById('status').textContent = cmd.replace('/', '').toUpperCase();
  });

  document.addEventListener('keyup', e => {
    if (!keyMap[e.key]) return;
    heldKeys.delete(e.key);
    const cmd = keyMap[e.key];
    const btnId = cmd.replace('/', '');
    const el = document.getElementById(btnId);
    if (el) el.classList.remove('held');
    if (heldKeys.size === 0 && cmd !== '/exit') {
      fetch('/release').catch(() => {});
      document.getElementById('status').textContent = 'Ready';
    }
  });

  // Load games when page loads
  loadGames();
</script>
</body>
</html>
)rawliteral";
