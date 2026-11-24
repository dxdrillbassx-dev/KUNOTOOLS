#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <cstring>
#include <cmath>
#include <conio.h>
#include <windows.h>
#include "tools.h"

using namespace std;

// ====== ANSI truecolor ======
bool enableVT() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (!GetConsoleMode(h, &mode)) return false;
    return SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}
void ansiRGB(int r, int g, int b) { cout << "\x1b[38;2;" << r << ";" << g << ";" << b << "m"; }
void ansiReset() { cout << "\x1b[0m"; }

void gradRGB(int cellPos, int totalCells, int& r, int& g, int& b) {
    double t = totalCells > 0 ? cellPos / (double)totalCells : 0;
    if (t < 0) t = 0; if (t > 1) t = 1;
    r = (int)(0x65 + (0xed - 0x65) * t + 0.5);
    g = (int)(0x19 + (0x9d - 0x19) * t + 0.5);
    b = (int)(0xb7 + (0xfe - 0xb7) * t + 0.5);
}

void clearScreen() { system("cls"); }
void hideCursor() {
    CONSOLE_CURSOR_INFO ci = {1, FALSE};
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}
int consoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO ci;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
    return ci.srWindow.Right - ci.srWindow.Left + 1;
}
int consoleHeight() {
    CONSOLE_SCREEN_BUFFER_INFO ci;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
    return ci.srWindow.Bottom - ci.srWindow.Top + 1;
}
COORD fontCellSize() {
    CONSOLE_FONT_INFO fi;
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetCurrentConsoleFont(h, FALSE, &fi)) return {0, 0};
    return GetConsoleFontSize(h, fi.nFont);
}
void resizeWindow(int needCols) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ci;
    if (!GetConsoleScreenBufferInfo(hOut, &ci)) return;
    int curW = ci.srWindow.Right - ci.srWindow.Left + 1;
    if (curW >= needCols) return;
    SHORT bottom = ci.srWindow.Bottom;
    SMALL_RECT win = {0, 0, (SHORT)(needCols - 1), bottom};
    SetConsoleWindowInfo(hOut, TRUE, &win);
}
int cellWidth(const string& s) {
    int w = 0;
    for (unsigned char ch : s) {
        if ((ch & 0xC0) == 0x80) continue;
        w += (ch >= 0xE0) ? 2 : 1;
    }
    return w;
}

int centerWidth();
int centerHeight();
void printCentered(const string& text, int r, int g, int b);

