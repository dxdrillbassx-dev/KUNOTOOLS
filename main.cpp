#include <iostream>
#include <string>
#include <vector>
#include <conio.h>
#include <windows.h>
#include "tools.h"

using namespace std;

// Цвета
void setColor(int text, int bg = 0) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), bg * 16 + text);
}
void purple()      { setColor(13); }              // фиолетовый
void purpleBright(){ setColor(14); }              // яркий (подсветка)
void reset()       { setColor(7); }               // серый по умолчанию

void clearScreen() {
    system("cls");
    setColor(0, 0); // черный фон
}

// Убрать мигающий курсор
void hideCursor() {
    CONSOLE_CURSOR_INFO ci = {100, FALSE};
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

void printLogo() {
    setColor(13);
    cout << R"(
  _  __                    ____  _                _
 | |/ /                   / __ \| |              | |
 | ' / _   _ _ __   ___  | |  | | |_   _ __ _  __| |
 |  < | | | | '_ \ / _ \ | |  | | __| | '__| |/ _` |
 | . \| |_| | | | | (_) || |__| | |_ _| |  | | (_| |
 |_|\_\\___/|_| |_|\___(_)_____/ \__(_) |_|  |_|\__,_|
)" << endl;
    reset();
    cout << "        * KUNOTOOLS MULTI-FRAMEWORK - v1.0 *\n\n";
}

// Фейковая загрузка как на скрине
void fakeLoading() {
    const char* lines[] = {
        "Initialisation du noyau KUNO ......... OK",
        "Chargement des modules internes ...... OK",
        "Analyse du systeme ................... OK",
        "Montage des peripheriques OSINT ...... OK",
        "Liaison telegraphique ................ OK",
    };
    setColor(8);
    for (auto l : lines) { cout << "  " << l << endl; Sleep(250); }
    cout << "\n";
    setColor(13);
    cout << "  KERNEL: [";
    for (int i = 0; i < 30; i++) {
        cout << "#"; Sleep(40);
    }
    setColor(10);
    cout << "] 100/100 OK\n\n";
    reset();
    setColor(8);
    cout << "  Appuyez sur ENTRER pour continuer...";
    cin.get();
}

// Главное меню
int showMenu() {
    vector<string> items = {
        "Module 1 - Info systeme",
        "Module 2 - Network scan (demo)",
        "Module 3 - Hash generateur",
        "Quitter"
    };
    int sel = 0;
    while (true) {
        clearScreen();
        printLogo();
        setColor(8);
        cout << "  [ MENU PRINCIPAL ]\n\n";
        for (int i = 0; i < (int)items.size(); i++) {
            if (i == sel) {
                setColor(0, 13); // фиолетовый фон, чёрный текст
                cout << "  > " << items[i] << " <" << endl;
                setColor(0, 0);
            } else {
                setColor(8);
                cout << "    " << items[i] << endl;
            }
        }
        reset();
        cout << "\n  Fleches haut/bas + ENTRER\n";

        int key = _getch();
        if (key == 224) { // стрелка
            key = _getch();
            if (key == 72) sel = (sel - 1 + items.size()) % items.size();
            if (key == 80) sel = (sel + 1) % items.size();
        } else if (key == 13) {
            return sel;
        }
    }
}

int main() {
    setColor(0, 0);
    hideCursor();
    clearScreen();
    fakeLoading();

    while (true) {
        int choice = showMenu();
        clearScreen();
        switch (choice) {
            case 0: modSysInfo(); break;
            case 1: modNetScan(); break;
            case 2: modHashGen(); break;
            case 3:
                setColor(13);
                cout << "\n  Arret de KUNOTOOLS. A bientot!\n\n";
                reset();
                return 0;
        }
        setColor(8);
        cout << "\n\n  Appuyez sur une touche pour retourner au menu...";
        _getch();
    }
}