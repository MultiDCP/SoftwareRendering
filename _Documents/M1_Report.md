# M1 - 창 띄우기
## 개요
Win32 창이 열리고, 닫으면 프로그램이 정상 종료되는 상태를 만들었다.
다큐멘테이션을 보고 구현한 다음 분석해 보는 탑다운 형태로 학습했다.

## 소스 코드
`HelloWindows/HelloWindowsDesktop.cpp`

## 애플리케이션 창
가장 기본적인 프레임의 형태를 띠는 창을 '애플리케이션 창' 또는 '주 창'이라고 부른다.
프레임의 영역은 다음과 같이 구분된다.
- 비클라이언트 영역
    - 제목 표시줄, 최소화, 최대화 버튼 및 기타 표준 UI 요소로 구성
    - 운영체제가 창의 해당 부분을 관리함
- 클라이언트 영역
    - 프레임 내의 영역
    - 프로그램이 관리하는 창의 일부

창을 다음과 같은 프로그래밍 구성 요소로 생각하는 것이 좋다.
- 화면의 특정 부분을 차지함
- 특정 시점에 표시될 수도, 표시되지 않을 수도 있다
- 자신을 그리는 방법을 알고 있다
- 사용자 또는 운영체제의 이벤트에 응답한다

애플리케이션 창은 대화 상자 창을 소유하며, 대화 상자 창은 버튼 창의 부모이다.

좌표는 장치 독립적 픽셀로 측정된다.
작업에 따라 다음과 같은 기준으로 좌표를 측정할 수 있다.
- 화면 기준 (Screen Coordinates)
- 창(프레임 포함) 기준 (Window Coordinates)
- 창의 클라이언트 영역 기준 (Client Coordinates)

## WinMain / wWinMain
- 창 애플리케이션의 진입점 함수. 명령줄 인자를 유니코드로 받을 때는 `wWinMain`, ANSI 문자열로 받을 때는 `WinMain`을 사용한다. 나는 유니코드를 사용하므로 `wWinMain`을 쓴다.
- 링커의 하위 시스템이 `콘솔(/SUBSYSTEM:CONSOLE)`로 지정되어 있으면 진입점을 `wWinMain`이 아니라 `main`으로 잡아 `main` 확인할 수 없는 외부 기호 오류가 발생한다. 프로젝트 속성 - 구성 속성 - 링커 - 시스템의 하위 시스템을 `창(/SUBSYSTEM:WINDOWS)`으로 지정해 해결했다.

## 창 클래스
```cpp
wc.lpfnWndProc = WindowProc;    // 창 프로시저
wc.hInstance = hInstance;       // 애플리케이션 인스턴스 핸들
wc.lpszClassName = CLASS_NAME;  // 창 클래스 식별 문자열

RegisterClass(&wc);             // OS에 창 클래스 등록
```
- `WNDCLASS`의 세 멤버는 각각 창 프로시저, 애플리케이션 인스턴스 핸들, 창 클래스 식별 문자열이다.
- `RegisterClass`로 OS에 창 클래스를 등록한다.

## 창 만들기
```cpp
HWND hwnd = CreateWindowEx(
    0,                              // Optional window styles.
    CLASS_NAME,                     // Window class
    L"Learn to Program Windows",    // Window text (창 이름이 된다)
    WS_OVERLAPPEDWINDOW,            // Window style
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    NULL, NULL, hInstance, NULL
);
```
- 세 번째 인자인 Window text는 창 이름이 된다.

## 창 메시지
```cpp
MSG msg = { };
while (GetMessage(&msg, NULL, 0, 0) > 0) // 큐에서 메세지를 끌어옴
{
    TranslateMessage(&msg);
    DispatchMessage(&msg); // 메세지의 대상인 창의 프로시저를 호출하도록 지시
}
```
- 애플리케이션은 실행되는 동안 수천 개의 메시지를 받고, 각 앱은 날아오는 수천 개의 메시지 중 자기 것을 검색해 올바른 창으로 디스패치하는 루프가 필요하다.
  - **어긋나는 지점**: "날아오는 수천 개의 메시지 중 자기 것을 검색"한다는 부분이 레퍼런스와 다르다.
- `GetMessage`는 큐에서 메시지를 끌어온다.
- `DispatchMessage`는 메시지의 대상인 창의 프로시저를 호출하도록 지시한다.

## 창 프로시저 작성
```cpp
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        // WM_CLOSE, WM_DESTROY, WM_PAINT 처리
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
```
- `uMsg`는 메시지 코드이다. `wParam`과 `lParam`을 통해 함수가 관련된 추가 데이터를 받을 수 있다.
- 특정 메시지를 처리하지 않는 경우 `DefWindowProc`로 기본 작업을 수행한다.

## 창 그리기
```cpp
case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
    }
    return 0;
```
- `FillRect`는 GDI 함수이다. 두 번째 매개변수에서 채울 사각형의 좌표를 제공하며, 후속 `WM_PAINT` 메시지에서는 더 작은 사각형을 포함할 수 있다.
- **?** 그렇다면 모든 드로우 과정은 `BeginPaint`와 `EndPaint` 사이에서 이루어지는가?
  - 해석: (미해결)
- **?** 새로운 것을 그릴 때 함수 작성 순서대로 렌더링되는가? (GDI 학습 때 더 자세히 볼 수 있을 것 같다.)
  - 해석: (미해결)

## 창 닫기
```cpp
case WM_CLOSE:
    if (MessageBox(hwnd, L"Really quit?", L"My application", MB_OKCANCEL) == IDOK)
    {
        DestroyWindow(hwnd);
    }
    return 0;

case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
```
- **?** 바로 꺼지게 하려면 메시지 박스 없이 `DestroyWindow`만 호출해도 되는가?
  - 해결: 그렇다. `MessageBox`를 호출하지 않고 바로 `DestroyWindow`를 호출하면 된다.

## 레퍼런스
- https://learn.microsoft.com/ko-kr/windows/win32/learnwin32/learn-to-program-for-windows
- https://chanos.tistory.com/entry/Windows-API-WIN32-%ED%94%84%EB%A1%9C%EA%B7%B8%EB%9E%98%EB%B0%8D%EC%9D%98-%EA%B5%AC%EC%A1%B0%EC%99%80-%EC%9C%88%EB%8F%84%EC%9A%B0-%ED%94%84%EB%A1%9C%EC%A0%9D%ED%8A%B8-%EC%83%9D%EC%84%B1
- 창 만들기: https://learn.microsoft.com/ko-kr/windows/win32/learnwin32/creating-a-window
- 창 메시지: https://learn.microsoft.com/ko-kr/windows/win32/learnwin32/window-messages
- 창 그리기: https://learn.microsoft.com/ko-kr/windows/win32/learnwin32/painting-the-window
- 창 닫기: https://learn.microsoft.com/ko-kr/windows/win32/learnwin32/closing-the-window
- 창 프로시저 작성: https://learn.microsoft.com/ko-kr/windows/win32/learnwin32/writing-the-window-procedure
