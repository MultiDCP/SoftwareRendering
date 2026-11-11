#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <gdiplus.h>
#include <Eigen/Dense>
#include <vector>

// 내 이해: Color의 4인자 순서는 R G B A가 아니라 A R G B다
#define COLOR_BLACK Color(255, 0, 0, 0)
#define COLOR_WHITE Color(255, 255, 255, 255)
#define COLOR_RED Color(255, 255, 0, 0)
#define COLOR_GREEN Color(255, 0, 255, 0)
#define COLOR_BLUE Color(255, 0, 0, 255)

using namespace Eigen;
using namespace Gdiplus;

#pragma comment (lib,"Gdiplus.lib")

void Render(HWND hwnd, HDC hdc)
{
	RECT rcClient;
	GetClientRect(hwnd, &rcClient);
	int WIDTH = rcClient.right - rcClient.left;
	int HEIGHT = rcClient.bottom - rcClient.top;

	HDC hdcMem = CreateCompatibleDC(hdc);
	const int nMemDC = SaveDC(hdcMem);

	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
	SelectObject(hdcMem, hBitmap);

	Graphics graphics(hdc);

	Pen whitePen(COLOR_WHITE, 1);
	SolidBrush whiteBrush(COLOR_WHITE);
	SolidBrush blackBrush(COLOR_BLACK);
	SolidBrush greenBrush(COLOR_GREEN);

	graphics.FillRectangle(&blackBrush, (int)rcClient.left, (int)rcClient.top, WIDTH, HEIGHT);

	std::vector<Vector2f> vertexBuffer =
	{
		{ 100, 100 },
		{ 100, 200 },
		{ 200, 200 },
		{ 200, 100 }
	};

	std::vector<Vector3i> indexBuffer =
	{
		{0, 1, 2},
		{0, 2, 3}
	};

	for (int i = 0; i < indexBuffer.size(); i++)
	{
		PointF points[3] =
		{
			PointF(vertexBuffer[indexBuffer[i](0)](0),vertexBuffer[indexBuffer[i](0)](1)),
			PointF(vertexBuffer[indexBuffer[i](1)](0),vertexBuffer[indexBuffer[i](1)](1)),
			PointF(vertexBuffer[indexBuffer[i](2)](0),vertexBuffer[indexBuffer[i](2)](1))
		};

		// DrawPolygon은 내부를 채우지 않는다.
		// graphics.DrawPolygon(&whitePen, points, 3);
		graphics.FillPolygon(&greenBrush, points, 3);
	}

	RestoreDC(hdcMem, nMemDC);
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// uMsg : 메세지 코드. wParam과 lParam을 통해 함수가 관련된 추가 데이터를 받을 수 있다.
	switch (uMsg)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd);
		// Else : User canceled. Do nothing.
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		Render(hwnd, hdc);
		EndPaint(hwnd, &ps);
	}
	return 0;
	}
	// 특정 메세지를 처리하지 않는 경우 기본 작업 수행
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR           gdiplusToken;

	// Initialize GDI+.
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// Register the window class.
	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = WindowProc; // 창 프로시저
	wc.hInstance = hInstance; // 애플리케이션 인스턴스 핸들
	wc.lpszClassName = CLASS_NAME; // 창 클래스 식별 문자열
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

	RegisterClass(&wc); // OS에 창 클래스 등록

	// Create the window.
	HWND hwnd = CreateWindowEx(
		0,								// Optional window styles.
		CLASS_NAME,						// Window class
		L"Learn to Program Windows",	// Window text (창 이름이 된다)
		WS_OVERLAPPEDWINDOW,			// Window style

		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,		// Parent window
		NULL,		// Menu
		hInstance,  // Instance handle
		NULL
	);

	if (hwnd == NULL)
	{
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	// Run the message loop.
	/*
	* 애플리케이션은 실행되는 동안 수천 개의 메세지를 받음
	* 각 앱은 날아오는 수천개의 메세지 중 자기 꺼를 검색하고
	* 올바른 창으로 디스패치하는 루프가 필요함
	*/

	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0) > 0) // 큐에서 메세지를 끌어옴
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg); // 메세지의 대상인 창의 프로시저를 호출하도록 지시
	}

	GdiplusShutdown(gdiplusToken);
	return 0;
}

