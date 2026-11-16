# M6 & M8 - 정육면체 원근 렌더링과 키 입력 회전
## 개요
정육면체의 뼈대(선)가 원근감 있게 보이고(M6), 키를 입력하면 화면이 갱신되어 정육면체가 회전하는(M8) 상태를 만들었다.
- 변환 흐름(월드 → 뷰 → 투영 → 뷰포트)을 한 번에 확인하기 위해 M6과 M8을 함께 진행했다. (결정 기록: 카메라를 고정하고 큐브를 회전시킨다)
- 레퍼런스(gdi-3d)의 구조를 참고하되, 수학은 Eigen으로 직접 구현했다. 벡터 규약과 좌표계는 레퍼런스와 다르게 정했다.
- 최종 구현은 정점마다 `proj · view · world` 합성 행렬을 곱하고, w로 나눈 뒤, 뷰포트 변환으로 화면 좌표를 만든다. 방향키로 X·Y축 회전, Enter로 초기화가 된다.
- `RubiksCube` 프로젝트는 커밋 `15f7019`에서 추가했다.

## 소스 코드
`RubiksCube/CubeMain.cpp`, `RubiksCube/Transition.h`

## 1. 좌표계와 벡터 규약
### 벡터 규약
- Eigen이 열 벡터 규약을 쓰므로 Eigen의 방식을 따른다. (결정 기록: 벡터 규약으로 Eigen의 열 벡터 규약을 따른다)
- 레퍼런스는 행 벡터 규약이라, 레퍼런스의 수식을 가져올 때만 전치해서 열 벡터 형태로 바꾼다.
  - (A·v)ᵀ = vᵀ·Aᵀ
  - (W·V·P)ᵀ = Pᵀ·Vᵀ·Wᵀ. 행렬 여러 개를 곱할 때는 각 행렬을 전치하고 곱하는 순서도 뒤집는다.
  - 변환된 좌표 값은 같다. 처음부터 열 벡터 기준으로 만드는 행렬은 전치할 필요가 없다.

### 좌표계 확인 과정
- GDI+ 화면 좌표계는 원점이 왼쪽 위이고, x는 오른쪽, y는 아래쪽을 향한다.
- Eigen의 Rotation2D와 AngleAxis(z축 회전)를 GDI+ 화면에 그려 보면, 양의 각도가 내 시선에서 시계 방향으로 돈다.
- 오른손 규칙: 오른손 손가락을 x에서 y 쪽으로 감으면 엄지가 z를 가리킨다. GDI+ 축에 적용하면 z는 모니터 안쪽을 향하고, 모니터 안쪽을 바라볼 때 손가락은 시계 방향으로 감긴다. 위의 관찰과 일치한다.
- Eigen `cross`는 외적의 표준 정의를 따른다.
- x축과 y축이 정해지면 z축 방향을 정하는 규칙이 법선 벡터와 같은 얘기인지 물었다.
  - *Claude 설명*: z축이 x축, y축 둘 다와 수직이라는 점은 법선 벡터와 같다. 다만 수직인 방향은 서로 반대인 두 방향이고, 그중 하나를 고르는 규칙이 오른손·왼손 규칙이다.
- x 오른쪽, y 위에 오른손 규칙을 적용하면 z(엄지)가 화면 밖을 향하므로, 화면 안쪽을 보는 카메라는 −z를 바라본다.
- 결정: 오른손 좌표계, y축 위, 카메라는 −z를 바라본다. 화면의 y축 뒤집기는 뷰포트 변환에서 처리한다. (결정 기록: 3D 좌표계를 오른손 좌표계, y축 위로 정한다)
  - y축을 위로 둔 이유는 3D 모델을 보기에 편하다는 개인 선호다.

> "Construct a 2D counter clock wise rotation from the angle a in radian." (Eigen 문서, Rotation2D 생성자)

