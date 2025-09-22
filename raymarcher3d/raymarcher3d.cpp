#include <iostream>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include "input.hpp"
#include <vector>
#include <string>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_dx9.h>
#include <imgui/backends/imgui_impl_win32.h>

#include "rlImGui/rlImGui.h"

using namespace std;

// WORLD POSITION INFO:
// +X: left
// +Y: down
// +Z: backward

// DEFINE INPUT ACTIONS
int action_mvLeft[2] = {KEY_A, KEY_A };
int action_mvRight[2] = { KEY_D, KEY_D };
int action_mvForward[2] = { KEY_W, KEY_W };
int action_mvBackward[2] = { KEY_S, KEY_S };

int action_rotLeft[2] = { KEY_LEFT,KEY_LEFT };
int action_rotRight[2] = { KEY_RIGHT,KEY_RIGHT };
int action_rotUp[2] = { KEY_UP,KEY_UP };
int action_rotDown[2] = { KEY_DOWN,KEY_DOWN };

int action_descend[2] = { KEY_LEFT_CONTROL, KEY_LEFT_CONTROL };
int action_ascend[2] = { KEY_SPACE, KEY_SPACE };

constexpr int screenX = 800;
constexpr int screenY = 600;
float resScale = 0.5;

// PARAMETERS TO EDIT
float k = 1.0f;
float speed = 1.8f;
float mouseSens = 0.2;

Vector3 lightPos = { 5.0, -6.0, 5.0 };
Vector3 lightCol = { 1.0, 1.0, 1.0 };

Vector3 bgColor = { 0.1, 0.1, 0.2 };

float shininess = 22.0;
float specular = 1.0;

float shadowIntensity = 1.0;
float shadowSmoothness = 1.0;
float shadowBias = 100;

Vector3 glowCol = { 1.0, 1.0, 1.0 };
float glowIntensity = 0.01;

int aoSteps = 5;
float aoStepSize = 0.05;
float aoBias = 0.5;


bool isCursor = true;

class Shape;
class Sphere;
class Box;
class Cam3d;
class RayMarch;

Vector2 oneDtoTwoD(int, int);
Vector3 absVec(Vector3);
float SdfMinOfAll(Shape*[], Vector3, int, float);
float smin(float, float, float);
float min(float, float);
Vector3 EulerToDirection(Vector3);
void printVec(Vector3);
void printDirs(vector<RayMarch>);
Color addCols(Color, Color, float);
void swapCursor();
Vector2 resolution(bool);

class Shape
{
public:	
	Vector3 origin;

	virtual float sdf(Vector3 pt)
	{
		return 6.7f;
	}
};

class Sphere : public Shape
{
public:
	float radius;

	Sphere(Vector3 origin, float radius)
	{
		this->origin = origin;
		this->radius = radius;
	}

	float sdf(Vector3 pt) override
	{
		Vector3 newPt = pt - origin;
		return Vector3Length(newPt) - radius;
	}
};
class Box : public Shape
{
public:
	Vector3 lengths;
	float& x = lengths.x;
	float& y = lengths.y;
	float& z = lengths.z;

	Box(Vector3 origin, Vector3 lengths)
	{
		this->lengths = lengths;
		this->origin = origin;
	}
	
	float sdf(Vector3 pt) override
	{
		Vector3 newPt = pt - origin;

		Vector3 q = absVec(newPt) - lengths;
		return Vector3Length(Vector3Max(q, Vector3Zero())) + 
			min(max(q.x, max(q.y, q.z)), 0.0f);
	}
};
class Torus : public Shape
{
public:
	Vector2 torusValues = { 1.0, 1.0 };

	Torus(Vector3 origin, Vector2 values)
	{
		this->origin = origin;
		torusValues = values;
	}

	float sdf(Vector3 pt) override
	{
		Vector3 newPt = pt - origin;

		Vector2 q = { Vector2Length({newPt.x, newPt.z}) - torusValues.x, newPt.y };
		return Vector2Length(q) - torusValues.y;
	}
};

