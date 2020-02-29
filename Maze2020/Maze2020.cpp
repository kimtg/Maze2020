// Maze2020.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <gdiplus.h>
#include <memory>
#include <vector>
#include <ctime>
#include <cstdlib>
#include "Maze2020.h"

using namespace std;
using namespace Gdiplus;

#pragma comment(lib, "gdiplus.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

enum class Mat { clear, wall, visited, deadend };

HWND hWnd;

int wallsize = 10;
vector<vector<Mat>> mat;
int maxx, maxy, curx, cury;
Color ColorWall(0, 0, 0);
Color ColorCur(50, 200, 50);
Color ColorDeadEnd(200, 0, 0);
bool finished = false;
RECT rect;
Bitmap *bm;
Graphics *memG;
const int dirs[][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

class GdiplusStart {
	ULONG_PTR gpToken;
public:
	GdiplusStart() {
		GdiplusStartupInput gpsi;
		if (GdiplusStartup(&gpToken, &gpsi, NULL) != Ok) {
			MessageBox(NULL, TEXT("GDI+ 라이브러리를 초기화할 수 없습니다."),
				TEXT("알림"), MB_OK);
			PostQuitMessage(1);
		}
	}

	~GdiplusStart() {
		GdiplusShutdown(gpToken);
	}
} gdiplusstart;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	srand((unsigned int) time(nullptr));

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MAZE2020, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAZE2020));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
	
    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAZE2020));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_MAZE2020);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void drawMat() {
	int x, y, x2, y2;
	Graphics g(hWnd);
	if (bm != nullptr) delete bm;
	bm = new Bitmap(rect.right, rect.bottom, &g);
	if (memG != nullptr) delete memG;
	memG = new Graphics(bm);
	memG->Clear(Color(255, 255, 255));
	Pen penWall(ColorWall, wallsize / 4.0f);
	static const Pen penDeadEnd(ColorDeadEnd, wallsize / 4.0f);
	static const Pen penCur(ColorCur, wallsize / 4.0f);

	for (y = 0; y <= maxy / 2; y++) {
		for (x = 0; x <= maxx / 2; x++) {
			x2 = x * 2;
			y2 = y * 2;
			if (mat[y2][x2 + 1] == Mat::wall) { // horizontal
				memG->DrawLine(&penWall, x2*wallsize, y2*wallsize, (x2 + 2)*wallsize, y2*(wallsize));
			}
			if (mat[y2 + 1][x2] == Mat::wall) { // vertical
				memG->DrawLine(&penWall, x2*wallsize, y2*wallsize, x2*wallsize, (y2 + 2)*wallsize);
			}
		}
	}

	for (y = 1; y < maxy; y += 2) {
		for (x = 1; x < maxx; x += 2) {
			if (mat[y][x] == Mat::deadend) {
				memG->DrawEllipse(&penDeadEnd, x*wallsize - wallsize / 2, y*wallsize - wallsize / 2, wallsize, wallsize);
			}
		}
	}

	memG->DrawLine(&penCur, 0 * wallsize, 1 * wallsize, curx*wallsize, cury*wallsize);
}

double rand_double() {
	return rand() / (RAND_MAX + 1.0);
}