### NDC 깊이 범위
- NDC z 범위는 OpenGL 계열처럼 −1~1로 정했다. 참고 교재와 같은 관례를 따라 교재의 수식을 그대로 참고할 수 있게 하기 위해서다. (결정 기록: NDC 깊이(z) 범위를 −1~1로 정한다)
- *Claude 설명 (NDC의 손잡이)*: 우리 구성에서 NDC는 왼손 좌표계가 된다. 원근 투영 행렬이 near를 −1, far를 +1로 보내므로 NDC에서는 멀수록 z가 커지고 +z가 화면 안쪽을 향한다. 투영 행렬에는 z 방향을 뒤집는 성분이 들어 있고, 축 하나를 뒤집는 변환(반사)은 손잡이를 바꾼다.

| 좌표계 | 손잡이 | 축 방향 |
|---|---|---|
| 월드, 시점 | 오른손 | y 위, 카메라는 −z를 봄 |
| NDC | 왼손 | y 위, +z는 화면 안쪽(멀수록 큼) |
| 화면(GDI+) + 깊이 | 다시 오른손 | 뷰포트에서 y를 뒤집어 x 오른쪽, y 아래, z 화면 안쪽 |

## 2. LookAt
```cpp
Matrix4f LootAtTransition(Vector3f eye, Vector3f at, Vector3f up)
{
	// normalize : 직접 수정, normalized : 새 객체 반환
	Vector3f zaxis = Vector3f(eye - at).normalized();
	Vector3f xaxis = up.cross(zaxis).normalized();
	Vector3f yaxis = zaxis.cross(xaxis);

	return Matrix4f
	{
		{xaxis(0), xaxis(1), xaxis(2), -(xaxis.dot(eye))},
		{yaxis(0), yaxis(1), yaxis(2), -(yaxis.dot(eye))},
		{zaxis(0), zaxis(1), zaxis(2), -(zaxis.dot(eye))},
		{ 0, 0, 0, 1 }
	};
}
```
- 입력: 카메라 위치 eye, 바라보는 지점 at(원점), 월드 상향 벡터 up
- z = normalize(eye − at). at − eye를 z축으로 두면 카메라가 바라보는 방향도 +z가 되므로, 반대 방향인 eye − at이 z축이 되어야 카메라가 −z를 바라본다.
- x = normalize(up × z)
- y = z × x. z와 x가 서로 수직인 단위 벡터라 정규화하지 않아도 된다.
- y축은 상향 벡터를 시선에 수직인 평면에 투상한 방향이다. (「OpenGL로 배우는 3차원 컴퓨터 그래픽스」)
  - 처음에는 이 때문에 축을 정의하는 순서가 바뀌어야 한다고 생각했지만, 교재의 계산 과정을 보니 레퍼런스와 순서는 같았다.
- *Claude 설명*: up × z는 카메라의 오른쪽(+x)을 향한다. 예: eye = (0, 0, 5), at = 원점, up = (0, 1, 0)이면 z = (0, 0, 1), up × z = (1, 0, 0). 레퍼런스(LH)는 z = at − eye를 쓰므로, 같은 식을 오른손 좌표계에서 쓰면 왼쪽(−1, 0, 0)이 나온다.
- 결과 행렬은 직접 연산해서 확인했다.

### up이 시선 방향과 평행한 경우
- up이 시선 방향(eye − at)과 평행하면 up × z가 영벡터가 된다. 카메라가 바라보는 지점의 바로 위나 바로 아래에 있을 때다.
- 레퍼런스처럼 카메라를 (−8, 0, 0)에 고정하고 큐브를 회전시키기로 해서, 시선 방향이 항상 x축 위에 있고 이 경우는 생기지 않는다. (결정 기록: 카메라를 고정하고 큐브를 회전시킨다)

### normalize와 normalized
- normalize는 직접 수정하고, normalized는 새 객체를 반환한다.

