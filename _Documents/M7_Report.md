# M7 - 백페이스 컬링으로 정육면체 면 채우기
## 개요
정육면체의 면이 색으로 채워지고, 뒤쪽 면이 앞쪽 면에 가려지는 상태를 만들었다.
- 대상을 정점 8개짜리 정육면체에서 면마다 3×3 칸이 있는 루빅스 큐브 형태(정점 96개, 삼각형 108개)로 바꿨다.
- 삼각형마다 뷰 공간에서 법선과 카메라→면 벡터를 내적해 앞면만 그린다. (결정 기록: M7 백페이스 컬링 판단을 뷰 공간에서 수행)
- 면마다 다른 색으로 `FillPolygon`을 호출해 채운다.
- 쓰이지 않던 메모리 DC를 실제로 쓰도록 고쳐서 회전할 때의 깜빡임을 없앴다.

## 소스 코드
`RubiksCube/CubeMain.cpp`, `RubiksCube/RubiksCubeMesh.h`, `RubiksCube/Transition.h`

## 1. 루빅스 큐브 정점 데이터
- 정점 좌표와 인덱스는 요청해서 Claude가 생성했다(`RubiksCubeMesh.h`).
- 범위는 기존 정육면체와 같은 −1~1이고, 칸 경계는 ±1/3(상수 `T`)이다.
- 면 하나는 4×4 격자 정점 16개로 사각형 9개(삼각형 18개)를 만든다. 면 6개를 합쳐 정점 96개, 삼각형 108개다.
- 면 순서는 기존 `indexBuffer`와 같다: z = −1, z = +1, x = −1, x = +1, y = −1, y = +1.
- 면 f의 i번째 사각형(0~8)은 `rubiksIndexBuffer[f*18 + i*2]`, `[f*18 + i*2 + 1]` 두 삼각형이다. 그래서 삼각형 번호를 18로 나누면 면 번호가 나온다.
- 칸 사이의 검은 틈을 표현하는 문제는 보류했다.

## 2. 감김 방향과 법선
- 삼각형의 앞면은 인덱스 버퍼에 적힌 정점 순서(감김 방향)로 정해진다.
- 세 점으로 v0→v1, v0→v2 벡터를 만들고 외적하면 법선이 나온다. 외적 순서를 바꾸면 법선의 방향이 반대가 된다.
- 프로젝트는 오른손 좌표계다. z = +1 면을 큐브 바깥에서 −z 방향으로 바라보면 정점이 반시계 방향으로 감기고, 안에서 보면 시계 방향으로 감긴다.
- 그래서 지금 데이터에서 (v1 − v0) × (v2 − v0)으로 구한 법선은 큐브 바깥을 향한다.

## 3. 판단 공간과 카메라→면 벡터
- 처음에는 카메라의 시선 벡터(−z) 하나와 면의 법선만 있으면 된다고 생각했다.
- *Claude 설명*: 고정된 시선 방향으로만 판단하는 방식은 직교 투영에서만 맞는다. 원근 투영에서는 화면 가장자리에 있는 면일수록 "카메라에서 면을 향한 방향"이 시선 방향과 달라진다. 뷰 공간에서는 카메라가 원점에 있으므로 카메라→면 벡터는 v0 그대로다.
  - 예: 뷰 공간의 점 B (4, 0, −1)에 법선이 (1, 0, 0)인 면이 있으면, 시선 방향 (0, 0, −1)과의 내적은 0이고, 카메라→B 벡터와의 내적은 4 > 0이라 뒷면으로 걸러진다.
- 판단 공간은 뷰 공간으로 정했다. 현재 구성에서는 정점이 카메라 뒤로 갈 일이 없지만, 교재에서 후면 제거를 시점 공간에서 절단 공간으로 넘어가는 시점에 수행하는 것으로 이해했고 이 흐름을 따랐다. (「이득우의 게임 수학」 11장, 「OpenGL로 배우는 3차원 컴퓨터 그래픽스」 8장)
- *Claude 설명 (화면 공간에서 판단할 경우)*: 2D 외적의 z성분 (x1 − x0)(y2 − y0) − (y1 − y0)(x2 − x0)을 쓴다. 뷰포트에서 y를 뒤집으므로 바깥에서 반시계였던 앞면은 픽셀 좌표에서 시계 방향이 되어 이 값이 0보다 작다. 정점이 카메라 뒤로 가면(w ≤ 0) 이 판단이 틀어진다.

