#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <gdiplus.h>
#include <vector>

#include "Transition.h"

// 내 이해: Color의 4인자 순서는 R G B A가 아니라 A R G B다
#define COLOR_BLACK Color(255, 0, 0, 0)
#define COLOR_WHITE Color(255, 255, 255, 255)
#define COLOR_RED Color(255, 255, 0, 0)
#define COLOR_GREEN Color(255, 0, 255, 0)
#define COLOR_BLUE Color(255, 0, 0, 255)

using namespace Eigen;
using namespace Gdiplus;

// 내 이해: GDI+는 헤더(gdiplus.h)만 포함하면 링크 오류(LNK2019)가 나므로 Gdiplus.lib을 링크해야 한다
#pragma comment (lib,"Gdiplus.lib")

Viewport mainViewport;
float objRotationY = 0, objRotationX = 0;
Matrix4f worldMatrix, viewMatrix, projMatrix;
Vector3f camPos(-8, 0, 0);
Vector3f zeroVector(0, 0, 0);
Vector3f upVector(0, 1, 0);

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

	// 내 이해: LONG(rcClient.left, top)과 unsigned(WIDTH, HEIGHT)를 섞어 넘기면 '오버로드된 함수에 대한 호출이 모호합니다' 오류가 난다. 인자를 전부 int로 맞춰서 해결
	graphics.FillRectangle(&blackBrush, (int)rcClient.left, (int)rcClient.top, WIDTH, HEIGHT);

	std::vector<Vector4f> vertexBuffer =
	{
		{ -1, -1, -1, 1 },	// 0
		{  1, -1, -1, 1 },	// 1
		{  1,  1, -1, 1 },	// 2
		{ -1,  1, -1, 1 },	// 3
		{ -1, -1,  1, 1 },	// 4
		{  1, -1,  1, 1 },	// 5
		{  1,  1,  1, 1 },	// 6
		{ -1,  1,  1, 1 }	// 7
	};

	std::vector<Vector3i> indexBuffer =
	{
		{0, 3, 2}, {0, 2, 1},	// z = -1
		{4, 5, 6}, {4, 6, 7},	// z = +1
		{0, 4, 7}, {0, 7, 3},	// x = -1
		{1, 2, 6}, {1, 6, 5},	// x = +1
		{0, 1, 5}, {0, 5, 4},	// y = -1
		{3, 7, 6}, {3, 6, 2}	// y = +1
	};

	std::vector<Vector4f> main_pixel_buffer;
	mainViewport.Update(0, 0, WIDTH, HEIGHT);

	viewMatrix = LootAtTransition(camPos, zeroVector, upVector);
	projMatrix = PerspectiveFovTransition(DegToRad(60), float(WIDTH) / float(HEIGHT), 1, 10);

	// 아핀 변환 -> T * R * S * V
	Transform<float, 3, Affine> t = Translation<float, 3>(0, 0, 0) * (AngleAxisf(objRotationY, Vector3f::UnitY()) * AngleAxisf(objRotationX, Vector3f::UnitX())) * Scaling(1.f);
	worldMatrix = t.matrix();

	Matrix4f multMatrix = projMatrix * viewMatrix * worldMatrix;
	main_pixel_buffer.clear();
	for (int i = 0; i < vertexBuffer.size(); i++)
	{
		main_pixel_buffer.push_back(ProjectTransition(vertexBuffer[i], mainViewport, multMatrix));
	}


	for (int i = 0; i < indexBuffer.size(); i++)
	{
		PointF points[3] =
		{
			PointF(main_pixel_buffer[indexBuffer[i](0)](0),main_pixel_buffer[indexBuffer[i](0)](1)),
			PointF(main_pixel_buffer[indexBuffer[i](1)](0),main_pixel_buffer[indexBuffer[i](1)](1)),
			PointF(main_pixel_buffer[indexBuffer[i](2)](0),main_pixel_buffer[indexBuffer[i](2)](1))
		};

		// 내 이해: DrawPolygon은 내부를 채우지 않는다.
		graphics.DrawPolygon(&whitePen, points, 3);
		// graphics.FillPolygon(&greenBrush, points, 3);
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

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_RIGHT:
			objRotationY += 0.02f;
			break;
		case VK_LEFT:
			objRotationY -= 0.02f;
			break;
		case VK_UP:
			objRotationX += 0.02f;
			break;
		case VK_DOWN:
			objRotationX -= 0.02f;
			break;
		case VK_RETURN:
			objRotationX = 0.f;
			objRotationY = 0.f;
			break;
		}
		InvalidateRect(hwnd, NULL, FALSE);
		UpdateWindow(hwnd);
		return 0;

	case WM_SIZE:
		InvalidateRect(hwnd, NULL, FALSE);
		UpdateWindow(hwnd);
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
	// 내 이해: GDI+를 쓰려면 프로그램 시작 시 GdiplusStartup, 종료 시 GdiplusShutdown을 호출해야 한다
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