> "Normalizes the vector, i.e. divides it by its own norm." (Eigen 문서, MatrixBase::normalize())
>
> (요약) 벡터를 자기 노름으로 나눠 그 자리에서 바꾼다. 반환형은 void다. 노름이 0이면 `*this`를 바꾸지 않고 그대로 둔다. (Eigen 문서, MatrixBase::normalize())
>
> "Returns an expression of the quotient of `*this` by its own norm." (Eigen 문서, MatrixBase::normalized())
>
> (요약) 원래 벡터는 바꾸지 않는다. 노름이 0이면 입력의 복사본을 돌려준다. (Eigen 문서, MatrixBase::normalized())

## 3. 원근 투영
```cpp
Matrix4f PerspectiveFovTransition(float fovy, float aspect, float zn, float zf)
{
	float yScale = 1 / tan(fovy / 2);
	float xScale = yScale / aspect;

	return Matrix4f
	{
		{xScale, 0, 0, 0},
		{0, yScale, 0, 0},
		{0, 0, (zn + zf) / (zn - zf), (2 * zn * zf) / (zn - zf)},
		{0, 0, -1, 0}
	};
}
```
```cpp
projMatrix = PerspectiveFovTransition(DegToRad(60), float(WIDTH) / float(HEIGHT), 1, 10);
```
### 투상면으로 이해하기
- 카메라 앞에 있는 점의 z는 음수다. 카메라에서 멀어질수록 z는 작아지고 −z는 커진다.
- 투상면 z = −d에 투상하면 동차 좌표는 P′ = [x, y, z, −z/d]이다.
- 마지막 값으로 나누면 x′ = x·d / (−z), y′ = y·d / (−z), z′ = −d가 된다.
- 멀어질수록 −z가 커지고, 그 값으로 나누니까 작아 보인다. z로 그대로 나누면 x′, y′의 부호가 뒤집힌다.
- 검증: d = 1, P = (1, 0, −2)이면 [1, 0, −2, 2]이고, 나누면 (0.5, 0, −1)이 된다. 처음에 생각한 [x, y, −z, z/d]로 계산하면 x′ = −0.5가 나와서 틀린 형태임을 확인했다.

### near, far
- near, far는 가시 부피(절두체)의 앞뒤 경계를 정해서 그릴 범위를 제한한다.
- *Claude 설명*: z′를 −d 하나로 뭉개지 않고 따로 계산하는 이유는 깊이 정보를 남기기 위해서다. 깊이 정보가 없으면 M7에서 어느 면이 앞에 있는지 판단할 수 없다. 원근 투영 행렬은 near~far의 깊이를 정해진 범위로 옮긴다(OpenGL −1~1, D3D 0~1). 앞뒤 순서는 유지되지만 깊이 간격은 일정하지 않아서, 가까운 쪽은 촘촘하고 먼 쪽은 성기다.

### 행렬
- FOV, 종횡비, near, far로 원근 투영 행렬을 만드는 식의 유도와 증명은 「이득우의 게임 수학」을 따랐다(열 벡터 규약, 오른손 좌표계, NDC z 범위 −1~1 기준).
- *Claude 설명 (레퍼런스와 원근 투영 행렬 비교)*: 레퍼런스의 행렬을 전치해서 열 벡터 형태로 놓고 비교하면, 차이는 손잡이(LH/RH)와 NDC 깊이 범위(0~1, −1~1) 두 가지에서 온다. n = near, f = far, s_y = 1/tan(fov/2), s_x = s_y/aspect.

