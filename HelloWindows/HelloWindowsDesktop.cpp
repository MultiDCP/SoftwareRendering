#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// uMsg : 메세지 코드. wParam과 lParam을 통해 함수가 관련된 추가 데이터를 받을 수 있다.
	switch (uMsg)
	{
		case WM_CLOSE:
			// ? : 그럼 바로 꺼지게 하려면 메세지 박스 없이 DestroyWindow만 호출해도 되는가?
			// 해결 : 그렇다. MessageBox를 호출하지 않고 바로 DestroyWindow를 호출하면 됨.
			if (MessageBox(hwnd, L"Really quit?", L"My application", MB_OKCANCEL) == IDOK)
			{
				DestroyWindow(hwnd);
			}
			// Else : User canceled. Do nothing.
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);

				// All painting occurs here, between BeginPaint and EndPaint.
				// ? : 그렇다면 모든 드로우 과정은 여기서 이루어지는가?

				// ? : 새로운 걸 그릴 때 함수 작성 순서대로 렌더링되는가?
				// GDI 학습할 때 더 자세히 볼 수 있을 거 같다.

				// FillRect : GDI 함수
				// 두 번째 매개변수에서 채울 사각형의 좌표를 제공한다.
				// 후속 WM_PAINT 메세지에서 더 작은 사각형을 포함할 수 있다.
				FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

				EndPaint(hwnd, &ps);
			}
		return 0;
	}
	// 특정 메세지를 처리하지 않는 경우 기본 작업 수행
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}