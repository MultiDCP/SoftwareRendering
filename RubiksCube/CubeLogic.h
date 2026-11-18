#pragma once

#include <cstring>

using namespace std;

enum ESurface
{
	Left,
	Right,
	Front,
	Back,
	Down,
	Up
};

char L_arr[3][3] = { {'O', 'O', 'O'},
					 {'O', 'O', 'O'},
					 {'O', 'O', 'O'} };
char R_arr[3][3] = { {'R', 'R', 'R'},
					 {'R', 'R', 'R'},
					 {'R', 'R', 'R'} };
char F_arr[3][3] = { {'G', 'G', 'G'},
					 {'G', 'G', 'G'},
					 {'G', 'G', 'G'} };
char B_arr[3][3] = { {'B', 'B', 'B'},
					 {'B', 'B', 'B'},
					 {'B', 'B', 'B'} };
char D_arr[3][3] = { {'Y', 'Y', 'Y'},
					 {'Y', 'Y', 'Y'},
					 {'Y', 'Y', 'Y'} };
char U_arr[3][3] = { {'W', 'W', 'W'},
					 {'W', 'W', 'W'},
					 {'W', 'W', 'W'} };

void SurfaceClockwise(char (*arr)[3])
{
	char tmp[3][3] = { {arr[2][0], arr[1][0], arr[0][0]},
					   {arr[2][1], arr[1][1], arr[0][1]},
					   {arr[2][2], arr[1][2], arr[0][2]} };
	memcpy(arr, tmp, sizeof(tmp));
	return;
}

void L_Clockwise()
{
	SurfaceClockwise(L_arr);

	char B_tmp[3] = { B_arr[0][2], B_arr[1][2], B_arr[2][2] };
	char U_tmp[3] = { U_arr[0][0], U_arr[1][0], U_arr[2][0] };
	char F_tmp[3] = { F_arr[0][0], F_arr[1][0], F_arr[2][0] };
	char D_tmp[3] = { D_arr[2][0], D_arr[1][0], D_arr[0][0] };

	for (int i = 0; i < 3; i++)
	{
		B_arr[i][2] = D_tmp[i];
		U_arr[i][0] = B_tmp[2-i];
		F_arr[i][0] = U_tmp[i];
		D_arr[i][0] = F_tmp[i];
	}
	return;
}

void R_Clockwise()
{
	SurfaceClockwise(R_arr);

	char F_tmp[3] = { F_arr[0][2], F_arr[1][2], F_arr[2][2] };
	char U_tmp[3] = { U_arr[2][2], U_arr[1][2], U_arr[0][2] };
	char B_tmp[3] = { B_arr[0][0], B_arr[1][0], B_arr[2][0] };
	char D_tmp[3] = { D_arr[0][2], D_arr[1][2], D_arr[2][2] };

	for (int i = 0; i < 3; i++)
	{
		F_arr[i][2] = D_tmp[i];
		U_arr[i][2] = F_tmp[i];
		B_arr[i][0] = U_tmp[i];
		D_arr[i][2] = B_tmp[2-i];
	}
	return;
}

void F_Clockwise()
{
	SurfaceClockwise(F_arr);

	char L_tmp[3] = { L_arr[0][2], L_arr[1][2], L_arr[2][2] };
	char U_tmp[3] = { U_arr[2][0], U_arr[2][1], U_arr[2][2] };
	char R_tmp[3] = { R_arr[0][0], R_arr[1][0], R_arr[2][0] };
	char D_tmp[3] = { D_arr[0][0], D_arr[0][1], D_arr[0][2] };

	for (int i = 0; i < 3; i++)
	{
		L_arr[i][2] = D_tmp[i];
		U_arr[2][i] = L_tmp[2-i];
		R_arr[i][0] = U_tmp[i];
		D_arr[0][i] = R_tmp[2-i];
	}
	return;
}

void B_Clockwise()
{
	SurfaceClockwise(B_arr);

	char R_tmp[3] = { R_arr[0][2], R_arr[1][2], R_arr[2][2] };
	char U_tmp[3] = { U_arr[0][2], U_arr[0][1], U_arr[0][0] };
	char L_tmp[3] = { L_arr[0][0], L_arr[1][0], L_arr[2][0] };
	char D_tmp[3] = { D_arr[2][2], D_arr[2][1], D_arr[2][0] };

	for (int i = 0; i < 3; i++)
	{
		R_arr[i][2] = D_tmp[i];
		U_arr[0][i] = R_tmp[i];
		L_arr[i][0] = U_tmp[i];
		D_arr[2][i] = L_tmp[i];
	}
	return;
}

void U_Clockwise()
{
	SurfaceClockwise(U_arr);

	char L_tmp[3] = { L_arr[0][0], L_arr[0][1], L_arr[0][2] };
	char B_tmp[3] = { B_arr[0][2], B_arr[0][1], B_arr[0][0] };
	char R_tmp[3] = { R_arr[0][2], R_arr[0][1], R_arr[0][0] };
	char F_tmp[3] = { F_arr[0][0], F_arr[0][1], F_arr[0][2] };

	for (int i = 0; i < 3; i++)
	{
		L_arr[0][i] = F_tmp[i];
		B_arr[0][i] = L_tmp[i];
		R_arr[0][i] = B_tmp[2-i];
		F_arr[0][i] = R_tmp[2-i];
	}
	return;
}

void D_Clockwise()
{
	SurfaceClockwise(D_arr);

	char L_tmp[3] = { L_arr[2][2], L_arr[2][1], L_arr[2][0] };
	char F_tmp[3] = { F_arr[2][0], F_arr[2][1], F_arr[2][2] };
	char R_tmp[3] = { R_arr[2][0], R_arr[2][1], R_arr[2][2] };
	char B_tmp[3] = { B_arr[2][2], B_arr[2][1], B_arr[2][0] };

	for (int i = 0; i < 3; i++)
	{
		L_arr[2][i] = B_tmp[2-i];
		F_arr[2][i] = L_tmp[2-i];
		R_arr[2][i] = F_tmp[i];
		B_arr[2][i] = R_tmp[i];
	}
	return;
}