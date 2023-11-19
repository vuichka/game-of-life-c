#include "include/raylib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

// consts
const int SCREEN_HEIGHT = 900;
const int SCREEN_WIDTH = 1300;
static int CELL_WIDTH = 3;
static int CELL_HEIGHT = 3;
static int ROWS;
static int COLS;
static int ERASE_AREA = 3;
static int UPDATE_SPEED = 0;

// raylib ui colors
const int GRID_COLOR = 0x212120ff;
const int ALIVE_COLOR = 0x2ff768ff;
const int DEAD_COLOR = 0x000000ff;

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

typedef struct RLE
{
    size_t width;
    size_t height;
    bool **cells;
} RLE;

typedef struct Cell
{
    int neighbours;
    bool alive;
} Cell;

typedef struct World
{
    size_t width;
    size_t height;
    Cell **cells;
    int framesSinceLastUpd;
} World;

World NewWorld(World *w, int factor)
{
    w->width = ROWS;
    w->height = COLS;
    w->framesSinceLastUpd = 0;
    w->cells = malloc(ROWS * sizeof(Cell));
    for (size_t r = 0; r < ROWS; r++)
    {
        w->cells[r] = malloc(COLS * sizeof(Cell));
        for (size_t c = 0; c < COLS; c++)
        {

            w->cells[r][c].neighbours = 0;

            int v = GetRandomValue(0, 100);
            if (v > factor)
            {
                w->cells[r][c].alive = true;
            }
            else
            {
                w->cells[r][c].alive = false;
            }
        }
    }
    return *w;
}

void UpdateNeighbours(World *w, size_t ox, size_t oy)
{
    int neighbours = 0;

    for (int i = -1; i <= 1; ++i)
    {
        for (int k = -1; k <= 1; ++k)
        {
            if (i == 0 && k == 0)
            {
                continue;
            }

            size_t x = (ox + i + w->width) % w->width;
            size_t y = (oy + k + w->height) % w->height;

            if (w->cells[x][y].alive)
            {
                neighbours++;
            }
        }
    }
    w->cells[ox][oy].neighbours = neighbours; // Assuming each Cell has a 'neighbours' member
}

bool NextStatus(World *w, size_t x, size_t y)
{
    int nb = w->cells[x][y].neighbours;
    bool status = w->cells[x][y].alive;

    if (status && nb > 1 && nb < 4)
    {
        return true;
    }
    if (!status && nb == 3)
    {
        return true;
    }
    return false;
}

World NextState(World *w)
{

    for (size_t r = 0; r < ROWS; r++)
    {
        for (size_t c = 0; c < COLS; c++)
        {
            UpdateNeighbours(w, r, c);
        }
    }
    for (size_t r = 0; r < ROWS; r++)
    {
        for (size_t c = 0; c < COLS; c++)
        {
            w->cells[r][c].alive = NextStatus(w, r, c);
        }
    }
    return *w;
}

World WithRandom(World *w, int factor)
{
    w->cells = malloc(ROWS * sizeof(Cell));
    for (size_t r = 0; r < ROWS; r++)
    {
        w->cells[r] = malloc(COLS * sizeof(Cell));
        for (size_t c = 0; c < COLS; c++)
        {

            w->cells[r][c].neighbours = 0;

            int v = GetRandomValue(0, 100);
            if (v > factor)
            {
                w->cells[r][c].alive = true;
            }
            else
            {
                w->cells[r][c].alive = false;
            }
        }
    }
    return *w;
}

World SpawnFromRle(World *w, RLE *rle, size_t x, size_t y)
{
    for (size_t row = 0; row < rle->height; row++)
    {
        for (size_t cell = 0; cell < rle->width; cell++)
        {
            size_t ox = (x + cell + w->height) % w->height, oy = (y + row % w->width) % w->width;
            w->cells[oy][ox].alive = rle->cells[row][cell];
        }
    }
    return *w;
}