## 4. 백페이스 컬링 구현
```cpp
// 백페이스 컬링을 위해 뷰 공간 변환 행렬을 따로 만들어둔다.
Matrix4f viewTransMatrix = viewMatrix * worldMatrix;

// 뷰 공간에서의 정점 좌표를 따로 계산하여 저장한다.
for (int i = 0; i < rubiksVertexBuffer.size(); i++)
{
	Vector4f newV = viewTransMatrix * rubiksVertexBuffer[i];
	viewVertex.push_back(newV);
}

for (int i = 0; i < rubiksIndexBuffer.size(); i++)
{
	Vector4f v0 = viewVertex[rubiksIndexBuffer[i](0)];
	Vector4f v1 = viewVertex[rubiksIndexBuffer[i](1)];
	Vector4f v2 = viewVertex[rubiksIndexBuffer[i](2)];

	Vector4f v1_0 = v1 - v0;
	Vector4f v2_0 = v2 - v0;

	Vector4f n = v1_0.cross3(v2_0);

	// 앞면인 경우 렌더링을 수행하며, 아닐 경우 렌더하지 않는다.
	n.dot(v0) < 0 ? isRenderTarget.push_back(true) : isRenderTarget.push_back(false);
}
```
### 뷰 공간 좌표 버퍼와 인덱스 버퍼
- 뷰 공간 좌표를 변환과 동시에 판단할 필요 없이, 버퍼(`viewVertex`)에 따로 담아 두고 나중에 인덱스 버퍼로 정점 세 개씩 꺼내 계산하면 된다. `main_pixel_buffer`와 같은 구조다.
- 컬링 판단 단위는 삼각형이다.
- *Claude 설명*: 정점 번호 k는 `rubiksVertexBuffer`, `viewVertex`, `main_pixel_buffer`에서 모두 같은 정점을 가리킨다. 삼각형 번호 t로 `rubiksIndexBuffer[t]`를 꺼내면 정점 번호 세 개가 나오고, 그 번호로 각 버퍼에서 값을 꺼낸다.
- 판단 결과는 삼각형 번호 순서대로 `isRenderTarget`에 모아 두고, 그리는 루프에서 같은 번호로 확인해 앞면만 `FillPolygon`으로 채운다.

### 판정식
- n · v0 < 0이면 앞면으로 보고 그린다. 0 이상이면 그리지 않는다.

### cross와 cross3
- `cross`를 `Vector4f`에 쓸 수 없어서 `cross3`을 썼다.
- 내 이해: `cross`는 크기가 3인 벡터에서만 쓸 수 있다. `cross3`은 `Vector4f`도 쓸 수 있으며, 앞의 세 성분으로만 외적을 계산한다.

> (요약) `cross()`는 고정 크기 2인 벡터끼리와 고정 크기 3인 벡터끼리 두 경우에 구현되어 있다. 크기 3이면 일반적인 외적을, 크기 2이면 스칼라를 돌려준다. (Eigen 문서, Geometry 모듈, cross())
>
> (요약) `cross3()`은 x, y, z 성분만 써서 외적을 구하고, `*this`와 상대 벡터의 크기는 4여야 한다. 3D 대신 4D 벡터를 써서 SSE/AltiVec 벡터화의 이점을 얻을 때 특히 유용하다. (Eigen 문서, Geometry 모듈, cross3())

- 어긋나는 지점: 문서에서는 `cross()`가 크기 2인 벡터에도 구현되어 있다.
- *Claude 설명*: `cross3` 결과의 w는 0이라서, w가 1인 v0와 4성분 그대로 내적해도 w가 결과에 섞이지 않는다. 문서에는 4번째 성분 값이 따로 적혀 있지 않다.

### 화면 좌표 계산
```cpp
main_pixel_buffer.clear();
for (int i = 0; i < rubiksVertexBuffer.size(); i++)
{
	main_pixel_buffer.push_back(ProjectTransition(viewVertex[i], mainViewport, projMatrix));
}
```
- 이미 구한 `viewVertex`에 `projMatrix`만 곱해 화면 좌표를 만든다. 처음 구현에서는 `projMatrix * viewTransMatrix`를 원본 정점에 다시 곱했는데, 결정 기록의 '영향'에 맞춰 바꿨다. (결정 기록: M7 백페이스 컬링 판단을 뷰 공간에서 수행)

