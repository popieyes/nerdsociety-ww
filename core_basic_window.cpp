/*******************************************************************************************
*
*   raylib [core] example - Basic window (adapted for HTML5 platform)
*
*   This example is prepared to compile for PLATFORM_WEB and PLATFORM_DESKTOP
*   As you will notice, code structure is slightly different to the other examples...
*   To compile it for PLATFORM_WEB just uncomment #define PLATFORM_WEB at beginning
*
*   This example has been created using raylib 1.3 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2015 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
int screenWidth = 1280;
int screenHeight = 720;

Camera3D camera = { 0 };
Vector3 position = { 0.0f, 0.0f, 0.0f };
Model model;
Model water;
float boat_speed = 2.5f; 
//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
void UpdateDrawFrame(void);     // Update and Draw one frame

//----------------------------------------------------------------------------------
// Program main entry point
//----------------------------------------------------------------------------------
int main()
{
    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    
    // Camera
    
    camera.position = {5.0f, 2.0f, 0.0f};
    camera.target = { 0.0f, 1.5f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f};
    camera.fovy = 30.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    model = LoadModel("low_poly_viking_ship.obj");
    Texture2D texture = LoadTexture("_Barkito_Barcowire_088144225_albedo.png");
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    water = LoadModel("water.obj");
    Texture2D waterTexture = LoadTexture("Water_001_COLOR.png");
    water.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = waterTexture;

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif
    UnloadTexture(texture);
    UnloadModel(model);
    UnloadTexture(waterTexture);
    UnloadModel(water);
    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    // TODO: Update your variables here
    //----------------------------------------------------------------------------------
    if (IsKeyPressed(KEY_SPACE))
        {
            if (IsCursorHidden()) {
                EnableCursor();
            } else {
                DisableCursor();
            }
        }

    UpdateCamera(&camera, CAMERA_FREE);
    
    position = Vector3{position.x, position.y, position.z + (boat_speed * GetFrameTime())}  ;
    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();

        ClearBackground(RAYWHITE);
        
        BeginMode3D(camera);
            DrawModel(model, { 0.0f, 1.5f, 0.0f }, 3.0f, WHITE);
            DrawModel(water, position, 1.0f, WHITE);
            

        EndMode3D();
        DrawFPS(10, 10);

    EndDrawing();
    //----------------------------------------------------------------------------------
}
