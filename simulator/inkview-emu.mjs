const EVT_INIT = 21
const EVT_EXIT = 22
const EVT_SHOW = 23
const EVT_REPAINT = 23
const EVT_HIDE = 24
const EVT_KEYDOWN = 25
const EVT_KEYPRESS = 25
const EVT_KEYUP = 26
const EVT_KEYRELEASE = 26
const EVT_KEYREPEAT = 28
const EVT_POINTERUP = 29
const EVT_POINTERDOWN = 30
const EVT_POINTERMOVE = 31
const EVT_POINTERLONG = 34
const EVT_POINTERHOLD = 35
const EVT_ORIENTATION = 32

const ROTATE0   = 0  // portrait
const ROTATE90  = 1  // landscape (rotated 90° CCW)
const ROTATE270 = 2  // landscape (rotated 90° CW)
const ROTATE180 = 3  // portrait upside-down

const BASE_W = 600;
const BASE_H = 800;
const EVT_FOCUS = 36
const EVT_UNFOCUS = 37
const EVT_ACTIVATE = 38


let canvas, ctx;
let _Module;

function updateStatus(msg) {
  document.getElementById('status').innerText = msg;
}

let main_handler;

class InkviewApi {
  _fontsByPtr = new Map();   // pointer → JS font object, set from EM_JS jsOpenFont
  _currentFontData = null;
  _currentColor = 0;
  _orientation = ROTATE0;

  OpenScreen() {}
  OpenScreenExt() {}

  InkViewMain(pfnCallback) {
    main_handler = pfnCallback;
    callMainHandler(EVT_INIT, 0, 0);
    callMainHandler(EVT_SHOW, 0, 0);
  }

  CloseApp() {
    goHome();
  }

  ScreenWidth() { return canvas.width }
  ScreenHeight() { return canvas.height }

  SetOrientation(n) {
    this._orientation = n;
    const landscape = (n === ROTATE90 || n === ROTATE270);
    canvas.width  = landscape ? BASE_H : BASE_W;
    canvas.height = landscape ? BASE_W : BASE_H;
  }
  GetOrientation() { return this._orientation }
  SetGlobalOrientation(n) {}
  GetGlobalOrientation() { return this._orientation }
  QueryGSensor() { return 1 }

  // --- Drawing ---

  ClearScreen() {
    ctx.fillStyle = '#FFFFFF';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
  }

  SetClip(x, y, w, h) {
    ctx.beginPath();
    ctx.rect(x, y, w, h);
    ctx.clip();
  }

  DrawPixel(x, y, color) {
    ctx.fillStyle = colorToCSS(color);
    ctx.fillRect(x, y, 1, 1);
  }

  DrawLine(x1, y1, x2, y2, color) {
    ctx.beginPath();
    ctx.moveTo(x1, y1);
    ctx.lineTo(x2, y2);
    ctx.strokeStyle = colorToCSS(color);
    ctx.lineWidth = 1;
    ctx.stroke();
  }

  DrawRect(x, y, w, h, color) {
    ctx.strokeStyle = colorToCSS(color);
    ctx.lineWidth = 1;
    ctx.strokeRect(x + 0.5, y + 0.5, w - 1, h - 1);
  }

  FillArea(x, y, w, h, color) {
    ctx.fillStyle = colorToCSS(color);
    ctx.fillRect(x, y, w, h);
  }

  DrawCircle(x0, y0, radius, color) {
    ctx.beginPath();
    ctx.arc(x0, y0, radius, 0, 2 * Math.PI);
    ctx.fillStyle = colorToCSS(color);
    ctx.fill();
  }

  InvertAreaBW(x, y, w, h) {
    const prev = ctx.globalCompositeOperation;
    ctx.globalCompositeOperation = 'difference';
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(x, y, w, h);
    ctx.globalCompositeOperation = prev;
  }

  // --- Fonts & Text ---

  OpenFont(name, size, aa) {
    // Alias fonts not available in the simulator
    const cssName = name.startsWith('DejaVu') ? 'LiberationSans' : name;
    // Returns JS object; jsOpenFont in inkview.c will call _create_ifont
    // and store the ptr → fontData mapping in _fontsByPtr.
    return {
      name: cssName,
      family: cssName,
      size: size,
      aa: aa,
      isbold: 0,
      isitalic: 0,
      charset: 1,
      color: 0,
      height: Math.round(size * 1.2),
      linespacing: Math.round(size * 1.4),
      baseline: Math.round(size * 0.8),
      fdata: 0,
    };
  }

  CloseFont(f) {}

