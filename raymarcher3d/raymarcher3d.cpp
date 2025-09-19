#include <iostream>
#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include <vector>

using namespace std;

float k = 1.0f;
constexpr int screenX = 250;
constexpr int screenY = 150;
const int resScale = 5;
const float speed = 1.8f;

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

	void die()
	{
		// stop existing
	}
};

class Cam3d
{
public:
	Vector3 origin = { 0.0f, 0.0f, 0.0f };
	Vector3 dir = { 0, 0, -1 };
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

	float clipEnd = 100.0f;
	float hitThreshold = 0.01f;

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
			//rays[idx].dirOffset = Vector3Normalize({ x, y, -1.0f });
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
	/*
	// setup shader stuff
	Shader shader = LoadShader(NULL, "raymarcher3d.frag");
	// get shader locations
	int resLoc = GetShaderLocation(shader, "iResolution");
	int timeLoc = GetShaderLocation(shader, "iTime");
	int countLoc = GetShaderLocation(shader, "shapeCount");
	int typesLoc = GetShaderLocation(shader, "shapeTypes");
	int origLoc = GetShaderLocation(shader, "shapeOrigins");
	int sizeLoc = GetShaderLocation(shader, "shapeSizes");
	*/

	Sphere* s = new Sphere({ 0.0, 0.0, 0.0 }, 1.0f);
	Box* b = new Box({ 3.0, 0.0, 0.0 }, { 2.0, 1.0, 1.0 });
	Torus* t = new Torus({ 5.0, 0.0, 0.0 }, { 1.0, 0.5 });

	Shape* shapes[] = { s, b};
	int shapesLength = 2;
	int shapeTypes[] = {0, 1}; // sphere, box
	Vector3 shapePositions[] = {
		{0.0f, 0.0f, 0.0f},
		{1.5f, 0.0f, -3.0f}
	};
	Vector3 shapeSizes[] = {
		{1.0f, 0.0f, 0.0f},      // sphere radius in x
		{0.5f, 0.5f, 1.5f}       // box size
	};

	/*
	SetShaderValue(shader, countLoc, &shapesLength, SHADER_UNIFORM_INT);
	SetShaderValueV(shader, typesLoc, shapeTypes, SHADER_UNIFORM_INT, shapesLength);
	SetShaderValueV(shader, origLoc, shapePositions, SHADER_UNIFORM_VEC3, shapesLength);
	SetShaderValueV(shader, sizeLoc, shapeSizes, SHADER_UNIFORM_VEC3, shapesLength);
	*/
	Cam3d cam = Cam3d();
	cam.origin = { 0.0, 0.0, 5.0 };
	

	// raylib setup
	InitWindow(screenX*resScale, screenY*resScale, "program");
	SetTargetFPS(60);

	// pixel array and image texture
	Color* singlePixelColArray = new Color[screenX * screenY];
	Image img;
	img.data = singlePixelColArray;
	img.width = screenX;
	img.height = screenY;
	img.mipmaps = 1;
	img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
	Texture2D tex = LoadTextureFromImage(img);
	Rectangle src{ 0, 0, screenX, screenY };
	Rectangle dest{ 0, 0, screenX*resScale, screenY*resScale };
	Vector2 org{ 0, 0 };
	float rot{ 0 };

	// ----------------- GAME LOOP
	while (WindowShouldClose() == false)
	{
		cam.initRays();

		int centerIdx = (screenY / 2) * screenX + (screenX / 2);

		for (int idx = 0; idx < cam.rays.size(); idx++)
		{
			bool printData = false;

			if (idx == centerIdx)
			{
				printData = false;
			}

			RayMarch& r = cam.rays[idx];
			bool missed = cam.marchRay(r, shapes, shapesLength, printData);

			if (printData)
			{
				if (missed)
					cout << "Result: Ray MISSED (dead air)" << endl;
				else
					cout << "Result: Ray HIT object!" << endl;
			}

		
			// glow effect
			float glow = 1.0f - (float)r.stepsTaken / 100.0f;
			if (glow < 0.0f) glow = 0.0f;
			unsigned char glowVal = 255 - (unsigned char)(glow * 255.0f);
			Color glowCol = { glowVal, 0, glowVal, 255 };

			singlePixelColArray[idx] = missed ? Color{0, 0, 0, 255} : Color{210, 210, 210, 255};
		
			singlePixelColArray[idx] = addCols(singlePixelColArray[idx], glowCol, 3.0);
		}
		singlePixelColArray[centerIdx] = { 255, 0, 0, 255 };

		UpdateTexture(tex, singlePixelColArray);



		//------------------------INPUT
		if (IsKeyDown(KEY_A))
		{
			cam.origin.x += GetFrameTime() * speed;
			printVec(cam.origin);
		}
		if (IsKeyDown(KEY_D))
		{
			cam.origin.x -= GetFrameTime() * speed;
			printVec(cam.origin);
		}

		if (IsKeyDown(KEY_E))
		{
			cam.origin.y += GetFrameTime() * speed;
			printVec(cam.origin);
		}
		if (IsKeyDown(KEY_Q))
		{
			cam.origin.y -= GetFrameTime() * speed;
			printVec(cam.origin);
		}

		if (IsKeyDown(KEY_S))
		{
			cam.origin.z += GetFrameTime() * speed;
			printVec(cam.origin);
		}
		if (IsKeyDown(KEY_W))
		{
			cam.origin.z -= GetFrameTime() * speed;
			printVec(cam.origin);
		}
		if (IsKeyPressed(KEY_SPACE))
		{
			cout << "K VALUE: " << k << endl;
		}

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

		//cout << "SDF FROM CAM TO SPHERE: " << cam.origin.x << cam.origin.y << cam.origin.z << ": " << shapes[0]->sdf(cam.origin) << endl;

		//------------------------DRAWING
		BeginDrawing();

		DrawTexturePro(tex, src, dest, org, rot, WHITE);




		DrawFPS(10, 10);
		ClearBackground(WHITE);

		EndDrawing();
	}

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