class RayMarch
{
public:
	Vector2 pixel;
	Vector3 origin;
	Vector3 dir;
	float totalDistance = 0.0f;
	int stepsTaken = 0.0f;

	RayMarch()
	{

	}

	RayMarch(Vector3 origin, Vector3 offset)
	{
		this->origin = origin;
		dir = offset;
	}

	void march(float length, Vector3 dir)
	{
		origin += dir * length;
		totalDistance += length;
		stepsTaken++;
	}
};

class Cam3d
{
public:
	Vector3 origin = { 0.0f, 0.0f, 0.0f };
	Vector3 dir = { 0, 0, -1 };
	Vector2 rotation = { 0, 0 };

	float moveSpeed = 0.1;
	float rotSpeed = 0.02;

	float fov;

	Vector3 worldUp()
	{
		if (fabs(Vector3DotProduct(forward(), { 0.0f, 1.0f, 0.0f })) > 0.999f)
		{
			return { 1.0f, 0.0f, 0.0f };
		}
		return { 0.0f, 1.0f, 0.0f };
	}

	Vector3 forward()
	{
		return Vector3Normalize(dir);
	}
	Vector3 right()
	{
		return Vector3Normalize(Vector3CrossProduct(worldUp(), forward()));
	}
	Vector3 up()
	{
		return Vector3CrossProduct(forward(), right());
	}

	void rotate(bool isYaw, float amount)
	{
		amount *= rotSpeed;
		if (isYaw)
		{
			dir = Vector3RotateByAxisAngle(dir, { 0.0, 1.0, 0.0 }, amount);
		}
		else
		{
			dir = Vector3RotateByAxisAngle(dir, right(), amount);

			if (dir.y > 1.0f) dir.y = 1.0f;
			if (dir.y < -1.0f) dir.y = -1.0f;

		}

		dir = Vector3Normalize(dir);
	}
	void move(Vector3 d)
	{
		Vector3 dir = Vector3Zero();
		cout << dir.x << " " << dir.z << endl;
		dir = Vector3Add(Vector3Scale(right(), d.x), dir);
		dir = Vector3Add(Vector3Scale(forward(), -d.z), dir);
		dir.y += d.y;

		cout << dir.x << " " << dir.z << endl;

		origin += dir * moveSpeed;
	}

	float clipEnd = 100.0f;
	float hitThreshold = 0.001f;

	std::vector<RayMarch> rays{ screenX * screenY };

	Cam3d()
	{
		dir = { 0.0f, 0.0f, -1.0f };
		fov = PI/2.5;
	}

	void initRays()
	{
		// FOV stuff
		float aspect = (float)screenX / (float)screenY;
		float halfHeight = tanf(fov / 2.0f);
		float halfWidth = aspect * halfHeight;

		for (int idx = 0; idx < screenX * screenY; idx++)
		{
			Vector2 pixel = oneDtoTwoD(idx, screenX);
			rays[idx].pixel = pixel;

			float u = (pixel.x + 0.5f) / screenX;
			float v = (pixel.y + 0.5f) / screenY;

			float x = (2.0f * u - 1.0f) * halfWidth;
			float y = (1.0f - 2.0f * v) * halfHeight;

			Vector3 dir = Vector3Normalize(
				Vector3Add(Vector3Add(forward(), Vector3Scale(right(), x)), Vector3Scale(up(), y))
			);

			rays[idx].origin = origin;
			rays[idx].totalDistance = 0.0f;
			rays[idx].dir = dir;
			rays[idx].stepsTaken = 0;
		}
	}