| 행 | 레퍼런스 (LH, 깊이 0~1) | 우리 (RH, 깊이 −1~1) | 차이의 원인 |
|---|---|---|---|
| 1 | s_x, 0, 0, 0 | s_x, 0, 0, 0 | 없음 |
| 2 | 0, s_y, 0, 0 | 0, s_y, 0, 0 | 없음 |
| 3 | 0, 0, f/(f−n), −nf/(f−n) | 0, 0, (n+f)/(n−f), 2nf/(n−f) | 손잡이 + 깊이 범위 |
| 4 | 0, 0, 1, 0 | 0, 0, −1, 0 | 손잡이 |

  - 4행: LH는 카메라 앞의 z가 양수라서 w = z, RH는 음수라서 w = −z다.
  - 3행: z_ndc = (A·z + B) / w의 A, B를 정한다. 레퍼런스는 z = n → 0, z = f → 1이고, 우리는 z = −n → −1, z = −f → +1이다.
  - 검증: 우리 식에 z = −n을 넣으면 −n / n = −1, z = −f를 넣으면 f / f = +1이다.

## 4. 뷰포트 변환
```cpp
struct Viewport
{
	unsigned x, y;
	unsigned w, h;
	// ...
};
```
```cpp
return Vector4f(
	vp.x + (1 + projectedPos(0)) * vp.w / 2,
	vp.y + (1 - projectedPos(1)) * vp.h / 2,
	projectedPos(2) * 0.5f + 0.5f,
	1
);
```
- NDC의 x, y 범위는 −1~1이고(절두체 안의 점 기준), 화면 범위는 x가 0~WIDTH, y가 0~HEIGHT다.
- *Claude 설명*: x_screen = vp.x + (x_ndc + 1) · W / 2. +1로 이동하고, W/2를 곱해 크기를 맞추고, vp.x만큼 오프셋을 더한다.
  - 처음에는 크기 조절과 뷰포트 오프셋만 생각했는데, NDC는 가운데가 0이고 화면은 왼쪽 끝이 0이라서 이동이 한 번 더 필요했다.
- y_screen = vp.y + (1 − y_ndc) · H / 2. NDC −1은 화면 맨 아래(vp.y + H), 1은 맨 위(vp.y)로 가야 하므로, y_ndc = −1일 때 곱해지는 값이 2가 되도록 1 − y_ndc로 만들었다.
- z는 크기를 ½로 줄인 뒤 ½만큼 이동해서 −1~1을 0~1로 옮긴다(z·½ + ½). 순서를 바꾸면 (z + ½)·½이 되어 −¼~¾이 나온다. OpenGL처럼 뷰포트에서 깊이를 0~1로 옮기는 관례를 따른다. (결정 기록: 뷰포트에서 z를 0~1로 다시 옮긴다)
- 레퍼런스의 Viewport 구조체를 가져오되, minz와 maxz는 0과 1이면 결과에 영향이 없으므로 지웠다.
- *Claude 설명*: OpenGL은 뷰포트에서 z_ndc를 깊이 범위(기본 0~1)로 다시 옮긴다. 하드웨어 깊이 버퍼의 저장 형식 때문이고, 앞뒤를 비교할 때는 −1~1을 그대로 써도 결과가 같다.

## 5. 변환 파이프라인과 Project
```cpp
// 아핀 변환 -> T * R * S * V
Transform<float, 3, Affine> t = Translation<float, 3>(0, 0, 0) * (AngleAxisf(objRotationY, Vector3f::UnitY()) * AngleAxisf(objRotationX, Vector3f::UnitX())) * Scaling(1.f);
worldMatrix = t.matrix();

Matrix4f multMatrix = projMatrix * viewMatrix * worldMatrix;
main_pixel_buffer.clear();
for (int i = 0; i < vertexBuffer.size(); i++)
{
	main_pixel_buffer.push_back(ProjectTransition(vertexBuffer[i], mainViewport, multMatrix));
}
```
```cpp
Vector4f ProjectTransition(Vector4f v, Viewport vp, Matrix4f multMatrix)
{
	Vector4f projectedPos = multMatrix * v;

	projectedPos = projectedPos / projectedPos(3);
	// 뷰포트 변환 (4절)
}
```
### 월드 변환
- 월드 변환은 M5의 T·R·S 아핀 변환을 3차원으로 넓혔다. 회전은 `AngleAxisf` 두 개를 곱해서 만든다. 두 `AngleAxisf`를 곱하면 합쳐진 회전(`Quaternionf`)이 된다.
- `AngleAxisf(Y) * AngleAxisf(X)`는 정점에 X 회전을 먼저, Y 회전을 나중에 적용한다. 레퍼런스의 `RotationX * RotationY`(행 벡터)와 같은 순서다.
- `t.matrix()`는 4×4 행렬을 돌려준다. `Affine` 모드라 마지막 행은 (0, 0, 0, 1)이다. (`AffineCompact` 모드면 3×4)