  SetFont(fontPtr, color) {
    if (!fontPtr) return;
    const font = this._fontsByPtr.get(fontPtr);
    if (!font) return;
    this._currentFontData = font;
    this._currentColor = color;
    ctx.font = `${font.size}px "${font.name}"`;
    ctx.fillStyle = colorToCSS(color);
  }

  DrawString(x, y, text) {
    ctx.textBaseline = 'top';
    ctx.fillStyle = colorToCSS(this._currentColor);
    ctx.fillText(text, x, y);
  }

  StringWidth(text) {
    return Math.round(ctx.measureText(text).width);
  }

  CharWidth(charCode) {
    return Math.round(ctx.measureText(String.fromCharCode(charCode)).width);
  }

  DrawTextRect(x, y, w, h, text, flags) {
    const ALIGN_LEFT = 1, ALIGN_CENTER = 2, ALIGN_RIGHT = 4;
    const VALIGN_TOP = 16, VALIGN_MIDDLE = 32, VALIGN_BOTTOM = 64;

    let tx;
    if (flags & ALIGN_CENTER) {
      ctx.textAlign = 'center';
      tx = x + w / 2;
    } else if (flags & ALIGN_RIGHT) {
      ctx.textAlign = 'right';
      tx = x + w;
    } else {
      ctx.textAlign = 'left';
      tx = x;
    }

    let ty;
    if (flags & VALIGN_MIDDLE) {
      ctx.textBaseline = 'middle';
      ty = y + h / 2;
    } else if (flags & VALIGN_BOTTOM) {
      ctx.textBaseline = 'bottom';
      ty = y + h;
    } else {
      ctx.textBaseline = 'top';
      ty = y;
    }

    ctx.fillStyle = colorToCSS(this._currentColor);
    ctx.fillText(text, tx, ty);

    ctx.textAlign = 'left';
    ctx.textBaseline = 'top';
    return null;
  }

  OpenMenu(items, x, y, hproc) {
    const existing = document.getElementById('iv-menu');
    if (existing) existing.remove();

    const canvasEl = document.getElementById('canvas');
    const rect = canvasEl.getBoundingClientRect();
    const scaleX = rect.width / canvasEl.width;
    const scaleY = rect.height / canvasEl.height;

    const menuEl = document.createElement('div');
    menuEl.id = 'iv-menu';
    menuEl.className = 'iv-menu';
    menuEl.style.left = (rect.left + x * scaleX) + 'px';
    menuEl.style.top  = (rect.top  + y * scaleY) + 'px';

    // Pressing outside closes the menu. Listener is added with setTimeout
    // so the mousedown that triggered OpenMenu doesn't immediately close it.
    const onOutsideMousedown = (e) => {
      if (!menuEl.contains(e.target)) closeMenu();
    };

    const closeMenu = () => {
      if (!document.body.contains(menuEl)) return;
      menuEl.remove();
      document.removeEventListener('mousedown', onOutsideMousedown);
    };

    for (const item of items) {
      if (item.type === 1) { // ITEM_HEADER
        const hdr = document.createElement('div');
        hdr.className = 'iv-menu-header';
        hdr.textContent = item.text;
        menuEl.appendChild(hdr);
      } else if (item.type === 2) { // ITEM_ACTIVE
        const btn = document.createElement('div');
        btn.className = 'iv-menu-item';
        btn.textContent = item.text;
        // click handles normal tap; mouseup handles drag-from-canvas.
        // fired flag prevents double-trigger.
        let fired = false;
        const select = (e) => {
          if (fired) return;
          fired = true;
          e.stopPropagation();
          closeMenu();
          if (_Module && _Module.wasmTable && hproc) {
            _Module.wasmTable.get(hproc)(item.index);
          }
        };
        btn.addEventListener('click', select);
        btn.addEventListener('mouseup', select);
        menuEl.appendChild(btn);
      }
    }

    document.body.appendChild(menuEl);
    setTimeout(() => document.addEventListener('mousedown', onOutsideMousedown), 0);
  }

  // ibitmap: 1-bit packed (depth=1, MSB first) or 8-bit grayscale
  DrawBitmap(x, y, w, h, depth, scanline, data) {
    const imgData = ctx.createImageData(w, h);
    const pixels = imgData.data;
    for (let row = 0; row < h; row++) {
      for (let col = 0; col < w; col++) {
        let c;
        if (depth === 1) {
          const bit = (data[row * scanline + (col >> 3)] >> (7 - (col & 7))) & 1;
          c = bit ? 0 : 255;
        } else {
          c = data[row * scanline + col];
        }
        const idx = (row * w + col) * 4;
        pixels[idx] = pixels[idx + 1] = pixels[idx + 2] = c;
        pixels[idx + 3] = 255;
      }
    }
    ctx.putImageData(imgData, x, y);
  }

