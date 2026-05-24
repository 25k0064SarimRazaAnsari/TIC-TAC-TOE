#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Quick Initializations of Constants
#define WINDOW_WIDTH 900  // Width of Game Window
#define WINDOW_HEIGHT 700 // Height of Game Window

#define MAX_USERS 1024 // Max amount of Users to be stored in .txt file
#define NAME_LENGTH 64 // Max length that a Username can have
#define PASS_LENGTH 64 // Max length that a Password can have

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Struct for Player's Info
typedef struct
{
    char name[NAME_LENGTH];
    char pass[PASS_LENGTH];
    int wins;
    int losses;
    int draws;
    float wlratio;
    int gamesPlayed;
} User;  // --> Alias Datatype


// Struct for Game Board
typedef struct
{
    int cells[3][3];  // 0 -> Empty Cell, 1 -> Player 1, 2 -> Player 2
} Board;  // --> Alias Datatype


// Game States/Windows
typedef enum
{
    SCR_INTRO = 1,      // = 1
    SCR_LOGIN,          // = 2
    SCR_TITLE,          // = 3
    SCR_SETTINGS,       // = 4
    SCR_LEADERBOARD,    // = 5
    SCR_GAME,           // = 6
    SCR_GAMEOVER        // = 7
} Screen;   // --> Alias


//_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________

// FUNCTION DEFINITIONS
void LoadUsers(const char *fname, User users[], int *count);
void SaveUsers(const char *fname, User users[], int count);
int FindUser(User users[], int count, const char *name);
int RegisterUser(User users[], int *count, const char *name, const char *pass);
int ValidateLogin(User users[], int count, const char *name, const char *pass);

void UpdateMusicIfNeeded(Music *background, bool musicOn, bool *musicPlaying);

bool GuiButtonWithHover(Rectangle bounds, const char *text, Sound *hoverSound, bool *hoverState);

void ResetBoard(Board *board);
bool BoardFull(Board *board);
bool CheckWin(Board *board, int player);

void ApplyGameResultAndSave(User users[], int userCount, int p1_index, int p2_index, int winner, bool isDraw, bool *statsUpdated);

void SortLeaderboard(User users[], int count);


//_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________

// FUNCTION DEFINITIONS

