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
    touch-action: none;
  }

  html, body {
    margin: 0;
    padding: 0;
    width: 100%;
    height: 100%;
    overflow: hidden;
    background: #1a1a2e;
  }

  /* ---- SHELL ---- */
  .shell {
    width: 100vw;
    height: 100vh;
    background: linear-gradient(160deg, #2a2a4a 0%, #1a1a2e 100%);
    display: flex;
    flex-direction: row;
    align-items: center;
    justify-content: space-between;
    padding: 0 20px;
    position: relative;
  }

  /* ---- SCREEN WINDOW (centre) ---- */
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

  /* START / SELECT row */
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

  .btn-sys.held,
  .btn-sys:active {
    background: #0ff;
    color: #000;
    transform: translateY(2px);
    box-shadow: 0 1px 0 #111;
  }

  /* ---- LEFT SIDE — D-PAD ---- */
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
    grid-template-rows:    52px 52px 52px;
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

  .btn-dpad.held,
  .btn-dpad:active {
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

  /* ---- RIGHT SIDE — A / B ---- */
  .right-side {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
  }

  .ab-grid {
    display: grid;
    grid-template-columns: 64px 64px;
    grid-template-rows:    64px 64px;
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

  .btn-ab.held,
  .btn-ab:active {
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

  /* status pill */
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
<div class="shell">

  <!-- ========== LEFT: D-PAD ========== -->
  <div class="left-side">
    <div class="dpad">
      <div></div>
      <button id="up" class="btn-dpad"
        ontouchstart="press('/up',this,event)"   ontouchend="release(this,event)"
        onmousedown="press('/up',this,event)"    onmouseup="release(this,event)">&#9650;</button>
      <div></div>

      <button id="left" class="btn-dpad"
        ontouchstart="press('/left',this,event)"  ontouchend="release(this,event)"
        onmousedown="press('/left',this,event)"   onmouseup="release(this,event)">&#9664;</button>
      <div class="btn-dpad dpad-center"></div>
      <button id="right" class="btn-dpad"
        ontouchstart="press('/right',this,event)" ontouchend="release(this,event)"
        onmousedown="press('/right',this,event)"  onmouseup="release(this,event)">&#9654;</button>

      <div></div>
      <button id="down" class="btn-dpad"
        ontouchstart="press('/down',this,event)"  ontouchend="release(this,event)"
        onmousedown="press('/down',this,event)"   onmouseup="release(this,event)">&#9660;</button>
      <div></div>
    </div>
  </div>

  <!-- ========== CENTRE: SCREEN + START/SELECT/MENU ========== -->
  <div class="screen-area">
    <div class="screen-bezel">
      <div class="screen-label">&#9632;&nbsp;&nbsp;Nintendo&nbsp;&nbsp;&#9632;</div>
    </div>
    <div class="sys-row">
      <button id="select" class="btn-sys"
        ontouchstart="press('/select',this,event)" ontouchend="release(this,event)"
        onmousedown="press('/select',this,event)"  onmouseup="release(this,event)">SELECT</button>
      <button id="start" class="btn-sys"
        ontouchstart="press('/start',this,event)"  ontouchend="release(this,event)"
        onmousedown="press('/start',this,event)"   onmouseup="release(this,event)">START</button>
      <button id="menu" class="btn-menu"
        onclick="exitToMenu()">MENU</button>
    </div>
  </div>

  <!-- ========== RIGHT: A / B ========== -->
  <div class="right-side">
    <div class="ab-grid">
      <button id="a" class="btn-ab btn-a"
        ontouchstart="press('/a',this,event)"  ontouchend="release(this,event)"
        onmousedown="press('/a',this,event)"   onmouseup="release(this,event)">A</button>
      <button id="b" class="btn-ab btn-b"
        ontouchstart="press('/b',this,event)"  ontouchend="release(this,event)"
        onmousedown="press('/b',this,event)"   onmouseup="release(this,event)">B</button>
    </div>
    <div class="ab-labels">
      <div class="ab-label">B</div>
      <div class="ab-label">A</div>
    </div>
  </div>

</div>

<div id="status">Ready</div>

<script>

  let held = null;

  function press(cmd, el, e) {
    if (e) e.preventDefault();
    if (held && held !== el) release(held);
    held = el;
    el.classList.add('held');
    fetch(cmd).catch(() => {});
    document.getElementById('status').textContent = cmd.replace('/', '').toUpperCase();
  }

  function release(el, e) {
    if (e) e.preventDefault();
    if (!el) return;
    el.classList.remove('held');
    held = null;
    fetch('/release').catch(() => {});
    document.getElementById('status').textContent = 'Ready';
  }

  function exitToMenu() {
    fetch('/exit').then(() => {
      window.location.href = '/';
    }).catch(() => {
      window.location.href = '/';
    });
  }

  document.addEventListener('contextmenu', e => e.preventDefault());

  const keyMap = {
    ArrowUp:    ['/up',    'up'],
    ArrowDown:  ['/down',  'down'],
    ArrowLeft:  ['/left',  'left'],
    ArrowRight: ['/right', 'right'],
    z:          ['/a',     'a'],
    Z:          ['/a',     'a'],
    x:          ['/b',     'b'],
    X:          ['/b',     'b'],
    Enter:      ['/start', 'start'],
    Shift:      ['/select','select'],
    Escape:     ['/exit',  'menu'],
  };

  const heldKeys = new Set();

  document.addEventListener('keydown', e => {
    if (heldKeys.has(e.key) || !keyMap[e.key]) return;
    e.preventDefault();
    heldKeys.add(e.key);
    const [cmd, id] = keyMap[e.key];
    const el = document.getElementById(id);
    if (el && cmd !== '/exit') el.classList.add('held');
    fetch(cmd).catch(() => {});
    if (cmd === '/exit') {
      window.location.href = '/';
    }
    document.getElementById('status').textContent = cmd.replace('/', '').toUpperCase();
  });

  document.addEventListener('keyup', e => {
    if (!keyMap[e.key]) return;
    heldKeys.delete(e.key);
    const [, id] = keyMap[e.key];
    const el = document.getElementById(id);
    if (el) el.classList.remove('held');
    if (heldKeys.size === 0 && keyMap[e.key][0] !== '/exit') {
      fetch('/release').catch(() => {});
      document.getElementById('status').textContent = 'Ready';
    }
  });

</script>
</body>
</html>
)rawliteral";