RLE NewRLE(char *str)
{
    RLE result;
    result.width = 0;
    result.height = 0;
    result.cells = NULL;

    // Temporary variables
    int tempWidth = 0, tempHeight = 0;
    char *line = strtok(str, "\n");

    static size_t row = 0;
    static size_t col = 0;
    row = 0;
    col = 0;

    while (line != NULL)
    {
        if (line[0] != '#' && line[0] != '!')
        {
            if (line[0] == 'x') // Parsing the size
            {
                sscanf(line, "x = %d, y = %d", &tempWidth, &tempHeight);
                result.width = tempWidth;
                result.height = tempHeight;
                result.cells = malloc(tempHeight * sizeof(bool *));
                for (size_t i = 0; i < tempHeight; ++i)
                {
                    result.cells[i] = calloc(tempWidth, sizeof(bool));
                }
            }
            else // Parsing the pattern
            {

                int times = -1;

                for (size_t i = 0; line[i] != '\0' && row < tempHeight; ++i)
                {

                    //     if (!isdigit(line[i])) {
                    // printf ("%d %c | ", times, line[i]);
                    //     }
                    if (isdigit(line[i]))
                    {
                        if (times != -1)
                        {
                            times = times * 10 + ((int)line[i] - 48);
                        }
                        else
                        {
                            times = (int)(line[i]) - 48;
                        }
                    }
                    else if (line[i] == 'b')
                    {
                        if (times != -1)
                        {
                            for (int t = 0; t < times; t++)
                            {
                                result.cells[row][col] = false;
                                col++;
                            }
                            times = -1;
                        }
                        else
                        {
                            result.cells[row][col] = false;
                            col++;
                        }
                    }
                    else if (line[i] == 'o')
                    {
                        if (times != -1)
                        {
                            for (int t = 0; t < times; t++)
                            {
                                result.cells[row][col] = true;
                                col++;
                            }
                            times = -1;
                        }
                        else
                        {
                            result.cells[row][col] = true;
                            col++;
                        }
                    }
                    else if (line[i] == '$')
                    {
                        if (col < result.width)
                        {
                            for (int k = col; k < result.width; k++)
                            {
                                result.cells[row][k] = false;
                            }
                        }
                        row++;
                        col = 0;
                        // printf("\n");
                    }
                }
            }
        }
        line = strtok(NULL, "\n");
    }

    return result;
}

char *RLEToString(const RLE *rle)
{
    // Calculate the total size needed for the string
    // +1 for newline at the end of each row and +1 for the null terminator
    size_t totalSize = rle->height * (rle->width + 1) + 1;
    char *str = malloc(totalSize);

    if (str == NULL)
    {
        fprintf(stderr, "Failed to allocate memory.\n");
        return NULL;
    }

    size_t pos = 0;
    for (size_t i = 0; i < rle->height; ++i)
    {
        for (size_t j = 0; j < rle->width; ++j)
        {
            str[pos++] = rle->cells[i][j] ? 'o' : '_';
        }
        str[pos++] = '\n';
    }
    str[pos] = '\0'; // Null-terminating the string

    return str;
}

void freeRLE(RLE rle)
{
    for (size_t i = 0; i < rle.height; ++i)
    {
        free(rle.cells[i]);
    }
    free(rle.cells);
}

// WEB FUNCS

EM_JS(int, getH, (), {
    return window.innerHeight;
});

EM_JS(int, getW, (), {
    return window.innerWidth;
});

EM_ASYNC_JS(char *, getClipText, (), {
    try
    {
        const text = await navigator.clipboard.readText();
        const length = lengthBytesUTF8(text) + 1;
        const buffer = _malloc(length);
        if (buffer == 0)
        {
            throw 'malloc failed';
        }
        stringToUTF8(text, buffer, length);
        return buffer;
    }
    catch (err)
    {
        console.error('Failed to read from clipboard:', err);
        return 0;
    }
});

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static const int screenWidth = 800;
static const int screenHeight = 600;

static bool paused = false;
static bool eraseMode = false;
static World w = {0};

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);        // Initialize game
static void UpdateGame(void);      // Update game (one frame)
static void DrawGame(void);        // Draw game (one frame)
static void UnloadGame(void);      // Unload game
static void UpdateDrawFrame(void); // Update and Draw (one frame)

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization (Note windowTitle is unused on Android)
    //---------------------------------------------------------
    ROWS = SCREEN_HEIGHT / CELL_HEIGHT;
    COLS = SCREEN_WIDTH / CELL_WIDTH;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#if defined(PLATFORM_WEB)
    ROWS = getH() / CELL_HEIGHT;
    COLS = getW() / CELL_WIDTH;
    InitWindow(getW(), getH(), "game-of-life");
#else
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "game-of-life");
#endif
    InitGame();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        UpdateDrawFrame();
    }
#endif
    UnloadGame();  // Unload loaded data (textures, sounds, models...)
    CloseWindow(); // Close window and OpenGL context
    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//------------------------------------------------------------------------------------

// Initialize game variables
void InitGame(void)
{
    w = NewWorld(&w, 50);
}