// Leaderboard Sort
void SortLeaderboard(User users[], int count)
{
    for (int i = 0; i < count - 1; i++)
    {
        for (int j = 0; j < count - i - 1; j++)
        {
            if (users[j].wlratio < users[j + 1].wlratio)
            {
                User temp = users[j];
                users[j] = users[j + 1];
                users[j + 1] = temp;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function for Loading User's data, from DB (.txt file) into array of Users
void LoadUsers(const char *fname, User users[], int *count)   // fname = file name, users[] = array of users (structs), count = no. of users in that array
{
    *count = 0; // Initial no. of users is set to 0 (Whether file exists or not)

    FILE *usersDB = fopen(fname, "r"); // Locates address and OPENS file 'usersDB' in read (r) mode
    if (!usersDB)
        return; // If file doesn't exist, then it returns out.

    char line[256]; // Temporary buffer to store one line from file [Max characters (255) + null terminator (\0)]

    while (fgets(line, sizeof(line), usersDB) && *count < MAX_USERS) // Reads one line at a time from the file (usersDB).. Stops when MAX USERS (1024) is reached
    {
        User u = {0};   // Alias for User Struct -> fully initializes struct to zero

        int scanned = sscanf(line, "%63s %63s %d %d %d %f %d", u.name, u.pass, &u.wins, &u.losses, &u.draws, &u.wlratio, &u.gamesPlayed);

        if (scanned == 7)   // Checks if user is valid (by checking ALL 6 of it's data entries)
        {
            if (u.gamesPlayed > 0) u.wlratio = (float)u.wins / (u.gamesPlayed + 5);
            else u.wlratio = 0.0f;

            users[*count] = u;   // Copies (LOADS) user, from DB (.txt file) into users array..
            (*count)++;   // Increments User count by 1..
        }
    }
    fclose(usersDB); // Close file 'usersDB'
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function for Saving User's data, from users array, into DB (.txt file)
void SaveUsers(const char *fname, User users[], int count) // fname = file name, users[] = array of users (structs), count = no. of users in that array
{
    FILE *usersDB = fopen(fname, "w"); // Locates address and OPENS file 'usersDB' in write (w) mode
    // If file doesn't exists, creates file.
    // If file exists, overwrites file

    if (!usersDB) return;   // ERROR: File failed to open

    for (int i = 0; i < count; i++) // Writes every registered users' data into file..
    {
        // Recalculate W/L ratio properly BEFORE saving
        if (users[i].gamesPlayed > 0) users[i].wlratio = (float)users[i].wins / (users[i].gamesPlayed + 5);
        else users[i].wlratio = 0.0f;
    
        // Now save the UPDATED ratio
        fprintf(usersDB, "%s %s %d %d %d %.2f %d\n",
                users[i].name, users[i].pass,
                users[i].wins, users[i].losses, users[i].draws,
                users[i].wlratio, users[i].gamesPlayed);
    }

    fclose(usersDB);   // Close file 'usersDB'
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function for Finding Username in DB (.txt file)
int FindUser(User users[], int count, const char *name) // users[] = user array, count = users Count, name = Entered name
{
    for (int i = 0; i < count; i++)
        if (strcmp(users[i].name, name) == 0)
            return i; // Compares Entered Username, with Username at index i, in DB. If Found, return the found Username's index
    return -1;        // ERROR: Username not Found (No Username like the one entered, exists in DB)
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Register User Function
int RegisterUser(User users[], int *count, const char *name, const char *pass) // users[] = user array, count = users Count, name = Entered name, pass = Entered password
{
    if (*count >= MAX_USERS)
        return -1; // No Space for any more NEW Users
    if (FindUser(users, *count, name) >= 0)
        return -2; // Check if Username already exists...
    if (strlen(name) == 0 || strlen(pass) == 0)
        return -3; // Username and Password required in their fields
    if (strchr(name, ' ') || strchr(pass, ' '))
        return -4; // No Spaces are allowed in Username/Password

    User u; // Alias for User struct

    strncpy(u.name, name, NAME_LENGTH - 1); // Copies Entered Username into DB's Username
    u.name[NAME_LENGTH - 1] = 0;            // Ensures last element of username array is null terminator (\0)

    strncpy(u.pass, pass, PASS_LENGTH - 1); // Copies Entered Password into DB's Password
    u.pass[PASS_LENGTH - 1] = 0;            // Ensures last element of password array is null terminator (\0)

    // Sets the new user's wins, losses, draws, gamesPlayed, and w/l ratio --> 0
    u.wins = 0;
    u.losses = 0;
    u.draws = 0;
    u.gamesPlayed = 0;
    u.wlratio = 0.0f;

    users[*count] = u;   // Adds new user to array
    (*count)++;          // +1 to User Count in .txt file
    return (*count) - 1; // ID (index) assigned to this User
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// LOG IN Validation Function
int ValidateLogin(User users[], int count, const char *name, const char *pass) // users[] = user array, count = users Count, name = Entered username, pass = Entered password
{
    int idx = FindUser(users, count, name); // Calls Function to Find User..
    if (idx < 0)
        return -1; // If User is NOT found, then return -1 (ERROR: User Not Found - Register first)
    if (strcmp(users[idx].pass, pass) == 0)
        return idx; // If User's password and Stored Password matches, return user index (Logged In)
    return -2;      // Else Wrong Password Entered
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Music Update Function
void UpdateMusicIfNeeded(Music *backgroundMusic, bool musicOn, bool *musicPlaying)
{
    if (!backgroundMusic)
        return; // If There's no background music loaded at initialization

    if (musicOn) // If Music Shoud be ON
    {
        if (!(*musicPlaying)) // If music is NOT Playing
        {
            // Turn Music ON
            PlayMusicStream(*backgroundMusic);
            *musicPlaying = true;
        }
        UpdateMusicStream(*backgroundMusic);
    }
    else // If Music Shoud NOT be ON
    {
        if (*musicPlaying) // If music is Playing
        {
            // Turn Music OFF
            StopMusicStream(*backgroundMusic);
            *musicPlaying = false;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function for Making GUI Buttons with Hovering Function
bool GuiButtonWithHover(Rectangle bounds, const char *text, Sound *hoverSound, bool *hoverState)
{
    Vector2 mouse = GetMousePosition();
    bool isHovered = CheckCollisionPointRec(mouse, bounds);

    if (isHovered && !(*hoverState))
    {
        *hoverState = true;
        if (hoverSound)
            PlaySound(*hoverSound);
    }
    else if (!isHovered && *hoverState)
    {
        *hoverState = false;
    }

    return GuiButton(bounds, text);
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function for Resetting Game Board
void ResetBoard(Board *board)
{
    for (int x=0 ; x<3 ; x++)
    {
        for (int y=0 ; y<3 ; y++)
        {
            board->cells[x][y] = 0;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function for Checking if Game Board is FULL (Non-Empty)
bool BoardFull(Board *board)
{
    for (int x=0 ; x<3 ; x++)
    {
        for (int y=0 ; y<3 ; y++)
        {
            if (board->cells[x][y] == 0) return false;
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Function for Checking Win of a Player (returns 1 if Won, 0 if Not Won)
bool CheckWin(Board *board, int player)
{
    for (int i=0 ; i<3 ; i++)
    {
        if (board->cells[i][0] == player && board->cells[i][1] == player && board->cells[i][2] == player) return true;  // Checking of EACH (1-3) ROW
        if (board->cells[0][i] == player && board->cells[1][i] == player && board->cells[2][i] == player) return true;  // Checking of EACH (1-3) COLUMN
    }

    if (board->cells[0][0] == player && board->cells[1][1] == player && board->cells[2][2] == player) return true;  // Checking of 1st Diagonal
    if (board->cells[2][0] == player && board->cells[1][1] == player && board->cells[0][2] == player) return true;  // Checking of 2nd Diagonal

    return false;  // If No Win Condition is Found
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ApplyGameResultAndSave(User users[], int userCount, int p1_index, int p2_index, int winner, bool isDraw, bool *statsUpdated)
{
    if (*statsUpdated) return;  // Check if this Function is ALREADY Applied

    if (p1_index < 0 || p2_index < 0)  // NO VALID Players are Logged in
    {
        *statsUpdated = true;  // Prevent Repeated Attempts
        return;
    }

    if (isDraw)  // If the Game is a DRAW
    {
        users[p1_index].draws++;
        users[p2_index].draws++;
    }
    else if (winner == 1)  // If PLAYER 1 is the WINNER
    {
        users[p1_index].wins++;
        users[p2_index].losses++;
    }
    else if (winner == 2)  // If PLAYER 2 is the WINNER
    {
        users[p2_index].wins++;
        users[p1_index].losses++;
    }

    users[p1_index].gamesPlayed++;
    users[p2_index].gamesPlayed++;

    SaveUsers("users.txt", users, userCount);  // Saving of Stats in "users.txt" Text file, via above defined "SaveUsers" Function
    *statsUpdated = true;  // Tell that this Function is NOW Applied (No Repetition)
}



//_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________

// Main Function
int main(void)
{
    // Initialization
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "GUI Based Tic-Tac-Toe");
    InitAudioDevice();
    SetTargetFPS(240);

    //___________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________

    // Assets Loading

    //────────────────────────────────────────────────────────────
    // Testing Purposes..
    //────────────────────────────────────────────────────────────

    bool backgroundLoaded = false;

    //leaderboard-------------------------------------------------------------------------------------
    
    Texture2D leaderBackground = {0}; // Title BG
    if (FileExists("assets/images/TitleBackground/background.png"))
    {
        leaderBackground = LoadTexture("assets/images/Leaderboard/leaderBoard.png"); // Game Background
        backgroundLoaded = true;
    }
    Texture2D leaderBackground_hover = {0}; // Title BG
    if (FileExists("assets/images/TitleBackground/background.png"))
    {
        leaderBackground_hover = LoadTexture("assets/images/Leaderboard/leaderBoard_hover.png"); // Game Background
        backgroundLoaded = true;
    }

//-----------------------------------------------------------------------------------------------------------

    //────────────────────────────────────────────────────────────
    // Settings Screen
    //────────────────────────────────────────────────────────────

    Texture2D settingBackground = {0};
    Texture2D settingBackground_hover = {0};

    //combo
    Texture2D setting_P1circle_P2circle = {0};
    Texture2D setting_P1circle_P2cross = {0};
    Texture2D setting_P1circle_P2diamond = {0};
    Texture2D setting_P1circle_P2heart = {0};

    Texture2D setting_P1cross_P2circle = {0};
    Texture2D setting_P1cross_P2cross = {0};
    Texture2D setting_P1cross_P2diamond = {0};
    Texture2D setting_P1cross_P2heart = {0};

    Texture2D setting_P1diamond_P2circle = {0};
    Texture2D setting_P1diamond_P2cross = {0};
    Texture2D setting_P1diamond_P2diamond = {0};
    Texture2D setting_P1diamond_P2heart = {0};

    Texture2D setting_P1heart_P2circle = {0};
    Texture2D setting_P1heart_P2cross = {0};
    Texture2D setting_P1heart_P2diamond = {0};
    Texture2D setting_P1heart_P2heart = {0};

    if (FileExists("assets/images/settings/settingBackground.png"))
    {
        settingBackground = LoadTexture("assets/images/settings/settingBackground.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/setting_hover.png"))
    {
        settingBackground_hover = LoadTexture("assets/images/settings/setting_hover.png");
        backgroundLoaded = true;
    }

    //combo 
    if (FileExists("assets/images/settings/P1circle_P2circle.png"))
    {
        setting_P1circle_P2circle = LoadTexture("assets/images/settings/P1circle_P2circle.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/P1circle_P2cross.png"))
    {
        setting_P1circle_P2cross = LoadTexture("assets/images/settings/P1circle_P2cross.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/P1circle_P2diamond.png"))
    {
        setting_P1circle_P2diamond = LoadTexture("assets/images/settings/P1circle_P2diamond.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/P1circle_P2heart.png"))
    {
        setting_P1circle_P2heart = LoadTexture("assets/images/settings/P1circle_P2heart.png");
        backgroundLoaded = true;
    }

    if (FileExists("assets/images/settings/P1cross_P2circle.png"))
    {
        setting_P1cross_P2circle = LoadTexture("assets/images/settings/P1cross_P2circle.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/P1cross_P2cross.png"))
    {
        setting_P1cross_P2cross = LoadTexture("assets/images/settings/P1cross_P2cross.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/P1cross_P2diamond.png"))
    {
        setting_P1cross_P2diamond = LoadTexture("assets/images/settings/P1cross_P2diamond.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/P1cross_P2heart.png"))
    {
        setting_P1cross_P2heart = LoadTexture("assets/images/settings/P1cross_P2heart.png");
        backgroundLoaded = true;
    }

    if (FileExists("assets/images/settings/P1diamond_P2circle.png"))
    {
        setting_P1diamond_P2circle = LoadTexture("assets/images/settings/P1diamond_P2circle.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/P1diamond_P2cross.png"))
    {
        setting_P1diamond_P2cross = LoadTexture("assets/images/settings/P1diamond_P2cross.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/P1diamond_P2diamond.png"))
    {
        setting_P1diamond_P2diamond = LoadTexture("assets/images/settings/P1diamond_P2diamond.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/P1diamond_P2heart.png"))
    {
        setting_P1diamond_P2heart = LoadTexture("assets/images/settings/P1diamond_P2heart.png");
        backgroundLoaded = true;
    }

    if (FileExists("assets/images/settings/P1heart_P2circle.png"))
    {
        setting_P1heart_P2circle = LoadTexture("assets/images/settings/P1heart_P2circle.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/P1heart_P2cross.png"))
    {
        setting_P1heart_P2cross = LoadTexture("assets/images/settings/P1heart_P2cross.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/P1heart_P2diamond.png"))
    {
        setting_P1heart_P2diamond = LoadTexture("assets/images/settings/P1heart_P2diamond.png");
        backgroundLoaded = true;
    }
    if (FileExists("assets/images/settings/P1heart_P2heart.png"))
    {
        setting_P1heart_P2heart = LoadTexture("assets/images/settings/P1heart_P2heart.png");
        backgroundLoaded = true;
    }

    //────────────────────────────────────────────────────────────
    // Title Screen
    //────────────────────────────────────────────────────────────

    Texture2D gameBackground = {0}; // Title BG
    if (FileExists("assets/images/TitleBackground/background.png"))
    {
        gameBackground = LoadTexture("assets/images/TitleBackground/background.png"); // Game Background
        backgroundLoaded = true;
    }

    Texture2D hoverStartGame_TS = {0}; // start game
    if (FileExists("assets/images/TitleBackground/hoverStartGame_TS.png"))
    {
        hoverStartGame_TS = LoadTexture("assets/images/TitleBackground/hoverStartGame_TS.png");
        backgroundLoaded = true;
    }

    Texture2D hoverSetting_TS = {0}; // setting
    if (FileExists("assets/images/TitleBackground/hoverSetting_TS.png"))
    {
        hoverSetting_TS = LoadTexture("assets/images/TitleBackground/hoverSetting_TS.png");
        backgroundLoaded = true;
    }

    Texture2D hoverLeaderBoard_TS = {0}; // leader
    if (FileExists("assets/images/TitleBackground/hoverLeaderBoard_TS.png"))
    {
        hoverLeaderBoard_TS = LoadTexture("assets/images/TitleBackground/hoverLeaderBoard_TS.png");
        backgroundLoaded = true;
    }

    Texture2D hoverLogout_TS = {0}; // logout
    if (FileExists("assets/images/TitleBackground/hoverLogout_TS.png"))
    {
        hoverLogout_TS = LoadTexture("assets/images/TitleBackground/hoverLogout_TS.png");
        backgroundLoaded = true;
    }

    Texture2D hoverQuit_TS = {0}; // quit
    if (FileExists("assets/images/TitleBackground/hoverQuit_TS.png"))
    {
        hoverQuit_TS = LoadTexture("assets/images/TitleBackground/hoverQuit_TS.png");
        backgroundLoaded = true;
    }

    //────────────────────────────────────────────────────────────
    // Intro Screen
    //────────────────────────────────────────────────────────────

    //--------------------------------------------------------------------------------------------------------
    Texture2D introBackground = {0}; // Intro Screen Background Initialization
    if (FileExists("assets/images/IntroBackground/intro_background.png"))
    {
        introBackground = LoadTexture("assets/images/IntroBackground/intro_background.png"); // Intro Screen Background
        backgroundLoaded = true;
    }

    //────────────────────────────────────────────────────────────
    // Login Screen
    //────────────────────────────────────────────────────────────

    Texture2D loginBackground = {0};   // Login Screen Background Initialization
    if (FileExists("assets/images/LoginBackground/login_background.png"))
    {
        loginBackground = LoadTexture("assets/images/LoginBackground/login_background.png"); // Login Screen Background
        backgroundLoaded = true;
    }

    Texture2D hoverContinue_loginBackground = {0}; // (Hover on Continue Button) Login Screen Background Initialization
    if (FileExists("assets/images/LoginBackground/hoverContinue_login_background.png"))
    {
        hoverContinue_loginBackground = LoadTexture("assets/images/LoginBackground/hoverContinue_login_background.png"); // (Hover on Continue Button) Login Screen Background
        backgroundLoaded = true;
    }

    Texture2D hoverP1Login_loginBackground = {0}; // (Hover on P1 Login Button) Login Screen Background Initialization
    if (FileExists("assets/images/LoginBackground/hoverP1Login_login_background.png"))
    {
        hoverP1Login_loginBackground = LoadTexture("assets/images/LoginBackground/hoverP1Login_login_background.png"); // (Hover on P1 Login Button) Login Screen Background
        backgroundLoaded = true;
    }

    Texture2D hoverP2Login_loginBackground = {0}; // (Hover on P2 Login Button) Login Screen Background Initialization
    if (FileExists("assets/images/LoginBackground/hoverP2Login_login_background.png"))
    {
        hoverP2Login_loginBackground = LoadTexture("assets/images/LoginBackground/hoverP2Login_login_background.png"); // (Hover on P2 Login Button) Login Screen Background
        backgroundLoaded = true;
    }

    Texture2D hoverP1Register_loginBackground = {0}; // (Hover on P1 Register Button) Login Screen Background Initialization
    if (FileExists("assets/images/LoginBackground/hoverP1Register_login_background.png"))
    {
        hoverP1Register_loginBackground = LoadTexture("assets/images/LoginBackground/hoverP1Register_login_background.png");   // (Hover on P1 Register Button) Login Screen Background
        backgroundLoaded = true;
    }

    Texture2D hoverP2Register_loginBackground = {0}; // (Hover on P2 Register Button) Login Screen Background Initialization
    if (FileExists("assets/images/LoginBackground/hoverP2Register_login_background.png"))
    {
        hoverP2Register_loginBackground = LoadTexture("assets/images/LoginBackground/hoverP2Register_login_background.png");   // (Hover on P2 Register Button) Login Screen Background
        backgroundLoaded = true;
    }

    //────────────────────────────────────────────────────────────
    // Game Screen
    //────────────────────────────────────────────────────────────

    Texture2D red_GameBackground = {0};   // Red Game Screen Background Initialization
    if (FileExists("assets/images/GameBackground/red_game_background.png"))
    {
        red_GameBackground = LoadTexture("assets/images/GameBackground/red_game_background.png");   // Red Game Screen Background
        backgroundLoaded = true;
    }

    Texture2D hoverForfeit_red_GameBackground = {0};   // (Hover on Forfeit Button) Red Game Screen Background Initialization
    if (FileExists("assets/images/GameBackground/hoverForfeit_red_game_background.png"))
    {
        hoverForfeit_red_GameBackground = LoadTexture("assets/images/GameBackground/hoverForfeit_red_game_background.png");   // (Hover on Forfeit Button) Red Game Screen Background
        backgroundLoaded = true;
    }

    Texture2D blue_GameBackground = {0};   // Blue Game Screen Background Initialization
    if (FileExists("assets/images/GameBackground/blue_game_background.png"))
    {
        blue_GameBackground = LoadTexture("assets/images/GameBackground/blue_game_background.png");   // Blue Game Screen Background
        backgroundLoaded = true;
    }

    Texture2D hoverForfeit_blue_GameBackground = {0};   // (Hover on Forfeit Button) Blue Game Screen Background Initialization
    if (FileExists("assets/images/GameBackground/hoverForfeit_blue_game_background.png"))
    {
        hoverForfeit_blue_GameBackground = LoadTexture("assets/images/GameBackground/hoverForfeit_blue_game_background.png");   // (Hover on Forfeit Button) Blue Game Screen Background
        backgroundLoaded = true;
    }

    // --- Cell ---

    Texture2D cell_GameBackground = {0};   // Cell Background Initialization
    if (FileExists("assets/images/GameBackground/cell_game_background.png"))
    {
        cell_GameBackground = LoadTexture("assets/images/GameBackground/cell_game_background.png");   // Cell Background
        backgroundLoaded = true;
    }

    Texture2D hoverCell_GameBackground = {0};   // (Hover) Cell Background Initialization
    if (FileExists("assets/images/GameBackground/hoverCell_game_background.png"))
    {
        hoverCell_GameBackground = LoadTexture("assets/images/GameBackground/hoverCell_game_background.png");   // (Hover) Cell Background
        backgroundLoaded = true;
    }

    //────────────────────────────────────────────────────────────
    // Game Over Screen
    //────────────────────────────────────────────────────────────

    Texture2D red_GameOverBackground = {0};   // Red Game Over Screen Background Initialization
    if (FileExists("assets/images/GameOverBackground/red_gameover_background.png"))
    {
        red_GameOverBackground = LoadTexture("assets/images/GameOverBackground/red_gameover_background.png");   // Red Game Over Screen Background
        backgroundLoaded = true;
    }

    Texture2D hoverReturn_red_GameOverBackground = {0};   // (Hover on Return Button) Red Game Over Screen Background Initialization
    if (FileExists("assets/images/GameOverBackground/hoverReturn_red_gameover_background.png"))
    {
        hoverReturn_red_GameOverBackground = LoadTexture("assets/images/GameOverBackground/hoverReturn_red_gameover_background.png");   // (Hover on Return Button) Red Game Over Screen Background
        backgroundLoaded = true;
    }

    Texture2D blue_GameOverBackground = {0};   // Blue Game Over Screen Background Initialization
    if (FileExists("assets/images/GameOverBackground/blue_gameover_background.png"))
    {
        blue_GameOverBackground = LoadTexture("assets/images/GameOverBackground/blue_gameover_background.png");   // Blue Game Over Screen Background
        backgroundLoaded = true;
    }

    Texture2D hoverReturn_blue_GameOverBackground = {0};   // (Hover on Return Button) Blue Game Over Screen Background Initialization
    if (FileExists("assets/images/GameOverBackground/hoverReturn_blue_gameover_background.png"))
    {
        hoverReturn_blue_GameOverBackground = LoadTexture("assets/images/GameOverBackground/hoverReturn_blue_gameover_background.png");   // (Hover on Return Button) Blue Game Over Screen Background
        backgroundLoaded = true;
    }

    Texture2D draw_GameOverBackground = {0};   // Draw Game Over Screen Background Initialization
    if (FileExists("assets/images/GameOverBackground/draw_gameover_background.png"))
    {
        draw_GameOverBackground = LoadTexture("assets/images/GameOverBackground/draw_gameover_background.png");   // Draw Game Over Screen Background
        backgroundLoaded = true;
    }

    Texture2D hoverReturn_draw_GameOverBackground = {0};   // (Hover on Return Button) Draw Game Over Screen Background Initialization
    if (FileExists("assets/images/GameOverBackground/hoverReturn_draw_gameover_background.png"))
    {
        hoverReturn_draw_GameOverBackground = LoadTexture("assets/images/GameOverBackground/hoverReturn_draw_gameover_background.png");   // (Hover on Return Button) Draw Game Over Screen Background
        backgroundLoaded = true;
    }

    //────────────────────────────────────────────────────────────
    // Player Symbols
    //────────────────────────────────────────────────────────────

    // --- Red ---

    Texture2D X_Red_PlayerSymbols = {0};   // Red X Initialization
    if (FileExists("assets/images/PlayerSymbols/Red/X.png"))
    {
        X_Red_PlayerSymbols = LoadTexture("assets/images/PlayerSymbols/Red/X.png");   // Red X
        backgroundLoaded = true;
    }

    Texture2D O_Red_PlayerSymbols = {0};   // Red O Initialization
    if (FileExists("assets/images/PlayerSymbols/Red/O.png"))
    {
        O_Red_PlayerSymbols = LoadTexture("assets/images/PlayerSymbols/Red/O.png");   // Red O
        backgroundLoaded = true;
    }

    Texture2D Diamond_Red_PlayerSymbols = {0};   // Red Diamond Initialization
    if (FileExists("assets/images/PlayerSymbols/Red/Diamond.png"))
    {
        Diamond_Red_PlayerSymbols = LoadTexture("assets/images/PlayerSymbols/Red/Diamond.png");   // Red Diamond
        backgroundLoaded = true;
    }

    Texture2D Heart_Red_PlayerSymbols = {0};   // Red Heart Initialization
    if (FileExists("assets/images/PlayerSymbols/Red/Heart.png"))
    {
        Heart_Red_PlayerSymbols = LoadTexture("assets/images/PlayerSymbols/Red/Heart.png");   // Red Heart
        backgroundLoaded = true;
    }

    // --- Blue ---

    Texture2D X_Blue_PlayerSymbols = {0};   // Blue X Initialization
    if (FileExists("assets/images/PlayerSymbols/Blue/X.png"))
    {
        X_Blue_PlayerSymbols = LoadTexture("assets/images/PlayerSymbols/Blue/X.png");   // Blue X
        backgroundLoaded = true;
    }

    Texture2D O_Blue_PlayerSymbols = {0};   // Blue O Initialization
    if (FileExists("assets/images/PlayerSymbols/Blue/O.png"))
    {
        O_Blue_PlayerSymbols = LoadTexture("assets/images/PlayerSymbols/Blue/O.png");   // Blue O
        backgroundLoaded = true;
    }

    Texture2D Diamond_Blue_PlayerSymbols = {0};   // Blue Diamond Initialization
    if (FileExists("assets/images/PlayerSymbols/Blue/Diamond.png"))
    {
        Diamond_Blue_PlayerSymbols = LoadTexture("assets/images/PlayerSymbols/Blue/Diamond.png");   // Blue Diamond
        backgroundLoaded = true;
    }

    Texture2D Heart_Blue_PlayerSymbols = {0};   // Blue Heart Initialization
    if (FileExists("assets/images/PlayerSymbols/Blue/Heart.png"))
    {
        Heart_Blue_PlayerSymbols = LoadTexture("assets/images/PlayerSymbols/Blue/Heart.png");   // Blue Heart
        backgroundLoaded = true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //music
    float musicVolume = 0.5f;   //default
    float soundVolume = 0.5f;   //defalut


    Music backgroundMusic = {0};
    bool musicLoaded = false;
    if (FileExists("assets/music/background.mp3"))
    {
        backgroundMusic = LoadMusicStream("assets/music/background.mp3"); // Background Music
        musicLoaded = true;
        SetMusicVolume(backgroundMusic, musicVolume); //music slider
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    Sound clickSound = {0}, hoverSound = {0}, soundWin = {0};
    bool clickSound_Loaded = false, hoverSound_Loaded = false, winLoaded = false;

    if (FileExists("assets/sounds/click.wav"))
    {
        clickSound = LoadSound("assets/sounds/click.wav");
        clickSound_Loaded = true;
        SetSoundVolume(clickSound, soundVolume);    //sound slider
    } // Click Sound

    if (FileExists("assets/sounds/hover.wav"))
    {
        hoverSound = LoadSound("assets/sounds/hover.wav");
        hoverSound_Loaded = true;
        SetSoundVolume(hoverSound, soundVolume);    //sound slider
    } // Hover Sound

    if (FileExists("assets/sounds/win.wav"))
    {
        soundWin = LoadSound("assets/sounds/win.wav");
        winLoaded = true;
        SetSoundVolume(soundWin, soundVolume);      //sound slider
    } // Win Sound


    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    Font gameFont = GetFontDefault(); // Default Font
    if (FileExists("assets/fonts/Heroic_Font.ttf"))
    {
        Font HeroicFont = LoadFont("assets/fonts/Heroic_Font.ttf"); // Heroic Font
        if (HeroicFont.texture.id != 0)
            gameFont = HeroicFont;
        GuiSetFont(gameFont);
    }

    //___________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________

    // Users Initialization
    User users[MAX_USERS];
    int userCount = 0;
    LoadUsers("users.txt", users, &userCount); // users.txt = file,  users = array,  &userCount = Address of userCount variable (which stores no. of users in .txt file)

    // Intro Screen Initializations
    double introStartTime = GetTime();
    double elapsed;
    bool introShown = false;
    float opacity = 0.0f;   // Initial opacity ( 0.0f = NULL, 1.0f = FULL)
    float fadeSpeed = 1.0f; // fade duration in seconds
    bool unfaded = false;
    bool faded = false;
    bool initialPause = false;

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    // Game Buttons (Invisible Hitboxes) Iniitializations

    //=======================================================     LOGIN SCREEN     =====================================================================================================================================================

    // Title Button:
    Rectangle btnStartGame = {313, 300, 302, 52};
    bool hover_btnStartGame = false;

    Rectangle btnSetting = {312, 363, 302, 52};
    bool hover_btnSetting = false;

    Rectangle btnLeaderBoard = {314, 426, 302, 52};
    bool hover_btnLeaderBorad = false;

    Rectangle btnLogout = {310, 489, 302, 52};
    bool hover_btnLogout = false;

    Rectangle btnQuitGame = {310, 552, 302, 52};
    bool hover_btnQuitgame = false;

    //Leader/setting BUTTON:
    Rectangle btnBack_leader = {383.5, 636.3, 155, 61.8};
    bool hover_btnBack_leader = false;

    Rectangle p1SymbolO = {410.8, 340.8, 59.5, 59.6};
    Rectangle p1SymbolX = {481, 340.8, 59.5, 59.6};
    Rectangle p1Symboldiamond = {551.2, 340.8, 59.5, 59.6};
    Rectangle p1SymbolHeart = {621.5, 340.8, 59.5, 59.6};

    Rectangle p2SymbolO = {410.8, 413, 59.5, 59.6};
    Rectangle p2SymbolX = {481, 413, 59.5, 59.6};
    Rectangle p2Symboldiamond = {551.2, 413, 59.5, 59.6};
    Rectangle p2SymbolHeart = {621.5, 413, 59.5, 59.6};

    //----------------------------------------------------------------------------------

    // Continue to Title Screen Button (On Login Screen)
    Rectangle btnContinueToTitle = {570, 19, 177, 67}; // The Starting x,y , length and width of the CONTINUE button (repectively)
    bool hover_btnContinueToTitle = false;             // Bool (0 or 1) of Hovering Over Continue Button

    // P1 Login Button (On Login Screen)
    Rectangle btnP1Login = {735.66, 154.16, 79, 79}; // The Starting x,y , length and width of the P1 Login button (repectively)
    bool hover_btnP1Login = false;                   // Bool (0 or 1) of Hovering Over P1 Login Button

    // P2 Login Button (On Login Screen)
    Rectangle btnP2Login = {85.34, 354.16, 79, 79}; // The Starting x,y , length and width of the P2 Login button (repectively)
    bool hover_btnP2Login = false;                  // Bool (0 or 1) of Hovering Over P2 Login Button

    // P1 Register Button (On Login Screen)
    Rectangle btnP1Register = {735.66, 235.16, 79, 79}; // The Starting x,y , length and width of the P1 Register button (repectively)
    bool hover_btnP1Register = false;                   // Bool (0 or 1) of Hovering Over P1 Register Button

    // P2 Register Button (On Login Screen)
    Rectangle btnP2Register = {85.34, 435.16, 79, 79}; // The Starting x,y , length and width of the P2 Register button (repectively)
    bool hover_btnP2Register = false;                  // Bool (0 or 1) of Hovering Over P2 Register Button

    //=======================================================     GAME SCREEN     =====================================================================================================================================================

    // (Red Background) Forfeit (To Title Screen) Button (On Game Screen)
    Rectangle btnForfeit_RedBackground = {291, 640.84, 318, 46.3};
    bool hover_btnForfeit_RedBackground = false;

    // (Blue Background) Forfeit (To Title Screen) Button (On Game Screen)
    Rectangle btnForfeit_BlueBackground = {291, 640.84, 318, 46.3};
    bool hover_btnForfeit_BlueBackground = false;

    // =====  EMPTY CELLS  =====

    // -> Top-Row

    // North-West Cell Button (On Game Screen)
    Rectangle NW_Cell = {194, 63, 160, 160};
    bool hover_NW_Cell = false;

    // North Cell Button (On Game Screen)
    Rectangle N_Cell = {370, 63, 160, 160};
    bool hover_N_Cell = false;

    // North-East Cell Button (On Game Screen)
    Rectangle NE_Cell = {545, 63, 160, 160};
    bool hover_NE_Cell = false;

    // -> Middle Row

    // West Cell Button (On Game Screen)
    Rectangle W_Cell = {194, 241, 160, 160};
    bool hover_W_Cell = false;

    // Origin Cell Button (On Game Screen)
    Rectangle O_Cell = {370, 241, 160, 160};
    bool hover_O_Cell = false;

    // East Cell Button (On Game Screen)
    Rectangle E_Cell = {545, 241, 160, 160};
    bool hover_E_Cell = false;

    // -> Bottom Row

    // South West Cell Button (On Game Screen)
    Rectangle SW_Cell = {194, 418, 160, 160};
    bool hover_SW_Cell = false;

    // South Cell Button (On Game Screen)
    Rectangle S_Cell = {370, 418, 160, 160};
    bool hover_S_Cell = false;

    // South East Cell Button (On Game Screen)
    Rectangle SE_Cell = {545, 418, 160, 160};
    bool hover_SE_Cell = false;

    //=======================================================     GAME OVER SCREEN     =====================================================================================================================================================

    // (Red Background) Return (To Title Screen) Button (On Game Over Screen)
    Rectangle btnReturn_RedBackground = {291, 605.84, 318, 46.3};
    bool hover_btnReturn_RedBackground = false;

    // (Blue Background) Return (To Title Screen) Button (On Game Over Screen)
    Rectangle btnReturn_BlueBackground = {291, 605.84, 318, 46.3};
    bool hover_btnReturn_BlueBackground = false;

    // (Draw Background) Return (To Title Screen) Button (On Game Over Screen)
    Rectangle btnReturn_DrawBackground = {291, 605.84, 318, 46.3};
    bool hover_btnReturn_DrawBackground = false;

    //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    // UI state
    Screen screen = SCR_INTRO;

    GuiSetStyle(BUTTON, BASE_COLOR_NORMAL, 0x00000000);  // Normal state = transparent
    GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED, 0x00000000); // Hover state = transparent
    GuiSetStyle(BUTTON, BASE_COLOR_PRESSED, 0x00000000); // Clicked state = transparent
    GuiSetStyle(BUTTON, BORDER_WIDTH, 0);                // No border
    GuiSetStyle(DEFAULT, TEXT_SIZE, 28);

    // Player 1 Data Initialization
    char p1_name[NAME_LENGTH] = "";
    char p1_pass[PASS_LENGTH] = "";
    char p1_symbol = 'X';
    bool editingP1User = false;
    bool editingP1Pass = false;
    char p1_msg[128];
    int p1_index = -1;
    float msgTimerP1 = 0.0f;

    // Player 2 Data Initialization
    char p2_name[NAME_LENGTH] = "";
    char p2_pass[PASS_LENGTH] = "";
    char p2_symbol = 'O';
    bool editingP2User = false;
    bool editingP2Pass = false;
    char p2_msg[128];
    int p2_index = -1;
    float msgTimerP2 = 0.0f;

    // Default symbol selections
    bool cricleP1 = false;
    bool crossP1 = true; // default
    bool diamondP1 = false;
    bool heartP1 = false;

    bool cricleP2 = true; // default
    bool crossP2 = false;
    bool diamondP2 = false;
    bool heartP2 = false;

    // Game - Logic Initialization
    Board board;
    ResetBoard(&board);
    int currentPlayer = 1;
    bool gameOver = false;
    int winner = 0;
    bool draw = false;
    bool statsUpdatedForThisRound = false;
    bool hasPlayerDone = 0;



    // Sound Utils Initialization
    bool musicOn = true, soundOn = true, musicPlaying = false;

    //_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________

    // Main Game Loop
    while (!WindowShouldClose())
    {
        // Mouse Position Updates
        Vector2 mouse = GetMousePosition();

        // Background Music
        if (introShown)
        {
            if (musicLoaded)
                UpdateMusicIfNeeded(&backgroundMusic, musicOn, &musicPlaying);
        }

        // Screen Initialization
        BeginDrawing();
        ClearBackground(BLACK);

        //_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________

        // Intro Screen
        if (screen == SCR_INTRO)
        {

            if (!introShown)
            {
                // Fade IN
                if (!unfaded)
                {
                    // Increase in opacity according to time elapsed this frame (normalized by fade duration)
                    opacity += (GetFrameTime() / fadeSpeed);
                    if (opacity >= 1.0f)
                    {
                        opacity = 1.0f;
                        unfaded = true;
                        introStartTime = GetTime(); // start hold timer
                    }
                }
                // Hold full image for 1.5 seconds
                else if (unfaded && !faded)
                {
                    if ((GetTime() - introStartTime) >= 1.5)
                    {
                        // start fading out
                        faded = false; // keep faded=false until fade out completes
                    }
                    else
                    {
                        
                    }
                }

                if (unfaded && !faded && (GetTime() - introStartTime) >= 2.5) // Fade OUT (starts when we finished hold and opacity == 1.0f)
                {
                    opacity -= (GetFrameTime() / fadeSpeed);
                    if (opacity <= 0.0f)
                    {
                        opacity = 0.0f;
                        faded = true;
                    }
                }

                if (backgroundLoaded) // Draws Intro Screen with CURRENT (Varied) Opacity (On Loop)
                {
                    DrawTexturePro(
                        introBackground,
                        (Rectangle){0, 0, (float)introBackground.width, (float)introBackground.height},
                        (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                        (Vector2){0, 0}, 0.0f,
                        (Color){255, 255, 255, (unsigned char)(opacity * 255)});
                }
                else
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(SKYBLUE, opacity));

                if (faded && (GetTime() - introStartTime) >= 4.5) // When fade-out finished, advance to login
                {
                    introShown = true;
                    screen = SCR_LOGIN;
                }
            }
        }

        //_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________

        // LOGIN screen
        if (screen == SCR_LOGIN)
        {
            Vector2 mouse = GetMousePosition(); // Update Mouse Position 

            // ────────────────────────────────────────────────────────────
            //  Decide which background to draw based on hover
            // ────────────────────────────────────────────────────────────
            Texture2D *currentBg = &loginBackground;

            if (hover_btnContinueToTitle)
                currentBg = &hoverContinue_loginBackground;
            else if (hover_btnP1Login)
                currentBg = &hoverP1Login_loginBackground;
            else if (hover_btnP1Register)
                currentBg = &hoverP1Register_loginBackground;
            else if (hover_btnP2Login)
                currentBg = &hoverP2Login_loginBackground;
            else if (hover_btnP2Register)
                currentBg = &hoverP2Register_loginBackground;

            // Draw selected background
            DrawTexturePro(
                *currentBg,
                (Rectangle){0, 0, (float)currentBg->width, (float)currentBg->height},
                (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                (Vector2){0, 0},
                0.0f,
                WHITE);

            // ────────────────────────────────────────────────────────────
            //  Continue Button
            // ────────────────────────────────────────────────────────────
            if (GuiButtonWithHover(btnContinueToTitle, " ", &hoverSound, &hover_btnContinueToTitle))
            {
                if (clickSound_Loaded)
                    PlaySound(clickSound);

                if (p1_index >= 0 && p2_index >= 0)
                {
                    screen = SCR_TITLE;
                    hover_btnContinueToTitle = false;
                }
                else
                {
                    if (p1_index < 0)
                        strcpy(p1_msg, "Please login/register Player 1");
                    if (p2_index < 0)
                        strcpy(p2_msg, "Please login/register Player 2");
                }
            }

            // ────────────────────────────────────────────────────────────
            //  P1 Login Button
            // ────────────────────────────────────────────────────────────
            if (GuiButtonWithHover(btnP1Login, " ", &hoverSound, &hover_btnP1Login))
            {
                if (clickSound_Loaded)
                    PlaySound(clickSound);

                int response = ValidateLogin(users, userCount, p1_name, p1_pass);

                if (response >= 0)
                {
                    p1_index = response;
                    sprintf(p1_msg, "Logged in as %s", users[p1_index].name);
                    msgTimerP1 = 2.0f;
                }
                else if (response == -1)
                {
                    sprintf(p1_msg, "User not found (register first)");
                    msgTimerP1 = 2.0f;
                }
                else
                {
                    sprintf(p1_msg, "Wrong password");
                    msgTimerP1 = 2.0f;
                }
            }

            // ────────────────────────────────────────────────────────────
            //  P2 Login Button
            // ────────────────────────────────────────────────────────────
            if (GuiButtonWithHover(btnP2Login, " ", &hoverSound, &hover_btnP2Login))
            {
                if (clickSound_Loaded)
                    PlaySound(clickSound);

                int response = ValidateLogin(users, userCount, p2_name, p2_pass);

                if (response >= 0)
                {
                    p2_index = response;
                    sprintf(p2_msg, "Logged in as %s", users[p2_index].name);
                    msgTimerP2 = 2.0f;
                }
                else if (response == -1)
                {
                    sprintf(p2_msg, "User not found (register first)");
                    msgTimerP2 = 2.0f;
                }
                else
                {
                    sprintf(p2_msg, "Wrong password");
                    msgTimerP2 = 2.0f;
                }
            }

            // ────────────────────────────────────────────────────────────
            //  P1 Register Button
            // ────────────────────────────────────────────────────────────
            if (GuiButtonWithHover(btnP1Register, " ", &hoverSound, &hover_btnP1Register))
            {
                if (clickSound_Loaded)
                    PlaySound(clickSound);

                int response = RegisterUser(users, &userCount, p1_name, p1_pass);

                if (response >= 0)
                {
                    SaveUsers("users.txt", users, userCount);
                    p1_index = response;
                    sprintf(p1_msg, "Registered and logged in");
                    msgTimerP1 = 2.0f;
                }
                else if (response == -2)
                {
                    sprintf(p1_msg, "Username already exists");
                    msgTimerP1 = 2.0f;
                }
                else if (response == -3)
                {
                    sprintf(p1_msg, "Username and password required");
                    msgTimerP1 = 2.0f;
                }
                else if (response == -4)
                {
                    sprintf(p1_msg, "No spaces allowed in name/password");
                    msgTimerP1 = 2.0f;
                }
                else
                {
                    sprintf(p1_msg, "Registration failed");
                    msgTimerP1 = 2.0f;
                }
            }

            // ────────────────────────────────────────────────────────────
            //  P2 Register Button
            // ────────────────────────────────────────────────────────────
            if (GuiButtonWithHover(btnP2Register, " ", &hoverSound, &hover_btnP2Register))
            {
                if (clickSound_Loaded)
                    PlaySound(clickSound);

                int response = RegisterUser(users, &userCount, p2_name, p2_pass);

                if (response >= 0)
                {
                    SaveUsers("users.txt", users, userCount);
                    p2_index = response;
                    sprintf(p2_msg, "Registered and logged in");
                    msgTimerP2 = 2.0f;
                }
                else if (response == -2)
                {
                    sprintf(p2_msg, "Username already exists");
                    msgTimerP2 = 2.0f;
                }
                else if (response == -3)
                {
                    sprintf(p2_msg, "Username and password required");
                    msgTimerP2 = 2.0f;
                }
                else if (response == -4)
                {
                    sprintf(p2_msg, "No spaces allowed in name/password");
                    msgTimerP2 = 2.0f;
                }
                else
                {
                    sprintf(p2_msg, "Registration failed");
                    msgTimerP2 = 2.0f;
                }
            }

            // ────────────────────────────────────────────────────────────
            //  Player 1 Input Boxes
            // ────────────────────────────────────────────────────────────
            Rectangle p1UserBox = {91, 168, 610, 60};
            if (GuiTextBox(p1UserBox, p1_name, NAME_LENGTH, editingP1User))
                editingP1User = !editingP1User;

            if (CheckCollisionPointRec(GetMousePosition(), p1UserBox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                editingP1User = true;
                editingP1Pass = false;
            }

            Rectangle p1PassBox = {91, 238, 610, 60};
            if (GuiTextBox(p1PassBox, p1_pass, PASS_LENGTH, editingP1Pass))
                editingP1Pass = !editingP1Pass;

            if (CheckCollisionPointRec(GetMousePosition(), p1PassBox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                editingP1Pass = true;
                editingP1User = false;
            }

            // ────────────────────────────────────────────────────────────
            //  Player 2 Input Boxes
            // ────────────────────────────────────────────────────────────
            Rectangle p2UserBox = {200, 369, 610, 60};
            if (GuiTextBox(p2UserBox, p2_name, NAME_LENGTH, editingP2User))
                editingP2User = !editingP2User;

            if (CheckCollisionPointRec(GetMousePosition(), p2UserBox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                editingP2User = true;
                editingP2Pass = false;
            }

            Rectangle p2PassBox = {200, 439, 610, 60};
            if (GuiTextBox(p2PassBox, p2_pass, PASS_LENGTH, editingP2Pass))
                editingP2Pass = !editingP2Pass;

            if (CheckCollisionPointRec(GetMousePosition(), p2PassBox) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                editingP2Pass = true;
                editingP2User = false;
            }

            // ────────────────────────────────────────────────────────────
            //  P1 and P2 's Notification Boxes
            // ────────────────────────────────────────────────────────────
            if (msgTimerP1 > 0.0f) // For P1's Notifications
            {
                msgTimerP1 -= GetFrameTime();
                DrawRectangleRounded((Rectangle){500, 98, 398, 50}, 0.2f, 8, Fade(BLACK, 0.6f));
                DrawText(p1_msg, 520, 115, 20, RAYWHITE);
            }

            if (msgTimerP2 > 0.0f) // For P2's Notifications
            {
                msgTimerP2 -= GetFrameTime();
                DrawRectangleRounded((Rectangle){40, 520, 398, 50}, 0.2f, 8, Fade(BLACK, 0.6f));
                DrawText(p2_msg, 60, 535, 20, RAYWHITE);
            }
        }

        //_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________

        // TITLE SCREEN
        else if (screen == SCR_TITLE)
        {
            Vector2 mouse = GetMousePosition();

            
            Texture2D *currentBg = &gameBackground;

            if (hover_btnStartGame)
                currentBg = &hoverStartGame_TS;
            else if (hover_btnSetting)
                currentBg = &hoverSetting_TS;
            else if (hover_btnLeaderBorad)
                currentBg = &hoverLeaderBoard_TS;
            else if (hover_btnLogout)
                currentBg = &hoverLogout_TS;
            else if (hover_btnQuitgame)
                currentBg = &hoverQuit_TS;

            
            if (backgroundLoaded)
            {
                DrawTexturePro(
                    *currentBg,
                    (Rectangle){0, 0, (float)currentBg->width, (float)currentBg->height},
                    (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                    (Vector2){0, 0},
                    0.0f,
                    WHITE);
            }
            else
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(SKYBLUE, 0.15f));


            //---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
            

            // START GAME BUTTON
            if (GuiButtonWithHover(btnStartGame, " ", &hoverSound, &hover_btnStartGame))
            {
                if (clickSound_Loaded)
                    PlaySound(clickSound);

                if (p1_index >= 0 && p2_index >= 0)
                {
                    screen = SCR_GAME;
                    hover_btnStartGame = false;
                }
            }


            // SETTINGS BUTTON
            if (GuiButtonWithHover(btnSetting, " ", &hoverSound, &hover_btnSetting))
            {
                if (clickSound_Loaded)
                    PlaySound(clickSound);
                screen = SCR_SETTINGS;
                hover_btnSetting = false;
            }


            // LEADERBOARD BUTTON
            if (GuiButtonWithHover(btnLeaderBoard, " ", &hoverSound, &hover_btnLeaderBorad))
            {
                if (clickSound_Loaded)
                    PlaySound(clickSound);
                screen = SCR_LEADERBOARD;
                hover_btnLeaderBorad = false;
            }


            // LOGOUT PLAYERS BUTTON
            if (GuiButtonWithHover(btnLogout, " ", &hoverSound, &hover_btnLogout))
            {
                if (clickSound_Loaded)
                    PlaySound(clickSound);

                p1_index = p2_index = -1;
                strcpy(p1_name, "");
                strcpy(p1_pass, "");
                strcpy(p2_name, "");
                strcpy(p2_pass, "");

                screen = SCR_LOGIN;
                hover_btnLogout = false;
            }


            // QUIT BUTTON
            if (GuiButtonWithHover(btnQuitGame, " ", &hoverSound, &hover_btnQuitgame))
            {
                if (clickSound_Loaded) PlaySound(clickSound);
                CloseWindow(); // Quit the game
            }


            //--------------------------------  DISPLAY PLAYER STATS  -----------------------------------

            char titleLine[256];
            char P1_SYM_DISPLAY[10], P2_SYM_DISPLAY[10];

            switch(p1_symbol)  // Choosing which Symbol Name to display for P1..
            {
                case 'X': strcpy(P1_SYM_DISPLAY, "Cross"); break;
                case 'O': strcpy(P1_SYM_DISPLAY, "Circle"); break;
                case 'T': strcpy(P1_SYM_DISPLAY, "Diamond"); break;
                case 'H': strcpy(P1_SYM_DISPLAY, "Heart"); break;
            }

            switch(p2_symbol)  // Choosing which Symbol Name to display for P2..
            {
                case 'X': strcpy(P2_SYM_DISPLAY, "Cross"); break;
                case 'O': strcpy(P2_SYM_DISPLAY, "Circle"); break;
                case 'T': strcpy(P2_SYM_DISPLAY, "Diamond"); break;
                case 'H': strcpy(P2_SYM_DISPLAY, "Heart"); break;
            }


            if (p1_index >= 0)
            {
                sprintf(titleLine, "Player 1: %s  (W:%d L:%d D:%d) (Matches:%d) (W/L Ratio:%.3f)\nSYMBOL: %s",
                        users[p1_index].name, users[p1_index].wins, users[p1_index].losses,
                        users[p1_index].draws, users[p1_index].gamesPlayed, users[p1_index].wlratio, P1_SYM_DISPLAY);
                DrawTextEx(gameFont, titleLine, (Vector2){40, 7}, 25, 1, MAROON);
            }
            if (p2_index >= 0)
            {
                sprintf(titleLine, "Player 2: %s  (W:%d L:%d D:%d) (Matches:%d) (W/L Ratio:%.3f)\nSYMBOL: %s",
                        users[p2_index].name, users[p2_index].wins, users[p2_index].losses,
                        users[p2_index].draws, users[p2_index].gamesPlayed, users[p2_index].wlratio, P2_SYM_DISPLAY);
                DrawTextEx(gameFont, titleLine, (Vector2){40, 650}, 25, 1, DARKBLUE);
            }
        }

        //_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________

        // GAME SCREEN
        else if (screen == SCR_GAME)
        {
            Vector2 mouse = GetMousePosition();   // Update Mouse Position

            // Choosing of Game Background (Based of Current Player)
            Texture2D *currentBg = (currentPlayer == 1) ? &red_GameBackground : &blue_GameBackground;
            if (currentPlayer == 1 && hover_btnForfeit_RedBackground) currentBg = &hoverForfeit_red_GameBackground;
            if (currentPlayer == 2 && hover_btnForfeit_BlueBackground) currentBg = &hoverForfeit_blue_GameBackground;

            // Drawing of Chosen Game Background
            DrawTexturePro(
                *currentBg,
                (Rectangle){0, 0, (float)currentBg->width, (float)currentBg->height},
                (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                (Vector2){0, 0},
                0.0f,
                WHITE
            );


            // Displaying of Player Symbol's Icons on Game Screen
            Texture2D *P1_SYM_DISPLAY = NULL;
            Texture2D *P2_SYM_DISPLAY = NULL;

            switch(p1_symbol)  // Choosing which Symbol Name to display for P1..
            {
                case 'X': P1_SYM_DISPLAY = &X_Red_PlayerSymbols; break;
                case 'O': P1_SYM_DISPLAY = &O_Red_PlayerSymbols; break;
                case 'T': P1_SYM_DISPLAY = &Diamond_Red_PlayerSymbols; break;
                case 'H': P1_SYM_DISPLAY = &Heart_Red_PlayerSymbols; break;
            }

            switch(p2_symbol)  // Choosing which Symbol Name to display for P2..
            {
                case 'X': P2_SYM_DISPLAY = &X_Blue_PlayerSymbols; break;
                case 'O': P2_SYM_DISPLAY = &O_Blue_PlayerSymbols; break;
                case 'T': P2_SYM_DISPLAY = &Diamond_Blue_PlayerSymbols; break;
                case 'H': P2_SYM_DISPLAY = &Heart_Blue_PlayerSymbols; break;
            }


            DrawTexturePro(  // Drawing of P1's Icon
                *P1_SYM_DISPLAY,                          
                (Rectangle){ 0, 0, (float)(*P1_SYM_DISPLAY).width, (float)(*P1_SYM_DISPLAY).height },  
                (Rectangle){ 33, 582, 84, 84 },   
                (Vector2){ 0, 0 },                        
                0.0f,                    
                WHITE                       
            );

            DrawTexturePro(  // Drawing of P2's Icon
                *P2_SYM_DISPLAY,                          
                (Rectangle){ 0, 0, (float)(*P2_SYM_DISPLAY).width, (float)(*P2_SYM_DISPLAY).height },  
                (Rectangle){ 783, 582, 84, 84 },   
                (Vector2){ 0, 0 },                        
                0.0f,                    
                WHITE                       
            );


            // Choosing of Correct Forfeit Button (Based on Current Player)
            Rectangle *forfeitBtn = (currentPlayer == 1) ? &btnForfeit_RedBackground : &btnForfeit_BlueBackground;
            bool *forfeitHover = (currentPlayer == 1) ? &hover_btnForfeit_RedBackground : &hover_btnForfeit_BlueBackground;
        
            if (GuiButtonWithHover(*forfeitBtn, " ", &hoverSound, forfeitHover))
            {
                if (clickSound_Loaded) PlaySound(clickSound);
                
                gameOver = true;
                winner = (currentPlayer == 1) ? 2 : 1;
                draw = false;
                ApplyGameResultAndSave(users, userCount, p1_index, p2_index, winner, draw, &statsUpdatedForThisRound);
                screen = SCR_GAMEOVER;
            }
        

            // Preparation of Cell Rects
            Rectangle cellRects[3][3] = {
                { NW_Cell, N_Cell, NE_Cell },
                { W_Cell,  O_Cell, E_Cell  },
                { SW_Cell, S_Cell, SE_Cell }
            };

            // Preparation of Hover Pointers
            bool *hoverVars[3][3] = {
                { &hover_NW_Cell, &hover_N_Cell, &hover_NE_Cell },
                { &hover_W_Cell,  &hover_O_Cell,  &hover_E_Cell  },
                { &hover_SW_Cell, &hover_S_Cell, &hover_SE_Cell }
            };
        

            // Drawing of Cells / Handling of Placed Symbols / Clicks
            for (int row = 0; row < 3; row++)
            {
                for (int col = 0; col < 3; col++)
                {
                    Rectangle r = cellRects[row][col];
                    bool *hov = hoverVars[row][col];
                
                    int cellValue = board.cells[col][row];
                
                    if (cellValue == 0)  // Empty Cell: Draw Normal Cell or Hover Cell Background
                    {
                        Texture2D *cellBg = (*hov) ? &hoverCell_GameBackground : &cell_GameBackground;
                        DrawTexturePro(
                            *cellBg,
                            (Rectangle){0, 0, (float)cellBg->width, (float)cellBg->height},
                            (Rectangle){r.x, r.y, r.width, r.height},
                            (Vector2){0,0}, 0.0f, WHITE
                        );
                    
                        if (GuiButtonWithHover(r, " ", &hoverSound, hov))
                        {
                            // Placing of Current Player's Symbol onto clicked Cell
                            board.cells[col][row] = currentPlayer;
                            hasPlayerDone = 1;
                            if (clickSound_Loaded) PlaySound(clickSound);
                        }
                    }
                    else
                    {
                        Texture2D *symTex = NULL;
                    
                        if (cellValue == 1)  // Player 1's placed symbol
                        {
                            if (p1_symbol == 'X')      symTex = &X_Red_PlayerSymbols;
                            else if (p1_symbol == 'O') symTex = &O_Red_PlayerSymbols;
                            else if (p1_symbol == 'T') symTex = &Diamond_Red_PlayerSymbols;
                            else if (p1_symbol == 'H') symTex = &Heart_Red_PlayerSymbols;
                            else                       symTex = &X_Red_PlayerSymbols;
                        }
                        else if (cellValue == 2)  // Player 2's placed symbol
                        {
                            if (p2_symbol == 'X')      symTex = &X_Blue_PlayerSymbols;
                            else if (p2_symbol == 'O') symTex = &O_Blue_PlayerSymbols;
                            else if (p2_symbol == 'T') symTex = &Diamond_Blue_PlayerSymbols;
                            else if (p2_symbol == 'H') symTex = &Heart_Blue_PlayerSymbols;
                            else                       symTex = &X_Blue_PlayerSymbols;
                        }
                    
                        if (symTex && symTex->id != 0)
                        {
                            // Drawing of Symbol on Cell's Bounds
                            DrawTexturePro(
                                *symTex,
                                (Rectangle){0, 0, (float)symTex->width, (float)symTex->height},
                                (Rectangle){r.x, r.y, r.width, r.height},
                                (Vector2){0,0}, 0.0f, WHITE
                            );
                        }
                    }
                }
            }
        

            // --- After drawing / input handling: check win / draw / switch player ---
            if (!gameOver)
            {
                if (CheckWin(&board, currentPlayer))
                {
                    gameOver = true;
                    winner = currentPlayer;
                    draw = false;
                    if (soundOn && winLoaded) PlaySound(soundWin);
                    ApplyGameResultAndSave(users, userCount, p1_index, p2_index, winner, draw, &statsUpdatedForThisRound);
                    screen = SCR_GAMEOVER;
                }
                else if (BoardFull(&board))
                {
                    gameOver = true;
                    winner = 0;
                    draw = true;
                    ApplyGameResultAndSave(users, userCount, p1_index, p2_index, winner, draw, &statsUpdatedForThisRound);
                    screen = SCR_GAMEOVER;
                }
                else if (hasPlayerDone)
                {
                    hasPlayerDone = 0;
                    currentPlayer = (currentPlayer == 1) ? 2 : 1;  // switch turn
                }
            }
            
        }  // End of GAME SCREEN


        //_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________


        // GAME OVER SCREEN
        else if (screen == SCR_GAMEOVER)
        {
            Vector2 mouse = GetMousePosition();

            //-----------------------------------------------------
            // Initial Hover Detection
            //-----------------------------------------------------

            if (winner == 1) GuiButtonWithHover(btnReturn_RedBackground, " ", &hoverSound, &hover_btnReturn_RedBackground);
            else if (winner == 2) GuiButtonWithHover(btnReturn_BlueBackground, " ", &hoverSound, &hover_btnReturn_BlueBackground);
            else if (winner == 0) GuiButtonWithHover(btnReturn_DrawBackground, " ", &hoverSound, &hover_btnReturn_DrawBackground);
        

            //-----------------------------------------------------
            // Choosing of winner's background
            //-----------------------------------------------------

            Texture2D *bg = NULL;
        
            if (winner == 1)
            {
                bg = (hover_btnReturn_RedBackground) ? &hoverReturn_red_GameOverBackground : &red_GameOverBackground;
            } 
            else if (winner == 2)
            {
                bg = (hover_btnReturn_BlueBackground) ? &hoverReturn_blue_GameOverBackground : &blue_GameOverBackground;
            } 
            else if (winner == 0)
            {
                bg = (hover_btnReturn_DrawBackground) ? &hoverReturn_draw_GameOverBackground : &draw_GameOverBackground;
            } 
        

            //-----------------------------------------------------
            // Drawing of Chosen Winner's Background 
            //-----------------------------------------------------

            DrawTexturePro(
                *bg,
                (Rectangle){0,0,(float)bg->width,(float)bg->height},
                (Rectangle){0,0,(float)GetScreenWidth(),(float)GetScreenHeight()},
                (Vector2){0,0},
                0.0f,
                WHITE
            );


            //-----------------------------------------------------
            // Drawing of Winner's Symbol ONLY
            //-----------------------------------------------------

            Texture2D *P1_SYM_DISPLAY = NULL;
            Texture2D *P2_SYM_DISPLAY = NULL;

            switch(p1_symbol)  // Choosing which Symbol Name to display for P1..
            {
                case 'X': P1_SYM_DISPLAY = &X_Red_PlayerSymbols; break;
                case 'O': P1_SYM_DISPLAY = &O_Red_PlayerSymbols; break;
                case 'T': P1_SYM_DISPLAY = &Diamond_Red_PlayerSymbols; break;
                case 'H': P1_SYM_DISPLAY = &Heart_Red_PlayerSymbols; break;
            }

            switch(p2_symbol)  // Choosing which Symbol Name to display for P2..
            {
                case 'X': P2_SYM_DISPLAY = &X_Blue_PlayerSymbols; break;
                case 'O': P2_SYM_DISPLAY = &O_Blue_PlayerSymbols; break;
                case 'T': P2_SYM_DISPLAY = &Diamond_Blue_PlayerSymbols; break;
                case 'H': P2_SYM_DISPLAY = &Heart_Blue_PlayerSymbols; break;
            }


            if (winner == 1)
            {
                DrawTexturePro(  // Drawing of P1's Icon
                *P1_SYM_DISPLAY,                          
                (Rectangle){ 0, 0, (float)(*P1_SYM_DISPLAY).width, (float)(*P1_SYM_DISPLAY).height },  
                (Rectangle){ 382, 196, 135, 135 },   
                (Vector2){ 0, 0 },                        
                0.0f,                    
                WHITE                       
                );
            }
            else if (winner == 2)
            {
                DrawTexturePro(  // Drawing of P2's Icon
                *P2_SYM_DISPLAY,                          
                (Rectangle){ 0, 0, (float)(*P2_SYM_DISPLAY).width, (float)(*P2_SYM_DISPLAY).height },  
                (Rectangle){ 382, 196, 135, 135 },
                (Vector2){ 0, 0 },                        
                0.0f,                    
                WHITE                       
                );
            }
    
        
            //-----------------------------------------------------
            // Return Button Code Bodies (Based on winner)
            //-----------------------------------------------------

            if (winner == 1)
            {
                if (GuiButton(btnReturn_RedBackground, " "))
                {
                    if (clickSound_Loaded) PlaySound(clickSound);

                    ResetBoard(&board);
                    winner = 0;
                    draw = false;
                    gameOver = false;

                    currentPlayer = 2;
                    statsUpdatedForThisRound = false;

                    screen = SCR_TITLE;
                }
            }
            else if (winner == 2)
            {
                if (GuiButton(btnReturn_BlueBackground, " "))
                {
                    if (clickSound_Loaded) PlaySound(clickSound);

                    ResetBoard(&board);
                    winner = 0;
                    draw = false;
                    gameOver = false;

                    currentPlayer = 1;
                    statsUpdatedForThisRound = false;

                    screen = SCR_TITLE;
                }
            }
            else if (winner == 0)
            {
                if (GuiButton(btnReturn_DrawBackground, " "))
                {
                    if (clickSound_Loaded) PlaySound(clickSound);

                    ResetBoard(&board);
                    winner = 0;
                    draw = false;
                    gameOver = false;

                    currentPlayer = 1;
                    statsUpdatedForThisRound = false;

                    screen = SCR_TITLE;
                } 
            }
       
        }  // End of GAME OVER SCREEN


        //_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________


        // LEADERBOARD SCREEN
        else if (screen == SCR_LEADERBOARD)
        {
            Vector2 mouse = GetMousePosition();
        
            Texture2D *currentBg = &leaderBackground;
            if (hover_btnBack_leader) currentBg = &leaderBackground_hover;
        
            if (backgroundLoaded)
            {
                DrawTexturePro(
                    *currentBg,
                    (Rectangle){0, 0, (float)currentBg->width, (float)currentBg->height},
                    (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                    (Vector2){0, 0},
                    0.0f,
                    WHITE);
            }
            else DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(SKYBLUE, 0.15f));
        

            // Sorting a Copy of User[] Array
            User leaderboard[1024];
            memcpy(leaderboard, users, sizeof(User) * userCount);  // Copying users[] array -> leaderboard[] array

        
            // Recalculate W/L ratio (wins / gamesPlayed + 5) [Weighted Ranking Formula]
            for (int i = 0; i < userCount; i++)
            {
                leaderboard[i].wlratio = (float)leaderboard[i].wins / (leaderboard[i].gamesPlayed + 5);
            }


        
            // Sort leaderboard copy
            SortLeaderboard(leaderboard, userCount);
        
            // Top 10 players
            int displayCount = (userCount < 10) ? userCount : 10;
        
            // Starting positions
            float startX = 150.0f;     // X axis 
            float startY = 200.0f;     // Y axis
            float lineHeight = 42.0f;  // Height difference
        
            // X - axis Starting Points of Each Category
            float nameX = startX;
            float winX = 447.0f;
            float loseX = 525.0f;
            float drawX = 603.0f;
            float ratioX = 750.0f;
            float matchesX = 675.0f;


            // Dislaying of ALL TOP 10 Players.. 
            for (int i = 0; i < displayCount; i++)
            {
                float currentY = startY + (i * lineHeight);
            
                // player name
                DrawTextEx(gameFont, leaderboard[i].name, (Vector2){nameX, currentY}, 22, 1, DARKBLUE);
            
                // Draw wins
                char winsText[16];
                sprintf(winsText, "%d", leaderboard[i].wins);
                DrawTextEx(gameFont, winsText, (Vector2){winX, currentY}, 22, 1, DARKGREEN);
            
                // Draw losses
                char lossesText[16];
                sprintf(lossesText, "%d", leaderboard[i].losses);
                DrawTextEx(gameFont, lossesText, (Vector2){loseX, currentY}, 22, 1, RED);
            
                // Draw draws
                char drawsText[16];
                sprintf(drawsText, "%d", leaderboard[i].draws);
                DrawTextEx(gameFont, drawsText, (Vector2){drawX, currentY}, 22, 1, ORANGE);
            
                // Draw W/L ratio
                char ratioText[16];
                sprintf(ratioText, "%.3f", (float) leaderboard[i].wlratio);
                DrawTextEx(gameFont, ratioText, (Vector2){ratioX, currentY}, 22, 1, PURPLE);

                // Draw Total No. of GamesPlayed
                char matchesText[16];
                sprintf(matchesText, "%d", leaderboard[i].gamesPlayed);
                DrawTextEx(gameFont, matchesText, (Vector2){matchesX, currentY}, 22, 1, BLACK);
            }

        
            // Back (To Title Screen) button
            if (GuiButtonWithHover(btnBack_leader, " ", &hoverSound, &hover_btnBack_leader))
            {
                if (clickSound_Loaded) PlaySound(clickSound);
            
                screen = SCR_TITLE;
                hover_btnBack_leader = false;
            }
        
        }  // End of LEADERBOARD SCREEN 


        //_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________


        // SETTINGS SCREEN
        else if (screen == SCR_SETTINGS)
        {
            Vector2 mouse = GetMousePosition();

            
            Texture2D *currentBg = &settingBackground;

            if (hover_btnBack_leader)
            {
                currentBg = &settingBackground_hover;
            }
            else
            {
                // Check all 16 combinations
                if (cricleP1 && cricleP2)
                    currentBg = &setting_P1circle_P2circle;
                else if (cricleP1 && crossP2)
                    currentBg = &setting_P1circle_P2cross;
                else if (cricleP1 && diamondP2)
                    currentBg = &setting_P1circle_P2diamond;
                else if (cricleP1 && heartP2)
                    currentBg = &setting_P1circle_P2heart;

                else if (crossP1 && cricleP2)
                    currentBg = &setting_P1cross_P2circle;
                else if (crossP1 && crossP2)
                    currentBg = &setting_P1cross_P2cross;
                else if (crossP1 && diamondP2)
                    currentBg = &setting_P1cross_P2diamond;
                else if (crossP1 && heartP2)
                    currentBg = &setting_P1cross_P2heart;

                else if (diamondP1 && cricleP2)
                    currentBg = &setting_P1diamond_P2circle;
                else if (diamondP1 && crossP2)
                    currentBg = &setting_P1diamond_P2cross;
                else if (diamondP1 && diamondP2)
                    currentBg = &setting_P1diamond_P2diamond;
                else if (diamondP1 && heartP2)
                    currentBg = &setting_P1diamond_P2heart;

                else if (heartP1 && cricleP2)
                    currentBg = &setting_P1heart_P2circle;
                else if (heartP1 && crossP2)
                    currentBg = &setting_P1heart_P2cross;
                else if (heartP1 && diamondP2)
                    currentBg = &setting_P1heart_P2diamond;
                else if (heartP1 && heartP2)
                    currentBg = &setting_P1heart_P2heart;
            }

            
            if (backgroundLoaded)
            {
                DrawTexturePro(
                    *currentBg,
                    (Rectangle){0, 0, (float)currentBg->width, (float)currentBg->height},
                    (Rectangle){0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()},
                    (Vector2){0, 0},
                    0.0f,
                    WHITE);
            }
            else
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(SKYBLUE, 0.15f));

            //-------------------------------------------------------
            // Sound Slider
            //-------------------------------------------------------
            Rectangle soundSlider = {410, 230, 200, 20};

            // Invisible slider
            GuiSetStyle(SLIDER, BORDER_WIDTH, 0);
            GuiSetStyle(SLIDER, BASE_COLOR_NORMAL, 0x00000000);
            GuiSetStyle(SLIDER, BASE_COLOR_FOCUSED, 0x00000000);
            GuiSetStyle(SLIDER, BASE_COLOR_PRESSED, 0x00000000);

            float prevSoundVol = soundVolume;
            GuiSlider(soundSlider, NULL, NULL, &soundVolume, 0.0f, 1.0f);

            // Apply sound volume changes
            if (prevSoundVol != soundVolume)
            {
                if (clickSound_Loaded)
                    SetSoundVolume(clickSound, soundVolume);
                if (hoverSound_Loaded)
                    SetSoundVolume(hoverSound, soundVolume);
                if (winLoaded)
                    SetSoundVolume(soundWin, soundVolume);

                // Turn sound on/off
                if (soundVolume > 0.0f)
                    soundOn = true;
                else
                    soundOn = false;
            }

            //-------------------------------------------------------
            // Music Slider
            //-------------------------------------------------------
            Rectangle musicSliderBounds = {410, 285, 200, 20};

            float prevMusicVol = musicVolume;
            GuiSlider(musicSliderBounds, NULL, NULL, &musicVolume, 0.0f, 1.0f);

            //Music
            if (prevMusicVol != musicVolume && musicLoaded)
            {
                SetMusicVolume(backgroundMusic, musicVolume);

                //volume
                if (musicVolume > 0.0f && !musicOn)
                {
                    musicOn = true;
                }
                else if (musicVolume == 0.0f && musicOn)
                {
                    musicOn = false;
                }
            }

            //-------------------------------------------------------
            // Player 1 Symbol Selection
            //-------------------------------------------------------
            if (GuiButton(p1SymbolO, " "))
            {
                if (clickSound_Loaded && soundOn)
                    PlaySound(clickSound);
                p1_symbol = 'O';
                cricleP1 = true;
                crossP1 = false;
                diamondP1 = false;
                heartP1 = false;
            }
            if (GuiButton(p1SymbolX, " "))
            {
                if (clickSound_Loaded && soundOn)
                    PlaySound(clickSound);
                p1_symbol = 'X';
                cricleP1 = false;
                crossP1 = true;
                diamondP1 = false;
                heartP1 = false;
            }
            if (GuiButton(p1Symboldiamond, " "))
            {
                if (clickSound_Loaded && soundOn)
                    PlaySound(clickSound);
                p1_symbol = 'T';
                cricleP1 = false;
                crossP1 = false;
                diamondP1 = true;
                heartP1 = false;
            }
            if (GuiButton(p1SymbolHeart, " "))
            {
                if (clickSound_Loaded && soundOn)
                    PlaySound(clickSound);
                p1_symbol = 'H';
                cricleP1 = false;
                crossP1 = false;
                diamondP1 = false;
                heartP1 = true;
            }

            //-------------------------------------------------------
            // Player 2 Symbol Selection
            //-------------------------------------------------------
            if (GuiButton(p2SymbolO, " "))
            {
                if (clickSound_Loaded && soundOn)
                    PlaySound(clickSound);
                p2_symbol = 'O';
                cricleP2 = true;
                crossP2 = false;
                diamondP2 = false;
                heartP2 = false;
            }
            if (GuiButton(p2SymbolX, " "))
            {
                if (clickSound_Loaded && soundOn)
                    PlaySound(clickSound);
                p2_symbol = 'X';
                cricleP2 = false;
                crossP2 = true;
                diamondP2 = false;
                heartP2 = false;
            }
            if (GuiButton(p2Symboldiamond, " "))
            {
                if (clickSound_Loaded && soundOn)
                    PlaySound(clickSound);
                p2_symbol = 'T';
                cricleP2 = false;
                crossP2 = false;
                diamondP2 = true;
                heartP2 = false;
            }
            if (GuiButton(p2SymbolHeart, " "))
            {
                if (clickSound_Loaded && soundOn)
                    PlaySound(clickSound);
                p2_symbol = 'H';
                cricleP2 = false;
                crossP2 = false;
                diamondP2 = false;
                heartP2 = true;
            }

            //-------------------------------------------------------
            // Back button
            //-------------------------------------------------------
            if (GuiButton(btnBack_leader, " "))
            {
                if (clickSound_Loaded && soundOn)
                    PlaySound(clickSound);
                screen = SCR_TITLE;
            }
        } // End of SETTINGS SCREEN
        EndDrawing();
    }



    // Game Assets Unloading
    if (backgroundLoaded)
    {
        UnloadTexture(gameBackground);  // For Testing Purposes..
        //===============================================================================================================================================================================================================================================
        UnloadTexture(hoverStartGame_TS);
        UnloadTexture(hoverSetting_TS);  // Title Screen Backgrounds Unloading
        UnloadTexture(hoverLeaderBoard_TS);
        UnloadTexture(hoverLogout_TS);
        UnloadTexture(hoverQuit_TS);
        //===============================================================================================================================================================================================================================================
        UnloadTexture(introBackground);  // Intro Background Unloading
        //===============================================================================================================================================================================================================================================
        UnloadTexture(loginBackground);
        UnloadTexture(hoverContinue_loginBackground);
        UnloadTexture(hoverP1Login_loginBackground);  // Login Screen Backgrounds Unloading
        UnloadTexture(hoverP2Login_loginBackground);
        UnloadTexture(hoverP1Register_loginBackground);
        UnloadTexture(hoverP2Register_loginBackground);
        //===============================================================================================================================================================================================================================================
        UnloadTexture(leaderBackground);
        UnloadTexture(leaderBackground_hover);  // LeaderBoard Screen Backgrounds Unloading
        //===============================================================================================================================================================================================================================================
        UnloadTexture(settingBackground);
        UnloadTexture(settingBackground_hover);

        UnloadTexture(setting_P1circle_P2circle);
        UnloadTexture(setting_P1circle_P2cross);
        UnloadTexture(setting_P1circle_P2diamond);  // Setings Screen Background Unloading
        UnloadTexture(setting_P1circle_P2heart);

        UnloadTexture(setting_P1cross_P2circle);
        UnloadTexture(setting_P1cross_P2cross);
        UnloadTexture(setting_P1cross_P2diamond);
        UnloadTexture(setting_P1cross_P2heart);

        UnloadTexture(setting_P1diamond_P2circle);
        UnloadTexture(setting_P1diamond_P2cross);
        UnloadTexture(setting_P1diamond_P2diamond);
        UnloadTexture(setting_P1diamond_P2heart);

        UnloadTexture(setting_P1heart_P2circle);
        UnloadTexture(setting_P1heart_P2cross);
        UnloadTexture(setting_P1heart_P2diamond);
        UnloadTexture(setting_P1heart_P2heart);
        //===============================================================================================================================================================================================================================================
        UnloadTexture(red_GameBackground);  // Red Game Background Unloading
        UnloadTexture(blue_GameBackground);  // Blue Game Background Unloading
        UnloadTexture(hoverForfeit_red_GameBackground);
        UnloadTexture(hoverForfeit_blue_GameBackground);
        //===============================================================================================================================================================================================================================================
        UnloadTexture(cell_GameBackground);  // Cell Background Unloading
        UnloadTexture(hoverCell_GameBackground);
        //===============================================================================================================================================================================================================================================
        UnloadTexture(X_Red_PlayerSymbols);
        UnloadTexture(O_Red_PlayerSymbols);  // Red Player Symbols Unloading
        UnloadTexture(Diamond_Red_PlayerSymbols);
        UnloadTexture(Heart_Red_PlayerSymbols);
        //===============================================================================================================================================================================================================================================
        UnloadTexture(X_Blue_PlayerSymbols);
        UnloadTexture(O_Blue_PlayerSymbols);  // Blue Player Symbols Unloading
        UnloadTexture(Diamond_Blue_PlayerSymbols);
        UnloadTexture(Heart_Blue_PlayerSymbols);
        //===============================================================================================================================================================================================================================================
        UnloadTexture(red_GameOverBackground);  // Red Game Over Background Unloading
        UnloadTexture(blue_GameOverBackground);  // Blue Game Over Background Unloading
        UnloadTexture(draw_GameOverBackground);  // Draw Game Over Background Unloading
        UnloadTexture(hoverReturn_red_GameOverBackground);
        UnloadTexture(hoverReturn_blue_GameOverBackground);
        UnloadTexture(hoverReturn_draw_GameOverBackground);

    }
    
    if (musicLoaded)
    {
        StopMusicStream(backgroundMusic);
        UnloadMusicStream(backgroundMusic);
    }

    if (clickSound_Loaded) UnloadSound(clickSound);
    if (hoverSound_Loaded) UnloadSound(hoverSound);
    if (winLoaded) UnloadSound(soundWin);


    if (gameFont.texture.id != GetFontDefault().texture.id) UnloadFont(gameFont);

    // Window Closing
    CloseAudioDevice();
    CloseWindow();

    return 0;
}