### 동차 좌표
- Eigen 문서에서 "Likewise, when applying the transform to points, the latters are automatically promoted to homogeneous vectors before doing the matrix product."라는 문장을 찾았다. (Eigen 문서, Transform 클래스, Detailed Description)
- *Claude 설명*: 이 자동 승격은 `Transform`에 점을 곱할 때만 적용되고, `Matrix4f * Vector3f`는 크기가 맞지 않아 컴파일되지 않는다. 또 `Transform`은 1을 붙여 곱한 뒤 앞의 3개 성분만 돌려주므로 w가 버려진다. 월드 변환은 아핀이라 w가 항상 1이어서 문제가 없지만, 투영을 거치면 w가 −z가 되므로 w를 버리면 안 된다.
- `t.matrix()`로 4×4가 된 건 변환(행렬)이고, 점은 따로 4개짜리 값으로 맞춰야 한다.
- "1 붙이기 → 4×4 곱하기 → w로 나누기"를 누군가는 해야 한다. 레퍼런스는 3차원 정점을 함수 안에서 처리하고, 나는 정점 버퍼에 미리 w = 1을 붙이기로 했다.

### Project 단계의 결정
- 정점 버퍼에 미리 w = 1을 붙인다. 변환할 때마다 동차 좌표로 늘리는 연산을 따로 하지 않아도 된다.
- proj·view·world는 Project 밖에서 한 번만 합친다. 레퍼런스처럼 정점마다 행렬을 곱하면 행렬 곱 연산이 불필요하게 늘어나므로, 한 번 합친 결과를 모든 정점에 재사용한다. (결정 기록: 정점 버퍼에 동차 좌표를 붙이고, 변환 행렬은 한 번만 합친다)
- w가 0 이하인 점(카메라 뒤나 카메라 위치)은 레퍼런스처럼 따로 처리하지 않는다. (결정 기록: w가 0 이하인 점은 따로 처리하지 않는다)
- Project는 z까지 돌려주지만, 지금은 x, y만 그리는 데 쓴다.
- 처음 만든 `ProjectTransition`에는 w로 나누는 단계가 빠져 있었다. 세 단계와 비교해서 찾고 추가했다.

### 정점 데이터
- 원점 중심, 한 변의 길이 2인 정육면체 정점 8개와 삼각형 12개의 인덱스는 요청해서 Claude가 생성했다.
- 카메라가 (−8, 0, 0)에 있으므로 정육면체는 카메라에서 7~9만큼 떨어져 near 1과 far 10 사이에 들어간다.

### 키 입력 (M8)
```cpp
case WM_KEYDOWN:
	switch (wParam)
	{
	case VK_RIGHT:
		objRotationY += 0.02f;
		break;
	// VK_LEFT, VK_UP, VK_DOWN ...
	case VK_RETURN:
		objRotationX = 0.f;
		objRotationY = 0.f;
		break;
	}
	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);
	return 0;
```
- 방향키로 X·Y축 회전값을 바꾸고, Enter로 초기화한 뒤 화면을 다시 그리게 한다.
- 회전할 때 약간의 깜빡임이 있지만 감수할 만한 수준으로 판단했다.

