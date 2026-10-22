/**
 * @file    hp_ui.h
 * @brief   Hacker-Protocol shared TUI component library
 *
 *   ST7789 320x240 / efontCN_16 (8x16 px ASCII)
 *   Green-on-black CRT aesthetic, double-border chrome, matches bbt_ui.
 *
 *   Header-only, template <typename LCD>, no state — all components are pure
 *   draw functions. State is owned by each app.
 *
 *   ┌══════════════════════════════════════╗  y=0
 *   ║  [ TITLE ]                 [BADGE]  ║  y=0..24    HEADER  (25 px)
 *   ╞══════════════════════════════════════╡  y=24
 *   │ ┃> row 0              dotline at 49 │▌  y=26..212  CONTENT (7 rows × 24 px)
 *   │   row 1 ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄   │
 *   ╞══════════════════════════════════════╡  y=212 (dim)
 *   ╞══════════════════════════════════════╡  y=214 (bright double-line)
 *   ║  [A] hint                hint [B]   ║  y=215..239  FOOTER  (25 px)
 *   └══════════════════════════════════════┘  y=239
 *
 *   Components (call in this order to compose a screen):
 *     hp::drawChrome(lcd)                     C1  outer double-border frame
 *     hp::drawHeader(lcd, title, badge, c)    C1  header text + status badge
 *     hp::drawFooter(lcd, leftHint, rightHint)C3  footer hints
 *     hp::clearContent(lcd)                   --  wipe content area
 *     hp::drawListItem(lcd, row, text, sel)   C2  one scrollable list row
 *     hp::drawScrollbar(lcd, total, top, vis) C2  right-edge scrollbar
 *     hp::drawStat(lcd, row, label, val, col) C6  key:value stat row
 *     hp::drawProgress(lcd, x,y,w, pct, col)  C5  horizontal progress bar
 *     hp::drawDialog(lcd, line1, line2)       C7  centered message box
 *     hp::drawToast(lcd, msg, color)          --  transient bottom banner
 *     hp::drawScanWave(lcd, phase)            C9  animated scan line
 *     hp::drawBadge(lcd, x,y, text, color)    C4  small color tag
 */
#pragma once
#include <LovyanGFX.hpp>
#include <cstring>
#include <cstdio>
#include <cmath>

