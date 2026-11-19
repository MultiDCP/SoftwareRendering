#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <gdiplus.h>
#include <vector>

#include "Transition.h"
#include "RubiksCubeMesh.h"
#include "CubeLogic.h"

// 내 이해: Color의 4인자 순서는 R G B A가 아니라 A R G B다
#define COLOR_BLACK Color(255, 0, 0, 0)
#define COLOR_WHITE Color(255, 255, 255, 255)
#define COLOR_RED Color(255, 255, 0, 0)
#define COLOR_GREEN Color(255, 0, 255, 0)
#define COLOR_BLUE Color(255, 0, 0, 255)
#define COLOR_YELLOW Color(255, 255, 255, 0)
#define COLOR_ORANGE Color(255, 255, 165, 0)

using namespace Eigen;
using namespace Gdiplus;

// 내 이해: GDI+는 헤더(gdiplus.h)만 포함하면 링크 오류(LNK2019)가 나므로 Gdiplus.lib을 링크해야 한다
#pragma comment (lib,"Gdiplus.lib")

unsigned WIDTH = 500;
unsigned HEIGHT = 500;

Viewport mainViewport;
float objRotationY = 0, objRotationX = 0;
Matrix4f worldMatrix, viewMatrix, projMatrix;
Vector3f camPos(-8, 0, 0);
Vector3f zeroVector(0, 0, 0);
Vector3f upVector(0, 1, 0);

std::vector<Vector4f> main_pixel_buffer;
std::vector<Vector4f> viewVertex;
std::vector<bool> isRenderTarget;

