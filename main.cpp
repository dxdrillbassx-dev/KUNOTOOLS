#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <conio.h>
#include <windows.h>
#include "tools.h"

using namespace std;

// ====== ANSI truecolor режим ======
bool enableVT() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (!GetConsoleMode(h, &mode)) return false;
    return SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

// Полный 24-битный цвет
void ansiRGB(int r, int g, int b) {
    cout << "\x1b[38;2;" << r << ";" << g << ";" << b << "m";
}
void ansiReset() { cout << "\x1b[0m"; }

// Градиент слева направо: #6519b7 -> #ed9dfe
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
// Пытаемся расширить окно до нужной ширины (только окно, буфер не трогаем)
void resizeWindow(int needCols) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ci;
    if (!GetConsoleScreenBufferInfo(hOut, &ci)) return;
    int curW = ci.srWindow.Right - ci.srWindow.Left + 1;
    if (curW >= needCols) return;
    SHORT bottom = ci.srWindow.Bottom;
    SMALL_RECT win = {0, 0, (SHORT)(needCols - 1), bottom};
    SetConsoleWindowInfo(hOut, TRUE, &win); // если не вышло — просто игнорим
}


// Ширина строки в ячейках консоли (UTF-8: 3-байтовые символы = 2 ячейки)
int cellWidth(const string& s) {
    int w = 0;
    for (unsigned char ch : s) {
        if ((ch & 0xC0) == 0x80) continue;
        w += (ch >= 0xE0) ? 2 : 1;
    }
    return w;
}

int centerWidth();  // forward declaration

// ====== Логотип из внешнего файла logo.txt ======
vector<string> g_logo;

void loadLogo() {
    ifstream f("logo.txt");
    string s;
    while (getline(f, s)) {
        if (!s.empty() && s.back() == '\r') s.pop_back(); // Windows-концы строк
        g_logo.push_back(s);
    }
    if (g_logo.empty()) g_logo.push_back("KUNOTOOLS"); // запасной вариант
}

int logoWidth() {
    int m = 0;
    for (const string& l : g_logo) {
        int w = cellWidth(l);
        if (w > m) m = w;
    }
    return m;
}

// ====== Измерение РЕАЛЬНОЙ ширины окна ======
// ====== Измерение РЕАЛЬНОЙ ширины окна ======
// Спрашиваем у терминала размер через ANSI DECRQTP ([18t).
// Windows Terminal отвечает: [8;rows;cols t
// Классическая консоль может не ответить — fallback на API.
int g_realW = 0;
int measureWidth() {
    cout << "[18t" << flush;
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD n = 0;
    int waited = 0;
    while (waited < 400) {
        GetNumberOfConsoleInputEvents(hIn, &n);
        if (n > 0) break;
        Sleep(20); waited += 20;
    }
    if (n == 0) { g_realW = consoleWidth(); return g_realW; }

    if (_getch() != 27) { g_realW = consoleWidth(); return g_realW; }
    if (_getch() != '[') { g_realW = consoleWidth(); return g_realW; }

    int c = _getch();
    if (c != '8') { g_realW = consoleWidth(); return g_realW; }

    c = _getch();
    if (c != ';') { g_realW = consoleWidth(); return g_realW; }

    int rows = 0;
    while ((c = _getch()) >= '0' && c <= '9') rows = rows * 10 + (c - '0');
    if (c != ';') { g_realW = consoleWidth(); return g_realW; }

    int cols = 0;
    while ((c = _getch()) >= '0' && c <= '9') cols = cols * 10 + (c - '0');
    if (c != 't') { g_realW = consoleWidth(); return g_realW; }

    if (cols > 40 && cols < 1000) g_realW = cols;
    else g_realW = consoleWidth();
    return g_realW;
}

int centerWidth() { return g_realW > 0 ? g_realW : consoleWidth(); }

// Цвета интерфейса (RGB)
const int C_MARKER_R = 0x8a, C_MARKER_G = 0x3c, C_MARKER_B = 0xd8;
const int C_DIM_R = 0x60, C_DIM_G = 0x60, C_DIM_B = 0x60;
const int C_OK_R = 0x4c, C_OK_G = 0xd1, C_OK_B = 0x3a;
const int C_PINK_R = 0xed, C_PINK_G = 0x9d, C_PINK_B = 0xfe;
const int C_GRAY_R = 0xaa, C_GRAY_G = 0xaa, C_GRAY_B = 0xaa;
const int C_YEL_R = 0xe8, C_YEL_G = 0xc5, C_YEL_B = 0x3a;

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

void printCentered(const string& text, int r, int g, int b) {
    int pad = (centerWidth() - (int)text.size()) / 2;
    if (pad < 0) pad = 0;
    ansiRGB(r, g, b);
    cout << string(pad, ' ') << text << "\n";
    ansiReset();
}

void loadLine(const string& text) {
    int dots = 45 - (int)text.size();
    if (dots < 3) dots = 3;
    int lineW = 1 + 1 + (int)text.size() + 1 + dots + 1 + 2; // "◼ text .... OK"
    int pad = (centerWidth() - lineW) / 2;
    if (pad < 0) pad = 0;
    cout << string(pad, ' ');
    ansiRGB(C_MARKER_R, C_MARKER_G, C_MARKER_B);
    cout << "◼ ";
    ansiRGB(0xd8, 0xb4, 0xf5);
    cout << text;
    ansiRGB(C_DIM_R, C_DIM_G, C_DIM_B);
    cout << " " << string(dots, '.') << " ";
    ansiRGB(C_OK_R, C_OK_G, C_OK_B);
    cout << "OK\n";
    ansiReset();
}

void modulesBar() {
    const int BAR = 34;
    int lineW = 8 + BAR + 13; // "MODULES " + шкала + " 100/100 READY"
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
    cout << " 100/100 READY\n";
    ansiReset();
}

void bootScreen() {
    clearScreen();
    cout << "\n\n";
    printLogo();
    cout << "\n";
    printCentered("* KUNOTOOLS MULTI-TOOL FRAMEWORK - NATIVE EDITION *", C_PINK_R, C_PINK_G, C_PINK_B);
    printCentered("v1.0  -  3 native modules  -  powered by Kuno", C_GRAY_R, C_GRAY_G, C_GRAY_B);
    cout << "\n";

    loadLine("Kernel initialization");
    Sleep(200);
    loadLine("Network engine loading");
    Sleep(200);
    loadLine("DNS resolvers sync");
    Sleep(200);
    loadLine("Mounting OSINT modules");
    Sleep(200);
    loadLine("Scanners arming");
    Sleep(200);
    loadLine("Holographic interface calibration");
    Sleep(200);

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
    loadLogo();       // арт из logo.txt
    resizeWindow(logoWidth() + 20); // расширить окно, если арт не влезает
    measureWidth();   // узнаём реальную ширину окна
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
