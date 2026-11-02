# M2-M3 - 점 찍기 / 선 그리기
## 개요
마우스로 클릭한 지점에 점과 선을 그려서 클릭 지점을 표시하는 상태를 만들었다.
- 드로잉 마커 예제를 참고했다.
- 예제에 없는 원(Ellipse)은 클릭 지점을 점으로 더 잘 보이게 하려고 추가했다. 원 그리는 법은 [GDI] Part 2. 작도함수 #1 글의 함수들을 참고했다.

## 소스 코드
`BasicRendering/BasicRendering.cpp`

## GDI 개체
- GDI
  - 화면, 프린터와 같은 모든 출력 장치를 제어하는 인터페이스
- GDI 개체
  - 윈도우 운영체제가 모니터, 프린터 등 다양한 출력 장치에 선, 도형, 텍스트, 이미지 등의 그래픽을 그리기 위해 사용하는 핵심 리소스
- DC
  - GDI 개체가 담기는 곳
  - DC에서 원하는 내용을 변경하여 출력함
  - `GetDC`, `BeginPaint` 등 함수로 DC를 생성해 핸들을 얻어서 개체를 사용

- 종류
  - Pen (펜)
    - 선과 곡선을 그리는 데에 사용할 수 있음
  - Brush (브러시)
    - 폴리곤, 타원, Path의 내부를 그리는 데 사용할 수 있음
  - Font (폰트)
    - 디스플레이 및 기타 출력 디바이스에 텍스트를 그리는 데 사용할 수 있음
  - Bitmap (비트맵)
    - 픽셀 데이터를 담고 있는 이미지 객체를 만들고 조작하며 디스크에 파일로 저장
  - Region (영역)
    - 직사각형, 다각형, 또는 타원 등으로 화면의 특정 클리핑 영역이나 도형 범위를 정의함
    - 적중 횟수 테스트 등을 수행하는 데에 사용 가능
  - Palette (팔레트)
    - 색상표를 정의함

## 클릭에서 그리기까지의 흐름
1. `WM_PAINT`: 첫 그리기로 화면을 그린 뒤 `bCollectPoints`를 켜서 점을 수집할 수 있게 한다.
2. `WM_LBUTTONDOWN`: 클라이언트 영역으로 영역(`HRGN`)을 만들고, `lParam`에서 클릭 좌표를 꺼낸다.
3. `PtInRegion`: 클릭 좌표가 클라이언트 영역 안에 있는지 검사한다.
4. `Marker`: 클릭 지점에 십자선과 원을 그린다.
5. `DeleteObject`: 검사에 쓴 영역을 지운다.

## 좌표 받기: lParam과 MAKEPOINTS
```cpp
ptTmp = MAKEPOINTS(lParam);
ptMouseDown[index].x = (LONG)ptTmp.x;
ptMouseDown[index].y = (LONG)ptTmp.y;
```
- **?** 왜 예제대로 `MAKEPOINTS((POINTS FAR*)lParam)`으로 작성하면 오류가 나는가?
  - 해결: `lParam`을 그대로 넘기면 된다.
- `MAKEPOINTS`는 함수가 아니라 `#define`으로 정의된 전처리 매크로다. 매크로 안에서 이미 `POINTS FAR*`로 변환하므로, 직접 변환하지 않고 `lParam`을 그대로 넘기면 된다.
  - **어긋나는 지점**: "이미 `POINTS FAR*`로 변환"한다는 이해가 매크로 정의의 `&`와 맨 앞 `*`와 다르다.
  - **어긋나는 지점**: 문서의 반환 값 설명이 헤더의 매크로 정의와 다르다.
- 좌표 x, y가 담긴 4바이트 값을 메모리에서 읽어서 낮은 2바이트는 x, 높은 2바이트는 y(`SHORT`)로 나눠 `POINTS` 구조체로 만든다.

> "MAKEPOINTS 매크로는 점의 x 좌표와 y 좌표를 포함하는 값을 POINTS 구조체로 변환합니다." / "x 좌표는 낮은 순서의 단어에 있고 y 좌표는 상위 단어에 있습니다." / "반환 값은 POINTS 구조체에 대한 포인터입니다." (MAKEPOINTS 매크로 문서)
>
> `#define MAKEPOINTS(l) (*((POINTS FAR *)&(l)))` (wingdi.h)

### FAR와 빈 매크로
- 16비트 시절에는 near 포인터로 한 세그먼트 안만 접근할 수 있어서 far 포인터가 필요했다.
- 지금은 `#define far`로 비어 있어서 전처리 후 사라진다. 옛 코드가 그대로 컴파일되도록 남겨 둔 호환용이다. (FAR의 역할은 Claude의 도움을 받아 정리)
- FAR가 쓰인 것으로 보아 이 예제는 16비트 시절 작성 방식이 남아 있는 오래된 예제다.

> `#define FAR far` / `#define far` (minwindef.h)

### 엔디안과 64비트
- 엔디안은 여러 바이트로 된 값을 메모리에 저장하는 바이트 순서다. 리틀 엔디안은 가장 낮은 바이트를, 빅 엔디안은 가장 높은 바이트를 낮은 주소에 먼저 저장한다.
  - 예: `0x12345678` → 리틀 `78 56 34 12`, 빅 `12 34 56 78`
  - Intel/AMD 계열 CPU는 리틀 엔디안, 네트워크와 과거 Mac의 PowerPC는 빅 엔디안을 쓴다.
- x64에서 `LPARAM`은 `__int64`(8바이트)지만, 리틀 엔디안에서는 매크로가 읽는 4바이트가 낮은 쪽 4바이트라서 좌표 값이 그대로 읽힌다. 그래서 64비트에서도 동작한다.

> `typedef LONG_PTR LPARAM;` (wtypes.h) · `typedef __int64 LONG_PTR` (basetsd.h)

### POINTS와 _MAC
- `POINTS`는 `_MAC`일 때 y, x 순서로 선언된다.
- `_MAC`은 Windows PC에서 매킨토시용 프로그램을 교차 컴파일할 때 쓰던 매크로이고, 당시 Mac의 CPU(PowerPC 등)가 빅 엔디안이라 순서가 반대다.

> `#ifndef _MAC SHORT x; SHORT y; #else SHORT y; SHORT x; #endif` (windef.h)

## 메시지 처리: WM_PAINT와 bCollectPoints
```cpp
case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
    }
    if (!bCollectPoints) bCollectPoints = TRUE;
    return 0;
```
- **?** 왜 클릭해도 아무것도 그려지지 않는가?
  - 해결: `bCollectPoints`는 값을 넣지 않은 static 변수라 0(`FALSE`)으로 시작하는데, 값을 바꾸는 곳이 없어서 `WM_LBUTTONDOWN`의 `if` 안으로 들어가지 못했다. 첫 그리기로 화면을 그린 뒤 점을 수집하도록 `WM_PAINT` 끝에서 `TRUE`로 바꾼다.
- 처음에는 이 case 끝에 `return`이나 `break`가 없어서 아래 `WM_LBUTTONDOWN` 코드까지 이어서 실행됐다(fall-through, Claude가 짚음). 그래서 클릭 전에 `bCollectPoints`가 켜지고, 다시 그릴 때마다 (0, 0)에 표시가 찍혔다.
- 메시지를 직접 처리했으면 `return 0`으로 끝내는 게 Win32의 관례다. `break`로 끝내면 `DefWindowProc`이 한 번 더 호출된다.
- `bCollectPoints`는 `BOOL`(int의 별칭)이라 bool의 `true`가 아니라 `TRUE` 매크로를 쓴다.

### fall-through
- switch 문에서 일치하는 case를 실행한 뒤 `break`나 `return` 같은 명시적 종료가 없으면, 다음 case의 명령어까지 이어서 실행되는 현상이다.
- C/C++는 이를 허용한다. C#은 비어 있는 case의 fall-through만 허용하고, 내용이 있는 case는 허용하지 않아 `break` 같은 명시적 종료로 끝내야 한다.

