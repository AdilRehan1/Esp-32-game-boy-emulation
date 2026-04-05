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

  /* Controller Screen */
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
    grid-template-columns: 60px 60px 60px;
    grid-template-rows: 60px 60px 60px;
    gap: 4px;
  }

  .btn-dpad {
    width: 60px;
    height: 60px;
    background: #2a2a4a;
    border: none;
    border-radius: 10px;
    color: #ccc;
    font-size: 28px;
    font-weight: bold;
    cursor: pointer;
    box-shadow: 0 4px 0 #0a0a1a;
    transition: background 0.08s, transform 0.08s, box-shadow 0.08s;
    display: flex;
    align-items: center;
    justify-content: center;
    touch-action: manipulation;
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
    grid-template-columns: 70px 70px;
    grid-template-rows: 70px 70px;
    gap: 8px;
  }

  .btn-ab {
    width: 70px;
    height: 70px;
    border-radius: 50%;
    border: none;
    font-size: 24px;
    font-weight: bold;
    font-family: sans-serif;
    cursor: pointer;
    box-shadow: 0 5px 0 #000;
    transition: background 0.08s, transform 0.08s, box-shadow 0.08s;
    color: #fff;
    touch-action: manipulation;
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
    grid-template-columns: 70px 70px;
    gap: 8px;
    margin-top: 6px;
  }

  .ab-label {
    text-align: center;
    font-size: 12px;
    color: #555;
    font-family: sans-serif;
    font-weight: bold;
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
      grid-template-columns: 45px 45px 45px;
      grid-template-rows: 45px 45px 45px;
      gap: 3px;
    }
    .btn-dpad {
      width: 45px;
      height: 45px;
      font-size: 22px;
    }
    .btn-ab {
      width: 55px;
      height: 55px;
      font-size: 18px;
    }
    .ab-grid {
      grid-template-columns: 55px 55px;
      grid-template-rows: 55px 55px;
      gap: 6px;
    }
    .ab-labels {
      grid-template-columns: 55px 55px;
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
          ontouchstart="pressStart('/up', this)" ontouchend="pressEnd('/up', this)"
          onmousedown="pressStart('/up', this)" onmouseup="pressEnd('/up', this)"
          onmouseleave="pressEnd('/up', this)">▲</button>
        <div></div>
        <button id="left" class="btn-dpad"
          ontouchstart="pressStart('/left', this)" ontouchend="pressEnd('/left', this)"
          onmousedown="pressStart('/left', this)" onmouseup="pressEnd('/left', this)"
          onmouseleave="pressEnd('/left', this)">◀</button>
        <div class="btn-dpad dpad-center"></div>
        <button id="right" class="btn-dpad"
          ontouchstart="pressStart('/right', this)" ontouchend="pressEnd('/right', this)"
          onmousedown="pressStart('/right', this)" onmouseup="pressEnd('/right', this)"
          onmouseleave="pressEnd('/right', this)">▶</button>
        <div></div>
        <button id="down" class="btn-dpad"
          ontouchstart="pressStart('/down', this)" ontouchend="pressEnd('/down', this)"
          onmousedown="pressStart('/down', this)" onmouseup="pressEnd('/down', this)"
          onmouseleave="pressEnd('/down', this)">▼</button>
        <div></div>
      </div>
    </div>

    <div class="screen-area">
      <div class="screen-bezel">
        <div class="screen-label">GAME BOY</div>
      </div>
      <div class="sys-row">
        <button id="select" class="btn-sys"
          ontouchstart="pressStart('/select', this)" ontouchend="pressEnd('/select', this)"
          onmousedown="pressStart('/select', this)" onmouseup="pressEnd('/select', this)"
          onmouseleave="pressEnd('/select', this)">SELECT</button>
        <button id="start" class="btn-sys"
          ontouchstart="pressStart('/start', this)" ontouchend="pressEnd('/start', this)"
          onmousedown="pressStart('/start', this)" onmouseup="pressEnd('/start', this)"
          onmouseleave="pressEnd('/start', this)">START</button>
        <button id="menu" class="btn-menu" onclick="exitToMenu()">MENU</button>
      </div>
    </div>

    <div class="right-side">
      <div class="ab-grid">
        <button id="a" class="btn-ab btn-a"
          ontouchstart="pressStart('/a', this)" ontouchend="pressEnd('/a', this)"
          onmousedown="pressStart('/a', this)" onmouseup="pressEnd('/a', this)"
          onmouseleave="pressEnd('/a', this)">A</button>
        <button id="b" class="btn-ab btn-b"
          ontouchstart="pressStart('/b', this)" ontouchend="pressEnd('/b', this)"
          onmousedown="pressStart('/b', this)" onmouseup="pressEnd('/b', this)"
          onmouseleave="pressEnd('/b', this)">B</button>
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
  // Track which buttons are currently pressed
  let activeButtons = new Set();
  let statusTimeout = null;

  function updateStatus() {
    if (activeButtons.size > 0) {
      let statusText = Array.from(activeButtons).join(' + ').toUpperCase();
      document.getElementById('status').textContent = statusText;
      if (statusTimeout) clearTimeout(statusTimeout);
      statusTimeout = setTimeout(() => {
        if (activeButtons.size === 0) {
          document.getElementById('status').textContent = 'Ready';
        }
      }, 500);
    }
  }

  function pressStart(cmd, el) {
    // Prevent default to avoid double events
    event.preventDefault();
    
    // Add button to active set if not already there
    if (!activeButtons.has(cmd)) {
      activeButtons.add(cmd);
      el.classList.add('held');
      fetch(cmd).catch(() => {});
      updateStatus();
    }
  }

  function pressEnd(cmd, el) {
    // Prevent default
    event.preventDefault();
    
    // Remove button from active set
    if (activeButtons.has(cmd)) {
      activeButtons.delete(cmd);
      el.classList.remove('held');
      
      // Only send release when NO buttons are pressed
      if (activeButtons.size === 0) {
        fetch('/release').catch(() => {});
        updateStatus();
      }
    }
  }

  function exitToMenu() {
    // Clear all active buttons first
    if (activeButtons.size > 0) {
      fetch('/release').catch(() => {});
      activeButtons.clear();
    }
    fetch('/exit').then(() => {
      location.reload();
    }).catch(() => {
      location.reload();
    });
  }

  function selectGame(gameId, gameName) {
    const gameGrid = document.getElementById('gameGrid');
    gameGrid.innerHTML = '<div style="text-align:center; padding:40px;">Loading ' + gameName + '...</div>';
    
    fetch('/selectgame?game=' + gameId)
      .then(() => {
        document.getElementById('gameSelector').style.display = 'none';
        document.getElementById('controller').style.display = 'block';
      })
      .catch(err => {
        console.error('Error:', err);
        alert('Failed to load game. Please try again.');
        location.reload();
      });
  }

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

  // Keyboard support for multiple simultaneous keys
  document.addEventListener('contextmenu', e => e.preventDefault());

  const keyMap = {
    'ArrowUp': '/up',
    'ArrowDown': '/down',
    'ArrowLeft': '/left',
    'ArrowRight': '/right',
    'z': '/a',
    'Z': '/a',
    'x': '/b',
    'X': '/b',
    'Enter': '/start',
    'Shift': '/select',
  };

  const activeKeys = new Set();

  document.addEventListener('keydown', e => {
    const cmd = keyMap[e.key];
    if (!cmd) return;
    
    e.preventDefault();
    
    if (!activeKeys.has(cmd)) {
      activeKeys.add(cmd);
      const btnId = cmd.replace('/', '');
      const el = document.getElementById(btnId);
      if (el) el.classList.add('held');
      fetch(cmd).catch(() => {});
      
      let statusText = Array.from(activeKeys).map(c => c.replace('/', '').toUpperCase()).join(' + ');
      document.getElementById('status').textContent = statusText;
    }
  });

  document.addEventListener('keyup', e => {
    const cmd = keyMap[e.key];
    if (!cmd) return;
    
    e.preventDefault();
    
    if (activeKeys.has(cmd)) {
      activeKeys.delete(cmd);
      const btnId = cmd.replace('/', '');
      const el = document.getElementById(btnId);
      if (el) el.classList.remove('held');
      
      if (activeKeys.size === 0) {
        fetch('/release').catch(() => {});
        document.getElementById('status').textContent = 'Ready';
      } else {
        let statusText = Array.from(activeKeys).map(c => c.replace('/', '').toUpperCase()).join(' + ');
        document.getElementById('status').textContent = statusText;
      }
    }
  });

  // Handle window/tab blur - release all keys
  window.addEventListener('blur', () => {
    if (activeKeys.size > 0 || activeButtons.size > 0) {
      fetch('/release').catch(() => {});
      activeKeys.clear();
      activeButtons.clear();
      document.querySelectorAll('.held').forEach(el => el.classList.remove('held'));
      document.getElementById('status').textContent = 'Ready';
    }
  });

  loadGames();
</script>
</body>
</html>
)rawliteral";