  // --- Screen update ---

  FullUpdate() {}
  SoftUpdate() {}
  PartialUpdate(x, y, w, h) {}
  PartialUpdateBW(x, y, w, h) {}

  // --- Timers ---

  SetHardTimer(name, proc, ms) {
    if (!_Module._timers) _Module._timers = {};
    if (_Module._timers[name]) { clearTimeout(_Module._timers[name]); delete _Module._timers[name]; }
    if (proc && ms > 0) {
      _Module._timers[name] = setTimeout(() => {
        delete _Module._timers[name];
        _Module.wasmTable.get(proc)();
      }, ms);
    }
  }

  SetWeakTimer(name, proc, ms) { this.SetHardTimer(name, proc, ms); }

  // --- Dialogs / UI overlays ---

  _makeOverlay() {
    const ov = document.createElement('div');
    ov.style.cssText = 'position:fixed;inset:0;background:rgba(0,0,0,.45);z-index:1000;display:flex;align-items:center;justify-content:center;';
    const box = document.createElement('div');
    box.style.cssText = 'background:#fff;border:2px solid #000;padding:24px 28px;min-width:320px;max-width:520px;font-family:sans-serif;box-shadow:4px 4px 0 #000;';
    ov.appendChild(box);
    document.body.appendChild(ov);
    return {ov, box};
  }

  _iconChar(icon) { return ['', 'ℹ️', '❓', '⚠️', '✖️'][icon] || ''; }

  _dialogHTML(icon, title, text) {
    return `<div style="font-size:18px;font-weight:bold;margin-bottom:10px;border-bottom:1px solid #000;padding-bottom:8px;">${this._iconChar(icon)} ${title}</div><div style="margin:12px 0;line-height:1.5;">${text.replace(/\n/g, '<br>')}</div>`;
  }

  Message(icon, title, text, timeout) {
    const {ov, box} = this._makeOverlay();
    box.innerHTML = this._dialogHTML(icon, title, text) + '<div style="font-size:12px;color:#666;margin-top:8px;">Click to dismiss</div>';
    const dismiss = () => ov.remove();
    ov.addEventListener('click', dismiss);
    if (timeout > 0) setTimeout(dismiss, timeout);
  }

  Dialog(icon, title, text, btn1, btn2, hproc) {
    const {ov, box} = this._makeOverlay();
    const btns = [btn1, btn2].filter(Boolean);
    let html = this._dialogHTML(icon, title, text) + '<div style="display:flex;gap:10px;margin-top:16px;">';
    btns.forEach((label, i) => {
      html += `<button data-idx="${i+1}" style="flex:1;padding:10px;border:2px solid #000;background:#fff;font-size:15px;cursor:pointer;">${label}</button>`;
    });
    html += '</div>';
    box.innerHTML = html;
    box.querySelectorAll('button').forEach(btn => {
      btn.addEventListener('click', () => {
        ov.remove();
        if (hproc && _Module) _Module.wasmTable.get(hproc)(+btn.dataset.idx);
      });
    });
  }

  _progressOverlay = null;
  _progressBar = null;
  _progressText = null;

  OpenProgressbar(icon, title, text, percent, hproc) {
    if (this._progressOverlay) this._progressOverlay.remove();
    const {ov, box} = this._makeOverlay();
    this._progressOverlay = ov;
    box.innerHTML = this._dialogHTML(icon, title, text) +
      `<div style="border:2px solid #000;height:24px;margin:12px 0;"><div id="iv-pb-bar" style="background:#000;height:100%;width:${percent}%;transition:width .2s;"></div></div>` +
      `<div id="iv-pb-text" style="font-size:14px;margin-bottom:12px;">${text}</div>` +
      (hproc ? `<button id="iv-pb-cancel" style="padding:8px 20px;border:2px solid #000;background:#fff;font-size:14px;cursor:pointer;">Cancel</button>` : '');
    this._progressBar  = box.querySelector('#iv-pb-bar');
    this._progressText = box.querySelector('#iv-pb-text');
    const cancelBtn = box.querySelector('#iv-pb-cancel');
    if (cancelBtn) {
      cancelBtn.addEventListener('click', () => {
        this.CloseProgressbar();
        if (_Module) _Module.wasmTable.get(hproc)(1);
      });
    }
  }

  UpdateProgressbar(text, percent) {
    if (this._progressText) this._progressText.textContent = text;
    if (this._progressBar)  this._progressBar.style.width  = Math.min(100, percent) + '%';
  }

  CloseProgressbar() {
    if (this._progressOverlay) { this._progressOverlay.remove(); this._progressOverlay = null; }
    this._progressBar = null;
    this._progressText = null;
  }

