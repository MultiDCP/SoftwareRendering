#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>

/*
* 목적 : 마우스로 클릭한 지점에 점과 선을 그려서 클릭 지점을 표시한다.
* - 드로잉 마커 예제(https://learn.microsoft.com/ko-kr/windows/win32/gdi/drawing-markers)를 참고했다.
* - 예제에 없는 원(Ellipse)은 클릭 지점을 점으로 더 잘 보이게 하려고 추가했다.
*   원 그리는 법은 https://luckygg.tistory.com/90 ([GDI] Part 2. 작도함수 #1)의 함수들을 참고했다.
*/

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ? : 왜 빌드하면 Marker 식별자를 찾을 수 없다는 오류가 나는가?
// 해결 : 컴파일러는 소스 파일을 위에서부터 읽는데, Marker를 호출하는 줄이 정의보다 먼저 온다.
//        함수 정의를 호출보다 위로 올리거나, 호출 전에 함수 선언(함수 프로토타입)을 적어 두면 된다.

/*
* 함수 선언과 빌드 단계에 대한 내 이해:
* - 컴파일러가 호출을 검사하려면 최소한 이름, 매개변수 타입, 반환 타입을 알아야 하고, 이건 선언에 들어 있다.
* - 컴파일 단계는 선언만 있으면 호출을 검사할 수 있고, 그 선언에 맞는 정의가 실제로 있는지는 링크 단계가 확인한다.
* - 선언의 매개변수 타입을 정의와 다르게(LONG -> int) 바꾸면 C++은 서로 다른 함수로 본다.
*   이때는 컴파일은 통과하고, 링커가 일치하는 정의를 찾지 못해 LNK2001, LNK1120 오류가 난다.
* - 링커는 각 개체 파일에서 참조하는 모든 외부 기호에 대해 일치하는 정의를 찾아야 하고, 못 찾으면 오류를 낸다.
* - VCR001은 IntelliSense가 알려 주는 항목이라 빌드를 막는 오류는 아니다.
*/
void Marker(LONG x, LONG y, HWND hwnd);

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
	static BOOL bCollectPoints;
	static POINT ptMouseDown[32];
	static int index;
	POINTS ptTmp;
	RECT rc;
	HRGN hrgn;

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
			// ? : 왜 클릭해도 아무것도 그려지지 않는가?
			// 해결 : bCollectPoints는 값을 넣지 않은 static 변수라 0(FALSE)으로 시작하는데, 값을 바꾸는 곳이 없어서
			//        WM_LBUTTONDOWN의 if 안으로 들어가지 못했다. 첫 그리기로 화면을 그린 뒤 점을 수집하도록 여기서 TRUE로 바꾼다.

			/*
			* WM_PAINT 처리에 대한 내 이해:
			* - 처음에는 이 case 끝에 return이나 break가 없어서 아래 WM_LBUTTONDOWN 코드까지 이어서 실행됐다.
			*   (fall-through, Claude가 짚음) 그래서 클릭 전에 bCollectPoints가 켜지고, 다시 그릴 때마다 (0, 0)에 표시가 찍혔다.
			* - 메시지를 직접 처리했으면 return 0으로 끝내는 게 Win32의 관례다. break로 끝내면 DefWindowProc이 한 번 더 호출된다.
			* - bCollectPoints는 BOOL(int의 별칭)이라 bool의 true가 아니라 TRUE 매크로를 쓴다.
			*/
			if (!bCollectPoints) bCollectPoints = TRUE;
			return 0;

		case WM_LBUTTONDOWN:
			if (bCollectPoints && index < 32)
			{
				// Create the region from the client area.
				GetClientRect(hwnd, &rc);
				hrgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);

				// ? : 왜 예제대로 MAKEPOINTS((POINTS FAR*)lParam) 으로 작성하면 오류가 나는가?
				// 해결 : lParam을 그대로 넘기면 된다.

				/*
				* MAKEPOINTS와 POINTS FAR* 포인터에 대한 내 이해:
				* - MAKEPOINTS는 함수가 아니라 #define으로 정의된 전처리 매크로다.
				*   매크로 안에서 이미 POINTS FAR*로 변환하므로, 직접 변환하지 않고 lParam을 그대로 넘기면 된다.
				* - 좌표 x, y가 담긴 4바이트 값을 메모리에서 읽어서 낮은 2바이트는 x, 높은 2바이트는 y(SHORT)로 나눠
				*   POINTS 구조체로 만든다. (MAKEPOINTS 매크로 문서)
				* - x64에서 LPARAM은 __int64(8바이트)지만, 리틀 엔디안에서는 매크로가 읽는 4바이트가 낮은 쪽 4바이트라서
				*   좌표 값이 그대로 읽힌다. 그래서 64비트에서도 동작한다.
				* - POINTS는 _MAC일 때 y, x 순서로 선언된다. _MAC은 Windows PC에서 매킨토시용 프로그램을
				*   교차 컴파일할 때 쓰던 매크로이고, 당시 Mac의 CPU(PowerPC 등)가 빅 엔디안이라 순서가 반대다.
				* - FAR: 16비트 시절에는 near 포인터로 한 세그먼트 안만 접근할 수 있어서 far 포인터가 필요했다.
				*   지금은 #define far 로 비어 있어서 전처리 후 사라진다. 옛 코드가 그대로 컴파일되도록 남겨 둔 호환용이다.
				*   (FAR의 역할은 Claude의 도움을 받아 정리)
				* - FAR가 쓰인 것으로 보아 이 예제는 16비트 시절 작성 방식이 남아 있는 오래된 예제다.
				*/

				ptTmp = MAKEPOINTS(lParam);
				ptMouseDown[index].x = (LONG)ptTmp.x;
				ptMouseDown[index].y = (LONG)ptTmp.y;

				// Test for a hit in the client rectangle.
				if (PtInRegion(hrgn, ptMouseDown[index].x, ptMouseDown[index].y))
				{
					// If a hit occurs, record the mouse coords.
					Marker(ptMouseDown[index].x, ptMouseDown[index].y, hwnd);
					index++;
				}
				// 내 이해: 영역은 PtInRegion 검사에만 필요하므로, 다 쓰면 바로 DeleteObject로 지워 GDI 자원이 쌓이지 않게 한다.
				DeleteObject(hrgn);
			}
		return 0;
	}
	// 특정 메세지를 처리하지 않는 경우 기본 작업 수행
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ? : 창을 RESIZE한 뒤 오브젝트가 사라진다. 이유는?
void Marker(LONG x, LONG y, HWND hwnd)
{
	HDC hdc;

	// hwnd로부터 DC를 얻음
	hdc = GetDC(hwnd);

	MoveToEx(hdc, (int)x - 10, (int)y, (LPPOINT)NULL); // 현재 위치를 (x-10, y)로 이동하기
	LineTo(hdc, (int)x + 10, (int)y); // 현재 지점인 (x-10, y)부터 (x+10, y)까지 직선 그리기
	MoveToEx(hdc, (int)x, (int)y - 10, (LPPOINT)NULL); // 현재 위치를 (x, y-10)으로 이동하기
	LineTo(hdc, (int)x, (int)y + 10); // 현재 지점인 (x, y-10)부터 (x, y+10)까지 직선 그리기

	// 클릭 지점으로부터 (x-2,y-2)~(x+3,y+3)까지의 원형 그리기
	// ? : 왜 (x-2,y-2)부터 (x+2,y+2)까지는 점이 원점에 더 치우쳐보이는가?
	// 해결 : Ellipse는 경계 사각형의 오른쪽과 아래쪽 가장자리를 제외하고 그린다.
	//        (x-2)~(x+2)로 주면 실제로는 x-2~x+1까지 칠해져 가운데가 x-0.5가 되므로 원점 쪽으로 치우친다.
	//        (x+3)까지 주면 x-2~x+2가 칠해져 가운데가 x에 맞는다. (About Ellipses, Rectangle 함수 문서)
	Ellipse(hdc, (int)x - 2, (int)y - 2, (int)x + 3, (int)y + 3);

	ReleaseDC(hwnd, hdc);
}