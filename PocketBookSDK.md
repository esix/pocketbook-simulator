# PocketBook SDK Documentation
*Revision 1.05*

---

## Table of Contents

1. [Introduction](#2-introduction)
   - 1.1 [Minimal app(lication)](#21-minimal-application)
   - 1.2 [Getting real](#22-getting-real)
2. [How it works](#3-how-it-works)
   - 2.1 [InkViewMain()](#31-inkviewmain)
   - 2.2 [main_handler()](#32-main_handler)
   - 2.3 [Events delivered to main_handler()](#33-events-delivered-to-main_handler)
3. [Output to a screen](#4-output-to-a-screen)
4. [Screen update](#5-screen-update)
5. [Bitmap handling](#6-bitmap-handling)

---

## 2 Introduction

### 2.1 Minimal app(lication)

```cpp
// hello_world.cpp
#include "inkview.h"

static const int kFontSize = 40;

static int main_handler(int event_type, int /*param_one*/, int /*param_two*/) {
    if (EVT_INIT == event_type) {
        ifont *font = OpenFont("LiberationSans", kFontSize, 0);
        ClearScreen();
        SetFont(font, BLACK);
        DrawTextRect(0, ScreenHeight()/2 - kFontSize/2, ScreenWidth(),
                     kFontSize, "Hello, world!", ALIGN_CENTER);
        FullUpdate();
        CloseFont(font);
    } else if (EVT_KEYPRESS == event_type) {
        CloseApp();
    }
    return 0;
}

int main (int argc, char* argv[]) {
    InkViewMain(main_handler);
    return 0;
}
```

**Command line to build:**

```
arm-none-linux-gnueabi-g++ hello_world.cpp -o hello_world -linkview
```

This simple app clears the screen and prints "Hello, world!" in the center of the screen. Pressing any key exits the app.

**Things to note:**

1. One has to provide handler function in order to receive events from keyboard (`EVT_KEYPRESS`) and on app state changes (`EVT_INIT`).
2. Resources (fonts) acquired have to be released.

---

### 2.2 Getting real

In this section structure for more realistic and thus complex app is suggested. This is one of many ways to organize the app, so you are free to adopt, adapt or ignore this suggestion.

As application grows the following should be addressed:

1. Separating and encapsulating application state.
2. Handling more events, while keeping `main_handler()` simple and straight forward.
3. Ability to configure app on initialization.
4. Cleanup on application exit.
5. Ability to handle key presses depending on key pressed.
6. Providing means to detect and handle errors.

```cpp
// app_common.h
#ifndef APP_COMMON_H
#define APP_COMMON_H

const int kErrorFail = -1;

#define RETURN_ERROR_IF_NULL(ptr, error_code) { \
    if (NULL == ptr) return error_code; }

#endif // APP_COMMON_H
```

```cpp
// app_state.h
#ifndef APP_STATE_H
#define APP_STATE_H

#include <cstddef>

struct ifont_s;
typedef struct ifont_s ifont;

namespace HelloWorldTwo {

class AppState;
AppState& GetAppState();

class AppState {
public:
    // Constructor ...
    int OnInit(int font_size);
    int OnKeyPress(int key_code);
    int OnShow();
    void OnExit();
private:
    void Deinit();
    ifont *font_ = NULL;
    int font_size_ = 0;
};

} // namespace HelloWorldTwo
#endif // APP_STATE_H
```

```cpp
// app_state.cpp
#include "app_state.h"
#include "app_common.h"
#include "inkview.h"

namespace HelloWorldTwo {

AppState& GetAppState() {
    static AppState app_state;
    return app_state;
}

int AppState::OnInit(int font_size) {
    font_size_ = font_size;
    font_ = OpenFont("LiberationSans", font_size_, 0);
    RETURN_ERROR_IF_NULL(font_, kErrorFail);
    return 0;
}

int AppState::OnKeyPress(int /*key_code*/) {
    CloseApp();
    return 0;
}

int AppState::OnShow() {
    ClearScreen();
    SetFont(font_, BLACK);
    DrawTextRect(0, ScreenHeight()/2 - font_size_/2, ScreenWidth(),
                 font_size_, "Hello, world!", ALIGN_CENTER);
    FullUpdate();
    return 0;
}

void AppState::OnExit() {
    Deinit();
}

void AppState::Deinit() {
    if (font_) {
        CloseFont(font_);
        font_ = NULL;
    }
}

} // namespace HelloWorldTwo
```

```cpp
// hello_world_two.cpp
#include "app_state.h"
#include "inkview.h"

using namespace HelloWorldTwo;

static const int kFontSize = 40;

static int main_handler(int event_type, int param_one, int /*param_two*/) {
    int rv = 0;
    AppState& app_state = GetAppState();
    switch (event_type) {
        case EVT_INIT:
            rv = app_state.OnInit(kFontSize);
            break;
        case EVT_SHOW:
            rv = app_state.OnShow();
            break;
        case EVT_KEYPRESS:
            rv = app_state.OnKeyPress(param_one);
            break;
        case EVT_EXIT:
            app_state.OnExit();
            break;
        default:
            break;
    }
    return rv;
} // int main_handler(3)

int main (int argc, char* argv[]) {
    InkViewMain(main_handler);
    return 0;
}
```

**Command line to build:**

```
arm-none-linux-gnueabi-g++ app_state.cpp hello_world_two.cpp -o hello_world_two -linkview
```

**Things to note:**

1. Header files don't include other header files unless absolutely necessary. `cstddef` is included because of `NULL`. This inclusion may be avoided by moving constructor to `.cpp` file.
2. Namespace is used for app specific classes and functions.
3. App state encapsulated into class (not struct!).
4. Handler calls methods of `AppState` class (rather than accessing struct members directly).
5. `AppState` instance is accessed via function, rather than using global variable. Time for `AppState` instance creation is definite: on the first `GetAppState()` call.
6. To pass `argc` and `argv` from `main()` to `InkViewMain()`, use the overloaded version `InkViewMain(main_handler, argc, argv)`.

---

## 3 How it works

### 3.1 InkViewMain()

`InkViewMain()` performs the following:

1. ...
2. ...

*(See also `OpenScreen()` in Section 7 for detailed initialization steps.)*

---

### 3.2 main_handler()

Handler function passed to `InkViewMain()` has the following signature:

**Syntax:**

```c
int main_handler(int event_type, int param_one, int param_two);
```

**Parameters:**

- `event_type` – event type, see `EVT_*` defines. Non-zero.
- `param_one` – meaning depends on event type.
- `param_two` – meaning depends on event type.

**Returns:**

Zero means that event wasn't handled by the app. In this case `InkViewMain()` will handle the event. Non-zero means that event was handled by the app, and `InkViewMain()` does nothing for the event.

---

### 3.3 Events delivered to main_handler()

**Events handled by InkViewMain() on any firmware:**

| Event type | Description |
|---|---|
| `EVT_INIT` | Causes wake time to be at least `SLEEPDELAY` from now regardless of `main_handler()` return value. |
| `EVT_PANEL_TASKLIST` | Opens tasks list. |
| `EVT_PANEL_MPLAYER` | Opens media player, if not open, otherwise closes media player. |
| `EVT_PANEL_BLUETOOTH` | Opens Bluetooth info. |
| `EVT_PANEL_BLUETOOTH_A2DP` | Opens A2dp info. |
| `EVT_PANEL_CLOCK` | Opens calendar. |

*(Events handled by InkViewMain() on firmware without panel also exist — see original doc.)*

**Frequently used events:**

| Event | Description |
|---|---|
| `EVT_SHOW, 0, 0` | Delivered when the app is required to draw itself: in `InkViewMain()` after `EVT_INIT` and before entering event loop; for new handler, when setting new handler. Note: event isn't delivered after dialog or task manager window closed, or after switching back to the app — in such cases library saves and restores app's screen content. |
| `EVT_REPAINT, 0, 0` | *(see EVT_SHOW notes above)* |
| `EVT_MP_STATECHANGED, new_state, 0` | Delivered (to active task only) on detecting media player state change. `new_state` is one of `MP_*` values e.g. `MP_PLAYING`. |
| `EVT_MP_TRACKCHANGED, new_track, 0` | Delivered (to active task only) on detecting track change. `new_track` – currently playing track's number. |
| `EVT_ORIENTATION, orientation, 0` | Delivered (to active task only) on detecting orientation change. Orientation values: `0` = portrait, `1` = landscape (portrait rotated 90° counterclockwise), `2` = landscape (portrait rotated 90° clockwise), `3` = portrait (portrait rotated 180°). |
| `EVT_KEYDOWN, key_code, 0` | Delivered when hardware key is pressed. `key_code` – code of pressed key (value of `KEY_*` macro). |
| `EVT_KEYREPEAT, key_code, repeat_count` | Delivered repeatedly while hardware key is held down. `repeat_count` – number of times (starting with 1) `EVT_KEYREPEAT` is received for given key since `EVT_KEYDOWN`. |
| `EVT_KEYUP, key_code, repeat_count` | Delivered when hardware key is released. `key_code` – code of released key. |
| `EVT_KEYRELEASE, key_code, repeat_count` | `repeat_count` – number of times `EVT_KEYREPEAT` was called since last `EVT_KEYDOWN`. May be zero. |
| `EVT_KEYPRESS` | *(shorthand event for key press handling)* |
| `EVT_POINTERDOWN, x, y` | Delivered when stylus touches the screen. `(x, y)` – coordinates of touch point. |
| `EVT_POINTERMOVE, x, y` | Delivered between `EVT_POINTERDOWN` and `EVT_POINTERUP`. App should be ready to receive up to 25 events per second. `(x, y)` – current stylus coordinates. |
| `EVT_POINTERUP, x, y` | Delivered when stylus moved from screen. `(x, y)` – last known stylus coordinates. |
| `EVT_EXIT` | Delivered when exiting event loop. |

---

## 4 Output to a screen

Output to screen involves two steps:

1. Drawing to current canvas which is buffer in memory.
2. Sending part or all of current canvas to screen driver. We will call this step "update", because functions that perform this step have "Update" in their names.

### 4.4 DrawLine()

Draws straight line with ends in given points. Line is drawn as a series of pixels of given color. Line ends are included. Line isn't antialiased.

```c
void DrawLine(int x_one, int y_one, int x_two, int y_two, int color)
```

- `(x_one, y_one)` and `(x_two, y_two)` – coordinates of line ends.
- `color` – 24-bit color value. See `DrawPixel()` for details.

---

### 4.5 DrawRect()

Draws rectangle of given color in current canvas. Rectangle has 1-pixel wide border and isn't filled inside. Lower right corner is `(x+width-1, y+height-1)`.

```c
void DrawRect(int x, int y, int width, int height, int color)
```

- `(x, y)` – upper left corner
- `width` – must be positive number
- `height` – must be positive number
- `color` – 24-bit color value. See `DrawPixel()` for details.

---

### 4.6 FillArea()

Draws filled rectangle of given color in current canvas. Rectangle has `(x, y)` and `(x+width-1, y+height-1)` as upper left and lower right corners, lower right corner included.

```c
void FillArea(int x, int y, int width, int height, int color)
```

- `(x, y)` – upper left corner
- `width`, `height` – dimensions
- `color` – 24-bit color value

---

### 4.10 DrawSelection()

Draws rounded rectangle with 3 pixel line width. Rectangle isn't filled and is inside given rectangle. Drawing is performed with given color and into current canvas.

```c
void DrawSelection(int x, int y, int width, int height, int color)
```

---

### 4.11 DitherArea()

Function is used to decrease number of colors (intensity levels) to a value of `levels` parameter. To reduce negative effects of color quantization dithering can be used: `method = DITHER_PATTERN` or `method = DITHER_DIFFUSION`. Function is applied to given rectangle in current canvas. Input can be 8 bpp grayscale or 24 bit color. Result has same depth as input, but always grayscale (for color image all components have the same value).

```c
void DitherArea(int x, int y, int width, int height, int levels, int method)
```

- `levels` – number between 2 and 16 inclusive
- `method` – one of:
  - `DITHER_THRESHOLD`
  - `DITHER_PATTERN`
  - `DITHER_DIFFUSION`

**Usages:**

- Call with `levels=2` and `method=DITHER_THRESHOLD` to have image that can be quickly updated using black-and-white screen update.
- Use `DITHER_PATTERN` or `DITHER_DIFFUSION` for higher visual quality.

---

### 4.12 StretchCanvas() / DrawBitmap()

**Usages:**

- May be used to scale image to fit/fill the screen.
- It is perfectly OK to have source and destination size the same and `flags=0` to simply draw image on screen.

**Notes:**

- Flags affects clipping rectangle of canvas the same way as it affects image: clipping rectangle is rotated and/or mirrored.

---

### 4.13 SetCanvas()

Function sets given canvas to be a current canvas.

```c
void SetCanvas(icanvas *new_canvas)
```

**`icanvas` structure:**

```c
typedef struct icanvas_s {
    int width;      /* in pixels */
    int height;     /* in pixels */
    int scanline;   /* size in bytes of single line (of pixels) */
    int depth;      /* bits per pixel (bpp), can only be 8 or 24 */
    /* (clipx1, clipy1) and (clipx2, clipy2) are respectively coordinates
       in pixels of upper left and bottom right corner of clipping rectangle.
       One must ensure that clipping rectangle is inside canvas, otherwise
       result is undefined. */
    int clipx1, clipx2;
    int clipy1, clipy2;
    unsigned char *addr; /* buffer to store pixel data. Buffer must be at least
                            height*scanline bytes in size */
} icanvas;
```

---

## 5 Screen update

### 5.1 FullUpdate()

Sends full screen content to the display driver. Performs a grayscale update.

```c
void FullUpdate()
```

---

### 5.2 FullUpdateHQ()

*Deprecated. Use `SoftUpdate()` instead.*

The only difference from `FullUpdate()` is that display depth is set to maximum: 4 bpp currently.

```c
void FullUpdateHQ()
```

---

### 5.3 SoftUpdate()

Alternative to `FullUpdate()`. In effect is (almost) `PartialUpdate()` for the whole screen.

```c
void SoftUpdate()
```

---

### 5.4 PartialUpdate()

Content of the given rectangle in screen buffer is sent to display driver. Function is smart and tries to perform the most suitable update possible: black and white update is performed if all pixels in given rectangle are black and white. Otherwise grayscale update is performed. If whole screen is specified (`0, 0, ScreenWidth(), ScreenHeight()`), then grayscale update is performed.

```c
void PartialUpdate(int x, int y, int width, int height)
```

---

### 5.5 PartialUpdateBlack()

*Deprecated. Use `PartialUpdate()` instead.*

```c
void PartialUpdateBlack(int x, int y, int width, int height)
```

---

### 5.6 PartialUpdateBW()

*Deprecated. Use `PartialUpdate()` instead.*

```c
void PartialUpdateBW(int x, int y, int width, int height)
```

---

## 6 Bitmap handling

### 6.1 ibitmap structure

```c
typedef struct ibitmap_s {
    unsigned short width;
    unsigned short height;
    unsigned short depth;
    unsigned short scanline;
    unsigned char data[];
} ibitmap;
```

---

### 6.2 LoadBitmap()

Loads bitmap from given BMP file. Input file must be uncompressed file with 16 or 256 colors. Input file's size can't exceed 1,500,000 bytes.

```c
ibitmap *LoadBitmap(const char *filename);
```

- `filename` – non-NULL string specifying path to BMP file to open.

**Returns:** pointer to newly created bitmap on success, NULL on failure. Returned bitmap has the same depth as display (4 bpp currently).

**Notes:**
- Color `R=128, G=128, B=64` in input file is transparent: when bitmap is drawn on screen, pixel with transparent color isn't drawn thus preserving current display pixel.
- If transparent color is present, then returned bitmap has most significant bit (`0x8000`) of `depth` field set.
- Use `DeleteBitmap()` to free memory used by bitmap.

---

### 6.3 SaveBitmap()

Saves given bitmap into file with given name.

---

### 6.4 BitmapFromScreen()

Captures screen content into a bitmap.

---

### 6.5 BitmapFromScreenR()

Captures a rectangular region of the screen into a bitmap.

---

### 6.6 NewBitmap()

Creates a new blank bitmap.

```c
ibitmap *NewBitmap(int width, int height);
```

---

### 6.7 LoadJPEG()

Loads image from given JPEG file into 2 bpp bitmap with given dimensions. Image is scaled to fit/fill given rectangle with or without preserving image's aspect ratio. Color correction is performed on intermediate 256 color grayscale image for each pixel using formula:

```
new_color = (current_color - 128) * co/100 + 128 + br - 100
```

After this, intermediate image is converted into resulting 2 bpp bitmap.

```c
ibitmap *LoadJPEG(const char *filename, int width, int height, int br, int co,
                  int proportional);
```

**Parameters:**

- `width`, `height` – dimensions of resulting bitmap.
- `br`, `co` – coefficients in color correction formula (see above). `br = co = 100` – don't perform correction.
- `proportional` – if non-zero, then aspect ratio for image is preserved; otherwise image's width and height may be changed independently in order to fit/fill dimensions of resulting bitmap.

---

### 6.8 SaveJPEG()

Saves given bitmap as JPEG file.

```c
int SaveJPEG(const char *filename, ibitmap *bitmap, ...);
```

---

## 7 OpenScreen()

`OpenScreen()` is called from `InkViewMain()` before the first event is delivered to `main_handler()`. Thus avoid calling `OpenScreen()` explicitly from your app.

`OpenScreen()` performs the following steps:

1. ...
2. ...
12. For device build, obtains address of `"GetBookInfo"` (`"_GetBookInfo"`) and `"GetBookCover"` (`"_GetBookCover"`) functions in `USERBOOKINFO` shared library. If not available, default internal functions are used.
13. Sets signal handlers for `SIGHUP`, `SIGQUIT`, `SIGINT` and `SIGTERM`.

**Returns:** Always returns 1.

---

## 8 Revision History

| Revision | Date | Comments |
|---|---|---|
| 1.0 | 20-Jan-2012 | The first revision |
| 1.01 | 23-Jan-2012 | Example in 1.1 finished. Example in 1.2 done. |
| 1.02 | 7-Feb-2012 | Frequently used events delivered to `main_handler()` are described. |
| 1.03 | — | Section "Output to screen" is in progress (95% complete). |
| 1.04 | 13-Feb-2012 | Section 5 "Screen update" is finished. |
| 1.05 | 21-Feb-2012 | Section 6 "Bitmap handling" in progress. |
