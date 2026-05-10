#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <windows.h>

#define MAX_STATIONS 200
#define MAX_EDGES 20
#define MAX_NAME_LEN 50
#define INF INT_MAX

typedef struct
{
	int to;
	int weight;
} Edge;

typedef struct
{
	char name[MAX_NAME_LEN];
	Edge adj[MAX_EDGES];
	int adj_count;
} Station;

typedef struct
{
	Station stations[MAX_STATIONS];
	int count;
} Graph;

int get_or_add_station(Graph* g, const char* name)
{
	for (int i = 0; i < g->count; i++)
		if (strcmp(g->stations[i].name, name) == 0)
			return i;

	if (g->count >= MAX_STATIONS)
	{
		fprintf(stderr, "Błąd: przekroczono MAX_STATIONS (%d)\n", MAX_STATIONS);
		exit(1);
	}

	int idx = g->count++;
	strncpy(g->stations[idx].name, name, MAX_NAME_LEN - 1);
	g->stations[idx].name[MAX_NAME_LEN - 1] = '\0';
	g->stations[idx].adj_count = 0;
	return idx;
}

void add_edge(Graph* g, int from, int to, int weight)
{
	Station* s = &g->stations[from];
	if (s->adj_count >= MAX_EDGES)
	{
		fprintf(stderr, "Błąd: stacja \"%s\" ma za dużo sąsiadów (max %d)\n", s->name, MAX_EDGES);
		exit(1);
	}

	s->adj[s->adj_count].to = to;
	s->adj[s->adj_count].weight = weight;
	s->adj_count++;
}

void load_graph(Graph* g, const char* filename)
{
	FILE* f = fopen(filename, "r");
	if (!f) {
		perror("Nie można otworzyć pliku");
		exit(1);
	}

	g->count = 0;
	char line[256];
	int  line_num = 0;
	char prev_name[MAX_NAME_LEN] = "";

	while (fgets(line, sizeof(line), f)) {
		line_num++;

		line[strcspn(line, "\r\n")] = '\0';

		if (line[0] == '#' || line[0] == '\0') {
			prev_name[0] = '\0';
			continue;
		}

		char name[MAX_NAME_LEN];
		int  w;

		if (sscanf(line, "%49[^;];%d", name, &w) != 2) {
			fprintf(stderr, "Ostrzeżenie: niepoprawna linia %d: \"%s\"\n",
				line_num, line);
			continue;
		}

		if (prev_name[0] != '\0') {
			int ia = get_or_add_station(g, prev_name);
			int ib = get_or_add_station(g, name);
			add_edge(g, ia, ib, w);
			add_edge(g, ib, ia, w);
		}
		else {
			get_or_add_station(g, name);
		}

		strncpy(prev_name, name, MAX_NAME_LEN - 1);
		prev_name[MAX_NAME_LEN - 1] = '\0';
	}

	fclose(f);
	printf("Wczytano %d stacji.\n", g->count);
}

void dijkstra(Graph* g, int src, int* dist, int* prev)
{
	int n = g->count;
	int visited[MAX_STATIONS] = { 0 };

	for (int i = 0; i < n; i++)
	{
		dist[i] = INF;
		prev[i] = -1;
	}
	dist[src] = 0;

	for (int i = 0; i < n; i++)
	{
		int u = -1;
		for (int j = 0; j < n; j++)
		{
			if (!visited[j] && (u == -1 || dist[j] < dist[u]))
				u = j;
		}

		if (u == -1 || dist[u] == INF) break;
		visited[u] = 1;

		for (int j = 0; j < g->stations[u].adj_count; j++)
		{
			int v = g->stations[u].adj[j].to;
			int w = g->stations[u].adj[j].weight;

			if (dist[u] != INF && dist[u] + w < dist[v])
			{
				dist[v] = dist[u] + w;
				prev[v] = u;
			}
		}
	}
}

void print_path(Graph* g, int* prev, int src, int dst)
{
	if (dst == src)
	{
		printf("%s", g->stations[src].name);
		return;
	}
	if (prev[dst] == -1)
	{
		printf("(brak drogi)");
		return;
	}
	print_path(g, prev, src, prev[dst]);
	printf(" -> %s", g->stations[dst].name);
}

int main()
{
	SetConsoleOutputCP(65001);

	char src_name[MAX_NAME_LEN];
	char dst_name[MAX_NAME_LEN];

	Graph g;
	load_graph(&g, "linie.txt");

	printf("Stacja startowa: ");
	scanf_s("%49[^\n]", src_name, MAX_NAME_LEN);

	printf("Stacja docelowa: ");
	scanf_s(" %49[^\n]", dst_name, MAX_NAME_LEN);

	int src = -1, dst = -1;
	for (int i = 0; i < g.count; i++)
	{
		if (strcmp(g.stations[i].name, src_name) == 0) src = i;
		if (strcmp(g.stations[i].name, dst_name) == 0) dst = i;
	}

	if (src == -1)
	{
		fprintf(stderr, "Błąd: nie znaleziono stacji \"%s\"\n", src_name);
		return 1;
	}
	if (dst == -1)
	{
		fprintf(stderr, "Błąd: nie znaleziono stacji \"%s\"\n", dst_name);
		return 1;
	}

	int dist[MAX_STATIONS];
	int prev[MAX_STATIONS];
	dijkstra(&g, src, dist, prev);

	if (dist[dst] == INF)
	{
		printf("Brak połączenia między \"%s\" a \"%s\".\n", src_name, dst_name);
	}
	else
	{
		printf("Najkrótsza droga (%d min): ", dist[dst]);
		print_path(&g, prev, src, dst);
		printf("\n");
	}
	
	return 0;
}