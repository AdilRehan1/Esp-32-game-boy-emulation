#pragma once
#include <pgmspace.h>

const char WEBPAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<title>ESP32 GameBoy</title>
<style>

* {
  box-sizing: border-box;
  -webkit-tap-highlight-color: transparent;
  user-select: none;
  touch-action: manipulation;
}

body {
  margin: 0;
  padding: 0;
  background: #1a1a2e;
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 100vh;
  font-family: sans-serif;
  overflow: hidden;
}

/* ── Shell ── */
.shell {
  width: 360px;
  background: linear-gradient(180deg, #7b68ee 0%, #6a5acd 40%, #483d8b 100%);
  border-radius: 20px 20px 60px 60px;
  padding: 12px 16px 40px 16px;
  box-shadow: 0 8px 32px rgba(0,0,0,0.6), inset 0 1px 0 rgba(255,255,255,0.2);
  position: relative;
}

/* ── Screen area ── */
.screen-bezel {
  background: #111;
  border-radius: 10px;
  padding: 10px;
  margin-bottom: 14px;
  box-shadow: inset 0 2px 8px rgba(0,0,0,0.8);
}

.screen-label {
  color: #aaa;
  font-size: 9px;
  letter-spacing: 3px;
  text-align: center;
  text-transform: uppercase;
  margin-bottom: 6px;
}

.screen {
  background: #0d1f0d;
  height: 70px;
  border-radius: 4px;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #3a7d44;
  font-size: 11px;
  letter-spacing: 1px;
}

/* ── Shoulder buttons ── */
.shoulders {
  display: flex;
  justify-content: space-between;
  margin: 0 -4px 8px -4px;
}

.btn-shoulder {
  width: 80px;
  height: 28px;
  font-size: 13px;
  font-weight: bold;
  background: linear-gradient(180deg, #5a4fcf, #3d35a0);
  color: #ddd;
  border: none;
  cursor: pointer;
  box-shadow: 0 3px 0 #1a1060;
  letter-spacing: 1px;
}

.btn-shoulder.left  { border-radius: 8px 4px 4px 14px; }
.btn-shoulder.right { border-radius: 4px 8px 14px 4px; }

.btn-shoulder:active, .btn-shoulder.held {
  background: linear-gradient(180deg, #0ff, #0cc);
  color: #000;
  transform: translateY(3px);
  box-shadow: 0 0 0 #1a1060;
}

/* ── Main controls row ── */
.controls {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 4px;
}

/* ── D-Pad ── */
.dpad-wrap {
  width: 120px;
  height: 120px;
  position: relative;
}

.dpad-h, .dpad-v {
  position: absolute;
  background: #222;
  border-radius: 4px;
  box-shadow: 0 3px 0 #000, inset 0 1px 0 rgba(255,255,255,0.05);
}

/* Horizontal bar */
.dpad-h {
  width: 100%;
  height: 34%;
  top: 33%;
  left: 0;
  display: flex;
  align-items: center;
  justify-content: space-between;
}

/* Vertical bar */
.dpad-v {
  width: 34%;
  height: 100%;
  top: 0;
  left: 33%;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: space-between;
}

.dpad-btn {
  background: transparent;
  border: none;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  flex: 1;
  width: 100%;
  color: #555;
  font-size: 16px;
  padding: 2px;
}

.dpad-btn:active, .dpad-btn.held {
  color: #0ff;
  background: rgba(0,255,255,0.08);
}

/* ── Center buttons (SELECT / START) ── */
.center-btns {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
}

.btn-sys {
  width: 52px;
  height: 18px;
  font-size: 9px;
  font-weight: bold;
  letter-spacing: 1px;
  background: linear-gradient(180deg, #3d3d5c, #2a2a40);
  color: #888;
  border: none;
  border-radius: 9px;
  cursor: pointer;
  box-shadow: 0 2px 0 #111, inset 0 1px 0 rgba(255,255,255,0.07);
  text-transform: uppercase;
}

.btn-sys:active, .btn-sys.held {
  background: linear-gradient(180deg, #0ff, #0cc);
  color: #000;
  transform: translateY(2px);
  box-shadow: 0 0 0 #111;
}

/* ── A / B buttons ── */
.ab-wrap {
  width: 120px;
  height: 120px;
  position: relative;
}

/* GBA diagonal layout: B bottom-left, A center-right */
.btn-ab {
  position: absolute;
  width: 46px;
  height: 46px;
  border-radius: 50%;
  border: none;
  cursor: pointer;
  font-size: 16px;
  font-weight: bold;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 4px 0 rgba(0,0,0,0.5);
  color: #fff;
}

.btn-b {
  background: radial-gradient(circle at 35% 35%, #e05050, #a00);
  bottom: 14px;
  left: 10px;
  box-shadow: 0 4px 0 #500;
}

.btn-a {
  background: radial-gradient(circle at 35% 35%, #50c050, #060);
  top: 14px;
  right: 10px;
  box-shadow: 0 4px 0 #030;
}

.btn-ab:active, .btn-ab.held {
  transform: translateY(4px);
  box-shadow: 0 0 0;
  filter: brightness(1.4);
}

/* ── Status bar ── */
#status {
  text-align: center;
  color: #555;
  font-size: 10px;
  letter-spacing: 1px;
  margin-top: 14px;
  text-transform: uppercase;
}

</style>
</head>
<body>

<div class="shell">

  <!-- Screen -->
  <div class="screen-bezel">
    <div class="screen-label">Game Boy</div>
    <div class="screen">&#9654; GAME RUNNING</div>
  </div>

  <!-- Shoulder buttons -->
  <div class="shoulders">
    <button class="btn-shoulder left"
      ontouchstart="press('/left_shoulder',this)" ontouchend="release(this)"
      onmousedown="press('/left_shoulder',this)"  onmouseup="release(this)">L</button>
    <button class="btn-shoulder right"
      ontouchstart="press('/right_shoulder',this)" ontouchend="release(this)"
      onmousedown="press('/right_shoulder',this)"  onmouseup="release(this)">R</button>
  </div>

  <!-- D-pad + Center + A/B -->
  <div class="controls">

    <!-- D-Pad -->
    <div class="dpad-wrap">

      <!-- Horizontal bar: LEFT | RIGHT -->
      <div class="dpad-h">
        <button class="dpad-btn"
          ontouchstart="press('/left',this)"  ontouchend="release(this)"
          onmousedown="press('/left',this)"   onmouseup="release(this)">&#9664;</button>
        <button class="dpad-btn"
          ontouchstart="press('/right',this)" ontouchend="release(this)"
          onmousedown="press('/right',this)"  onmouseup="release(this)">&#9654;</button>
      </div>

      <!-- Vertical bar: UP | DOWN -->
      <div class="dpad-v">
        <button class="dpad-btn"
          ontouchstart="press('/up',this)"   ontouchend="release(this)"
          onmousedown="press('/up',this)"    onmouseup="release(this)">&#9650;</button>
        <button class="dpad-btn"
          ontouchstart="press('/down',this)" ontouchend="release(this)"
          onmousedown="press('/down',this)"  onmouseup="release(this)">&#9660;</button>
      </div>

    </div>

    <!-- SELECT / START -->
    <div class="center-btns">
      <button class="btn-sys"
        ontouchstart="press('/select',this)" ontouchend="release(this)"
        onmousedown="press('/select',this)"  onmouseup="release(this)">SELECT</button>
      <button class="btn-sys"
        ontouchstart="press('/start',this)"  ontouchend="release(this)"
        onmousedown="press('/start',this)"   onmouseup="release(this)">START</button>
    </div>

    <!-- A / B diagonal -->
    <div class="ab-wrap">
      <button class="btn-ab btn-b"
        ontouchstart="press('/b',this)" ontouchend="release(this)"
        onmousedown="press('/b',this)"  onmouseup="release(this)">B</button>
      <button class="btn-ab btn-a"
        ontouchstart="press('/a',this)" ontouchend="release(this)"
        onmousedown="press('/a',this)"  onmouseup="release(this)">A</button>
    </div>

  </div>

  <div id="status">Ready</div>

</div>

<script>

  let currentBtn = null;

  function press(cmd, el) {
    if (currentBtn && currentBtn !== el) release(currentBtn);
    currentBtn = el;
    if (el) el.classList.add('held');
    fetch(cmd).catch(() => {});
    document.getElementById('status').textContent = cmd.replace('/','').toUpperCase();
  }

  function release(el) {
    if (el) el.classList.remove('held');
    currentBtn = null;
    fetch('/release').catch(() => {});
    document.getElementById('status').textContent = 'READY';
  }

  // Keyboard support
  const keyMap = {
    ArrowUp:    '/up',
    ArrowDown:  '/down',
    ArrowLeft:  '/left',
    ArrowRight: '/right',
    z: '/a', Z: '/a',
    x: '/b', X: '/b',
    Enter:  '/start',
    Shift:  '/select'
  };

  const held = new Set();

  document.addEventListener('keydown', e => {
    if (held.has(e.key) || !keyMap[e.key]) return;
    held.add(e.key);
    fetch(keyMap[e.key]).catch(() => {});
    document.getElementById('status').textContent = e.key.toUpperCase();
  });

  document.addEventListener('keyup', e => {
    if (!keyMap[e.key]) return;
    held.delete(e.key);
    if (held.size === 0) {
      fetch('/release').catch(() => {});
      document.getElementById('status').textContent = 'READY';
    }
  });

</script>
</body>
</html>
)rawliteral";