// ====== Курсор: перед движением ОБЯЗАТЕЛЕН flush ======
COORD getCursorPos() {
    CONSOLE_SCREEN_BUFFER_INFO ci;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
    return ci.dwCursorPosition;
}
void setCursorPos(COORD c) {
    cout << flush;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

// ====== Логотип ======
vector<string> g_logo;

void loadLogo() {
    ifstream f("logo.txt");
    string s;
    while (getline(f, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back();
        string fixed;
        for (char ch : s) {
            if (ch == '\t') fixed += "    ";
            else fixed += ch;
        }
        g_logo.push_back(fixed);
    }
    if (g_logo.empty()) g_logo.push_back("KUNOTOOLS");
}

int logoWidth() {
    int m = 0;
    for (const string& l : g_logo) {
        int w = cellWidth(l);
        if (w > m) m = w;
    }
    return m;
}

// ====== Реальный размер окна ======
int g_realW = 0, g_realH = 0;
int measureWidth() {
    cout << "\x1b[18t" << flush;
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD n = 0;
    int waited = 0;
    while (waited < 400) {
        GetNumberOfConsoleInputEvents(hIn, &n);
        if (n > 0) break;
        Sleep(20); waited += 20;
    }
    if (n == 0) { g_realW = consoleWidth(); g_realH = consoleHeight(); return g_realW; }
    if (_getch() != 27) { g_realW = consoleWidth(); g_realH = consoleHeight(); return g_realW; }
    if (_getch() != '[') { g_realW = consoleWidth(); g_realH = consoleHeight(); return g_realW; }
    int c = _getch();
    if (c != '8') { g_realW = consoleWidth(); g_realH = consoleHeight(); return g_realW; }
    c = _getch();
    if (c != ';') { g_realW = consoleWidth(); g_realH = consoleHeight(); return g_realW; }
    int rows = 0;
    while ((c = _getch()) >= '0' && c <= '9') rows = rows * 10 + (c - '0');
    if (c != ';') { g_realW = consoleWidth(); g_realH = consoleHeight(); return g_realW; }
    int cols = 0;
    while ((c = _getch()) >= '0' && c <= '9') cols = cols * 10 + (c - '0');
    if (c != 't') { g_realW = consoleWidth(); g_realH = consoleHeight(); return g_realW; }
    if (cols > 40 && cols < 1000) g_realW = cols;
    else g_realW = consoleWidth();
    if (rows > 10 && rows < 200) g_realH = rows;
    else g_realH = consoleHeight();
    return g_realW;
}

int centerWidth() { return g_realW > 0 ? g_realW : consoleWidth(); }
int centerHeight() { return g_realH > 0 ? g_realH : consoleHeight(); }

// Цвета
const int C_MARKER_R = 0x8a, C_MARKER_G = 0x3c, C_MARKER_B = 0xd8;
const int C_DIM_R = 0x60, C_DIM_G = 0x60, C_DIM_B = 0x60;
const int C_OK_R = 0x4c, C_OK_G = 0xd1, C_OK_B = 0x3a;
const int C_PINK_R = 0xed, C_PINK_G = 0x9d, C_PINK_B = 0xfe;
const int C_GRAY_R = 0xaa, C_GRAY_G = 0xaa, C_GRAY_B = 0xaa;
const int C_YEL_R = 0xe8, C_YEL_G = 0xc5, C_YEL_B = 0x3a;

mt19937& rnd() {
    static mt19937 r((unsigned)GetTickCount());
    return r;
}
int ri(int lo, int hi) {
    uniform_int_distribution<int> d(lo, hi);
    return d(rnd());
}

void fillTo(int curCells, int pad, int total) {
    int w = pad + total + 6;
    int fill = w - pad - curCells;
    if (fill > 0) cout << string(fill, ' ');
}

// ====== Обычный чистый логотип ======
void printLogo() {
    int total = logoWidth();
    int pad = (centerWidth() - total) / 2;
    if (pad < 0) pad = 0;
    int lastR = -1, lastG = -1, lastB = -1;
    for (const string& line : g_logo) {
        cout << string(pad, ' ');
        int cellPos = 0;
        for (size_t i = 0; i < line.size(); ) {
            unsigned char ch = line[i];
            int charBytes = (ch < 0x80) ? 1 : ((ch & 0xF0) == 0xE0 ? 3 : 2);
            int cells = (ch >= 0xE0) ? 2 : 1;
            if (ch != 0x20) {
                int rr, gg, bb;
                gradRGB(cellPos, total, rr, gg, bb);
                if (rr != lastR || gg != lastG || bb != lastB) {
                    ansiRGB(rr, gg, bb);
                    lastR = rr; lastG = gg; lastB = bb;
                }
                cout.write(line.data() + i, charBytes);
            } else {
                cout << " ";
            }
            cellPos += cells;
            i += charBytes;
        }
        cout << "\n";
    }
    ansiReset();
}

// ====== Краш-кадры ======
void printLogoFX(double frac, int hl) {
    int total = logoWidth();
    int pad = (centerWidth() - total) / 2;
    if (pad < 0) pad = 0;
    int lastR = -1, lastG = -1, lastB = -1;
    int li = 0;
    for (const string& line : g_logo) {
        cout << string(pad, ' ');
        int cellPos = 0;
        for (size_t i = 0; i < line.size(); ) {
            unsigned char ch = line[i];
            int charBytes = (ch < 0x80) ? 1 : ((ch & 0xF0) == 0xE0 ? 3 : 2);
            int cells = (ch >= 0xE0) ? 2 : 1;
            if (ch != 0x20) {
                int h = (li * 131 + (int)i * 37) % 100;
                bool show = (h < (int)(frac * 100.0));
                if (!show) {
                    int d = (hl < 0) ? 999 : abs(cellPos - hl);
                    int p = ri(0, 99);
                    int r, g, b;
                    const char* blk;
                    if (d <= 3)      { r = 255; g = 255; b = 255; blk = "\xE2\x96\x88"; }
                    else if (d <= 9) { r = 0xb0; g = 0x70; b = 0xe8; blk = "\xE2\x96\x93"; }
                    else if (p < 8)  { r = 0xff; g = 0x4d; b = 0xe8; blk = "\xE2\x96\x88"; }
                    else             { r = 0x45; g = 0x28; b = 0x5e; blk = (p < 50 ? "\xE2\x96\x91" : "\xE2\x96\x92"); }
                    if (r != lastR || g != lastG || b != lastB) { ansiRGB(r, g, b); lastR = r; lastG = g; lastB = b; }
                    cout << blk;
                } else {
                    int rr, gg, bb;
                    gradRGB(cellPos, total, rr, gg, bb);
                    if (rr != lastR || gg != lastG || bb != lastB) {
                        ansiRGB(rr, gg, bb);
                        lastR = rr; lastG = gg; lastB = bb;
                    }
                    cout.write(line.data() + i, charBytes);
                }
            } else {
                cout << " ";
            }
            cellPos += cells;
            i += charBytes;
        }
        fillTo(cellPos, pad, total);
        cout << "\n";
        li++;
    }
    ansiReset();
}

// ====== Глитч-кадр ======
void printLogoGlitch(int hot1, int dx1, int hot2, int dx2) {
    int total = logoWidth();
    int pad = (centerWidth() - total) / 2;
    if (pad < 0) pad = 0;
    int startY = getCursorPos().Y;
    int lastR = -1, lastG = -1, lastB = -1;

    for (int li = 0; li < (int)g_logo.size(); li++) {
        int y = startY + li;

        if (li != hot1 && li != hot2) {
            cout << string(pad, ' ');
            int cellPos = 0;
            const string& line = g_logo[li];
            for (size_t i = 0; i < line.size(); ) {
                unsigned char ch = line[i];
                int charBytes = (ch < 0x80) ? 1 : ((ch & 0xF0) == 0xE0 ? 3 : 2);
                int cells = (ch >= 0xE0) ? 2 : 1;
                if (ch != 0x20) {
                    int rr, gg, bb;
                    gradRGB(cellPos, total, rr, gg, bb);
                    if (rr != lastR || gg != lastG || bb != lastB) {
                        ansiRGB(rr, gg, bb);
                        lastR = rr; lastG = gg; lastB = bb;
                    }
                    cout.write(line.data() + i, charBytes);
                } else {
                    cout << " ";
                }
                cellPos += cells;
                i += charBytes;
            }
            fillTo(cellPos, pad, total);
            cout << "\n";
            continue;
        }

        int dx = (li == hot1) ? dx1 : dx2;
        int p = pad + dx;
        if (p < 0) p = 0;
        int len = cellWidth(g_logo[li]);

        setCursorPos({0, (SHORT)y});
        ansiRGB(0xed, 0x9d, 0xfe);
        cout << string(pad, ' ') << g_logo[li];

        setCursorPos({(SHORT)p, (SHORT)y});
        ansiRGB(255, 255, 255);
        cout << g_logo[li];
        ansiReset();

        int right = pad + len;
        if (p + len > right) right = p + len;
        setCursorPos({(SHORT)right, (SHORT)y});
        int w = pad + total + 6;
        if (right < w) cout << string(w - right, ' ');
        setCursorPos({0, (SHORT)(y + 1)});
    }
}

// ====== Анимация логотипа ======
void animateLogo() {
    int total = logoWidth();
    int hlSpan = total + 24;

    clearScreen();
    cout << "\n\n" << flush;
    COORD home = getCursorPos();

    for (int f = 0; f < 22; f++) {
        double frac = (f + 1) / 22.0;
        int hl = (int)((f * 11) % hlSpan) - 12;
        setCursorPos(home);
        printLogoFX(frac, hl);
        cout << flush;
        Sleep(35);
    }
    setCursorPos(home);
    printLogo();
    cout << flush;
    Sleep(150);

    for (int f = 0; f < 6; f++) {
        int lines = (int)g_logo.size();
        int hot1 = ri(0, lines - 1);
        int dx1 = ri(2, 5) * (ri(0, 1) ? 1 : -1);
        int hot2 = -1, dx2 = 0;
        if (lines > 1 && ri(0, 1)) {
            hot2 = ri(0, lines - 1);
            if (hot2 == hot1) hot2 = (hot2 + 1) % lines;
            dx2 = -dx1;
        }
        setCursorPos(home);
        printLogoGlitch(hot1, dx1, hot2, dx2);
        cout << flush;
        Sleep(55);
    }
}

// ============================================================
//  ДОЖДЬ ИЗ СИМВОЛОВ
// ============================================================
const char SYMS[] = "!<>-_\\/[]{}=+*^?#$%&@0123456789ABCDEFXYZ";
const int NSYM = (int)sizeof(SYMS) - 2;

void rainScreen(int ms) {
    int w = centerWidth() - 1;
    int h = centerHeight() - 1;
    if (w > 219) w = 219;
    if (h > 59) h = 59;

    const int fadeMs = 900;

    clearScreen();

    vector<int> y(w), len(w), spd(w);
    for (int c = 0; c < w; c++) {
        y[c] = -ri(0, h);
        len[c] = ri(6, 14);
        spd[c] = ri(1, 2);
    }

    int elapsed = 0;
    while (elapsed < ms) {
        double a = 1.0;
        if (ms - elapsed < fadeMs) a = (double)(ms - elapsed) / fadeMs;

        setCursorPos({0, 0});
        for (int r = 0; r < h; r++) {
            string line;
            int lastR = -1, lastG = -1, lastB = -1;
            for (int c = 0; c < w; c++) {
                int d = y[c] - r;
                if (d >= 0 && d < len[c]) {
                    int rr, gg, bb;
                    if (d == 0) { rr = (int)(255 * a); gg = (int)(255 * a); bb = (int)(255 * a); }
                    else {
                        double t = 1.0 - (double)d / len[c];
                        gradRGB((int)(t * 100.0), 100, rr, gg, bb);
                        rr = (int)(rr * a); gg = (int)(gg * a); bb = (int)(bb * a);
                    }
                    if (rr != lastR || gg != lastG || bb != lastB) {
                        line += "\x1b[38;2;";
                        line += to_string(rr); line += ";";
                        line += to_string(gg); line += ";";
                        line += to_string(bb); line += "m";
                        lastR = rr; lastG = gg; lastB = bb;
                    }
                    line += SYMS[ri(0, NSYM)];
                } else {
                    line += ' ';
                }
            }
            cout << line << "\n";
        }
        ansiReset();
        cout << flush;
        Sleep(35);
        elapsed += 35;
        for (int c = 0; c < w; c++) {
            y[c] += spd[c];
            if (y[c] - len[c] > h) {
                y[c] = -ri(0, h / 2);
                len[c] = ri(6, 14);
                spd[c] = ri(1, 2);
            }
        }
    }
}

// ============================================================
//  ГЛАЗ СЛЕЖЕНИЯ
// ============================================================
void eyeScreen(int ms) {
    int scrW = centerWidth() - 1;
    int scrH = centerHeight() - 1;

    COORD fs = fontCellSize();
    double cellAsp = (fs.X > 0 && fs.Y > 0) ? (double)fs.Y / fs.X : 2.0; // пропорции клетки шрифта

    int H = scrH - 8;          if (H > 22) H = 22; if (H < 14) H = 14;
    int W = (int)(H * 1.2 * cellAsp); // миндаль ~2.4:1 ВИЗУАЛЬНО, под любой шрифт
    int maxW = (scrW - 2) / 2; if (W > maxW) W = maxW; if (W > 90) W = 90; if (W < 34) W = 34;
    int cx = W / 2, cy = H / 2;
    double rx = W / 2.0 - 1.0, ry = H / 2.0 - 1.0;
    int topY = (scrH - H) / 2 - 1; if (topY < 1) topY = 1;
    int capY = topY + H + 1;
    const int FADE = 900;
    const double NN = 2.5;   // 2.2 мягче, 2.8 острее

    static const double GX[5] = { 0.60, -0.60, 0.0,  0.60, -0.60 };
    static const double GY[5] = { 0.50,  0.50, 0.0, -0.50, -0.50 };
    const int segMs = 750;
    double curx = 0, cury = 0;

    clearScreen();
    int elapsed = 0, frame = 0;
    while (elapsed < ms) {
        double a = (elapsed < FADE) ? (double)elapsed / FADE : 1.0;

        int seg = (elapsed / segMs) % 5;
        double prog = (double)(elapsed % segMs) / segMs;
        curx += (GX[seg] - curx) * 0.10;
        cury += (GY[seg] - cury) * 0.10;
        double blink = 1.0;
        if (seg == 2) blink = (prog < 0.5) ? (1.0 - prog * 2.0) : ((prog - 0.5) * 2.0);

        int icx = cx + (int)(curx * rx * 0.38);
        int icy = cy + (int)(cury * ry * 0.66);

        setCursorPos({0, (SHORT)topY});
        for (int y = 0; y < H; y++) {
            string line;
            line = string(30, ' ');   // 30 обычных пробелов перед глазом
            int lastR = -1, lastG = -1, lastB = -1;
            for (int x = 0; x < W; x++) {
                double u = (x - cx) / rx;
                double v = (y - cy) / ry;
                double f = pow(fabs(u), NN) + pow(fabs(v), NN);
                double gu = (x - icx) / (rx * 0.38);
                double gv = (y - icy) / (ry * 0.66);
                double g = gu * gu + gv * gv;

                bool pupil = false;
                if (blink > 0.05) {
                    double pu = gu / (0.38 * blink);
                    double pv = gv / (0.20 * blink);
                    pupil = (pu * pu + pv * pv <= 1.0);
                }

                int r, gg, b;
                const char* ch = nullptr;
                if (f > 1.0) {
                    // вне глаза
                } else if (pupil) {
                    ch = "\xE2\x96\x88"; r = 255; gg = 255; b = 255;
                } else if (g <= 1.0) {
                    double t = g;
                    r  = (int)(0x2a + (0xb0 - 0x2a) * t);
                    gg = (int)(0x1a + (0x74 - 0x1a) * t);
                    b  = (int)(0x40 + (0xf0 - 0x40) * t);
                    if (t < 0.35)      ch = "\xE2\x96\x91";
                    else if (t < 0.70) ch = "\xE2\x96\x92";
                    else               ch = "\xE2\x96\x93";
                } else if (f >= 0.90) {
                    gradRGB(x * 100 / W, 100, r, gg, b);
                    r = r + (0xc8 - r) * 2 / 3; gg = gg + (0xa4 - gg) * 2 / 3; b = b + (0xf8 - b) * 2 / 3;
                    ch = "\xE2\x96\x93";
                } else if (f >= 0.80) {
                    r = 0x86; gg = 0x58; b = 0xd0;
                    ch = "\xE2\x96\x93";
                } else {
                    int p = (x * 31 + y * 17 + frame * 7) % 100;
                    if (p < 78) { ch = "\xE2\x96\x91"; r = 0x40; gg = 0x28; b = 0x5e; }
                    else        { ch = "\xE2\x96\x92"; r = 0x5a; gg = 0x3a; b = 0x80; }
                }

                if (!ch) { line += ' '; continue; }
                r = (int)(r * a); gg = (int)(gg * a); b = (int)(b * a);
                if (r != lastR || gg != lastG || b != lastB) {
                    line += "\x1b[38;2;";
                    line += to_string(r); line += ";";
                    line += to_string(gg); line += ";";
                    line += to_string(b); line += "m";
                    lastR = r; lastG = gg; lastB = b;
                }
                line += ch;
            }
            cout << line << "\n";
        }

        setCursorPos({0, (SHORT)capY});
        printCentered("* SCANNING ENVIRONMENT *", (int)(C_PINK_R * a), (int)(C_PINK_G * a), (int)(C_PINK_B * a));
        cout << flush;
        Sleep(70);
        elapsed += 70;
        frame++;
    }
}

// ====== Печать текста ======
void typeCentered(const string& text, int r, int g, int b, int delayMs) {
    int pad = (centerWidth() - (int)text.size()) / 2;
    if (pad < 0) pad = 0;
    cout << string(pad, ' ');
    ansiRGB(r, g, b);
    for (size_t i = 0; i < text.size(); i++) {
        cout << text[i] << flush;
        Sleep(delayMs);
    }
    cout << "\n";
    ansiReset();
}

void printCentered(const string& text, int r, int g, int b) {
    int pad = (centerWidth() - (int)text.size()) / 2;
    if (pad < 0) pad = 0;
    ansiRGB(r, g, b);
    cout << string(pad, ' ') << text << "\n";
    ansiReset();
}

// ====== Пункт загрузки ======
void loadLine(const string& text) {
    int dots = 45 - (int)text.size();
    if (dots < 3) dots = 3;
    int gap = dots + 4;
    int lineW = 2 + (int)text.size() + gap + 2;
    int pad = (centerWidth() - lineW) / 2;
    if (pad < 0) pad = 0;
    cout << string(pad, ' ');
    ansiRGB(C_MARKER_R, C_MARKER_G, C_MARKER_B);
    cout << "◼ ";
    ansiRGB(0xd8, 0xb4, 0xf5);
    cout << text;
    cout << string(gap, ' ');
    ansiRGB(C_OK_R, C_OK_G, C_OK_B);
    cout << "OK\n";
    ansiReset();
    Sleep(140);
}

void modulesBar() {
    const int BAR = 34;
    int lineW = 8 + BAR + 13;
    int pad = (centerWidth() - lineW) / 2;
    if (pad < 0) pad = 0;
    cout << string(pad, ' ');
    ansiRGB(0xd8, 0xb4, 0xf5);
    cout << "MODULES ";
    for (int i = 0; i < BAR; i++) {
        int rr, gg, bb;
        gradRGB(i, BAR, rr, gg, bb);
        ansiRGB(rr, gg, bb);
        cout << "█";
    }
    ansiRGB(255, 255, 255);
    cout << " 100/100 ";
    ansiRGB(C_OK_R, C_OK_G, C_OK_B);
    cout << "READY\n";
    ansiReset();
}

void bootScreen() {
    animateLogo();

    clearScreen();
    cout << "\n\n";
    printLogo();
    cout << "\n";
    typeCentered("* KUNOTOOLS MULTI-TOOL FRAMEWORK - NATIVE EDITION *", C_PINK_R, C_PINK_G, C_PINK_B, 12);
    typeCentered("v1.0  -  3 native modules  -  powered by Kuno", C_GRAY_R, C_GRAY_G, C_GRAY_B, 8);
    cout << "\n";

    loadLine("Kernel initialization");
    loadLine("Network engine loading");
    loadLine("DNS resolvers sync");
    loadLine("Mounting OSINT modules");
    loadLine("Scanners arming");
    loadLine("Holographic interface calibration");

    cout << "\n";
    modulesBar();
    cout << "\n";
    printCentered("! For legal use only: scan targets you are authorized to test.", C_YEL_R, C_YEL_G, C_YEL_B);
    cout << "\n";
    printCentered("* PRESS ENTER TO ACCESS THE PANEL *", C_PINK_R, C_PINK_G, C_PINK_B);
    ansiReset();
    cin.get();
}

int showMenu() {
    vector<string> items = {
        "System info",
        "Network scan (demo)",
        "Hash generator",
        "Exit"
    };
    int sel = 0;
    while (true) {
        clearScreen();
        cout << "\n\n";
        printLogo();
        cout << "\n";
        printCentered("[ MAIN MENU ]", C_GRAY_R, C_GRAY_G, C_GRAY_B);
        cout << "\n";

        for (int i = 0; i < (int)items.size(); i++) {
            int w = (int)items[i].size() + 4;
            int pad = (centerWidth() - w) / 2;
            if (pad < 0) pad = 0;
            if (i == sel) {
                cout << string(pad, ' ') << "\x1b[48;2;120;60;180m\x1b[38;2;0;0;0m";
                cout << "  > " << items[i] << " <\n";
                ansiReset();
            } else {
                ansiRGB(0xaa, 0xaa, 0xaa);
                cout << string(pad, ' ') << "  " << items[i] << "  \n";
            }
        }
        ansiReset();
        cout << "\n";
        printCentered("Up/Down arrows + ENTER", C_GRAY_R, C_GRAY_G, C_GRAY_B);

        int key = _getch();
        if (key == 224) {
            key = _getch();
            if (key == 72) sel = (sel - 1 + (int)items.size()) % (int)items.size();
            if (key == 80) sel = (sel + 1) % (int)items.size();
        } else if (key == 13) {
            return sel;
        }
    }
}

int main() {
    SetConsoleOutputCP(65001);
    enableVT();
    hideCursor();
    loadLogo();
    resizeWindow(logoWidth() + 20);
    measureWidth();

    rainScreen(3000);
    eyeScreen(3000);
    bootScreen();

    while (true) {
        int choice = showMenu();
        clearScreen();
        switch (choice) {
            case 0: modSysInfo(); break;
            case 1: modNetScan(); break;
            case 2: modHashGen(); break;
            case 3:
                ansiRGB(C_PINK_R, C_PINK_G, C_PINK_B);
                cout << "\n  KUNOTOOLS shutting down. Goodbye!\n\n";
                ansiReset();
                return 0;
        }
        ansiRGB(C_GRAY_R, C_GRAY_G, C_GRAY_B);
        cout << "\n\n  Press any key to return to menu...";
        _getch();
    }
}