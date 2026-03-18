#pragma once
#include <pgmspace.h>

const char WEBPAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 GameBoy</title>
<style>

  * {
    box-sizing: border-box;
    -webkit-tap-highlight-color: transparent;
    user-select: none;
  }

  body {
    background: #111;
    color: #fff;
    font-family: sans-serif;
    text-align: center;
    margin: 0;
    padding: 20px;
  }

  h2 {
    color: #0ff;
    letter-spacing: 2px;
    margin-bottom: 24px;
  }

  .dpad {
    display: grid;
    grid-template-columns: repeat(3, 70px);
    grid-template-rows: repeat(3, 70px);
    gap: 6px;
    justify-content: center;
    margin-bottom: 20px;
  }

  .ab {
    display: flex;
    justify-content: center;
    gap: 20px;
    margin-bottom: 20px;
  }

  .sys {
    display: flex;
    justify-content: center;
    gap: 20px;
  }

  button {
    width: 70px;
    height: 70px;
    font-size: 16px;
    font-weight: bold;
    border: none;
    border-radius: 12px;
    cursor: pointer;
    background: #333;
    color: #fff;
    box-shadow: 0 4px 0 #000;
    transition: background 0.1s, transform 0.1s;
  }

  button:active,
  button.held {
    background: #0ff;
    color: #000;
    transform: translateY(3px);
    box-shadow: 0 1px 0 #000;
  }

  .btn-a  { background: #c00; }
  .btn-b  { background: #060; }
  .btn-start, .btn-select { width: 90px; height: 50px; font-size: 13px; }

  .empty { background: transparent; box-shadow: none; pointer-events: none; }

  #status {
    margin-top: 16px;
    font-size: 12px;
    color: #555;
  }

</style>
</head>
<body>

<h2>&#127918; ESP32 GameBoy</h2>

<!-- D-Pad -->
<div class="dpad">
  <div class="empty"></div>
  <button id="up"    class="btn-dpad"
    ontouchstart="press('/up',this)"    ontouchend="release(this)"
    onmousedown="press('/up',this)"     onmouseup="release(this)">&#9650;</button>
  <div class="empty"></div>

  <button id="left"  class="btn-dpad"
    ontouchstart="press('/left',this)"  ontouchend="release(this)"
    onmousedown="press('/left',this)"   onmouseup="release(this)">&#9664;</button>
  <div class="empty"></div>
  <button id="right" class="btn-dpad"
    ontouchstart="press('/right',this)" ontouchend="release(this)"
    onmousedown="press('/right',this)"  onmouseup="release(this)">&#9654;</button>

  <div class="empty"></div>
  <button id="down"  class="btn-dpad"
    ontouchstart="press('/down',this)"  ontouchend="release(this)"
    onmousedown="press('/down',this)"   onmouseup="release(this)">&#9660;</button>
  <div class="empty"></div>
</div>

<!-- A / B -->
<div class="ab">
  <button id="b" class="btn-b"
    ontouchstart="press('/b',this)"     ontouchend="release(this)"
    onmousedown="press('/b',this)"      onmouseup="release(this)">B</button>
  <button id="a" class="btn-a"
    ontouchstart="press('/a',this)"     ontouchend="release(this)"
    onmousedown="press('/a',this)"      onmouseup="release(this)">A</button>
</div>

<!-- Start / Select -->
<div class="sys">
  <button id="select" class="btn-select"
    ontouchstart="press('/select',this)" ontouchend="release(this)"
    onmousedown="press('/select',this)"  onmouseup="release(this)">SELECT</button>
  <button id="start"  class="btn-start"
    ontouchstart="press('/start',this)"  ontouchend="release(this)"
    onmousedown="press('/start',this)"   onmouseup="release(this)">START</button>
</div>

<div id="status">Connecting...</div>

<script>

  let currentBtn = null;

  function press(cmd, el) {
    if (currentBtn && currentBtn !== el) release(currentBtn);
    currentBtn = el;
    if (el) el.classList.add('held');
    fetch(cmd).catch(() => {});
    document.getElementById('status').textContent = 'Pressed: ' + cmd.replace('/','');
  }

  function release(el) {
    if (el) el.classList.remove('held');
    currentBtn = null;
    fetch('/release').catch(() => {});
    document.getElementById('status').textContent = 'Ready';
  }

  // Keyboard support for desktop testing
  const keyMap = {
    ArrowUp:    '/up',
    ArrowDown:  '/down',
    ArrowLeft:  '/left',
    ArrowRight: '/right',
    z: '/a', Z: '/a',
    x: '/b', X: '/b',
    Enter:      '/start',
    Shift:      '/select'
  };

  const heldKeys = new Set();

  document.addEventListener('keydown', e => {
    if (heldKeys.has(e.key)) return;
    if (keyMap[e.key]) {
      heldKeys.add(e.key);
      fetch(keyMap[e.key]).catch(() => {});
      document.getElementById('status').textContent = 'Key: ' + e.key;
    }
  });

  document.addEventListener('keyup', e => {
    if (keyMap[e.key]) {
      heldKeys.delete(e.key);
      if (heldKeys.size === 0) {
        fetch('/release').catch(() => {});
        document.getElementById('status').textContent = 'Ready';
      }
    }
  });

  document.getElementById('status').textContent = 'Ready';

</script>
</body>
</html>
)rawliteral";