void Render(HWND hwnd, HDC hdc)
{
	RECT rcClient;
	GetClientRect(hwnd, &rcClient);
	WIDTH = rcClient.right - rcClient.left;
	HEIGHT = rcClient.bottom - rcClient.top;

	// 메모리 DC 생성
	HDC hdcMem = CreateCompatibleDC(hdc);
	const int nMemDC = SaveDC(hdcMem);

	// 호환 비트맵 생성
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);

	// 메모리 DC에 비트맵 선택
	SelectObject(hdcMem, hBitmap);

	// 이후 메모리 DC에 배경 및 그림 그리기
	Graphics graphics(hdcMem);

	Pen whitePen(COLOR_WHITE, 1);
	Pen blackPen(COLOR_BLACK, 1);
	SolidBrush whiteBrush(COLOR_WHITE);
	SolidBrush blackBrush(COLOR_BLACK);
	SolidBrush greenBrush(COLOR_GREEN);
	SolidBrush redBrush(COLOR_RED);
	SolidBrush blueBrush(COLOR_BLUE);
	SolidBrush yellowBrush(COLOR_YELLOW);
	SolidBrush orangeBrush(COLOR_ORANGE);

	// 내 이해: LONG(rcClient.left, top)과 unsigned(WIDTH, HEIGHT)를 섞어 넘기면 '오버로드된 함수에 대한 호출이 모호합니다' 오류가 난다. 인자를 전부 int로 맞춰서 해결
	graphics.FillRectangle(&blackBrush, (int)rcClient.left, (int)rcClient.top, WIDTH, HEIGHT);

	mainViewport.Update(0, 0, WIDTH, HEIGHT);

	viewMatrix = LootAtTransition(camPos, zeroVector, upVector);
	projMatrix = PerspectiveFovTransition(DegToRad(60), float(WIDTH) / float(HEIGHT), 1, 10);

	// 아핀 변환 -> T * R * S * V
	Transform<float, 3, Affine> t = Translation<float, 3>(0, 0, 0) * (AngleAxisf(objRotationY, Vector3f::UnitY()) * AngleAxisf(objRotationX, Vector3f::UnitX())) * Scaling(1.f);
	worldMatrix = t.matrix();

	// 백페이스 컬링을 위해 뷰 공간 변환 행렬을 따로 만들어둔다.
	Matrix4f viewTransMatrix = viewMatrix * worldMatrix;

	viewVertex.clear();
	isRenderTarget.clear();

	// 뷰 공간에서의 정점 좌표를 따로 계산하여 저장한다.
	for (int i = 0; i < rubiksVertexBuffer.size(); i++)
	{
		Vector4f newV = viewTransMatrix * rubiksVertexBuffer[i];
		viewVertex.push_back(newV);
	}

	// 법선을 구한 뒤 카메라에서 면을 향하는 벡터와 내적을 수행하여 백페이스 컬링을 계산한다.
	// Claude 설명 : 뷰 공간에서는 카메라가 원점에 있기에 v0 그대로 사용한다.
	// 고정된 시선 방향으로만 판단하는 방식은 직교 투영에서만 맞으며,
	// 원근 투영에서는 화면 가장자리에 있는 면일수록 "카메라에서 면을 향한 방향"이 시선 방향과 달라진다.
	for (int i = 0; i < rubiksIndexBuffer.size(); i++)
	{
		Vector4f v0 = viewVertex[rubiksIndexBuffer[i](0)];
		Vector4f v1 = viewVertex[rubiksIndexBuffer[i](1)];
		Vector4f v2 = viewVertex[rubiksIndexBuffer[i](2)];

		Vector4f v1_0 = v1 - v0;
		Vector4f v2_0 = v2 - v0;

		// cross는 크기가 3인 벡터에서만 쓸 수 있다. cross3은 Vector4f도 사용할 수 있으며, 앞의 세 성분으로만 외적을 계산한다.
		Vector4f n = v1_0.cross3(v2_0);
		
		// 앞면인 경우 렌더링을 수행하며, 아닐 경우 렌더하지 않는다.
		n.dot(v0) < 0 ? isRenderTarget.push_back(true) : isRenderTarget.push_back(false);
	}

	main_pixel_buffer.clear();
	for (int i = 0; i < rubiksVertexBuffer.size(); i++)
	{
		main_pixel_buffer.push_back(ProjectTransition(viewVertex[i], mainViewport, projMatrix));
	}

	for (int i = 0; i < rubiksIndexBuffer.size(); i++)
	{
		PointF points[3] =
		{
			PointF(main_pixel_buffer[rubiksIndexBuffer[i](0)](0),main_pixel_buffer[rubiksIndexBuffer[i](0)](1)),
			PointF(main_pixel_buffer[rubiksIndexBuffer[i](1)](0),main_pixel_buffer[rubiksIndexBuffer[i](1)](1)),
			PointF(main_pixel_buffer[rubiksIndexBuffer[i](2)](0),main_pixel_buffer[rubiksIndexBuffer[i](2)](1))
		};

		if (isRenderTarget[i] == true)
		{
			int surfaceNum = i / 18;
			int faceNum = (i % 18) / 2;

			ESurface surface = (ESurface)surfaceNum;
			char color;

			switch(surfaceNum)
			{
				case Left:
					color = L_arr[faceNum / 3][faceNum % 3];
					break;
				case Right:
					color = R_arr[faceNum / 3][faceNum % 3];
					break;
				case Front:
					color = F_arr[faceNum / 3][faceNum % 3];
					break;
				case Back:
					color = B_arr[faceNum / 3][faceNum % 3];
					break;
				case Down:
					color = D_arr[faceNum / 3][faceNum % 3];
					break;
				case Up:
					color = U_arr[faceNum / 3][faceNum % 3];
					break;
			}

			switch (color)
			{
				case 'O':
					graphics.FillPolygon(&orangeBrush, points, 3);
					break;
				case 'R':
					graphics.FillPolygon(&redBrush, points, 3);
					break;
				case 'G':
					graphics.FillPolygon(&greenBrush, points, 3);
					break;
				case 'B':
					graphics.FillPolygon(&blueBrush, points, 3);
					break;
				case 'Y':
					graphics.FillPolygon(&yellowBrush, points, 3);
					break;
				case 'W':
					graphics.FillPolygon(&whiteBrush, points, 3);
					break;
			}
		}
	}

	for (int i = 0; i < 6; i++)
	{
		if (isRenderTarget[i * 18] == true)
		{
			for (int j = i * 8; j < (i + 1) * 8; j++)
			{
				graphics.DrawLine(&blackPen,
					main_pixel_buffer[rubiksLineIndexBuffer[j](0)](0), main_pixel_buffer[rubiksLineIndexBuffer[j](0)](1),
					main_pixel_buffer[rubiksLineIndexBuffer[j](1)](0), main_pixel_buffer[rubiksLineIndexBuffer[j](1)](1));
			}
		}
	}

	// 메모리 DC 내용을 실제 화면 DC로 복사
	RECT rcClip;
	GetClipBox(hdc, &rcClip);
	BitBlt(hdc, rcClip.left, rcClip.top, rcClip.right - rcClip.left, rcClip.bottom - rcClip.top, hdcMem, rcClip.left, rcClip.top, SRCCOPY);

	// 메모리 자원 해제
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
		case 'L':
			L_Clockwise();
			break;
		case 'R':
			R_Clockwise();
			break;
		case 'F':
			F_Clockwise();
			break;
		case 'B':
			B_Clockwise();
			break;
		case 'U':
			U_Clockwise();
			break;
		case 'D':
			D_Clockwise();
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
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

	RegisterClass(&wc); // OS에 창 클래스 등록

	// Create the window.
	HWND hwnd = CreateWindowEx(
		0,								// Optional window styles.
		CLASS_NAME,						// Window class
		L"Rubiks Cube Rendering",		// Window text (창 이름이 된다)
		WS_OVERLAPPEDWINDOW,			// Window style

		// Size and position
		0, 0, WIDTH, HEIGHT,

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