	int marchRay(RayMarch& r, Shape* shapes[], int size, bool printData)
	{
		float length = hitThreshold;
		while (r.totalDistance < clipEnd && length >= hitThreshold)
		{
			if (printData)
			{
				cout << "MARCHING! ";
			}
			length = SdfMinOfAll(shapes, r.origin, size, k);
			if (length < 0.0f)
			{
				return 0;
			}
			r.march(length, r.dir);
			
		}
		//cout << "ENDLOOP!";

		if (r.totalDistance > clipEnd)
		{
			return 1; // RAY HIT DEAD AIR!
			cout << endl << "NO HIT!";
		}
		else
		{
			return 0; // RAY HIT OBJECT!
			cout << endl << "HIT!";
		}
	}


	float verticalFOV()
	{
		return (fov / screenX) * screenY;
	}
};


//-------------------------------------------------------MAIN PROGRAM

int main()
{

	// window setup
	InitWindow((screenX) + 300, screenY, "program");

	// IMGUI SETUP
	rlImGuiSetup(true);
	
	SetTargetFPS(60);
	
	// setup shader stuff
	Shader shader = LoadShader(0, "raymarcher3d.fs");
	Shader aaShader = LoadShader(0, "antiAlias.fs");

	int resLocAA = GetShaderLocation(aaShader, "resolution");
	

	if (shader.id == 0) {
		std::cerr << "Shader failed to load or compile!" << std::endl;
	}

	// get shader locations
	int resLoc = GetShaderLocation(shader, "iResolution");
	int timeLoc = GetShaderLocation(shader, "iTime");

	int countLoc = GetShaderLocation(shader, "shapeCount");
	int kLoc = GetShaderLocation(shader, "k");

	int typesLoc = GetShaderLocation(shader, "shapeTypes");
	int origLoc = GetShaderLocation(shader, "shapeOrigins");
	int sizeLoc = GetShaderLocation(shader, "shapeSizes");
	int colsLoc = GetShaderLocation(shader, "shapeCols");

	int camOriginLoc = GetShaderLocation(shader, "camOrigin");
	int camDirLoc = GetShaderLocation(shader, "camDir");
	int camFovLoc = GetShaderLocation(shader, "camFOV");
	int clipEndLoc = GetShaderLocation(shader, "clipEnd");
	int hitThresholdLoc = GetShaderLocation(shader, "hitThreshold");

	// debug params
	int sbLoc = GetShaderLocation(shader, "sb");
	int lightColLoc = GetShaderLocation(shader, "lightColor");
	int lightLoc = GetShaderLocation(shader, "lightPos");
	int bgColLoc = GetShaderLocation(shader, "bgColor");
	int shininessLoc = GetShaderLocation(shader, "shininess");
	int glowColLoc = GetShaderLocation(shader, "glowCol");
	int glowIntensityLoc = GetShaderLocation(shader, "glowIntensity");
	int shadowSmoothLoc = GetShaderLocation(shader, "shadowSmoothness");
	int aoStepsLoc = GetShaderLocation(shader, "aoSteps");
	int aoStepSizeLoc = GetShaderLocation(shader, "aoStepSize");
	int aoBiasLoc = GetShaderLocation(shader, "aoBias");

	swapCursor();

	int shapesLength = 2;
	int shapeTypes[] = {1, 0, 3};
	Vector3 shapePositions[] = {
		{-2.0f, 3.5f, -1.0f},
		{0.0f, 0.0f, 0.0f},
		{2.0, -2.0, -2.0}
	};
	Vector3 shapeSizes[] = {
		{1.5f, 1.5f, 1.5f},
		{1.0f, 1.0f, 1.0f},
		{8.0, 10.0, 3.0}
	};
	Vector3 shapeCols[] = {
		{0.8, 0.2, 0.2},
		{0.8, 0.9, 0.1},
		{0.0, 1.0, 1.0}
	};

	Cam3d cam = Cam3d();
	cam.origin = { 0.0, 0.0, 5.0 };


	// ----------------- GAME LOOP
	while (WindowShouldClose() == false)
	{
		Vector2 r = resolution(true);
		Vector2 r2 = resolution(false);
		RenderTexture2D sceneRT = LoadRenderTexture(r.x, r.y);
		RenderTexture2D postRT = LoadRenderTexture(r.x, r.y);
	
		float time = GetTime();
		
		//------------------------INPUT
		Vector2 moveInput = actionVector(action_mvLeft, action_mvRight, action_mvForward, action_mvBackward);
		float ascendInput = actionAxis(action_ascend, action_descend);
		Vector3 fullMoveInput = { moveInput.x, ascendInput, moveInput.y };

		// toggle cursor
		if (IsKeyPressed(KEY_X))
		{
			swapCursor();
		}
		
		Vector2 lookInput = Vector2Zero();
		if(!isCursor) lookInput = mouseMovement() * mouseSens;

		if(fullMoveInput != Vector3Zero()) cam.move(fullMoveInput);

		if(lookInput.x) cam.rotate(true, lookInput.x);
		if(lookInput.y) cam.rotate(false,-lookInput.y);

		SetShaderValue(aaShader, resLocAA, &r, SHADER_UNIFORM_VEC2);

		SetShaderValue(shader, resLoc, &r, SHADER_UNIFORM_VEC2);
		SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);

		SetShaderValue(shader, countLoc, &shapesLength, SHADER_UNIFORM_INT);
		SetShaderValue(shader, kLoc, &k, SHADER_UNIFORM_FLOAT);

		SetShaderValueV(shader, typesLoc, &shapeTypes, SHADER_UNIFORM_INT, shapesLength);
		SetShaderValueV(shader, origLoc, &shapePositions, SHADER_UNIFORM_VEC3, shapesLength);
		SetShaderValueV(shader, sizeLoc, &shapeSizes, SHADER_UNIFORM_VEC3, shapesLength);
		SetShaderValueV(shader, colsLoc, &shapeCols, SHADER_UNIFORM_VEC3, shapesLength);

		SetShaderValue(shader, camOriginLoc, &cam.origin, SHADER_UNIFORM_VEC3);
		SetShaderValue(shader, camDirLoc, &cam.dir, SHADER_UNIFORM_VEC3);
		SetShaderValue(shader, camFovLoc, &cam.fov, SHADER_UNIFORM_FLOAT);
		SetShaderValue(shader, clipEndLoc, &cam.clipEnd, SHADER_UNIFORM_FLOAT);
		SetShaderValue(shader, hitThresholdLoc, &cam.hitThreshold, SHADER_UNIFORM_FLOAT);

		// debug params
		SetShaderValue(shader, sbLoc, &shadowBias, SHADER_UNIFORM_FLOAT);
		SetShaderValue(shader, lightLoc, &lightPos, SHADER_UNIFORM_VEC3);
		SetShaderValue(shader, lightColLoc, &lightCol, SHADER_UNIFORM_VEC3);
		SetShaderValue(shader, bgColLoc, &bgColor, SHADER_UNIFORM_VEC3);
		SetShaderValue(shader, shininessLoc, &shininess, SHADER_UNIFORM_FLOAT);
		SetShaderValue(shader, glowColLoc, &glowCol, SHADER_UNIFORM_VEC3);
		SetShaderValue(shader, glowIntensityLoc, &glowIntensity, SHADER_UNIFORM_FLOAT);
		SetShaderValue(shader, shadowSmoothLoc, &shadowSmoothness, SHADER_UNIFORM_FLOAT);
		SetShaderValue(shader, aoStepsLoc, &aoSteps, SHADER_UNIFORM_INT);
		SetShaderValue(shader, aoStepSizeLoc, &aoStepSize, SHADER_UNIFORM_FLOAT);
		SetShaderValue(shader, aoBiasLoc, &aoBias, SHADER_UNIFORM_FLOAT);

		if (IsKeyDown(KEY_ONE))
		{
			k -= GetFrameTime() * 0.5;
			if (k <= 0.0f)
			{
				k = 0.0f;
			}
		}
		if (IsKeyDown(KEY_TWO))
		{
			k += GetFrameTime() * 0.5;
		}

		if (IsKeyDown(KEY_THREE)) {
			shadowBias -= GetFrameTime() * 300;
			cout << "BIAS: " << shadowBias << endl;
		}
		if (IsKeyDown(KEY_FOUR)) {
			shadowBias += GetFrameTime() * 300;
			cout << "BIAS: " << shadowBias << endl;
		}


		//------------------------DRAWING
		
		
		// BEGIN DRAWING
		BeginTextureMode(sceneRT);
		ClearBackground(BLACK);
		BeginShaderMode(shader);
		DrawRectangle(0, 0, screenX*resScale, screenY*resScale, WHITE);
		EndShaderMode();
		EndTextureMode();

		BeginTextureMode(postRT);
		BeginShaderMode(aaShader);
		DrawTexture(sceneRT.texture, 0, 0, WHITE);
		EndShaderMode();
		EndTextureMode();

		BeginDrawing();		
			rlImGuiBegin();

				bool open = true;

				ImVec2 pos = ImVec2(screenX, 0);
				ImGui::SetNextWindowPos(pos, ImGuiCond_Always);

				if (ImGui::Begin("Test Window", &open))
				{
					ImGui::SliderFloat("Resolution Scale", &resScale, 0.01f, 1.0f);
					ImGui::SliderFloat("Smoothness", &k, 0.0f, 2.0f);

					ImGui::SliderFloat("Shadow Bias", &shadowBias, 1.0, 200.0);
					ImGui::SliderFloat("Shadow Softness", &shadowSmoothness, 0.0, 20.0);

					ImGui::SliderFloat("Shininess", &shininess, 3.0, 50.0);

					ImGui::ColorEdit3("Ambient Color", (float*)&bgColor);
					ImGui::ColorEdit3("Glow Color", (float*)&glowCol);
					ImGui::SliderFloat("Glow Intensity", &glowIntensity, 0.0, 0.02);

					ImGui::SliderInt("AO steps", &aoSteps, 0, 10);
					ImGui::SliderFloat("AO step size", &aoStepSize, 0.01, 0.1);
					ImGui::SliderFloat("AO bias", &aoBias, -2, 2);

					ImGui::TextUnformatted("Light Pos");
					ImGui::SliderFloat("X:", &lightPos.x, -8, 8);
					ImGui::SliderFloat("Y:", &lightPos.y, -8, 8);
					ImGui::SliderFloat("Z:", &lightPos.z, -8, 8);

					for (int i = 0; i < shapesLength; i++)
					{
						ImGui::Separator();

						ImGui::PushID(i);

						ImGui::DragInt("Shape Type", &shapeTypes[i], 0, 3);

						string name = to_string(i);

						// print titles and values
						switch (shapeTypes[i])
						{
							case 0:
								name += ": Sphere";
								ImGui::TextUnformatted(name.c_str());
								ImGui::SliderFloat("Radius", &shapeSizes[i].x, 0.1, 5.0);
								break;
							case 1:
								name += ": Box";
								ImGui::TextUnformatted(name.c_str());
								ImGui::SliderFloat("Size X", &shapeSizes[i].x, 0.1, 5.0);
								ImGui::SliderFloat("Size Y", &shapeSizes[i].y, 0.1, 5.0);
								ImGui::SliderFloat("Size Z", &shapeSizes[i].z, 0.1, 5.0);
								break;
							case 2:
								name += ": torus";
								ImGui::TextUnformatted(name.c_str());
								ImGui::SliderFloat("R", &shapeSizes[i].x, 0.1, 5.0);
								ImGui::SliderFloat("r", &shapeSizes[i].y, 0.1, 5.0);
								break;
							case 3:
								name += ": mandelbulb";
								ImGui::TextUnformatted(name.c_str());
								ImGui::SliderFloat("Iterations", &shapeSizes[i].x, 1.0, 15.0);
								ImGui::SliderFloat("Radius", &shapeSizes[i].y, 0.1, 5.0);
								ImGui::SliderFloat("Power", &shapeSizes[i].z, 0.1, 10.0);
								break;
						}

						

						ImGui::TextUnformatted("Position");
						ImGui::SliderFloat("X:", &shapePositions[i].x, -8, 8);
						ImGui::SliderFloat("Y:", &shapePositions[i].y, -8, 8);
						ImGui::SliderFloat("Z:", &shapePositions[i].z, -8, 8);

						ImGui::ColorEdit3("Color", (float*)&shapeCols[i]);

						// PRINT VALUES
						
						ImGui::PopID();
					}

				}
				ImGui::End();
			

			ClearBackground(BLACK);
			BeginShaderMode(aaShader);
			DrawTexturePro(postRT.texture, Rectangle{ 0, 0, screenX*resScale, screenY*resScale }, Rectangle{ 0, 0, screenX, -screenY }, { 0, 0 }, 0, WHITE);
			EndShaderMode();

			
			DrawFPS(10, 10);
			rlImGuiEnd();
		EndDrawing();
		UnloadRenderTexture(sceneRT);
		UnloadRenderTexture(postRT);
	}
	rlImGuiShutdown();
	CloseWindow();
	return 0;
}

