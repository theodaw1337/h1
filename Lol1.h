#pragma once

#include "DX9.h"
#include <Dwmapi.h>
#include "Couleurs.h"

#define CENTERX (GetSystemMetrics(SM_CXSCREEN)/2)-(screen_width/2)
#define CENTERY (GetSystemMetrics(SM_CYSCREEN)/2)-(screen_height/2)

enum circle_type { full, half, quarter };
enum text_alignment { lefted, centered, righted };

struct vertex
{
	FLOAT x, y, z, rhw;
	DWORD color;
};

extern int screen_width;
extern int screen_height;
extern LPDIRECT3D9 d3d;
extern LPDIRECT3DDEVICE9 d3ddev;
extern HWND hWnd;
extern HWND tWnd;
extern MARGINS margin;
extern LPD3DXFONT pFont;
extern LPD3DXFONT pFont2;
extern ID3DXLine* d3dLine;

void SText(int x, int y, int orientation, LPD3DXFONT g_pFont, bool bordered, DWORD color, DWORD bcolor, const wchar_t *fmt, ...);
void SText2(int x, int y, int orientation, LPD3DXFONT g_pFont2, bool bordered, DWORD color, DWORD bcolor, const wchar_t *fmt, ...);
void DrawLine(float x, float y, float xx, float yy, float WitdhAmount, D3DCOLOR color);
void DrawBox(float x, float y, float width, float height, D3DCOLOR color);
void GetScreenSize(int* width, int* height);
bool init(HWND hWnd);
bool ResizeRenderer();
void ReleaseRenderer();
void Loader();
