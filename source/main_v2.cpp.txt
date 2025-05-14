#include "raylib.h"
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"
#include "raymath.h"

#define GLSL_VERSION            330

int main(void)
{
	const int screenWidth = 1280;
	const int screenHeight = 720;

	SetConfigFlags(FLAG_MSAA_4X_HINT);      // Enable Multi Sampling Anti Aliasing 4x (if available)

	InitWindow(screenWidth, screenHeight, "raylib [shaders] example - model shader");

	// Define the camera to look into our 3d world
	Camera camera = { 0 };
	camera.position = { 4.0f, 4.0f, 4.0f };    // Camera position
	camera.target = { 0.0f, 1.0f, -1.0f };     // Camera looking at point
	camera.up = { 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
	camera.fovy = 45.0f;                                // Camera field-of-view Y
	camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

	Model model = LoadModel("../resources/models/watermill.obj"); // Load OBJ model
	Texture2D texture = LoadTexture("../resources/models/watermill_diffuse.png"); // Load model texture

	// Load shader for model
	// NOTE: Defining 0 (NULL) for vertex shader forces usage of internal default vertex shader
	Shader shader = LoadShader(TextFormat("../resources/shaders/glsl330/lighting.vs", GLSL_VERSION),
														 TextFormat("../resources/shaders/glsl330/lighting.fs", GLSL_VERSION));

	model.materials[0].shader = shader;                     // Set shader effect to 3d model
	model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture; // Bind texture to model

	Vector3 position = { 0.0f, 0.0f, 0.0f };    // Set model position
	
	// Ambient light level (some basic lighting)
	int ambientLoc = GetShaderLocation(shader, "ambient");
	float val_t[] { 0.1f, 0.1f, 0.1f, 1.0f };
	SetShaderValue(shader, ambientLoc, val_t, SHADER_UNIFORM_VEC4);
	
	// Create lights
	Light lights[MAX_LIGHTS] = { 0 };
	lights[0] = CreateLight(LIGHT_POINT, { -4, 1, -4 }, Vector3Zero(), YELLOW, shader);
	lights[1] = CreateLight(LIGHT_POINT, { 4, 1, 4 }, Vector3Zero(), RED, shader);
	lights[2] = CreateLight(LIGHT_POINT, { -4, 1, 4 }, Vector3Zero(), GREEN, shader);
	lights[3] = CreateLight(LIGHT_POINT, { 4, 1, -4 }, Vector3Zero(), BLUE, shader);

	DisableCursor();                    // Limit cursor to relative movement inside the window
	SetTargetFPS(60);                   // Set our game to run at 60 frames-per-second
	//--------------------------------------------------------------------------------------

	// Main game loop
	while (!WindowShouldClose())        // Detect window close button or ESC key
	{
		// Update
		//----------------------------------------------------------------------------------
		UpdateCamera(&camera, CAMERA_FREE);
		float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
		SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);
		
		if (IsKeyPressed(KEY_Y)) { lights[0].enabled = !lights[0].enabled; }
		if (IsKeyPressed(KEY_R)) { lights[1].enabled = !lights[1].enabled; }
		if (IsKeyPressed(KEY_G)) { lights[2].enabled = !lights[2].enabled; }
		if (IsKeyPressed(KEY_B)) { lights[3].enabled = !lights[3].enabled; }
		
		for (int i = 0; i < MAX_LIGHTS; i++) UpdateLightValues(shader, lights[i]);
		//----------------------------------------------------------------------------------

		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();

		ClearBackground(RAYWHITE);

		BeginMode3D(camera);

		DrawModel(model, position, 0.2f, WHITE);   // Draw 3d model with texture
		
		// Draw spheres to show where the lights are
		for (int i = 0; i < MAX_LIGHTS; i++)
		{
			if (lights[i].enabled) DrawSphereEx(lights[i].position, 0.2f, 8, 8, lights[i].color);
			else DrawSphereWires(lights[i].position, 0.2f, 8, 8, ColorAlpha(lights[i].color, 0.3f));
		}

		DrawGrid(10, 1.0f);     // Draw a grid

		EndMode3D();

		DrawText("(c) Watermill 3D model by Alberto Cano", screenWidth - 210, screenHeight - 20, 10, GRAY);

		DrawFPS(10, 10);

		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	UnloadShader(shader);       // Unload shader
	UnloadTexture(texture);     // Unload texture
	UnloadModel(model);         // Unload model

	CloseWindow();              // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	return 0;
}