Vector3 absVec(Vector3 v)
{
	return {
		abs(v.x),
		abs(v.y),
		abs(v.z)
	};
}

// COMBINE SHAPES
float min(float a, float b)
{
	return (a < b) ? a : b;
}
float smin(float a, float b, float k)
{
	if (k <= 0.05) { return min(a, b); }

	k *= 1.0;
	float r = exp2(-a / k) + exp2(-b / k);
	return -k * log2(r);
}

float SdfMinOfAll(Shape* shapes[], Vector3 pt, int length, float k)
{
	float min = shapes[0]->sdf(pt);

	for (int idx = 1; idx < length; idx++)
	{
		min = smin(min, shapes[idx]->sdf(pt), k);
	}

	return min;
}

Vector2 oneDtoTwoD(int index, int rowSize)
{
	int x = index % rowSize;
	int y = index / rowSize;
	return { (float)x, (float)y };

}

Vector3 EulerToDirection(Vector3 eulerRotation)
{
	// Convert degrees to radians
	float yaw =  eulerRotation.y;   // Yaw: rotation around Y
	float pitch = eulerRotation.x; // Pitch: rotation around X

	Vector3 direction;
	direction.x = cosf(pitch) * sinf(yaw);
	direction.y = sinf(pitch);
	direction.z = cosf(pitch) * cosf(yaw);

	return Vector3Normalize(direction); // Optional normalization
}

void printVec(Vector3 pt)
{
	cout << "X:" << pt.x << " Y:" << pt.y << " Z:" << pt.z << endl;
}

void printDirs(vector<RayMarch> rays)
{
	for (int i = 0; i < rays.size(); i++)
	{
		cout << "DIR:" << rays[i].dir.x << ":" << rays[i].dir.y << ":" << rays[i].dir.z << ". ";
	}
}

Color addCols(Color c, Color c2, float intensity)
{
	c.r = Clamp(c.r + (int)(c2.r * intensity), 0, 255);
	c.g = Clamp(c.g + (int)(c2.g * intensity), 0, 255);
	c.b = Clamp(c.b + (int)(c2.b * intensity), 0, 255);

	return c;
}

void swapCursor()
{
	(isCursor == true) ? DisableCursor() : EnableCursor();
	isCursor = !isCursor;
	resetMouse();

}

Vector2 resolution(bool isRender)
{
	if (isRender) return { screenX*resScale, screenY *resScale}; else return{ screenX, screenY };
}