  // --- Language ---

  GetLangText(s) {
    return s.startsWith('@') ? s.slice(1) : s;
  }

  // --- Device info ---

  GetDeviceModel() {
    return "PocketBook 631";
  }
}

function colorToCSS(color) {
  return `#${(color >>> 0).toString(16).padStart(6, '0')}`;
}

function goHome() {
  try { callMainHandler(EVT_EXIT, 0, 0); } catch(e) {}
  main_handler = null;
  location.hash = '';
  location.reload();
}

function callMainHandler(event_type, param_one, param_two) {
  if (!main_handler || !_Module) return 0;
  const fn = _Module.wasmTable.get(main_handler);
  return fn(event_type, param_one, param_two);
}

export async function initApp(projectName) {
  try {
    updateStatus("Initializing...");
    canvas = document.getElementById('canvas');
    canvas.width = 600;
    canvas.height = 800;
    ctx = canvas.getContext('2d');

    ctx.fillStyle = '#FFFFFF';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    updateStatus("Loading " + projectName + "...");

    // Use cache-busting query param so the browser doesn't return
    // a previously-cached WASM module for a different project.
    const modulePath = '../projects/' + projectName + '/index.mjs?t=' + Date.now();
    const {default: createPocketBookModule} = await import(modulePath);

    var api = new InkviewApi();
    _Module = await createPocketBookModule({api: api});

    updateStatus("Starting...");
    _Module._main();

    setupEventHandlers(api);
    updateStatus("Running: " + projectName);

  } catch (error) {
    updateStatus("Error: " + error.message);
    console.error(error);
  }
}

// Cycle: portrait → landscape 90 → portrait 180 → landscape 270 → portrait
const ORIENTATION_CYCLE = [ROTATE0, ROTATE90, ROTATE180, ROTATE270];

function setupEventHandlers(api) {
  function canvasCoords(event) {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    return {
      x: Math.round((event.clientX - rect.left) * scaleX),
      y: Math.round((event.clientY - rect.top) * scaleY),
    };
  }

  canvas.addEventListener('mousedown', function(event) {
    const {x, y} = canvasCoords(event);
    _Module._touchX = x; _Module._touchY = y;
    callMainHandler(EVT_POINTERDOWN, x, y);
  });

  canvas.addEventListener('mouseup', function(event) {
    const {x, y} = canvasCoords(event);
    _Module._touchX = x; _Module._touchY = y;
    callMainHandler(EVT_POINTERUP, x, y);
  });

  canvas.addEventListener('mousemove', function(event) {
    if (event.buttons === 0) return;
    const {x, y} = canvasCoords(event);
    _Module._touchX = x; _Module._touchY = y;
    callMainHandler(EVT_POINTERMOVE, x, y);
  });

  document.addEventListener('keydown', function(event) {
    if (event.keyCode === 27) { goHome(); return; }
    // Map browser key codes to InkView key constants
    const KEY_MAP = {
      37: 0x13,  // ArrowLeft  → KEY_LEFT
      38: 0x11,  // ArrowUp    → KEY_UP
      39: 0x14,  // ArrowRight → KEY_RIGHT
      40: 0x12,  // ArrowDown  → KEY_DOWN
      13: 0x0a,  // Enter      → KEY_OK
       8: 0x08,  // Backspace  → KEY_DELETE
      33: 0x18,  // PageUp     → KEY_PREV
      34: 0x19,  // PageDown   → KEY_NEXT
    };
    var code = KEY_MAP[event.keyCode] !== undefined ? KEY_MAP[event.keyCode] : event.keyCode;
    if (KEY_MAP[event.keyCode] !== undefined) event.preventDefault();
    callMainHandler(EVT_KEYPRESS, code, 0);
  });

  document.getElementById('prevPage').addEventListener('click', function() {
    callMainHandler(EVT_KEYPRESS, 0x18, 0);  // KEY_PREV
  });
  document.getElementById('nextPage').addEventListener('click', function() {
    callMainHandler(EVT_KEYPRESS, 0x19, 0);  // KEY_NEXT
  });
  document.getElementById('home').addEventListener('click', function() {
    goHome();
  });
  document.getElementById('back').addEventListener('click', function() {
    callMainHandler(EVT_KEYPRESS, 8, 0);
  });

  document.getElementById('rotate').addEventListener('click', function() {
    var cur = api._orientation;
    var idx = ORIENTATION_CYCLE.indexOf(cur);
    var next = ORIENTATION_CYCLE[(idx + 1) % ORIENTATION_CYCLE.length];
    callMainHandler(EVT_ORIENTATION, next, 0);
  });
}