void generate() {
	int x, y, dir;
	bool hasWork;
	bool finished = false;	
	
	maxy = rect.bottom / wallsize / 2 * 2;
	maxx = rect.right / wallsize / 2 * 2;
	
	mat.resize(maxy + 2);
	for (auto &x : mat) {
		x.clear();
		x.resize(maxx + 2, Mat::clear);		
	}
	
	for (x = 0; x <= maxx; x++) {
		mat[0][x] = Mat::wall;
		mat[maxy][x] = Mat::wall;
	}

	for (y = 0; y <= maxy; y++) {
		mat[y][0] = Mat::wall;
		mat[y][maxx] = Mat::wall;
	}

	mat[1][0] = Mat::clear; // start
	mat[maxy - 1][maxx] = Mat::clear; //end

	do {
a:
		hasWork = false;
		y = (int)(rand_double() * (maxy + 1) / 2) * 2;
		x = (int)(rand_double() * (maxx + 1) / 2) * 2;
		if (mat[y][x] != Mat::wall) {
			goto a;
		}		

		dir = (int) (rand_double() * 4);
		int newx = x + dirs[dir][0];
		int newy = y + dirs[dir][1];
		int newx2 = x + 2*dirs[dir][0];
		int newy2 = y + 2*dirs[dir][1];
		if (newx2 >= 0 && newx2 <= maxx && newy2 >= 0 && newy2 <= maxy && mat[newy2][newx2] != Mat::wall) {					
			mat[newy][newx] = Mat::wall;
			mat[newy2][newx2] = Mat::wall;
			goto a;
		}

		for (y = 0; y <= maxy; y += 2) {
			for (x = 0; x <= maxx; x += 2) {
				if (mat[y][x] != Mat::wall) {
					hasWork = true;
					goto a;
				}
			}
		}
	} while (hasWork);

	curx = 1;
	cury = 1;
	drawMat();
	mat[cury][curx] = Mat::visited;
	
	InvalidateRect(hWnd, NULL, true);
}

int matBlocked(int x, int y) {
	if (mat[y][x] == Mat::clear)
		return 0;
	return 1;
}

void solve() {
	int x, y;
	bool hasWork;

	do {
		hasWork = false;
		for (y = 1; y < maxy; y += 2) {
			for (x = 1; x < maxx; x += 2) {
				if (mat[y][x] != Mat::deadend && matBlocked(x + 1, y) + matBlocked(x - 1, y) + matBlocked(x, y + 1) + matBlocked(x, y - 1) >= 3) {
					mat[y][x] = Mat::deadend;
					for (int d = 0; d < 4; d++) {
						int newy = y + dirs[d][0];
						int newx = x + dirs[d][1];
						if (mat[newy][newx] == Mat::clear)
							mat[newy][newx] = Mat::deadend;
					}
					hasWork = true;
				}
			}
		}
	} while (hasWork);
	drawMat();
	InvalidateRect(hWnd, NULL, true);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_WINDOWPOSCHANGED:
	case WM_GETMINMAXINFO: // resize
		GetClientRect(hWnd, &rect);
		break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
			case IDM_GENERATE:
				generate();
				break;
			case IDM_SOLVE:
				solve();
				break;
            case IDM_ABOUT:				
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
	case WM_KEYDOWN:
	{
		int oldx, oldy;
		Pen pen(ColorCur, wallsize / 4.0f);
		oldx = curx;
		oldy = cury;
		switch (wParam) {
		case VK_UP:
			if (mat[cury - 1][curx] != Mat::wall) cury -= 2;
			break;
		case VK_DOWN:
			if (mat[cury + 1][curx] != Mat::wall) cury += 2;
			break;
		case VK_LEFT:
			if (curx > 1 && mat[cury][curx - 1] != Mat::wall) curx -= 2;
			break;
		case VK_RIGHT:
			if (curx < maxx - 1 && mat[cury][curx + 1] != Mat::wall) curx += 2;
			break;
		}
		if (mat[cury][curx] == Mat::visited) {
			mat[oldy][oldx] = Mat::clear;
			pen.SetColor(ColorDeadEnd);
		}
		mat[cury][curx] = Mat::visited;
		memG->DrawLine(&pen, oldx * wallsize, oldy * wallsize, curx * wallsize, cury * wallsize);
		InvalidateRect(hWnd, NULL, true);

		if (!finished && curx == maxx - 1 && cury == maxy - 1) {
			finished = true;
			MessageBox(hWnd, L"Congratulations!", L"", MB_OK);
		}
		break;
	}
    case WM_PAINT:
        {
			if (bm == nullptr) generate();
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
			Graphics g(hdc);
			CachedBitmap cbm(bm, &g);
			g.DrawCachedBitmap(&cbm, 0, 0);
			
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
