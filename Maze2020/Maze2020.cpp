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
INT_PTR CALLBACK    Options(HWND, UINT, WPARAM, LPARAM);

// Order is important. This should be before any GDI+ object. (Shutdown is last.) Otherwise, exception occurs at exit.
class GdiplusStart {
	ULONG_PTR gpToken;
public:
	GdiplusStart() {
		GdiplusStartupInput gpsi;
		if (GdiplusStartup(&gpToken, &gpsi, NULL) != Ok) {
			MessageBox(NULL, TEXT("Unable to initialize GDI+."), TEXT("Error"), MB_OK | MB_ICONERROR);
			PostQuitMessage(1);
		}
	}

	~GdiplusStart() {
		GdiplusShutdown(gpToken);
	}
} gdiplusstart;

enum class Mat { clear, wall, visited, deadend };
HWND hWnd;
int wallsize = 10;
vector<vector<Mat>> mat;
int maxx, maxy, curx, cury;
Color ColorWall = Color::Black;
Color ColorCur = Color::Green;
Color ColorDeadEnd  = Color::Red;
bool finished = false;
RECT rect;
const int dirs[][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

unique_ptr<Bitmap> bm;
unique_ptr<Graphics> memG;

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
      CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, nullptr, nullptr, hInstance, nullptr);

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
	bm.reset(new Bitmap(rect.right, rect.bottom, &g));
	memG.reset(new Graphics(bm.get()));
	memG->SetSmoothingMode(SmoothingMode::SmoothingModeAntiAlias);
	memG->Clear(Color::White);
	Pen penWall(ColorWall, wallsize / 4.0f);
	//Pen penDeadEnd(ColorDeadEnd, wallsize / 4.0f);
	SolidBrush brushDeadEnd(ColorDeadEnd);
	
	penWall.SetStartCap(LineCap::LineCapRound);
	penWall.SetEndCap(LineCap::LineCapRound);

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
				//memG->DrawEllipse(&penDeadEnd, x*wallsize - wallsize / 2, y*wallsize - wallsize / 2, wallsize, wallsize);
				memG->FillRectangle(&brushDeadEnd, x * wallsize - wallsize / 2, y * wallsize - wallsize / 2, wallsize, wallsize);
			}
		}
	}
}

double rand_double() {
	return rand() / (RAND_MAX + 1.0);
}

void generate() {
	int x, y, dir;
	bool hasWork;
	finished = false;
	Pen penCur(ColorCur, wallsize / 4.0f);
	penCur.SetStartCap(LineCap::LineCapRound);
	penCur.SetEndCap(LineCap::LineCapRound);
	
	if (wallsize > min(rect.bottom, rect.right) / 2) wallsize = min(rect.bottom, rect.right) / 2;
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
		hasWork = false;
		for (y = 0; y <= maxy; y += 2) {
			for (x = 0; x <= maxx; x += 2) {
				if (mat[y][x] != Mat::wall) {
					hasWork = true;
					continue;
				}

				dir = (int)(rand_double() * 4);
				int newx = x + dirs[dir][0];
				int newy = y + dirs[dir][1];
				int newx2 = x + 2 * dirs[dir][0];
				int newy2 = y + 2 * dirs[dir][1];
				if (newx2 >= 0 && newx2 <= maxx && newy2 >= 0 && newy2 <= maxy && mat[newy2][newx2] != Mat::wall) {
					mat[newy][newx] = Mat::wall;
					mat[newy2][newx2] = Mat::wall;
					hasWork = true;
					continue;
				}
			}
		}
	} while (hasWork);

	curx = 1;
	cury = 1;
	drawMat();
	mat[cury][curx] = Mat::visited;
	memG->DrawLine(&penCur, 0 * wallsize, 1 * wallsize, curx*wallsize, cury*wallsize);
	
	InvalidateRect(hWnd, NULL, false);
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
	InvalidateRect(hWnd, NULL, false);
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
	case WM_GETMINMAXINFO:
		// resize
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
            case IDM_OPTIONS:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_OPTIONSBOX), hWnd, Options);
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
		pen.SetStartCap(LineCap::LineCapRound);
		pen.SetEndCap(LineCap::LineCapRound);
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
		InvalidateRect(hWnd, NULL, false);

		if (!finished && curx == maxx - 1 && cury == maxy - 1) {
			finished = true;
			MessageBox(hWnd, L"Congratulations!", szTitle, MB_OK | MB_ICONINFORMATION);
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
			CachedBitmap cbm(bm.get(), &g);
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

// Message handler for options box.
INT_PTR CALLBACK Options(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
		SetDlgItemInt(hDlg, IDC_EDITWIDTH, wallsize, true);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
		int id = LOWORD(wParam);
        if (id == IDOK || id == IDCANCEL)
        {
			if (id == IDOK) {
				int ok;
				int r = GetDlgItemInt(hDlg, IDC_EDITWIDTH, &ok, false);
				if (ok && r > 0) wallsize = r;
			}
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
