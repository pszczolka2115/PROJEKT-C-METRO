#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <wchar.h>

#define MAX_STATIONS 300
#define MAX_EDGES 50
#define MAX_NAME_LEN 100
#define INF INT_MAX

typedef struct {
    int to;
    int weight;
} Edge;

typedef struct {
    wchar_t name[MAX_NAME_LEN];
    Edge adj[MAX_EDGES];
    int adj_count;
} Station;

typedef struct {
    Station stations[MAX_STATIONS];
    int count;
} Graph;

Graph g;
HWND hComboStart, hComboEnd, hResult;

int get_or_add_station(Graph* g, const wchar_t* name) {
    for (int i = 0; i < g->count; i++)
        if (wcscmp(g->stations[i].name, name) == 0) return i;
    if (g->count >= MAX_STATIONS) return -1;
    int idx = g->count++;
    wcsncpy(g->stations[idx].name, name, MAX_NAME_LEN - 1);
    g->stations[idx].adj_count = 0;
    return idx;
}

void add_edge(Graph* g, int from, int to, int weight) {
    Station* s = &g->stations[from];
    for (int i = 0; i < s->adj_count; i++) {
        if (s->adj[i].to == to) {
            if (weight < s->adj[i].weight) s->adj[i].weight = weight;
            return;
        }
    }
    if (s->adj_count < MAX_EDGES) {
        s->adj[s->adj_count].to = to;
        s->adj[s->adj_count].weight = weight;
        s->adj_count++;
    }
}

void load_graph(Graph* g, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return;
    g->count = 0;
    char line[512];
    wchar_t prev_name[MAX_NAME_LEN] = L"";
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0') { prev_name[0] = L'\0'; continue; }
        
        char name_a[MAX_NAME_LEN];
        int w;
        if (sscanf(line, "%99[^;];%d", name_a, &w) == 2) {
            wchar_t name_w[MAX_NAME_LEN];
            MultiByteToWideChar(CP_UTF8, 0, name_a, -1, name_w, MAX_NAME_LEN);
            int ib = get_or_add_station(g, name_w);
            if (prev_name[0] != L'\0') {
                int ia = get_or_add_station(g, prev_name);
                add_edge(g, ia, ib, w);
                add_edge(g, ib, ia, w);
            }
            wcscpy(prev_name, name_w);
        }
    }
    fclose(f);
}

void dijkstra(Graph* g, int src, int* dist, int* prev) {
    int visited[MAX_STATIONS] = { 0 };
    for (int i = 0; i < g->count; i++) { dist[i] = INF; prev[i] = -1; }
    dist[src] = 0;
    for (int i = 0; i < g->count; i++) {
        int u = -1;
        for (int j = 0; j < g->count; j++)
            if (!visited[j] && (u == -1 || dist[j] < dist[u])) u = j;
        if (u == -1 || dist[u] == INF) break;
        visited[u] = 1;
        for (int j = 0; j < g->stations[u].adj_count; j++) {
            int v = g->stations[u].adj[j].to;
            int w = g->stations[u].adj[j].weight;
            if (dist[u] + w < dist[v]) { dist[v] = dist[u] + w; prev[v] = u; }
        }
    }
}

void find_path_gui() {
    wchar_t start_name[MAX_NAME_LEN], end_name[MAX_NAME_LEN];
    GetWindowTextW(hComboStart, start_name, MAX_NAME_LEN);
    GetWindowTextW(hComboEnd, end_name, MAX_NAME_LEN);

    int src = -1, dst = -1;
    for (int i = 0; i < g.count; i++) {
        if (wcscmp(g.stations[i].name, start_name) == 0) src = i;
        if (wcscmp(g.stations[i].name, end_name) == 0) dst = i;
    }

    if (src == -1 || dst == -1) {
        SetWindowTextW(hResult, L"Blad: Wybierz przystanki z listy.");
        return;
    }

    int dist[MAX_STATIONS], prev[MAX_STATIONS];
    dijkstra(&g, src, dist, prev);

    if (dist[dst] == INF) {
        SetWindowTextW(hResult, L"Nie znaleziono polaczenia.");
    } else {
        wchar_t buffer[4096];
        swprintf(buffer, 4096, L"Czas przejazdu: %d min\n\nTrasa:\n", dist[dst]);
        
        int path[MAX_STATIONS], p_count = 0, curr = dst;
        while (curr != -1) { path[p_count++] = curr; curr = prev[curr]; }
        for (int i = p_count - 1; i >= 0; i--) {
            wcscat(buffer, g.stations[path[i]].name);
            if (i > 0) wcscat(buffer, L" -> ");
        }
        SetWindowTextW(hResult, buffer);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            
            HWND hL1 = CreateWindowW(L"static", L"Przystanek poczatkowy:", WS_VISIBLE | WS_CHILD, 10, 10, 360, 20, hwnd, NULL, NULL, NULL);
            hComboStart = CreateWindowW(L"combobox", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 10, 30, 360, 300, hwnd, NULL, NULL, NULL);
            
            HWND hL2 = CreateWindowW(L"static", L"Przystanek koncowy:", WS_VISIBLE | WS_CHILD, 10, 65, 360, 20, hwnd, NULL, NULL, NULL);
            hComboEnd = CreateWindowW(L"combobox", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, 10, 85, 360, 300, hwnd, NULL, NULL, NULL);
            
            SendMessageW(hL1, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessageW(hL2, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessageW(hComboStart, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessageW(hComboEnd, WM_SETFONT, (WPARAM)hFont, TRUE);

            FILE* f = fopen("przystanki.txt", "r");
            if (f) {
                char p_ansi[MAX_NAME_LEN];
                while (fgets(p_ansi, MAX_NAME_LEN, f)) {
                    p_ansi[strcspn(p_ansi, "\r\n")] = '\0';
                    if (p_ansi[0] == '\0') continue;
                    wchar_t p_w[MAX_NAME_LEN];
                    MultiByteToWideChar(CP_UTF8, 0, p_ansi, -1, p_w, MAX_NAME_LEN);
                    SendMessageW(hComboStart, CB_ADDSTRING, 0, (LPARAM)p_w);
                    SendMessageW(hComboEnd, CB_ADDSTRING, 0, (LPARAM)p_w);
                }
                fclose(f);
            }

            HWND hBtn = CreateWindowW(L"button", L"OBLICZ TRASE", WS_VISIBLE | WS_CHILD, 10, 125, 360, 40, hwnd, (HMENU)1, NULL, NULL);
            SendMessageW(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);

            hResult = CreateWindowW(L"edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 10, 175, 360, 170, hwnd, NULL, NULL, NULL);
            SendMessageW(hResult, WM_SETFONT, (WPARAM)hFont, TRUE);
            break;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) find_path_gui();
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    load_graph(&g, "linie.txt");

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MetroKrakowGUI";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    CreateWindowW(L"MetroKrakowGUI", L"MPK Krakow - Planer Podrozy", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 395, 400, NULL, NULL, hInstance, NULL);

    MSG msg = { 0 };
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}