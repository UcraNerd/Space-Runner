#include <raylib.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

// Compile on wayland
// g++ -o SR game.cpp -Iinclude -Llib -lraylib -lwayland-client -lGL -lopenal -lm -pthread -ldl

// Constants
const int screenWidth = 600; // larghezza finestra
const int screenHeight = 900; // altezza finestra
const int fontSize = 30; // dimensione del font
const float laneWidth = float(screenWidth) / 3; // larghezza di una corsia
const float spaceshipSpeed = 1500; // velocità navicella
const int maxAsteroids = 20; // asteroidi massimi per l'array di asteroidi
const int asteroidsSpeed = 1000; // velocità asteroidi
const float asteroidSpawnInterval = 0.7; // intervallo di spawn asteroidi
const int maxDistance = 10000; // distanza da raggiungere
const float distanceUpdateInterval = 0.5; // intervallo di aggiornamento della distanza
const int distanceIncrement = 100; // unità d'incremento della distanza

// Structures
typedef struct Spaceship {
    Texture2D texture;
    Vector2 position;
    int currentLane;
} Spaceship;

typedef struct Asteroid {
    Texture2D texture;
    Vector2 position;
    int currentLane;
} Asteroid;

// Global Variables
// Navicella
Texture2D background;
Spaceship spaceship;
Vector2 targetPosition;

// Asteroidi
Texture2D asteroidTexture;
Asteroid asteroids[maxAsteroids];
bool lastTwoAsteroids[2][3] = { {false} };
int stackedLane = 0; // corsia libera per più di 2 volte
int asteroidCount = 0; // asteroidi in campo
double lastAsteroidSpawnTime = 0.0;

// Schermate
bool gameMenu = true;
bool gamePause = false;
bool gameOver = false;
bool gameWon = false;
double totalPausedTime = 0.0;
double pauseStartTime = 0.0;

// Distanza
int metersTraveled = 0;
double lastDistanceUpdateTime = 0.0;

// Functions
// Funzioni di base del gioco
void InitGame();
void UpdateGame();
void DrawGame();
void UnloadGame();

// Funzioni per le schermate
void StartMenu();
void Text(const char* text, int textY, Color textColor);
void PauseGame();
void GameOverScreen();
void GameWinScreen();

// Funzioni del gioco
void MoveSpaceship(int direction);
void SpawnAsteroid();
bool CheckCollision(Asteroid asteroid, Spaceship spaceship);
void UpdateDistance();

int main() {
    // Initialize window
    InitWindow(screenWidth, screenHeight, "GO INTO THE SPACE");

    // Set frames per second
    SetTargetFPS(60);

    InitGame();

    // The Game Loop
    while (!WindowShouldClose()) {
        UpdateGame();
        DrawGame();
    }

    UnloadGame();
    CloseWindow();
    return 0;
}

void InitGame() {
    // Load Textures
    background = LoadTexture("assets/Background.png");
    spaceship.texture = LoadTexture("assets/spaceship.png");
    asteroidTexture = LoadTexture("assets/asteroid.png");

    // Inizializza la posizione della navicella
    spaceship.currentLane = 1; // Inizia nella lane centrale (0, 1, 2)
    spaceship.position.x = laneWidth * spaceship.currentLane + (laneWidth - spaceship.texture.width) / 2;
    spaceship.position.y = screenHeight - spaceship.texture.height - 20; // Posiziona in basso
    targetPosition = spaceship.position; // La posizione di destinazione iniziale è quella corrente

    // Inizializza le variabili di gioco
    srand(time(NULL));
    totalPausedTime = 0.0;

    metersTraveled = 0;
    lastDistanceUpdateTime = GetTime() - totalPausedTime;

    asteroidCount = 0;
    lastAsteroidSpawnTime = GetTime() - totalPausedTime;

    gameOver = false;
    gameWon = false;
}

