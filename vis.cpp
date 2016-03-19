#ifndef VIS_CPP_
#define VIS_CPP_
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <string>

const unsigned int  screenWidth     = 1280;
const unsigned int  screenHeight    = 720;
const unsigned int  borderSize      = 50;
const unsigned int  lineWidth       = 1;
const unsigned int  circleSize      = 5;
bool                quit            = false;

SDL_Window*         gWindow         = NULL;
SDL_Renderer*       gRenderer       = NULL;
SDL_Event           e;

bool initializeScreen(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf( "SDL was unable to initialize! Error: %s\n",
            SDL_GetError());
        return false;
    } else {
        // Create window
        gWindow = SDL_CreateWindow("Graphical representation",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth,
            screenHeight, SDL_WINDOW_SHOWN);

        if (gWindow == NULL) {
            printf("Unable to create window! Error: %s\n", SDL_GetError());
            return false;
        } else {
            // Create renderer for window
            gRenderer = SDL_CreateRenderer(gWindow, -1,
                SDL_RENDERER_ACCELERATED);
            if (gRenderer == NULL) {
                printf("Unable to create renderer! Error: %s\n",
                    SDL_GetError());
                return false;
            } else {
                SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
            }
        }
    }
    return true;
}

void closeScreen(void) {
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    gRenderer = NULL;
    gWindow = NULL;

    SDL_Quit();
}

void drawScreen(void) {
    // Set draw color (RGBA)
    SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
    // Clear renderer
    SDL_RenderClear(gRenderer);

    // Draw background rectangle
    SDL_Rect background = {0, 0, screenWidth, screenHeight};
    SDL_RenderDrawRect(gRenderer, &background);

    //SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
    //SDL_RenderDrawLine(gRenderer, borderSize, borderSize,
        //screenWidth - borderSize, borderSize);

    // Draw line
    hlineRGBA(gRenderer, borderSize, screenWidth - borderSize, borderSize,
        0, 0, 0, 255);

    // Draw info text
    stringRGBA(gRenderer, 3, borderSize / 2, "Stop", 0, 0, 0, 255);
    stringRGBA(gRenderer, 3, borderSize / 2 + 10, "Pass", 0, 0, 0, 255);
    char info[100];
    sprintf(info, "Time: %02d:%02d - Bus speed: %.1f m/s", clk.timeOfDay / 3600,
        clk.timeOfDay % 3600 / 60, driver.trafficDelay * driver.maxVelocity);
    stringRGBA(gRenderer, 8, screenHeight - borderSize / 2, info, 0, 0, 0, 255);

    // Draw stops
    for (int i = 0; i <= numStops; i++) {
        float position = (i * (screenWidth - 2 * borderSize)) / (numStops)
            + borderSize;

        filledCircleRGBA(gRenderer, (int) round(position), borderSize,
            circleSize, 255, 255, 255, 255);
        circleRGBA(gRenderer, (int) round(position), borderSize, circleSize,
            0, 0, 0, 255);

        char no[3];
        if (i == 0 || i == numStops)
            sprintf(no, "ST");
        else
            sprintf(no, "%d", i);
        stringRGBA(gRenderer, (int) round(position) - circleSize,
            borderSize / 2, no, 25, 25, 255, 255);

        if (i < numStops) {
            sprintf(no, "%d", numWaitingPassengers(i));
            stringRGBA(gRenderer, (int) round(position) - circleSize,
                borderSize / 2 + 10, no, 25, 25, 25, 255);
        }
    }

    // Draw buses
    float vertSpace = (screenHeight - 2 * borderSize - circleSize - 10)
        / maxBuses; // About 12px for 50 buses

    for (int i = 0; i < maxBuses; i++) {
        if (buses[i] == NULL)
            continue;

        float yPos = borderSize + circleSize + 10 + i * vertSpace;
        float xPos = (buses[i]->position * (screenWidth - 2*borderSize))
            / (lineLength) + borderSize - circleSize;
        char no[4];
        sprintf(no, "%01d", buses[i]->passengersOnBoard);

        hlineRGBA(gRenderer, borderSize, screenWidth - borderSize, yPos + 8,
            0, 0, 0, 255);

        if (buses[i]->line == 0) {
            stringRGBA(gRenderer, 8, yPos, "401", 0, 0, 0, 255);
        } else {
            stringRGBA(gRenderer, 8, yPos, "402", 0, 0, 0, 255);
        }

        if (buses[i] -> passengersOnBoard > 99) {
            boxRGBA(gRenderer, (int) round(xPos) - 1, (int) round(yPos) - 1,
                (int) round(xPos) + 26, (int) round(yPos) + 9,
                0xFF, 0xCC, 0, 255);
        } else {
            boxRGBA(gRenderer, (int) round(xPos) - 1, (int) round(yPos) - 1,
                (int) round(xPos) + 17, (int) round(yPos) + 9,
                0xFF, 0xCC, 0, 255);
        }

        if (atStop(buses[i]->position) == -1) {
            stringRGBA(gRenderer, (int) round(xPos), (int) round(yPos), no,
                0, 110, 0, 255);
        } else {
            stringRGBA(gRenderer, (int) round(xPos), (int) round(yPos), no,
                110, 0, 0, 255);
        }
    }

    SDL_RenderPresent(gRenderer);
}

void handleScreenEvents(void) {
    // Check if the user has pressed the quit button
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
    }
}

#endif
