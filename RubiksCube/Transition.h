#pragma once

#include <Eigen/Dense>
#include <Eigen/Geometry>

#define M_PI 3.14159265358979323846

using namespace Eigen;

float DegToRad(float deg)
{
	return deg * (float(M_PI)) / 180;
}

//Viewport
struct Viewport
{
	unsigned x, y;
	unsigned w, h;

	Viewport()
	{
	};

	void Update(unsigned i_x, unsigned i_y, unsigned i_w, unsigned i_h)
	{
		x = i_x; y = i_y;
		w = i_w; h = i_h;
	}
};

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

Vector4f ProjectTransition(Vector4f v, Viewport vp, Matrix4f multMatrix)
{
	Vector4f projectedPos = multMatrix * v;

	projectedPos = projectedPos / projectedPos(3);

	return Vector4f(
		vp.x + (1 + projectedPos(0)) * vp.w / 2,
		vp.y + (1 - projectedPos(1)) * vp.h / 2,
		projectedPos(2) * 0.5f + 0.5f,
		1
	);
}