void UpdateGame() {
    float dt = GetFrameTime();
    double currentTime = GetTime() - totalPausedTime;

    if (gameOver || gameWon || gamePause) return; // Ferma il gioco se Game Over, Game Won oppure Pausa

    // Aggiorna la distanza percorsa ogni intervallo di aggiornamento
    if (currentTime - lastDistanceUpdateTime >= distanceUpdateInterval) {
        UpdateDistance();
        lastDistanceUpdateTime = currentTime;
    }

    // Movimenti della navicella
    if (IsKeyPressed(KEY_LEFT)) {
        MoveSpaceship(-1);
    }
    if (IsKeyPressed(KEY_RIGHT)) {
        MoveSpaceship(1);
    }

    // Interpolazione della posizione della navicella
    float step = spaceshipSpeed * dt; // La velocità di movimento per frame
    if (fabs(spaceship.position.x - targetPosition.x) > step) { // fabs() valore assoluto
        if (targetPosition.x - spaceship.position.x > 0) {
            spaceship.position.x += step;
        } else {
            spaceship.position.x -= step;
        }
    } else {
        spaceship.position.x = targetPosition.x;
    }

    // Rimuovi gli asteroidi a fine schermo e sposta tutto a sinistra
    int i = 0;
    while (i < asteroidCount) {
        if (asteroids[i].position.y > screenHeight) {
            for (int j = i; j < asteroidCount - 1; j++) {
                asteroids[j] = asteroids[j + 1];
            }
            asteroidCount--;
        } else {
            i++;
        }
    }

    // Controlla le collisioni con gli asteroidi
    for (int i = 0; i < asteroidCount; i++) {
        if (CheckCollision(asteroids[i], spaceship)) {
            gameOver = true;
        }
    }

    // Controlla la vittoria
    if (metersTraveled >= maxDistance) {
        gameWon = true;
    }

    // Genera nuovi asteroidi
    if (!gamePause && !gameMenu && !gameOver && !gameWon && currentTime - lastAsteroidSpawnTime >= asteroidSpawnInterval) {
        SpawnAsteroid();
        lastAsteroidSpawnTime = currentTime;
    }

    // Sposta gli asteroidi
    for (int i = 0; i < asteroidCount; i++) {
        asteroids[i].position.y += asteroidsSpeed * dt;
    }
}

void DrawGame() {
    // Controlla se enter é stato premuto
    if (gameMenu && IsKeyPressed(KEY_ENTER)) {
        gameMenu = false;
    }

    // Controlla se il gioco va messo in pausa
    if (IsKeyPressed(KEY_SPACE)) {
        if (!gamePause && !gameOver && !gameWon) {
            gamePause = true;
            pauseStartTime = GetTime();  // memorizza quando è iniziata la pausa
        } else {
            gamePause = false;
            totalPausedTime += GetTime() - pauseStartTime; // somma il tempo passato in pausa
        }
    }

    // Draw Canvas
    BeginDrawing();

    // Setup Canvas
    ClearBackground(BLACK);

    // Background
    DrawTexture(background, 0, 0, WHITE);

    if (gameMenu) {
        StartMenu();
    } else if (gameOver) {
        GameOverScreen();
    } else if (gameWon) {
        GameWinScreen();
    } else if (gamePause) {
        PauseGame();
    } else {
        // Draw Spaceship
        DrawTexture(spaceship.texture, spaceship.position.x, spaceship.position.y, WHITE);

        // Draw asteroids
        for (int i = 0; i < asteroidCount; i++) {
            DrawTexture(asteroids[i].texture, asteroids[i].position.x, asteroids[i].position.y, WHITE);
        }

        // Draw distance traveled
        char distanceText[32];
        sprintf(distanceText, "Distanza: %d/%d Mm", metersTraveled, maxDistance);
        DrawText(distanceText, 10, 10, 20, WHITE);
    }

    // Teardown Canvas
    EndDrawing();
}

void StartMenu() {
    int textY = 100;
    const char* titolo = "SCOPO DEL GIOCO";
    const char* point1 = "Raggiungere i 10000 Mega metri";
    const char* point2 = "Evitare gli asteroidi";
    const char* point3 = "Usa i tasti <- -> per muoverti";
    const char* point4 = "Metti in pausa con [SPACE]";
    const char* start = "Premi [ENTER] per iniziare";
    Text(titolo, textY, WHITE);
    Text(point1, textY+50, WHITE);
    Text(point2, textY+100, WHITE);
    Text(point3, textY+150, WHITE);
    Text(point4, textY+200, WHITE);
    Text(start, screenHeight - 100, WHITE);
}

void PauseGame() {
    const char* warning = "IL GIOCO É IN PAUSA";
    Font font = GetFontDefault(); // (You can load custom fonts too)
    Vector2 textSize = MeasureTextEx(font, warning, fontSize + 20, 5);
    int textX = (screenWidth - textSize.x) / 2;
    int textY = (screenHeight - textSize.y) / 2;
    DrawTextEx(font, warning, Vector2({float(textX), float(textY)}), fontSize + 20, 5, RED);
}

void Text(const char* text, int textY, Color textColor) {
    Font font = GetFontDefault(); // (You can load custom fonts too)
    Vector2 textSize = MeasureTextEx(font, text, fontSize, 5);
    int textX = (screenWidth - textSize.x) / 2;
    DrawTextEx(font, text, Vector2({float(textX), float(textY)}), fontSize, 5, textColor);
}