## 그리기: Marker
```cpp
void Marker(LONG x, LONG y, HWND hwnd)
{
    HDC hdc;
    hdc = GetDC(hwnd);

    MoveToEx(hdc, (int)x - 10, (int)y, (LPPOINT)NULL);
    LineTo(hdc, (int)x + 10, (int)y);
    MoveToEx(hdc, (int)x, (int)y - 10, (LPPOINT)NULL);
    LineTo(hdc, (int)x, (int)y + 10);

    Ellipse(hdc, (int)x - 2, (int)y - 2, (int)x + 3, (int)y + 3);

    ReleaseDC(hwnd, hdc);
}
```
- `GetDC`로 `hwnd`로부터 DC를 얻는다.
- `MoveToEx`로 현재 위치를 (x-10, y)로 옮기고, `LineTo`로 현재 지점부터 (x+10, y)까지 직선을 그린다. 세로선도 같은 방식으로 (x, y-10)부터 (x, y+10)까지 그린다.
- `Ellipse`로 클릭 지점으로부터 (x-2, y-2)~(x+3, y+3)까지의 원형을 그린다.

> "원을 그리는 함수는 직사각형과 동일하게 좌상단, 우하단 좌표가 필요합니다." ([GDI] Part 2. 작도함수 #1)

- **?** 왜 (x-2, y-2)부터 (x+2, y+2)까지 그리면 점이 원점 쪽으로 더 치우쳐 보이는가?
  - 해결: `Ellipse`는 경계 사각형의 오른쪽과 아래쪽 가장자리를 제외하고 그린다. (x-2)~(x+2)로 주면 실제로는 x-2~x+1까지 칠해져 가운데가 x-0.5가 되므로 원점 쪽으로 치우친다. (x+3)까지 주면 x-2~x+2가 칠해져 가운데가 x에 맞는다.

> "When the system draws the ellipse, it excludes the right and lower sides if no world transformations are set. Therefore, for any rectangle measuring x units wide by y units high, the associated ellipse measures x1 units wide by y1 units high." (About Ellipses)
>
> "그려지는 사각형은 아래쪽 및 오른쪽 가장자리를 제외합니다." (Rectangle 함수 문서)

## 자원 관리: 영역과 DeleteObject
```cpp
GetClientRect(hwnd, &rc);
hrgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
// ...
if (PtInRegion(hrgn, ptMouseDown[index].x, ptMouseDown[index].y))
{
    Marker(ptMouseDown[index].x, ptMouseDown[index].y, hwnd);
    index++;
}
DeleteObject(hrgn);
```
- 영역은 `PtInRegion` 검사에만 필요하므로, 다 쓰면 바로 `DeleteObject`로 지워 GDI 자원이 쌓이지 않게 한다.
- `DeleteObject`는 사용이 끝난 GDI 개체를 제거하는 함수다. 직접 만든 GDI 개체는 사용이 끝나면 이 함수로 제거해야 하고, 해제하지 않고 반복해서 생성하면 누수가 생긴다.
- 단, `GetStockObject`로 얻은 스톡 개체는 Windows가 기본으로 제공하는 것이라 해제할 필요가 없다.

> "DeleteObject 함수는 논리 펜, 브러시, 글꼴, 비트맵, 지역 또는 팔레트를 삭제하여 개체와 연결된 모든 시스템 리소스를 해제합니다." (DeleteObject 함수 문서)

## 빌드 과정: 선언과 링크
```cpp
void Marker(LONG x, LONG y, HWND hwnd);
```
- **?** 왜 빌드하면 `Marker` 식별자를 찾을 수 없다는 오류가 나는가?
  - 해결: 컴파일러는 소스 파일을 위에서부터 읽는데, `Marker`를 호출하는 줄이 정의보다 먼저 온다. 함수 정의를 호출보다 위로 올리거나, 호출 전에 함수 선언(함수 프로토타입)을 적어 두면 된다.
- 함수 선언은 함수의 이름, 매개변수 타입, 반환 타입을 호출보다 먼저 알려 주는 것이다. 컴파일러는 이것만으로 호출을 검사할 수 있다.
- 컴파일 단계는 선언만 있으면 호출을 검사할 수 있고, 그 선언에 맞는 정의가 실제로 있는지는 링크 단계가 확인한다.

> "컴파일러는 코드가 기호를 선언하지 않는 경우를 식별할 수 있지만, 기호를 정의하지 않을 때는 식별할 수 없습니다." (링커 도구 오류 LNK2001)

### 선언 타입을 바꾼 실험
- 선언의 매개변수 타입을 정의와 다르게(`LONG` → `int`) 바꾸면 C++은 서로 다른 함수로 본다. 이때는 컴파일은 통과하고, 링커가 일치하는 정의를 찾지 못해 LNK2001, LNK1120 오류가 난다.
- 링커는 컴파일과 어셈블 이후 단계에서, 각 개체 파일이 참조하는 모든 외부 기호에 대해 일치하는 정의를 찾는다. 찾지 못하면 LNK 오류를 낸다.
- VCR001은 IntelliSense가 알려 주는 항목이라 빌드를 막는 오류는 아니다.

> "링커는 각 개체 파일에서 참조하는 모든 외부 기호에 대해 일치하는 정의를 확인하거나 찾아야 합니다. 링커는 외부 기호를 확인할 수 없을 때 오류를 생성합니다." (링커 도구 오류 LNK2001)
>
> "LNK1120 메시지가 마지막으로 표시되고 해결되지 않은 기호 오류 수가 표시됩니다." (링커 도구 오류 LNK1120)

## 남은 질문
- **?** 창을 리사이즈한 뒤 그린 표시가 사라지는 이유는? (별도 이슈로 다루기로 함)
  - 해석: (미해결)
- **?** `DeleteObject`로 지우는 순서와 DC에 선택된 개체의 관계는? 누수가 쌓이면 무엇이 먼저 실패하는가?
  - 해석: (미해결)
- **?** `MAKEPOINTS`가 최종적으로 돌려주는 값이 포인터인가? 매크로 인자 이름 `l`과 lvalue 개념은 어떻게 구분하는가?
  - 해석: (미해결)

## 레퍼런스
- 드로잉 마커: https://learn.microsoft.com/ko-kr/windows/win32/gdi/drawing-markers
- MAKEPOINTS 매크로(wingdi.h): https://learn.microsoft.com/ko-kr/windows/win32/api/wingdi/nf-wingdi-makepoints
- Windows SDK 10.0.26100.0 헤더: wingdi.h, windef.h, minwindef.h, wtypes.h, basetsd.h
- [GDI] Part 2. 작도함수 #1: https://luckygg.tistory.com/90
- Ellipse 함수(wingdi.h): https://learn.microsoft.com/ko-kr/windows/win32/api/wingdi/nf-wingdi-ellipse
- About Ellipses: https://learn.microsoft.com/en-us/windows/win32/gdi/about-ellipses
- Rectangle 함수(wingdi.h): https://learn.microsoft.com/ko-kr/windows/win32/api/wingdi/nf-wingdi-rectangle
- DeleteObject 함수(wingdi.h): https://learn.microsoft.com/ko-kr/windows/win32/api/wingdi/nf-wingdi-deleteobject
- 링커 도구 오류 LNK2001: https://learn.microsoft.com/ko-kr/cpp/error-messages/tool-errors/linker-tools-error-lnk2001?view=msvc-170
- 링커 도구 오류 LNK1120: https://learn.microsoft.com/ko-kr/cpp/error-messages/tool-errors/linker-tools-error-lnk1120
- Windows API (Win32 API)의 그래픽 - GDI와 스톡 오브젝트(Stock Object): https://chanos.tistory.com/entry/Windows-API-Win32-API%EC%9D%98-%EA%B7%B8%EB%9E%98%ED%94%BD-GDI%EC%99%80-%EC%8A%A4%ED%86%A1-%EC%98%A4%EB%B8%8C%EC%A0%9D%ED%8A%B8Stock-Object
- Pens: https://learn.microsoft.com/en-us/windows/win32/gdi/pens
- Brushes: https://learn.microsoft.com/en-us/windows/win32/gdi/brushes
- Fonts and Text: https://learn.microsoft.com/en-us/windows/win32/gdi/fonts-and-text
- Bitmaps: https://learn.microsoft.com/en-us/windows/win32/gdi/bitmaps
- Regions: https://learn.microsoft.com/ko-kr/windows/win32/gdi/regions
- 색(Colors): https://learn.microsoft.com/ko-kr/windows/win32/gdi/colors