namespace hp {

/* ─────────────── Palette ───────────────
 *  Dark-olive / phosphor-lime CRT aesthetic ("old-terminal" look).
 *  COL_BG  : dark olive/army green background
 *  COL_FG  : bright lime phosphor (text, borders, list-selection fill)
 *  COL_DIM : medium olive (dotted separators, inactive hints)
 *  COL_HL  : same as FG (selected list row is fully filled with lime)
 */
static constexpr uint16_t COL_BG       = 0x1A22;   // dark olive green   (RGB ~26,68,18)
static constexpr uint16_t COL_FG       = 0xC7E6;   // phosphor lime      (RGB ~198,252,50)
static constexpr uint16_t COL_DIM      = 0x4A86;   // medium olive       (RGB ~74,144,50)
static constexpr uint16_t COL_ACCENT   = 0xFFFF;   // white   (rare strong highlight)
static constexpr uint16_t COL_HL       = 0xC7E6;   // = COL_FG (full-row selection bar)
static constexpr uint16_t COL_WARN     = 0xFFE0;   // yellow
static constexpr uint16_t COL_ERR      = 0xF800;   // red
static constexpr uint16_t COL_INFO     = 0x07FF;   // cyan

/* ─────────────── Layout ─────────────── */
/*  320×240 pixel grid.  All y values are 0-indexed pixels:
 *
 *   HDR_TOP=0  top border
 *   HDR_SEP=24 header/content separator   (25 px header zone)
 *   CON_Y0=26  content top                (2 px gap)
 *   CON_Y1=212 content bottom             (186 px = 7 rows × 24 px)
 *   FTR_SEP=214 footer top border         (double-line: dim @212, bright @214)
 *   FTR_TXT=221 footer text baseline
 *   FTR_BOTTOM=239 bottom border          (25 px footer zone)
 */
static constexpr int W          = 320;
static constexpr int H          = 240;
static constexpr int HDR_TOP    = 0;
static constexpr int HDR_SEP    = 24;    // header bottom border y
static constexpr int CON_Y0     = 26;    // content area top y
static constexpr int CON_Y1     = 212;   // content area bottom y
static constexpr int FTR_SEP    = 214;   // footer top border y
static constexpr int FTR_TXT    = 221;   // footer text baseline y
static constexpr int FTR_BOTTOM = 239;   // bottom border y
static constexpr int CON_H      = CON_Y1 - CON_Y0;   // 186
static constexpr int PAD_X      = 8;     // horizontal text padding
static constexpr int ITEM_H     = 24;    // single-line item  (16 px text + 8 px padding)
static constexpr int LIST_VIS   = CON_H / ITEM_H;    // 7 visible rows
static constexpr int SBAR_W     = 5;     // scrollbar rail width
static constexpr int ACT_BAR_W  = 3;     // left active-indicator bar width

/* ─────────────── Chrome ─────────────── */

/* Outer double-border frame. Call once after fillScreen or to refresh borders. */
template<typename LCD>
inline void drawChrome(LCD& lcd)
{
    lcd.fillScreen(COL_BG);
    /* Header box */
    lcd.drawFastHLine(0, HDR_TOP,  W, COL_FG);
    lcd.drawFastHLine(0, HDR_SEP,  W, COL_FG);
    lcd.drawFastVLine(0,     HDR_TOP + 1, HDR_SEP - 1, COL_FG);
    lcd.drawFastVLine(W - 1, HDR_TOP + 1, HDR_SEP - 1, COL_FG);
    /* Content side borders */
    lcd.drawFastVLine(0,     CON_Y0, CON_H + 1, COL_FG);
    lcd.drawFastVLine(W - 1, CON_Y0, CON_H + 1, COL_FG);
    /* Double-line content/footer separator: dim line then gap then bright line */
    lcd.drawFastHLine(0, CON_Y1,      W, COL_DIM);  // y=212 dim  (bottom of content)
    lcd.drawFastHLine(0, FTR_SEP,     W, COL_FG);   // y=214 bright (top of footer)
    /* Footer box */
    lcd.drawFastHLine(0, FTR_BOTTOM,  W, COL_FG);
    lcd.drawFastVLine(0,     FTR_SEP + 1, FTR_BOTTOM - FTR_SEP - 1, COL_FG);
    lcd.drawFastVLine(W - 1, FTR_SEP + 1, FTR_BOTTOM - FTR_SEP - 1, COL_FG);
}

/* Draw header: "[ TITLE ]" left, optional status badge on the right.
 *  If `badgeColor == COL_FG` (default) the badge is rendered as plain lime
 *  text (e.g. "2026, EARTH").  If a different color is given, the badge is
 *  drawn as a filled rounded pill (used for status badges like "SD", "RUN"). */
template<typename LCD>
inline void drawHeader(LCD& lcd, const char* title,
                       const char* badge = nullptr,
                       uint16_t badgeColor = COL_FG)
{
    lcd.fillRect(1, 1, W - 2, HDR_SEP - 1, COL_BG);
    lcd.setFont(&fonts::efontCN_16);

    lcd.setTextColor(COL_FG, COL_BG);
    lcd.setCursor(8, 4);
    lcd.print("[ ");
    lcd.setTextColor(COL_ACCENT, COL_BG);
    lcd.print(title);
    lcd.setTextColor(COL_FG, COL_BG);
    lcd.print(" ]");

    if (badge && badge[0]) {
        int tw = (int)strlen(badge) * 8;
        if (badgeColor == COL_FG) {
            /* Plain phosphor text (e.g. flavor tag "2026, EARTH") */
            lcd.setTextColor(COL_FG, COL_BG);
            lcd.setCursor(W - tw - PAD_X, 4);
            lcd.print(badge);
        } else {
            /* Filled rounded pill (status badge) */
            int bw = tw + 10;
            int bx = W - bw - 5;
            lcd.fillRoundRect(bx, 2, bw, 20, 3, badgeColor);
            uint16_t txt = (badgeColor == COL_WARN || badgeColor == COL_ACCENT) ? COL_BG : COL_ACCENT;
            lcd.setTextColor(txt, badgeColor);
            lcd.setCursor(bx + 5, 3);
            lcd.print(badge);
        }
    }
}

/* Draw footer hints; left starts at PAD_X, right is right-aligned. */
template<typename LCD>
inline void drawFooter(LCD& lcd, const char* left, const char* right)
{
    lcd.fillRect(1, FTR_SEP + 1, W - 2, FTR_BOTTOM - FTR_SEP - 1, COL_BG);
    lcd.setFont(&fonts::efontCN_16);
    if (left && left[0]) {
        lcd.setTextColor(COL_FG, COL_BG);
        lcd.setCursor(PAD_X, FTR_TXT);
        lcd.print(left);
    }
    if (right && right[0]) {
        int tw = (int)strlen(right) * 8;
        lcd.setTextColor(COL_DIM, COL_BG);
        lcd.setCursor(W - tw - PAD_X, FTR_TXT);
        lcd.print(right);
    }
}

/* Wipe content area (keeps chrome). */
template<typename LCD>
inline void clearContent(LCD& lcd)
{
    lcd.fillRect(1, CON_Y0, W - 2, CON_H, COL_BG);
}

/* ─────────────── List rows ─────────────── */

/* Draw one list row at visible index `row` (0..LIST_VIS-1).
 *   selected     → full lime fill across the row, dark text, "> " prefix
 *   non-selected → lime text on dark bg, dotted bottom separator (COL_DIM)
 *   reserves SBAR_W on the right for the scrollbar rail. */
template<typename LCD>
inline void drawListItem(LCD& lcd, int row, const char* text, bool selected)
{
    int y = CON_Y0 + 2 + row * ITEM_H;
    int w = W - 2 - SBAR_W - 2;
    uint16_t bg = selected ? COL_HL : COL_BG;
    uint16_t fg = selected ? COL_BG : COL_FG;

    /* Background fill */
    lcd.fillRect(1, y, w, ITEM_H, bg);

    /* Text — vertically centered, "> " prefix on the selected row */
    lcd.setFont(&fonts::efontCN_16);
    lcd.setTextColor(fg, bg);
    lcd.setCursor(PAD_X, y + (ITEM_H - 16) / 2);
    lcd.print(selected ? "> " : "  ");

    /* Truncate to fit */
    int maxChars = (w - PAD_X - 16) / 8;
    char buf[64];
    int n = (int)strlen(text);
    if (n > maxChars) n = maxChars;
    if (n < 0) n = 0;
    if (n >= (int)sizeof(buf)) n = sizeof(buf) - 1;
    memcpy(buf, text, n); buf[n] = '\0';
    lcd.print(buf);

    /* Dotted bottom separator on non-selected rows */
    if (!selected) {
        for (int dx = 2; dx < w - 1; dx += 4)
            lcd.drawPixel(1 + dx, y + ITEM_H - 1, COL_DIM);
    }
}

/* Clear a single (unused) list row slot. */
template<typename LCD>
inline void clearListRow(LCD& lcd, int row)
{
    int y = CON_Y0 + 2 + row * ITEM_H;
    int w = W - 2 - SBAR_W - 2;
    lcd.fillRect(1, y, w, ITEM_H, COL_BG);
}

/* Constants for two-line list items */
static constexpr int ITEM2_H    = 34;    // 2-line item: 1px top + 16 + 16 + 1px bottom
static constexpr int LIST2_VIS  = CON_H / ITEM2_H;   // 5 visible rows

/* Two-line list row: title (line 1, bright) + sub (line 2, dim).
 *   `row` is the visible slot index (0..LIST2_VIS-1).
 * Selection style matches single-line drawListItem (badusb): full lime fill
 * with dark text on the selected row. */
template<typename LCD>
inline void drawListItemSub(LCD& lcd, int row, const char* title,
                            const char* sub, bool selected)
{
    int y = CON_Y0 + 2 + row * ITEM2_H;
    int w = W - 2 - SBAR_W - 2;
    uint16_t bg = selected ? COL_HL : COL_BG;
    uint16_t fgTitle = selected ? COL_BG : COL_FG;
    uint16_t fgSub   = selected ? COL_BG : COL_DIM;
    lcd.fillRect(1, y, w, ITEM2_H, bg);
    lcd.setFont(&fonts::efontCN_16);
    lcd.setTextColor(fgTitle, bg);
    lcd.setCursor(PAD_X, y + 1);
    lcd.print(selected ? "> " : "  ");
    lcd.print(title);
    if (sub && sub[0]) {
        lcd.setTextColor(fgSub, bg);
        lcd.setCursor(PAD_X + 16, y + 17);
        lcd.print(sub);
    }
}

template<typename LCD>
inline void clearListRow2(LCD& lcd, int row)
{
    int y = CON_Y0 + 2 + row * ITEM2_H;
    int w = W - 2 - SBAR_W - 2;
    lcd.fillRect(1, y, w, ITEM2_H, COL_BG);
}

/* Scrollbar variant for 2-line lists. */
template<typename LCD>
inline void drawScrollbar2(LCD& lcd, int total, int scrollTop, int visible)
{
    int x = W - 2 - SBAR_W;
    int trackY = CON_Y0 + 2;
    int trackH = LIST2_VIS * ITEM2_H;
    lcd.fillRect(x, trackY, SBAR_W, trackH, COL_BG);
    if (total <= visible) return;
    int thumbH = trackH * visible / total;
    if (thumbH < 6) thumbH = 6;
    int thumbY = trackY + (trackH - thumbH) * scrollTop / (total - visible);
    lcd.drawFastVLine(x + SBAR_W / 2, trackY, trackH, COL_DIM);
    lcd.fillRect(x, thumbY, SBAR_W, thumbH, COL_FG);
}

/* ─────────────── Loading / busy-wait animation ─────────────── */

/* Centered loading panel with rotating bar + caption.  Call drawLoadingBegin
 * once to lay out chrome and static text, then drawLoadingTick(phase) every
 * frame (~60 ms) to spin the bar.
 *
 *   ┌──────────────────────────┐
 *   │  Reading SD card...      │
 *   │  ████░░░░░░░░░░░░         │  <- sweeping fill
 *   │  /badusb/                │
 *   └──────────────────────────┘
 */
template<typename LCD>
inline void drawLoadingBegin(LCD& lcd, const char* title, const char* subtitle)
{
    clearContent(lcd);
    int bw = 240, bh = 90;
    int bx = (W - bw) / 2;
    int by = CON_Y0 + (CON_H - bh) / 2;
    lcd.drawRect(bx, by, bw, bh, COL_FG);
    lcd.drawRect(bx + 2, by + 2, bw - 4, bh - 4, COL_DIM);
    lcd.setFont(&fonts::efontCN_16);
    lcd.setTextColor(COL_ACCENT, COL_BG);
    int tw = (int)strlen(title) * 8;
    lcd.setCursor(bx + (bw - tw) / 2, by + 12);
    lcd.print(title);
    if (subtitle && subtitle[0]) {
        lcd.setTextColor(COL_DIM, COL_BG);
        int sw = (int)strlen(subtitle) * 8;
        lcd.setCursor(bx + (bw - sw) / 2, by + 64);
        lcd.print(subtitle);
    }
}

/* Advance the loading animation.  `phase` should monotonically increase. */
template<typename LCD>
inline void drawLoadingTick(LCD& lcd, uint32_t phase)
{
    int bw = 240, bh = 90;
    int bx = (W - bw) / 2;
    int by = CON_Y0 + (CON_H - bh) / 2;
    int barX = bx + 12, barY = by + 36, barW = bw - 24, barH = 12;
    lcd.drawRect(barX, barY, barW, barH, COL_DIM);
    /* Sweeping segment */
    int segW = 60;
    int travel = barW - 4 - segW;
    if (travel < 1) travel = 1;
    /* triangle wave (ping-pong) */
    int t = (int)(phase % (2 * travel));
    if (t >= travel) t = 2 * travel - t;
    lcd.fillRect(barX + 2, barY + 2, travel + segW, barH - 4, COL_BG);
    lcd.fillRect(barX + 2 + t, barY + 2, segW, barH - 4, COL_FG);
}

/* ─────────────── Image (PNG) ───────────────
 *  Shared PNG draw helper: target = any LovyanGFX surface (LCD or Sprite),
 *  (x, y) = top-left destination on the target, (data, len) = PNG byte
 *  stream from any source (SD, PSRAM, flash).  Use this everywhere instead
 *  of calling drawPng() directly so future image features (caching, tints,
 *  fade-in, …) can be added in one place. */
template<typename TARGET>
inline void drawPng(TARGET& target, int x, int y,
                    const uint8_t* data, size_t len)
{
    if (!data || len == 0) return;
    target.drawPng(data, len, x, y);
}

/* Draw right-edge scrollbar.  Pass total count, scrollTop, visibleCount. */
template<typename LCD>
inline void drawScrollbar(LCD& lcd, int total, int scrollTop, int visible)
{
    int x = W - 2 - SBAR_W;
    int trackY = CON_Y0 + 2;
    int trackH = LIST_VIS * ITEM_H;
    /* Always clear track */
    lcd.fillRect(x, trackY, SBAR_W, trackH, COL_BG);
    if (total <= visible) return;
    int thumbH = trackH * visible / total;
    if (thumbH < 6) thumbH = 6;
    int thumbY = trackY + (trackH - thumbH) * scrollTop / (total - visible);
    /* Track */
    lcd.drawFastVLine(x + SBAR_W / 2, trackY, trackH, COL_DIM);
    /* Thumb */
    lcd.fillRect(x, thumbY, SBAR_W, thumbH, COL_FG);
}

/* ─────────────── Stat / data rows ─────────────── */

/* Draw a "LABEL : VALUE" row at content row `row` (0..). */
template<typename LCD>
inline void drawStat(LCD& lcd, int row, const char* label,
                     const char* value, uint16_t valColor = COL_ACCENT)
{
    int y = CON_Y0 + 4 + row * ITEM_H;
    lcd.fillRect(1, y, W - 2, ITEM_H, COL_BG);
    lcd.setFont(&fonts::efontCN_16);
    lcd.setTextColor(COL_DIM, COL_BG);
    lcd.setCursor(PAD_X, y);
    lcd.print(label);
    lcd.setTextColor(valColor, COL_BG);
    lcd.setCursor(PAD_X + 14 * 8, y);   /* label column width = 14 chars */
    lcd.print(": ");
    lcd.print(value);
}

/* ─────────────── Progress bar ─────────────── */

/* Horizontal progress bar with optional caption.
 *   pct in [0.0, 1.0] */
template<typename LCD>
inline void drawProgress(LCD& lcd, int x, int y, int w,
                         float pct, uint16_t color = COL_FG)
{
    if (pct < 0) pct = 0; if (pct > 1) pct = 1;
    lcd.drawRect(x, y, w, 12, color);
    int innerW = w - 4;
    int filled = (int)(innerW * pct);
    lcd.fillRect(x + 2, y + 2, filled, 8, color);
    lcd.fillRect(x + 2 + filled, y + 2, innerW - filled, 8, COL_BG);
}

/* ─────────────── Dialog ─────────────── */

/* Centered message dialog (1-2 lines).  Caller draws footer hints separately. */
template<typename LCD>
inline void drawDialog(LCD& lcd, const char* line1, const char* line2 = nullptr,
                       uint16_t borderColor = COL_FG)
{
    int bw = 280, bh = (line2 ? 90 : 60);
    int bx = (W - bw) / 2;
    int by = (H - bh) / 2;
    lcd.fillRect(bx, by, bw, bh, COL_BG);
    lcd.drawRect(bx, by, bw, bh, borderColor);
    lcd.drawRect(bx + 2, by + 2, bw - 4, bh - 4, borderColor);
    lcd.setFont(&fonts::efontCN_16);
    lcd.setTextColor(COL_ACCENT, COL_BG);
    int tw1 = (int)strlen(line1) * 8;
    lcd.setCursor(bx + (bw - tw1) / 2, by + 14);
    lcd.print(line1);
    if (line2) {
        lcd.setTextColor(COL_FG, COL_BG);
        int tw2 = (int)strlen(line2) * 8;
        lcd.setCursor(bx + (bw - tw2) / 2, by + 40);
        lcd.print(line2);
    }
}

/* ─────────────── Toast banner (transient) ─────────────── */

/* Short banner overlaid just above the footer.  Caller is responsible for
 * removing it (typically by redrawing the scene). */
template<typename LCD>
inline void drawToast(LCD& lcd, const char* msg, uint16_t color = COL_FG)
{
    int bh = 18;
    int by = FTR_SEP - bh - 2;
    int bw = (int)strlen(msg) * 8 + 16;
    if (bw > W - 8) bw = W - 8;
    int bx = (W - bw) / 2;
    lcd.fillRect(bx, by, bw, bh, color);
    lcd.drawRect(bx, by, bw, bh, COL_ACCENT);
    lcd.setFont(&fonts::efontCN_16);
    uint16_t txt = (color == COL_WARN) ? COL_BG : COL_ACCENT;
    lcd.setTextColor(txt, color);
    lcd.setCursor(bx + 8, by + 1);
    lcd.print(msg);
}

/* ─────────────── Scan-wave animation (LearnWait, AP scan) ─────────────── */

/* Draws a scrolling phase-indexed wave in a 280x40 strip starting at content y+offsetY.
 *   `phase` should increment ~1 per frame; call repeatedly to animate. */
template<typename LCD>
inline void drawScanWave(LCD& lcd, int offsetY, uint32_t phase,
                         uint16_t color = COL_FG)
{
    int sx = 20, sy = CON_Y0 + offsetY, sw = 280, sh = 40;
    lcd.fillRect(sx, sy, sw, sh, COL_BG);
    lcd.drawRect(sx, sy, sw, sh, COL_DIM);
    int mid = sy + sh / 2;
    /* Sweeping bar */
    int barX = sx + 2 + (int)(phase % (sw - 6));
    lcd.fillRect(barX, sy + 2, 3, sh - 4, color);
    /* Center line */
    lcd.drawFastHLine(sx + 2, mid, sw - 4, COL_DIM);
    /* Tick marks */
    for (int i = 0; i < sw - 4; i += 10) {
        lcd.drawPixel(sx + 2 + i, mid - 2, color);
        lcd.drawPixel(sx + 2 + i, mid + 2, color);
    }
}

/* ─────────────── Small badge ─────────────── */

template<typename LCD>
inline void drawBadge(LCD& lcd, int x, int y, const char* text, uint16_t color)
{
    int tw = (int)strlen(text) * 8 + 8;
    lcd.fillRoundRect(x, y, tw, 16, 2, color);
    lcd.setFont(&fonts::efontCN_16);
    uint16_t txt = (color == COL_WARN || color == COL_ACCENT) ? COL_BG : COL_ACCENT;
    lcd.setTextColor(txt, color);
    lcd.setCursor(x + 4, y + 1);
    lcd.print(text);
}

/* ─────────────── Name editor (cursor / chevron style — keeps app_11 logic) ─────────────── */

/* Render a single-line text input with cursor highlight at `editPos`.
 *   Used by App11 name editor.  Up/Down/Left/Right semantics are owned by caller. */
template<typename LCD>
inline void drawTextInput(LCD& lcd, int x, int y, const char* buf,
                          int editPos, int maxLen)
{
    int bw = maxLen * 8 + 8;
    lcd.fillRect(x, y, bw, ITEM_H, COL_BG);
    lcd.drawRect(x, y, bw, ITEM_H, COL_DIM);
    lcd.setFont(&fonts::efontCN_16);
    int n = (int)strlen(buf);
    for (int i = 0; i < n && i < maxLen; i++) {
        char c[2] = { buf[i], 0 };
        if (i == editPos) {
            lcd.fillRect(x + 4 + i * 8, y + 1, 8, ITEM_H - 2, COL_FG);
            lcd.setTextColor(COL_BG, COL_FG);
        } else {
            lcd.setTextColor(COL_ACCENT, COL_BG);
        }
        lcd.setCursor(x + 4 + i * 8, y + 1);
        lcd.print(c);
    }
    if (editPos >= n && editPos < maxLen) {
        lcd.fillRect(x + 4 + editPos * 8, y + 1, 8, ITEM_H - 2, COL_FG);
        lcd.setTextColor(COL_BG, COL_FG);
        lcd.setCursor(x + 4 + editPos * 8, y + 1);
        lcd.print("_");
    }
}

/* ─────────────── Virtual keyboard (shared) ─────────────── */

/* Draw a simple directional virtual keyboard panel:
 *   - top input line (editable text preview)
 *   - N-key grid (rows computed from keyCount/cols)
 *   - selected key highlight by `selected` index */
template<typename LCD>
inline void drawVirtualKeyboard(LCD& lcd,
                                const char* heading,
                                const char* input,
                                int maxLen,
                                const char* const* keys,
                                int keyCount,
                                int cols,
                                int selected)
{
    if (!keys || keyCount <= 0 || cols <= 0) return;

    const int panelX = 8;
    const int panelY = CON_Y0 + 6;
    const int panelW = W - 16;
    const int panelH = CON_H - 12;

    lcd.fillRect(1, CON_Y0, W - 2, CON_H, COL_BG);
    lcd.drawRect(panelX, panelY, panelW, panelH, COL_FG);
    lcd.drawRect(panelX + 2, panelY + 2, panelW - 4, panelH - 4, COL_DIM);

    lcd.setFont(&fonts::efontCN_16);

    /* Input box with inline heading prefix ("Name: TV_POWER_") */
    const int inX = panelX + 8;
    const int inY = panelY + 8;
    const int inW = panelW - 16;
    const int inH = 22;
    lcd.drawRect(inX, inY, inW, inH, COL_FG);

    const char* head = heading ? heading : "Name:";
    int headLen = (int)strlen(head);
    lcd.setTextColor(COL_FG, COL_BG);
    lcd.setCursor(inX + 6, inY + 3);
    lcd.print(head);
    if (headLen > 0) {
        lcd.print(' ');
        headLen++;
    }

    int textX  = inX + 6 + headLen * 8;
    int maxShow = (inX + inW - 6 - textX) / 8;
    if (maxShow < 1) maxShow = 1;
    char shown[48];
    int n = input ? (int)strlen(input) : 0;
    if (n > maxShow) {
        memcpy(shown, input + (n - maxShow), maxShow);
        shown[maxShow] = '\0';
    } else {
        if (n >= (int)sizeof(shown)) n = (int)sizeof(shown) - 1;
        if (n > 0) memcpy(shown, input, n);
        shown[n] = '\0';
    }
    lcd.setTextColor(COL_ACCENT, COL_BG);
    lcd.setCursor(textX, inY + 3);
    lcd.print(shown);
    if (input && (int)strlen(input) < maxLen) {
        int tw = (int)strlen(shown) * 8;
        lcd.setCursor(textX + tw, inY + 3);
        lcd.print("_");
    }

    /* Key grid */
    const int rows = (keyCount + cols - 1) / cols;
    const int gap = 3;
    const int gridX = panelX + 8;
    const int gridY = inY + inH + 6;
    const int gridW = panelW - 16;
    const int gridH = panelY + panelH - 8 - gridY;
    const int keyW = (gridW - (cols - 1) * gap) / cols;
    const int keyH = (gridH - (rows - 1) * gap) / rows;

    for (int i = 0; i < keyCount; i++) {
        int r = i / cols;
        int c = i % cols;
        int x = gridX + c * (keyW + gap);
        int y = gridY + r * (keyH + gap);
        bool sel = (i == selected);
        uint16_t bg = sel ? COL_HL : COL_BG;
        uint16_t fg = sel ? COL_BG : COL_ACCENT;

        lcd.fillRect(x, y, keyW, keyH, bg);
        lcd.drawRect(x, y, keyW, keyH, COL_FG);
        if (sel) lcd.drawRect(x + 1, y + 1, keyW - 2, keyH - 2, COL_FG);
        lcd.setTextColor(fg, bg);

        const char* label = keys[i] ? keys[i] : "?";
        int tw = (int)strlen(label) * 8;
        int tx = x + (keyW - tw) / 2;
        if (tx < x + 1) tx = x + 1;
        lcd.setCursor(tx, y + (keyH - 16) / 2);
        lcd.print(label);
    }
}

/* ─────────────── Hourglass animation (blocking SD / file ops) ─────────────── */

/*  Draws a 32×48 px hourglass glyph centered at (cx, cy).
 *  phase increments over time (wrap 0..127) to animate sand flow.
 *  Sand drains from top chamber → bottom as phase 0→127, then restart.
 *
 *  Call repeatedly to animate; erase only the local bounding rect, not the
 *  whole screen.  drawHourglassScreen() composites it with title/dots. */
template<typename LCD>
inline void drawHourglass(LCD& lcd, int cx, int cy, uint32_t phase)
{
    const int HW = 16, HH = 48;           /* half-width, full height */
    int bx = cx - HW, by = cy - HH / 2;
    const int TW = HW * 2, nW = 4;        /* total width, neck width */
    int cH   = HH / 2 - 3;                /* chamber height ≈ 21 px */
    int neckY = by + cH;
    int neckX = bx + (TW - nW) / 2;
    int btmY0 = neckY + 6, btmY1 = by + HH;

    /* Erase bounding box */
    lcd.fillRect(bx - 2, by - 2, TW + 4, HH + 4, COL_BG);

    /* Top chamber outline (trapezoid: wide top, narrow neck) */
    lcd.drawFastHLine(bx, by, TW, COL_FG);
    lcd.drawLine(bx,      by, neckX,      neckY, COL_FG);
    lcd.drawLine(bx + TW, by, neckX + nW, neckY, COL_FG);

    /* Neck */
    lcd.fillRect(neckX, neckY, nW, 6, COL_DIM);

    /* Bottom chamber outline (inverted trapezoid) */
    lcd.drawLine(neckX,      btmY0, bx,      btmY1, COL_FG);
    lcd.drawLine(neckX + nW, btmY0, bx + TW, btmY1, COL_FG);
    lcd.drawFastHLine(bx, btmY1, TW, COL_FG);

    /* Sand phase: 0..127 */
    int p  = (int)(phase % 128);
    float t = (float)p / 127.0f;   /* 0 = top full, 1 = top empty */

    /* Top sand — rows from neck upward, drains as t increases */
    int topRows = (int)((1.0f - t) * (float)cH);
    for (int r = 0; r < topRows; r++) {
        int sw = nW + (TW - nW) * r / cH;
        int sy = neckY - 1 - r;
        if (sy > by) lcd.drawFastHLine(cx - sw / 2, sy, sw, COL_FG);
    }

    /* Bottom sand — rows from floor upward, grows as t increases */
    int botRows = (int)(t * (float)cH);
    for (int r = 0; r < botRows; r++) {
        int sw = TW - (TW - nW) * r / cH;
        int sy = btmY1 - 1 - r;
        if (sy >= btmY0) lcd.drawFastHLine(cx - sw / 2, sy, sw, COL_FG);
    }

    /* Drip pixel (yellow dot falling through neck) */
    if (t < 0.92f) {
        int dy = neckY + 2 + (int)((float)(phase % 14) / 14.0f * 4.0f);
        if (dy < btmY0) lcd.drawPixel(cx, dy, COL_WARN);
    }
}

/*  Full-content hourglass loading overlay.
 *  Pre-condition: chrome + header + footer already drawn once.
 *  Only redraws the content area (hourglass + animated dots + subtitle).
 *  subtitle: short path / filename shown at bottom of content area. */
template<typename LCD>
inline void drawHourglassScreen(LCD& lcd, const char* subtitle, uint32_t phase)
{
    /* Hourglass centered in content area */
    drawHourglass(lcd, W / 2, CON_Y0 + CON_H / 2, phase);

    /* Animated "..." dots */
    int dots = (int)(phase / 16) % 4;
    char dotBuf[5];
    for (int i = 0; i < 4; i++) dotBuf[i] = (i < dots) ? '.' : ' ';
    dotBuf[4] = '\0';
    int dotY = CON_Y0 + CON_H / 2 + 32;
    lcd.fillRect(W / 2 - 16, dotY, 32, 16, COL_BG);
    lcd.setFont(&fonts::efontCN_16);
    lcd.setTextColor(COL_WARN, COL_BG);
    lcd.setCursor(W / 2 - 12, dotY);
    lcd.print(dotBuf);

    /* Subtitle path at bottom of content area */
    if (subtitle && subtitle[0]) {
        char sbuf[34];
        int slen = (int)strlen(subtitle);
        if (slen > 33) slen = 33;
        memcpy(sbuf, subtitle, slen); sbuf[slen] = '\0';
        int subY = CON_Y1 - 20;
        lcd.fillRect(1, subY, W - 2, 18, COL_BG);
        lcd.setTextColor(COL_DIM, COL_BG);
        lcd.setCursor((W - slen * 8) / 2, subY);
        lcd.print(sbuf);
    }
}

/* ─────────────── Virtual button (touch-remote UI) ─────────────── */

/*  TUI-style rectangular virtual button.
 *  Normal:  black bg  |  green border + corner brackets  |  white label
 *  Pressed: green bg  |  white double-border              |  black label */
template<typename LCD>
inline void drawVirtualButton(LCD& lcd, int x, int y, int w, int h,
                               const char* label, bool pressed)
{
    uint16_t bg = pressed ? COL_FG  : COL_BG;
    uint16_t fg = pressed ? COL_BG  : COL_ACCENT;
    uint16_t bd = pressed ? COL_ACCENT : COL_FG;

    lcd.fillRect(x, y, w, h, bg);
    lcd.drawRect(x, y, w, h, bd);

    if (pressed) {
        /* Inner double border for tactile feedback */
        lcd.drawRect(x + 2, y + 2, w - 4, h - 4, COL_ACCENT);
    } else {
        /* Corner brackets (TUI style) */
        const int cs = 5;
        lcd.drawFastHLine(x + 3,         y + 3,         cs, COL_DIM);
        lcd.drawFastVLine(x + 3,         y + 3,         cs, COL_DIM);
        lcd.drawFastHLine(x + w - 3 - cs, y + 3,        cs, COL_DIM);
        lcd.drawFastVLine(x + w - 4,     y + 3,         cs, COL_DIM);
        lcd.drawFastHLine(x + 3,         y + h - 4,     cs, COL_DIM);
        lcd.drawFastVLine(x + 3,         y + h - 3 - cs, cs, COL_DIM);
        lcd.drawFastHLine(x + w - 3 - cs, y + h - 4,   cs, COL_DIM);
        lcd.drawFastVLine(x + w - 4,     y + h - 3 - cs, cs, COL_DIM);
    }

    lcd.setFont(&fonts::efontCN_16);
    lcd.setTextColor(fg, bg);
    int tw = (int)strlen(label) * 8;
    int tx = x + (w - tw) / 2;
    if (tx < x + 2) tx = x + 2;
    lcd.setCursor(tx, y + (h - 16) / 2);
    lcd.print(label);
}

/* ─────────────── Progress popup overlay ─────────────── */

/*  Centered modal dialog: title bar + horizontal progress bar + detail string.
 *  pct: 0.0 = empty bar, 1.0 = full bar.
 *  borderColor: COL_FG (normal), COL_WARN (warning), COL_ERR (error). */
template<typename LCD>
inline void drawProgressPopup(LCD& lcd, const char* title,
                               const char* detail, float pct,
                               uint16_t borderColor = COL_FG)
{
    const int bw = 268, bh = 104;
    const int bx = (W - bw) / 2, by = (H - bh) / 2;

    lcd.fillRect(bx, by, bw, bh, COL_BG);
    lcd.drawRect(bx,     by,     bw,     bh,     borderColor);
    lcd.drawRect(bx + 2, by + 2, bw - 4, bh - 4, COL_DIM);

    /* Title */
    lcd.setFont(&fonts::efontCN_16);
    lcd.setTextColor(COL_ACCENT, COL_BG);
    int tw = (int)strlen(title) * 8;
    lcd.setCursor(bx + (bw - tw) / 2, by + 10);
    lcd.print(title);

    /* Progress bar */
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;
    int barX = bx + 14, barY = by + 34, barW = bw - 28;
    lcd.drawRect(barX, barY, barW, 12, borderColor);
    int filled = (int)((barW - 4) * pct);
    if (filled > 0)
        lcd.fillRect(barX + 2, barY + 2, filled, 8, COL_FG);
    lcd.fillRect(barX + 2 + filled, barY + 2, barW - 4 - filled, 8, COL_BG);

    /* Detail text */
    if (detail && detail[0]) {
        uint16_t dCol = (pct >= 1.0f) ? COL_FG :
                        (borderColor == COL_WARN) ? COL_WARN : COL_DIM;
        lcd.setTextColor(dCol, COL_BG);
        int dw = (int)strlen(detail) * 8;
        lcd.setCursor(bx + (bw - dw) / 2, by + 60);
        lcd.print(detail);
    }
}

/* Reusable execution popup for long-running actions (batch send / scan / etc). */
template<typename LCD>
inline void drawExecPopup(LCD& lcd,
                          const char* title,
                          const char* line1,
                          const char* line2,
                          float pct,
                          int current,
                          int total,
                          uint16_t borderColor = COL_FG)
{
    const int bw = 284, bh = 136;
    const int bx = (W - bw) / 2, by = (H - bh) / 2;
    lcd.fillRect(bx, by, bw, bh, COL_BG);
    lcd.drawRect(bx, by, bw, bh, borderColor);
    lcd.drawRect(bx + 2, by + 2, bw - 4, bh - 4, COL_DIM);

    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;
    if (total < 1) total = 1;
    if (current < 0) current = 0;
    if (current > total) current = total;

    lcd.setFont(&fonts::efontCN_16);
    lcd.setTextColor(COL_ACCENT, COL_BG);
    const char* t = (title && title[0]) ? title : "EXECUTING";
    int tw = (int)strlen(t) * 8;
    lcd.setCursor(bx + (bw - tw) / 2, by + 10);
    lcd.print(t);

    if (line1 && line1[0]) {
        lcd.setTextColor(COL_FG, COL_BG);
        int w1 = (int)strlen(line1) * 8;
        lcd.setCursor(bx + (bw - w1) / 2, by + 32);
        lcd.print(line1);
    }
    if (line2 && line2[0]) {
        lcd.setTextColor(COL_DIM, COL_BG);
        int w2 = (int)strlen(line2) * 8;
        lcd.setCursor(bx + (bw - w2) / 2, by + 48);
        lcd.print(line2);
    }

    int barX = bx + 14, barY = by + 74, barW = bw - 28;
    lcd.drawRect(barX, barY, barW, 12, borderColor);
    int filled = (int)((barW - 4) * pct);
    if (filled > 0) lcd.fillRect(barX + 2, barY + 2, filled, 8, COL_FG);
    lcd.fillRect(barX + 2 + filled, barY + 2, barW - 4 - filled, 8, COL_BG);

    char pctBuf[8];
    int ipct = (int)(pct * 100.0f + 0.5f);
    snprintf(pctBuf, sizeof(pctBuf), "%d%%", ipct);
    lcd.setTextColor(COL_ACCENT, COL_BG);
    int pw = (int)strlen(pctBuf) * 8;
    lcd.setCursor(bx + (bw - pw) / 2, by + 94);
    lcd.print(pctBuf);

    char cnt[18];
    snprintf(cnt, sizeof(cnt), "%d/%d", current, total);
    lcd.setTextColor(COL_DIM, COL_BG);
    int cw = (int)strlen(cnt) * 8;
    lcd.setCursor(bx + (bw - cw) / 2, by + 112);
    lcd.print(cnt);
}

/* ── drawFooter3 ───────────────────────────────────────────────────────────
 * Evenly-distributed 3-segment footer: [Dir] left, [A] centre, [B] right.
 * Matches the "[^v]Select  [A]Enter  [B]Exit" pattern used across apps. */
template<typename LCD>
inline void drawFooter3(LCD& lcd, const char* dirHint,
                        const char* aHint, const char* bHint)
{
    lcd.fillRect(1, FTR_SEP + 1, W - 2, FTR_BOTTOM - FTR_SEP - 1, COL_BG);
    lcd.setFont(&fonts::efontCN_16);
    lcd.setTextColor(COL_FG, COL_BG);
    if (dirHint && dirHint[0]) {
        lcd.setCursor(PAD_X, FTR_TXT);
        lcd.print(dirHint);
    }
    if (aHint && aHint[0]) {
        int tw = (int)strlen(aHint) * 8;
        lcd.setCursor((W - tw) / 2, FTR_TXT);
        lcd.print(aHint);
    }
    if (bHint && bHint[0]) {
        int tw = (int)strlen(bHint) * 8;
        lcd.setCursor(W - tw - PAD_X, FTR_TXT);
        lcd.print(bHint);
    }
}

/* ── drawReadingFile ───────────────────────────────────────────────────────
 * Centered circular "READING FILE" spinner overlay (matches Momentum loader).
 *   - Outer dotted ring (static)
 *   - Animated arc sweep inside the ring (phase increments per frame)
 *   - "READING FILE" text centred
 *   - Three animated dots to the right of the ring
 *   - Optional subtitle (file path) drawn below the ring
 *   - "PLEASE WAIT..." line in the footer area (caller-supplied via footer)
 * Pre-condition: chrome + header already drawn; only paints content area. */
template<typename LCD>
inline void drawReadingFile(LCD& lcd, uint32_t phase,
                            const char* subtitle = nullptr)
{
    int cx = W / 2, cy = CON_Y0 + CON_H / 2;
    int rOuter = 62, rInner = 56;
    /* Clear content area (cheap: just the ring bbox) */
    int clrX = cx - rOuter - 4, clrY = cy - rOuter - 4;
    int clrW = (rOuter + 4) * 2, clrH = (rOuter + 4) * 2;
    lcd.fillRect(clrX, clrY, clrW, clrH, COL_BG);

    /* Dotted outer ring: 48 dots */
    const int DOTS = 48;
    for (int i = 0; i < DOTS; i++) {
        float a = (float)i * (2.0f * 3.14159265f / DOTS);
        int px = cx + (int)(cosf(a) * rOuter);
        int py = cy + (int)(sinf(a) * rOuter);
        lcd.drawPixel(px, py, COL_FG);
    }

    /* Sweeping arc — fill 90° pie slice rotating around the centre */
    int arc = (int)(phase % DOTS);
    for (int i = 0; i < 14; i++) {
        int j = (arc + i) % DOTS;
        float a = (float)j * (2.0f * 3.14159265f / DOTS);
        for (int r = rInner - 18; r < rInner; r++) {
            int px = cx + (int)(cosf(a) * r);
            int py = cy + (int)(sinf(a) * r);
            lcd.drawPixel(px, py, COL_FG);
        }
    }

    /* Centred "READING FILE" text */
    lcd.setFont(&fonts::efontCN_16);
    const char* lbl = "READING FILE";
    int tw = (int)strlen(lbl) * 8;
    lcd.setTextColor(COL_ACCENT, COL_BG);
    lcd.setCursor(cx - tw / 2, cy - 8);
    lcd.print(lbl);

    /* Animated trailing dots on the right of the ring */
    int dots = (int)(phase / 6) % 4;
    char dotBuf[5];
    for (int i = 0; i < 4; i++) dotBuf[i] = (i < dots) ? '.' : ' ';
    dotBuf[4] = '\0';
    lcd.fillRect(cx + rOuter + 6, cy - 8, 32, 16, COL_BG);
    lcd.setTextColor(COL_FG, COL_BG);
    lcd.setCursor(cx + rOuter + 6, cy - 8);
    lcd.print(dotBuf);

    /* Optional subtitle (e.g. file path) below the ring */
    if (subtitle && subtitle[0]) {
        char sbuf[34];
        int slen = (int)strlen(subtitle);
        if (slen > 33) slen = 33;
        memcpy(sbuf, subtitle, slen); sbuf[slen] = '\0';
        int subY = CON_Y1 - 20;
        lcd.fillRect(1, subY, W - 2, 18, COL_BG);
        lcd.setTextColor(COL_DIM, COL_BG);
        lcd.setCursor((W - slen * 8) / 2, subY);
        lcd.print(sbuf);
    }
}

/* ── drawFooter4 ───────────────────────────────────────────────────────────
 *   s1 @ x=PAD_X       (leftmost,  COL_FG — primary action)
 *   s2 @ x=90          (second,    COL_DIM)
 *   s3 @ x=190         (third,     COL_DIM)
 *   s4 right-aligned   (rightmost, COL_DIM)
 * Pass nullptr or "" for any segment to leave it blank. */
template<typename LCD>
inline void drawFooter4(LCD& lcd,
                        const char* s1, const char* s2,
                        const char* s3, const char* s4)
{
    lcd.fillRect(1, FTR_SEP + 1, W - 2, FTR_BOTTOM - FTR_SEP - 1, COL_BG);
    lcd.setFont(&fonts::efontCN_16);
    if (s1 && s1[0]) {
        lcd.setTextColor(COL_FG, COL_BG);
        lcd.setCursor(PAD_X, FTR_TXT);
        lcd.print(s1);
    }
    if (s2 && s2[0]) {
        lcd.setTextColor(COL_DIM, COL_BG);
        lcd.setCursor(90, FTR_TXT);
        lcd.print(s2);
    }
    if (s3 && s3[0]) {
        lcd.setTextColor(COL_DIM, COL_BG);
        lcd.setCursor(190, FTR_TXT);
        lcd.print(s3);
    }
    if (s4 && s4[0]) {
        lcd.setTextColor(COL_DIM, COL_BG);
        int tw = (int)strlen(s4) * 8;
        lcd.setCursor(W - tw - PAD_X, FTR_TXT);
        lcd.print(s4);
    }
}

}  /* namespace hp */

