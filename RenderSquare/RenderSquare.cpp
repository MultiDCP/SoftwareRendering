#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

#define COLOR_BLACK RGB(0, 0, 0)
#define COLOR_WHITE RGB(255, 255, 255)
#define COLOR_RED RGB(255, 0, 0)
#define COLOR_GREEN RGB(0, 255, 0)
#define COLOR_BLUE RGB(0, 0, 255)

void Render(HWND hwnd, HDC hdc)
{
	// ? : old가 따로 존재하는 이유?
	// 해결 : 사용 중인 GDI 오브젝트는 DeleteObject 함수를 사용해 메모리를 해제할 수 없음.
	// 가령 삭제한다고 해도 디폴트가 삭제되는 문제가 발생.
	// 따라서 DC에 선택된 GDI 오브젝트는 반드시 하나는 존재해야 함.
	HPEN myPen, oldPen;
	HBRUSH myBrush, oldBrush;

	POINT rectanglePoint[] =
	{
		{ 100, 100 },
		{ 100, 200 },
		{ 200, 200 },
		{ 200, 100 }
	};

	// GDI 오브젝트 핸들 선언
	myPen = CreatePen(PS_SOLID, 1, COLOR_BLACK);
	oldPen = (HPEN)SelectObject(hdc, myPen);
	myBrush = CreateSolidBrush(COLOR_WHITE);
	oldBrush = (HBRUSH)SelectObject(hdc, myBrush);

	// 다각형 그리기. 4는 꼭짓점 개수를 의미함
	Polygon(hdc, rectanglePoint, 4);

	// GDI 오브젝트 메모리 해제
	SelectObject(hdc, oldPen);
	SelectObject(hdc, oldBrush);
	DeleteObject(myPen);
	DeleteObject(myBrush);
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
	// Register the window class.
	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	WNDCLASS wc = { };

	wc.lpfnWndProc = WindowProc; // 창 프로시저
	wc.hInstance = hInstance; // 애플리케이션 인스턴스 핸들
	wc.lpszClassName = CLASS_NAME; // 창 클래스 식별 문자열

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

	return 0;
}

