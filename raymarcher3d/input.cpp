#include "raylib.h"
#include "raymath.h"
#include "input.hpp"
#include <iostream>

//int arrayLen(int[]);

int actionDown(int action[])
{
	for (int i = 0; i < arrayLen(action); i++)
	{
		if (IsKeyDown(action[i]))
		{
			return 1;
		}
	}
	return 0;
}

int actionPressed(int action[])
{
	for (int i = 0; i < arrayLen(action); i++)
	{
		if (IsKeyPressed(action[i]))
		{
			return 1;
		}
	}
	return 0;
}

int actionReleased(int action[])
{
	for (int i = 0; i < arrayLen(action); i++)
	{
		if (IsKeyReleased(action[i]))
		{
			return 1;
		}
	}
	return 0;
}

float actionAxis(int minusAction[], int plusAction[])
{
	float plus = actionDown(plusAction);
	float minus = actionDown(minusAction);
	return plus - minus;
}

Vector2 actionVector(int minusX[], int plusX[], int minusY[], int plusY[]) // NOT NORMALIZED!!!
{
	return { actionAxis(minusX, plusX), actionAxis(minusY, plusY) };
}




int arrayLen(int arr[])
{
	return sizeof(arr) / sizeof(arr[0]);
}
int arrayLen(Vector2 arr[])
{
	return sizeof(arr) / sizeof(arr[0]);
}