void SpawnAsteroid() {
    // Pick two different valid lanes
    int selectedLane1 = stackedLane;
    int selectedLane2 = rand() % 3;

    // Ensure they're not the same lane
    while (selectedLane2 == selectedLane1) {
        selectedLane2 = rand() % 3;
    }

    // salva le ultime due righe di asteriodi
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            if (i == 1) {
                if (j == selectedLane1 || j == selectedLane2) {
                    lastTwoAsteroids[i][j] = true;
                }
                else {
                    lastTwoAsteroids[i][j] = false;
                }

                if (lastTwoAsteroids[0][j] == false && lastTwoAsteroids[i][j] == false) {
                    stackedLane = j;
                }
            }
            else {
                lastTwoAsteroids[i][j] = lastTwoAsteroids[1][j];
            }
        }
    }

    // Spawn first asteroid
    asteroids[asteroidCount].texture = asteroidTexture;
    asteroids[asteroidCount].currentLane = selectedLane1;
    asteroids[asteroidCount].position.x = laneWidth * selectedLane1 + (laneWidth - asteroidTexture.width) / 2;
    asteroids[asteroidCount].position.y = -asteroidTexture.height;
    asteroidCount++;

    // Spawn second asteroid
    asteroids[asteroidCount].texture = asteroidTexture;
    asteroids[asteroidCount].currentLane = selectedLane2;
    asteroids[asteroidCount].position.x = laneWidth * selectedLane2 + (laneWidth - asteroidTexture.width) / 2;
    asteroids[asteroidCount].position.y = -asteroidTexture.height;
    asteroidCount++;
}

void MoveSpaceship(int direction) {
    int newLane = spaceship.currentLane + direction;
    if (newLane >= 0 && newLane < 3) {
        spaceship.currentLane = newLane;
        targetPosition.x = laneWidth * spaceship.currentLane + (laneWidth - spaceship.texture.width) / 2;
    }
}

void UpdateDistance() {
    if (metersTraveled < maxDistance) {
        metersTraveled += distanceIncrement;
        if (metersTraveled > maxDistance) {
            metersTraveled = maxDistance;
        }
    }
}

bool CheckCollision(Asteroid asteroid, Spaceship spaceship) {
    // Controlla le eventuali collisioni
    if (asteroid.currentLane == spaceship.currentLane) {
        if (asteroid.position.y + asteroid.texture.height > spaceship.position.y) {
            return true;
        }
    }
    return false;
}

void GameOverScreen() {
    const char* gameOverMessage = "HAI PERSO!";
    const char* start = "Premi [ENTER] per riprovare";
    char distanceText[32];
    sprintf(distanceText, "SCORE: %d/%d Mm", metersTraveled, maxDistance);

    Font font = GetFontDefault();
    Vector2 textSize = MeasureTextEx(font, gameOverMessage, fontSize + 20, 5);
    int textX = (screenWidth - textSize.x) / 2;
    int textY = (screenHeight - textSize.y) / 2;
    DrawTextEx(font, gameOverMessage, Vector2({float(textX), float(textY)}), fontSize + 20, 5, RED);
    Text(distanceText, textY+100, WHITE);
    Text(start, screenHeight - 100, WHITE);

    // Restart the game if ENTER is pressed
    if (IsKeyPressed(KEY_ENTER)) {
        InitGame();
    }
}

void GameWinScreen() {
    const char* winMessage = "HAI VINTO!";
    const char* credits = "SVILUPPATORI";
    const char* dev1 = "Tsymbalenko Stanislav";
    const char* dev2 = "Sartor Cristian";
    const char* dev3 = "Brändle Mathias";
    const char* start = "Premi [ENTER] per iniziare";

    Font font = GetFontDefault();
    Vector2 textSize = MeasureTextEx(font, winMessage, fontSize + 20, 5);
    int textX = (screenWidth - textSize.x) / 2;
    int textY = 100;
    DrawTextEx(font, winMessage, Vector2({float(textX), float(textY)}), fontSize + 20, 5, GREEN);
    Text(credits, textY+100, YELLOW);
    Text(dev1, textY+150, WHITE);
    Text(dev2, textY+200, WHITE);
    Text(dev3, textY+250, WHITE);
    Text(start, screenHeight - 100, WHITE);

    // Restart the game if ENTER is pressed
    if (IsKeyPressed(KEY_ENTER)) {
        InitGame();
    }
}

void UnloadGame() {
    UnloadTexture(background);
    UnloadTexture(spaceship.texture);
    UnloadTexture(asteroidTexture);
}