## 5. 더블 버퍼링
```cpp
// 메모리 DC 생성
HDC hdcMem = CreateCompatibleDC(hdc);
const int nMemDC = SaveDC(hdcMem);

// 호환 비트맵 생성
HBITMAP hBitmap = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);

// 메모리 DC에 비트맵 선택
SelectObject(hdcMem, hBitmap);

// 이후 메모리 DC에 배경 및 그림 그리기
Graphics graphics(hdcMem);

// ... 그리기 ...

// 메모리 DC 내용을 실제 화면 DC로 복사
RECT rcClip;
GetClipBox(hdc, &rcClip);
BitBlt(hdc, rcClip.left, rcClip.top, rcClip.right - rcClip.left, rcClip.bottom - rcClip.top, hdcMem, rcClip.left, rcClip.top, SRCCOPY);

// 메모리 자원 해제
RestoreDC(hdcMem, nMemDC);
DeleteObject(hBitmap);
DeleteDC(hdcMem);
```
- 기존 코드는 메모리 DC와 비트맵을 만들어 놓고, `Graphics`를 화면 DC(`hdc`)에 연결해서 화면에 바로 그리고 있었다. 메모리 DC는 쓰이지 않은 채 삭제됐다.
- *Claude 설명*: 화면에 바로 그리면 "배경만 칠해진 순간"이나 "삼각형이 일부만 그려진 순간"이 보일 수 있고, 이 중간 상태가 깜빡임으로 나타날 수 있다.
- 레퍼런스(gdi-3d)에서 가져온 메모리 DC 코드가 왜 쓰이는지 처음에는 이해하지 못했다. 더블 버퍼링 개념을 알고 나서 쓰임을 이해했고, 레퍼런스 코드를 다시 확인해서 `Graphics`를 메모리 DC에 연결하고, 다 그린 뒤 `BitBlt`로 화면 DC에 복사하도록 채워 넣었다. M6·M8에서 남은 질문이던 깜빡임이 없어졌다.

> (요약) `CreateCompatibleDC`는 지정한 장치와 호환되는 메모리 DC를 만든다. 메모리 DC는 메모리에만 있고, 처음 만들어졌을 때 표시 영역이 단색 1×1 픽셀이라서 그리기 전에 알맞은 크기의 비트맵을 선택해 넣어야 한다. (Microsoft Learn, CreateCompatibleDC, Remarks)
>
> (요약) `CreateCompatibleBitmap`은 지정한 DC에 연결된 장치와 호환되는 비트맵을 만든다. (Microsoft Learn, CreateCompatibleBitmap)
>
> (요약) `BitBlt`는 원본 DC의 사각형 픽셀 색 데이터를 대상 DC로 옮긴다. x, y는 대상 사각형의 왼쪽 위 좌표, cx, cy는 원본·대상 사각형의 너비와 높이, x1, y1은 원본 사각형의 왼쪽 위 좌표다. (Microsoft Learn, BitBlt, Parameters)

## 6. 디버깅 기록
- **`cross`를 `Vector4f`에 쓰면 컴파일 오류**: `cross3`을 쓰거나 `head<3>()`로 앞 세 성분만 잘라 쓰는 방법이 있다는 것을 확인하고 `cross3`을 썼다.
- **더블 버퍼링 적용 후 검은 화면만 나옴**: `BitBlt`의 대상 y 좌표 자리에 `rcClip.top` 대신 `rcClip.right`를 넘기고 있었다. 인자를 고쳐 해결했다.

## 남은 질문
- 칸 사이의 검은 틈을 지금 정점 구조로 표현할 수 있는지, 칸마다 정점을 따로 가져야 하는지 (보류)
- M9처럼 조각 27개가 서로 겹칠 때도 백페이스 컬링만으로 충분한지
- M7 코드 노트에 미해결 `?` 태그는 없다.

## 레퍼런스
- gdi-3d main.cpp: https://github.com/george7378/gdi-3d/blob/master/source/GDI%2B%203D/main.cpp
- 「이득우의 게임 수학」 11장: https://www.yes24.com/product/goods/107025224
- 「OpenGL로 배우는 3차원 컴퓨터 그래픽스」 8장
- Eigen 문서 - cross() (Geometry 모듈): https://libeigen.gitlab.io/eigen/docs-nightly/group__Geometry__Module.html#gac32d7ca309f8c0ef9ae04172d49a88e6
- Eigen 문서 - cross3() (Geometry 모듈): https://libeigen.gitlab.io/eigen/docs-nightly/group__Geometry__Module.html#gabde56e2a0baba550815a0b05139e4d42
- Microsoft Learn - CreateCompatibleDC: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createcompatibledc
- Microsoft Learn - CreateCompatibleBitmap: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createcompatiblebitmap
- Microsoft Learn - BitBlt: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt
