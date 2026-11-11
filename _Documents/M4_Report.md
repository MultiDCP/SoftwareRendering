# M4 - 사각형 채우기
## 개요
꼭짓점들로 만든 사각형의 안쪽이 색으로 채워지는 상태를 만들었다.
- 첫 구현은 상수 `POINT` 배열의 꼭짓점 4개를 GDI `Polygon`으로 그리고 채웠다. (커밋 `68045dc`)
- 이후 GDI가 실수 좌표를 지원하지 않아 GDI+로 전환했다. (결정 기록: 렌더링 API를 GDI에서 GDI+로 전환)
- 최종 구현은 정점 버퍼와 인덱스 버퍼로 사각형을 삼각형 2개로 나누고, GDI+ `FillPolygon`으로 채운다. (커밋 `233723e`)

## 소스 코드
`RenderSquare/RenderSquare.cpp`

## 첫 구현: GDI Polygon과 old 오브젝트
```cpp
// ? : old가 따로 존재하는 이유?
oldPen = (HPEN)SelectObject(hdc, myPen);
...
SelectObject(hdc, oldPen);
DeleteObject(myPen);
```
- **?** old가 따로 존재하는 이유?
  - 해결: 사용 중인 GDI 오브젝트는 DeleteObject 함수를 사용해 메모리를 해제할 수 없음. 가령 삭제한다고 해도 디폴트가 삭제되는 문제가 발생. 따라서 DC에 선택된 GDI 오브젝트는 반드시 하나는 존재해야 함.
  - **어긋나는 지점**: "사용 중인 GDI 오브젝트는 DeleteObject로 해제할 수 없음"과 "디폴트가 삭제되는 문제가 발생"은 SelectObject 문서에서 확인되지 않는다.

> (요약) 선택한 개체가 영역이 아니고 함수가 성공하면, 반환 값은 바뀌기 전에 선택돼 있던 개체의 핸들이다. 오류가 나면 NULL이다. (SelectObject 함수 문서, 반환 값)
>
> (요약) 새 개체로 그리기를 마치면 항상 새 개체를 원래의 기본 개체로 바꿔 놓아야 한다. (SelectObject 함수 문서, 설명)

## GDI+ 준비
### Gdiplus.lib 링크
```cpp
#pragma comment (lib,"Gdiplus.lib")
```
- GDI+는 헤더(gdiplus.h)만 포함하면 링크 오류(LNK2019)가 나므로 Gdiplus.lib을 링크해야 한다.
  - **어긋나는 지점**: "헤더만 포함하면 LNK2019가 난다"는 내용은 레퍼런스에 직접 나오지 않는다.

> (요약) 라이브러리 Gdiplus.lib, DLL Gdiplus.dll, 헤더 gdiplusinit.h (Gdiplus.h 포함). (GdiplusStartup 함수 문서, 요구 사항)
>
> `#pragma comment (lib,"Gdiplus.lib")` (Drawing a Line 샘플 코드)

### GdiplusStartup / GdiplusShutdown
```cpp
GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR           gdiplusToken;

// Initialize GDI+.
GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
// ...
GdiplusShutdown(gdiplusToken);
```
- GDI+를 쓰려면 프로그램 시작 시 GdiplusStartup, 종료 시 GdiplusShutdown을 호출해야 한다.
  - **어긋나는 지점**: 내 이해는 "프로그램 시작 시 / 종료 시"인데, 레퍼런스의 기준은 "다른 GDI+ 호출 전 / GDI+ 사용이 끝났을 때"다.
  - **어긋나는 지점**: "Shutdown 전에 GDI+ 개체가 모두 해제돼 있어야 한다"는 조건이 내 이해에 빠져 있다.

> "the required calls to GdiplusStartup and GdiplusShutdown" (Drawing a Line)
>
> (요약) GdiplusStartup의 첫 번째 인자는 토큰을 받는 ULONG_PTR 변수의 주소이고, 이 토큰은 나중에 GdiplusShutdown에 넘긴다. 두 번째 인자는 GdiplusStartupInput 구조체이며, 예제는 그 기본 생성자를 그대로 쓴다. (Drawing a Line, 코드 뒤 마지막 문단)
>
> (요약) GdiplusStartup은 GDI+를 초기화한다. 다른 GDI+ 호출 전에 호출하고, GDI+ 사용을 마치면 GdiplusShutdown을 호출한다. GDI+ 개체를 만들기 전에 GdiplusStartup을 호출해야 하고, GdiplusShutdown을 호출하기 전에 모든 GDI+ 개체를 삭제하거나 범위를 벗어나게 해야 한다. (GdiplusStartup 함수 문서, 요약과 설명)

## 색: Color의 인자 순서
```cpp
#define COLOR_BLACK Color(255, 0, 0, 0)
#define COLOR_WHITE Color(255, 255, 255, 255)
```
- Color의 4인자 순서는 R G B A가 아니라 A R G B다.

> (요약) 지정한 알파, 빨강, 초록, 파랑 값으로 Color 개체를 만든다. 매개변수는 a(알파 성분), r(빨강 성분), g(초록 성분), b(파랑 성분) 순서이고, 각각 해당 성분을 지정하는 바이트다. (Color::Color(BYTE,BYTE,BYTE,BYTE) 문서, 매개 변수)
>
> LuckyGg의 [GDI+] Part 2. 작도함수 '직선 그리기' 절은 코드 주석으로 `Color(255,255,0,0)`을 "Alpha 255, 빨간색"으로 설명한다.

## 배경 칠하기: FillRectangle 오버로드
```cpp
graphics.FillRectangle(&blackBrush, (int)rcClient.left, (int)rcClient.top, WIDTH, HEIGHT);
```
- LONG(rcClient.left, top)과 unsigned(WIDTH, HEIGHT)를 섞어 넘기면 '오버로드된 함수에 대한 호출이 모호합니다' 오류가 난다. 인자를 전부 int로 맞춰서 해결.
  - **어긋나는 지점**: FillRectangle 메서드 문서에는 INT 버전과 REAL 버전 중 무엇이 선택되는지, 왜 모호해지는지가 나와 있지 않다. 원인은 레퍼런스로 확인되지 않은 상태다.

> (요약) 오버로드 목록에 아래 4개가 있고, 모두 "브러시로 사각형 내부를 채운다"는 같은 설명이 붙어 있다. (Graphics.FillRectangle 메서드, 오버로드 목록)
> - `FillRectangle(Brush*,Rect&)`
> - `FillRectangle(Brush*,RectF&)`
> - `FillRectangle(Brush*,INT,INT,INT,INT)`
> - `FillRectangle(Brush*,REAL,REAL,REAL,REAL)`

## 삼각형 채우기: 정점 버퍼, 인덱스 버퍼, FillPolygon
```cpp
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

    // graphics.DrawPolygon(&whitePen, points, 3);
    graphics.FillPolygon(&greenBrush, points, 3);
}
```
- 임시로 내부에 정점 버퍼와 인덱스 버퍼를 배열(정확히는 벡터)로 구현하여 활용
- 정점 버퍼
  - 메모리에 정점 정보를 일렬로 나열해둠
- 인덱스 버퍼
  - 삼각형을 구성하는 정점의 인덱스(순번)를 기록한 배열을 별도 관리
- 인덱스 버퍼의 각 포인트를 이용해서 필요한 삼각형들을 모두 그린다. 이때 삼각형의 좌표는 정점 버퍼에 있는 것을 사용한다.
- DrawPolygon은 내부를 채우지 않는다.
  - **어긋나는 지점**: Graphics::DrawPolygon 문서에는 "내부를 채우지 않는다"는 문장이 직접 나오지 않는다.

> (요약) 브러시를 사용해 다각형의 내부를 채운다. (Graphics::FillPolygon 문서, 요약 문장)
>
> (요약) Draw 계열 메서드는 선을 그리고, Fill 계열 메서드는 도형을 채운다. 사각형의 경우 DrawRectangle은 테두리를, FillRectangle은 내부를 그린다. (hyonie's Stack, 기본적인 도형 그리기 - Graphics 클래스 / 사각형, 원형 그리기) — C# System.Drawing 기준 글

## 남은 질문
- M4 코드 노트에 미해결 `?` 태그는 없다.

## 레퍼런스
- gdi-3d: https://github.com/george7378/gdi-3d
- [GDI+] Part 2. 작도함수: https://luckygg.tistory.com/93
- Eigen - The Matrix class: https://libeigen.gitlab.io/eigen/docs-5.0/group__TutorialMatrixClass.html
- [C#] 그래픽 GDI+ 개념과 간단한 도형 그리기: https://dev-hyonie.tistory.com/23
- Graphics.FillRectangle 메서드: https://learn.microsoft.com/ko-kr/windows/win32/gdiplus/-gdiplus-class-graphics-fillrectangle-methods
- Drawing a Line: https://learn.microsoft.com/en-us/windows/win32/gdiplus/-gdiplus-drawing-a-line-use
- Graphics::FillPolygon: https://learn.microsoft.com/ko-kr/windows/win32/api/gdiplusgraphics/nf-gdiplusgraphics-graphics-fillpolygon(constbrush_constpointf_int)
- Color::Color(BYTE,BYTE,BYTE,BYTE): https://learn.microsoft.com/ko-kr/windows/win32/api/gdipluscolor/nf-gdipluscolor-color-color(byte_byte_byte_byte)
- Graphics::DrawPolygon: https://learn.microsoft.com/ko-kr/windows/win32/api/gdiplusgraphics/nf-gdiplusgraphics-graphics-drawpolygon(constpen_constpointf_int)
- SelectObject 함수(wingdi.h): https://learn.microsoft.com/ko-kr/windows/win32/api/wingdi/nf-wingdi-selectobject
- GdiplusStartup 함수(gdiplusinit.h): https://learn.microsoft.com/ko-kr/windows/win32/api/gdiplusinit/nf-gdiplusinit-gdiplusstartup
- 이득우의 게임 수학: https://www.yes24.com/product/goods/107025224