// Update game (one frame)
void UpdateGame()
{
    if (IsKeyPressed(KEY_ONE))
    {
        if (CELL_HEIGHT > 1 && CELL_WIDTH > 1)
        {
            CELL_HEIGHT -= 1;
            CELL_WIDTH -= 1;
            ROWS = GetScreenHeight() / CELL_HEIGHT;
            COLS = GetScreenWidth() / CELL_WIDTH;
            w = NewWorld(&w, 30);
        }
    }

    if (IsKeyPressed(KEY_TWO))
    {
        CELL_HEIGHT += 1;
        CELL_WIDTH += 1;
        ROWS = GetScreenHeight() / CELL_HEIGHT;
        COLS = GetScreenWidth() / CELL_WIDTH;
        w = NewWorld(&w, 30);
    }

    if (IsKeyPressed(KEY_THREE))
    {
        if (UPDATE_SPEED > 0)
        {
            UPDATE_SPEED -= 1;
        }
    }

    if (IsKeyPressed(KEY_FOUR))
    {
        UPDATE_SPEED += 1;
    }

    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE))
    {
        paused = !paused;
    }

    if (IsKeyPressed(KEY_M))
    {
        eraseMode = !eraseMode;
    }

    if (IsKeyDown(KEY_R))
    {
        w = WithRandom(&w, 50);
    }

    if (IsKeyPressed(KEY_A))
    {
        w = WithRandom(&w, -1);
    }

    if (IsKeyPressed(KEY_D))
    {
        w = WithRandom(&w, 101);
    }

    if (IsKeyPressed(KEY_H))
    {
        // #if defined(PLATFORM_WEB)
        const char *clpText = getClipText();
        // #else
        // const char *clpText = GetClipboardText();
        // #endif
        char *clipboardText = strdup(clpText);
        if (clpText == NULL)
        {
            TraceLog(LOG_ERROR, "cannot get clipboard text");
        }
        RLE rle = NewRLE(clipboardText);

        // char *str = RLEToString(&rle);
        // TraceLog(LOG_INFO, str);

        Vector2 pos = GetMousePosition();
        pos.x /= CELL_WIDTH;
        pos.y /= CELL_HEIGHT;
        w = SpawnFromRle(&w, &rle, (size_t)pos.x, (size_t)pos.y);

        free(clipboardText);
        freeRLE(rle);
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !eraseMode)
    {
        Vector2 pos = GetMousePosition();
        pos.x /= CELL_WIDTH;
        pos.y /= CELL_HEIGHT;
        w.cells[(int)(pos.y)][(int)(pos.x)].alive = true;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && eraseMode)
    {
        Vector2 pos = GetMousePosition();
        pos.x /= CELL_WIDTH;
        pos.y /= CELL_HEIGHT;
        for (int r = -ERASE_AREA; r <= ERASE_AREA; r++)
        {
            for (int c = -ERASE_AREA; c <= ERASE_AREA; c++)
            {
                if (pos.y + c >= 0 && pos.y + c < w.width && pos.x + r >= 0 && pos.x + r < w.height)
                {
                    w.cells[(int)pos.y + c][(int)pos.x + r].alive = false;
                }
            }
        }
    }

    if (!paused && w.framesSinceLastUpd > UPDATE_SPEED)
    {
        w = NextState(&w);
        w.framesSinceLastUpd = 0;
    }
    else
    {
        w.framesSinceLastUpd += 1;
    }
}

// Draw game (one frame)
void DrawGame(void)
{
    BeginDrawing();

    ClearBackground(BLACK);

    // Draw player bar
    for (int r = 0; r < ROWS; r++)
    {
        for (int c = 0; c < COLS; c++)
        {
            Color cellColor = w.cells[r][c].alive == true ? GetColor(ALIVE_COLOR) : GetColor(DEAD_COLOR);
            DrawRectangle(c * CELL_WIDTH, r * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT, cellColor);

            DrawRectangleLines(c * CELL_WIDTH, r * CELL_HEIGHT, CELL_WIDTH, CELL_HEIGHT, GetColor(GRID_COLOR));
        }
    }

    if (eraseMode)
    {
        Vector2 pos = GetMousePosition();
        pos.x /= CELL_WIDTH;
        pos.y /= CELL_HEIGHT;
        Rectangle rect = {(int)pos.x * CELL_WIDTH - (ERASE_AREA * CELL_WIDTH), (int)pos.y * CELL_HEIGHT - (ERASE_AREA * CELL_HEIGHT), CELL_WIDTH * 2 * ERASE_AREA + CELL_WIDTH, CELL_HEIGHT * 2 * ERASE_AREA + CELL_HEIGHT};

        DrawRectangleLinesEx(rect, 2, WHITE);
    }

    if (paused)
    {
        DrawText("GAME PAUSED", GetScreenWidth() / 2 - MeasureText("GAME PAUSED", 40) / 2, GetScreenHeight() / 2 - 40, 40, WHITE);
    }
    EndDrawing();
}

// Unload game variables
void UnloadGame(void)
{
    // TODO: Unload all dynamic loaded data (textures, sounds, models...)
}

// Update and Draw (one frame)
void UpdateDrawFrame(void)
{
    UpdateGame();
    DrawGame();
}