## 6. 디버깅 기록
- **식별자를 찾을 수 없음**: 호출한 함수 이름과 정의한 함수 이름이 달랐다.
- **'&에 l-value가 있어야 합니다'**: 임시 객체(`Vector3f(0, 0, 0)`)는 주소를 꺼낼 수 없다.
- **값이 0에 가깝게 나옴**: zaxis 계산에서 포인터끼리 연산하고 있었다. 포인터를 굳이 쓸 필요가 없어서 매개변수를 값으로 바꿔 해결했다.
- **'변수가 최적화되어 사용할 수 없습니다'**
  - *Claude 설명*: 컴파일러 최적화가 켜져 있으면 변수를 레지스터에만 두거나 없애 버려서 디버거가 값을 찾지 못한다. 보통 Release 구성에서 나오고, Debug 구성으로 빌드하면 대부분 볼 수 있다.
- **중단점에 끝없이 다시 걸림**
  - *Claude 설명*: 중단점에 걸리면 Visual Studio가 프로그램 창을 가리고, 다시 실행하면 창이 드러나면서 `WM_PAINT`가 와서 `Render`가 다시 호출된다. 중단점을 끄거나, 적중 횟수 조건을 쓰거나, 두 창이 겹치지 않게 두면 피할 수 있다.
- **Eigen 변수 값이 보이지 않음**
  - *Claude 설명*: Visual Studio는 Eigen 타입의 내부 구조를 몰라서 `m_storage` 같은 내부 멤버만 보여 준다. Eigen에 들어 있는 `eigen.natvis`를 프로젝트에 추가하거나 Visualizers 폴더에 복사하면 값이 보인다.
- **고친 내용이 반영되지 않음**: Claude가 파일 저장 시각과 빌드 시각을 비교해 보니, 소스를 저장한 뒤 다시 빌드하지 않아 이전 실행 파일로 디버깅하고 있었다.
- **`std::cout` 출력이 보이지 않음**
  - *Claude 설명*: 콘솔 창 없이 실행되는 Windows 프로그램이라 출력이 화면에 나오지 않는다.

## 남은 질문
- 회전할 때의 약간의 깜빡임
- 창을 최소화했을 때(`HEIGHT` = 0)의 종횡비
- 함수 이름(`LootAtTransition`), 헤더에 함수 본문과 `#define M_PI`를 그대로 둔 구성
- 각 면에 보이는 대각선 (삼각형 단위로 `DrawPolygon`을 호출하기 때문)
- M6·M8 코드 노트에 미해결 `?` 태그는 없다.

## 레퍼런스
- gdi-3d main.cpp: https://github.com/george7378/gdi-3d/blob/master/source/GDI%2B%203D/main.cpp
- gdi-3d definitions.h: https://github.com/george7378/gdi-3d/blob/master/source/GDI%2B%203D/definitions.h
- 「OpenGL로 배우는 3차원 컴퓨터 그래픽스」 6~7장
- 「이득우의 게임 수학」 10~12장: https://www.yes24.com/product/goods/107025224
- Eigen 문서 - Rotation2D: https://libeigen.gitlab.io/eigen/docs-nightly/classEigen_1_1Rotation2D.html
- Eigen 문서 - AngleAxis: https://libeigen.gitlab.io/eigen/docs-nightly/classEigen_1_1AngleAxis.html
- Eigen 문서 - cross (Geometry 모듈): https://libeigen.gitlab.io/eigen/docs-nightly/group__Geometry__Module.html#gac32d7ca309f8c0ef9ae04172d49a88e6
- Eigen 문서 - MatrixBase::normalize(): https://libeigen.gitlab.io/eigen/docs-nightly/classEigen_1_1MatrixBase.html#ad16303c47ba36f7a41ea264cb26bceb6
- Eigen 문서 - MatrixBase::normalized(): https://libeigen.gitlab.io/eigen/docs-nightly/classEigen_1_1MatrixBase.html#a5cf2fd4c57e59604fd4116158fd34308
- Eigen 문서 - Transform 클래스: https://libeigen.gitlab.io/eigen/docs-5.0/classEigen_1_1Transform.html
