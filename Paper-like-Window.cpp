#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <locale.h>
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1 : 0)

int wx, wy;
int w, h;
int iAngle = 0;
HBITMAP hBitmap;
HBRUSH hBrush;
LPWSTR lpWindowName;

HCRYPTPROV prov;
int random() {
	if (prov == NULL)
		if (!CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_SILENT | CRYPT_VERIFYCONTEXT))
			ExitProcess(1);

	int out;
	CryptGenRandom(prov, sizeof(out), (BYTE *)(&out));
	return out & 0x7fffffff;
}

DWORD WINAPI MsgBoxThread(LPVOID lpParameter){
	HWND hOwner = (HWND)lpParameter;
	int opt = MessageBoxW(NULL, L"我错了！！！\n按一下“是”，结束我的痛苦吧！", lpWindowName, MB_YESNO|MB_ICONERROR|MB_TOPMOST);
	if(opt == IDYES) SendMessage(hOwner, WM_DESTROY, 0, 0);
	
	return 0;
}

void cls(HANDLE hConsole)
{
	COORD coordScreen = {0,0};
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
	FillConsoleOutputCharacter(hConsole, (TCHAR) ' ', dwConSize, coordScreen, &cCharsWritten);
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
	SetConsoleCursorPosition(hConsole, coordScreen);
	return;
}
inline int RotateDC(HDC hDc, int iAngle, POINT centerPt)  
{  
    int nGraphicsMode = SetGraphicsMode(hDc, GM_ADVANCED);  
    XFORM xform;  
    if(iAngle != 0 ) {  
        double fangle = (double)iAngle / 180. * 3.1415926;  
        xform.eM11 = (float)cos(fangle); xform.eM12 = (float)sin(fangle);  
        xform.eM21 = (float)-sin(fangle); xform.eM22 = (float)cos(fangle);  
        xform.eDx = (float)(centerPt.x - cos(fangle)*centerPt.x + sin(fangle)*centerPt.y);  
        xform.eDy = (float)(centerPt.y - cos(fangle)*centerPt.y - sin(fangle)*centerPt.x);  
        SetWorldTransform(hDc, &xform);  
    }  
    return nGraphicsMode;  
}  

HBITMAP WINAPI StretchBitmap(HWND hwnd){
	HDC hdc = GetDC(hwnd);
	HDC hCp = CreateCompatibleDC(hdc);
	HBITMAP hBitmap2 = CreateCompatibleBitmap(hdc, w, h);
	SelectObject(hCp, hBitmap2);
	RECT rc = {0, 0, w, h};
	FillRect(hCp, &rc, CreateSolidBrush(999));
	StretchBlt(hCp, 10, 10,  w - 10, h - 10, hdc, 0, 0, w, h, SRCCOPY);
	DeleteObject(hBitmap);
	DeleteDC(hCp);
	return (hBitmap = hBitmap2);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch(Message) {		
		case WM_CREATE:{
			SetLayeredWindowAttributes(hwnd, 999, 0, LWA_COLORKEY);
			SetTimer(hwnd, 1, 10, NULL);
			break;
		}
		case WM_TIMER:{
			iAngle++;
			if(iAngle == 180) {
				StretchBitmap(hwnd);
				iAngle+=180;		
			}
			
			if(iAngle == 360) iAngle = 0;
			int x = (random()%5-2), y = (random()%5-2);
			int deltax = wx-x, deltay = wy-y;
			SetWindowPos(hwnd, NULL, wx+x, wy+y, w, h, 0);
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		}
		
		case WM_PAINT:{
			InvalidateRect(hwnd, NULL, FALSE);			
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			HDC hcdc = CreateCompatibleDC(hdc);
			SelectObject(hcdc, hBitmap);
			RotateDC(hdc, iAngle, (POINT){w/2,h/2});
			BitBlt(hdc, 0, 0, w, h, hcdc, 0, 0, SRCCOPY);
			DeleteDC(hcdc);
			EndPaint(hwnd, &ps);
			break;
		}
		case WM_LBUTTONUP:{
			CreateThread(NULL, 4096, MsgBoxThread, (LPVOID)hwnd, 0, NULL);
			break;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			break;
		}
		case WM_CLOSE: break;
		default:
			return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}
HWND hTarget;
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	setlocale(LC_ALL, "chs");
	POINT p;
	lpWindowName = (LPWSTR)LocalAlloc(LMEM_ZEROINIT,  512 * 2);
	HANDLE hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	HWND hPrev = NULL;
	while(1){
		
		GetCursorPos(&p);
		
		hTarget = WindowFromPoint(p);
		if(!hTarget){
			Sleep(20);
			continue;
		}
		
		HDC hdc = GetDC(HWND_DESKTOP);
		RECT rc;
		GetWindowRect(hTarget, &rc);
		FrameRect(hdc, &rc, CreateSolidBrush(RGB(255, 0, 0)));
		GetWindowTextW(hTarget, lpWindowName, 512);
		if(hPrev != hTarget) {
			cls(hStdOutput);
			wprintf(L"当前选定窗口名:%s\n", lpWindowName);
			wprintf(L"按[Enter]确定窗口。");
		}
		if(KEY_DOWN(VK_RETURN)) break;
		Sleep(20);
		hPrev = hTarget;
	}
	FreeConsole();
	RECT rc;
	GetWindowRect(hTarget, &rc);
	wx = rc.left, wy = rc.top;
	w = rc.right - rc.left, h = rc.bottom - rc.top;
	HDC hDesktop = GetWindowDC(hTarget);
	HDC hCp = CreateCompatibleDC(hDesktop);
	hBitmap = CreateCompatibleBitmap(hDesktop, w, h);
	SelectObject(hCp, hBitmap);
	BitBlt(hCp, 0, 0, w, h, hDesktop, 0, 0, SRCCOPY);
	MessageBoxW(hTarget, L"awsl!!!", lpWindowName, MB_ICONERROR);
	DeleteDC(hCp);
	hBrush = CreateSolidBrush(999);
	
	SendMessage(hTarget, WM_CLOSE, 0, 0);
	WNDCLASSEX wc;
	HWND hwnd;
	MSG msg;
	memset(&wc,0,sizeof(wc));
	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.lpfnWndProc	 = WndProc;
	wc.hInstance	 = hInstance;
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = hBrush;
	wc.lpszClassName = "WindowClass";
	wc.hIcon		 = LoadIcon(NULL, IDI_ERROR);
	wc.hIconSm		 = LoadIcon(NULL, IDI_ERROR);

	if(!RegisterClassEx(&wc)) return 1;
	hwnd = CreateWindowExW(WS_EX_TOPMOST|WS_EX_LAYERED,L"WindowClass",lpWindowName,WS_VISIBLE|WS_POPUP,
		                   rc.left, rc.top, w, h, NULL,NULL,hInstance,NULL);

	if(hwnd == NULL) return 1;

	while(GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	LocalFree(lpWindowName);
	return msg.wParam;
}
