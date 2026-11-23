#include <iostream>
#include <string>
#include <cmath>
#include <windows.h>

using namespace std;

void modSysInfo() {
    cout << "\n  [ INFO SYSTEME ]\n\n";
    cout << "  Processeurs : " << GetSystemMetrics(43) + 1 << endl;
    MEMORYSTATUSEX mem{sizeof(mem)};
    GlobalMemoryStatusEx(&mem);
    cout << "  RAM totale  : " << mem.ullTotalPhys / (1024*1024*1024) << " Go\n";
    cout << "  RAM libre   : " << mem.ullAvailPhys / (1024*1024*1024) << " Go\n";
    char pc[256]; DWORD n = 256;
    GetComputerNameA(pc, &n);
    cout << "  PC          : " << pc << "\n";
}

void modNetScan() {
    cout << "\n  [ NETWORK SCAN - DEMO ]\n\n";
    for (int i = 1; i <= 10; i++) {
        cout << "  Scan 192.168.1." << i << " ... ";
        Sleep(150);
        if (i % 3 == 0) cout << "ACTIF";
        else cout << "offline";
        cout << endl;
    }
}

void modHashGen() {
    string s;
    cout << "\n  [ HASH GENERATEUR ]\n\n  Texte : ";
    getline(cin, s); getline(cin, s);
    unsigned long h = 5381;
    for (char c : s) h = h * 33 + c;
    cout << "  DJB2  : " << hex << h << dec << "\n";
}