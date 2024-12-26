#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "particle.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define NUM_PARTICLES 100
#define ROTATION_SPEED 0.5f  // Radians per second
#define SETTLED_VELOCITY_THRESHOLD 1.0f
#define SETTLED_CHECK_TIME 1.0f  // Time to wait before checking if particles settled
#define BOX_SIZE_METERS 1000.0f  // Box is 1000 meters on each side
#define ROTATION_ANGLE M_PI      // 180 degrees in radians

typedef struct {
    Uint32 startTime;
    Uint32 currentTime;
} SimulationTime;

void init_particles(Particle* particles) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x = rand() % WINDOW_WIDTH;
        particles[i].y = rand() % (WINDOW_HEIGHT / 2);  // Start in top half
        particles[i].vx = (rand() % 200 - 100) / 100.0f;  // Random velocity
        particles[i].vy = 0;
        particles[i].radius = 5;
    }
}

void draw_box_labels(SDL_Renderer* renderer, TTF_Font* font, float centerX, float centerY, float boxSize, float angle) {
    char label[32]; 
    
    snprintf(label, sizeof(label), "%.0fm", BOX_SIZE_METERS);
    
    SDL_Color textColor = {255, 255, 255, 255};  // White text
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, label, textColor);
    if (!textSurface) {
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        SDL_FreeSurface(textSurface);
        return;
    }

    // Calculate positions for labels on each side
    //float s = sin(angle);
    //`float c = cos(angle);
    float halfSize = boxSize / 2.0f;
    SDL_Rect textRect = {0, 0, textSurface->w, textSurface->h};
    
    // Draw labels slightly outside each edge of the box
    // Bottom
    textRect.x = centerX - textSurface->w/2;
    textRect.y = centerY + halfSize + 10;
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    
    // Top
    textRect.y = centerY - halfSize - textSurface->h - 10;
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    
    // Left
    textRect.x = centerX - halfSize - textSurface->w - 10;
    textRect.y = centerY - textSurface->h/2;
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    
    // Right
    textRect.x = centerX + halfSize + 10;
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

void draw_time_counter(SDL_Renderer* renderer, TTF_Font* font, SimulationTime time) {
    Uint32 elapsed = time.currentTime - time.startTime;
    Uint32 jiffies = (elapsed % 1000) / 16; // 1/60th of a second
    Uint32 seconds = (elapsed / 1000) % 60;
    Uint32 minutes = (elapsed / 60000) % 60;
    Uint32 hours = elapsed / 3600000;
    
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02u:%02u:%02u:%02u", 
             hours, minutes, seconds, jiffies);
    
    SDL_Color textColor = {255, 255, 255, 255};  // White text
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, timeStr, textColor);
    if (!textSurface) {
        return;
    }
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        SDL_FreeSurface(textSurface);
        return;
    }

    // Position in top-left corner with a small margin
    SDL_Rect textRect = {10, 10, textSurface->w, textSurface->h};
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
}

int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() < 0) {
        printf("SDL_ttf initialization failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Load font
    TTF_Font* font = TTF_OpenFont("Helvetica.ttf", 16);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    window = SDL_CreateWindow("Physics Box Simulation",
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 1;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return 1;
    }
    
    // Initialize particles
    srand(time(NULL));
    Particle particles[NUM_PARTICLES];
    init_particles(particles);
    
    // Initialize box state
    BoxState box = {
        .angle = 0,
        .targetAngle = 0,
        .isRotating = false,
        .timeSinceLastMove = 0
    };
    
    // Initialize simulation time
    SimulationTime simTime = {
        .startTime = SDL_GetTicks(),
        .currentTime = SDL_GetTicks()
    };
    
    // Main game loop
    bool quit = false;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();
    
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_r) {
                    // Reset simulation
                    init_particles(particles);
                    box.angle = 0;
                    box.targetAngle = 0;
                    box.isRotating = false;
                    box.timeSinceLastMove = 0;
                    simTime.startTime = SDL_GetTicks();
                }
            }
        }
        
        // Update simulation time
        simTime.currentTime = SDL_GetTicks();
        
        // Calculate delta time
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        // Check if particles have settled
        bool allSettled = true;
        for (int i = 0; i < NUM_PARTICLES; i++) {
            float vel = sqrt(particles[i].vx * particles[i].vx + 
                           particles[i].vy * particles[i].vy);
            if (vel > SETTLED_VELOCITY_THRESHOLD) {
                allSettled = false;
                box.timeSinceLastMove = 0;
                break;
            }
        }

        if (allSettled) {
            box.timeSinceLastMove += deltaTime;
            if (box.timeSinceLastMove >= SETTLED_CHECK_TIME && !box.isRotating) {
                box.isRotating = true;
                //box.targetAngle = box.angle + M_PI/2;  // 90 degrees
                box.targetAngle = box.angle + ROTATION_ANGLE;
            }
        }

        if (box.isRotating) {
            box.angle += ROTATION_SPEED * deltaTime;
            if (box.angle >= box.targetAngle) {
                box.angle = box.targetAngle;
                box.isRotating = false;
                box.timeSinceLastMove = 0;
            }
        }
        
        // Update particles
        update_particles(particles, NUM_PARTICLES, deltaTime, WINDOW_WIDTH, WINDOW_HEIGHT, &box);
        
        // Render
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Draw box outline
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        float centerX = WINDOW_WIDTH / 2.0f;
        float centerY = WINDOW_HEIGHT / 2.0f;
        float minDimension = (WINDOW_WIDTH < WINDOW_HEIGHT) ? WINDOW_WIDTH : WINDOW_HEIGHT;
        float boxSize = (minDimension - 40) / sqrtf(2);  // 20px border on each side
        
        // Calculate rotated box corners
        float c = cos(box.angle);
        float s = sin(box.angle);
        SDL_Point points[5] = {
            {centerX + (-boxSize/2 * c - boxSize/2 * s),
             centerY + (-boxSize/2 * s + boxSize/2 * c)},
            {centerX + (boxSize/2 * c - boxSize/2 * s),
             centerY + (boxSize/2 * s + boxSize/2 * c)},
            {centerX + (boxSize/2 * c + boxSize/2 * s),
             centerY + (boxSize/2 * s - boxSize/2 * c)},
            {centerX + (-boxSize/2 * c + boxSize/2 * s),
             centerY + (-boxSize/2 * s - boxSize/2 * c)},
            {centerX + (-boxSize/2 * c - boxSize/2 * s),
             centerY + (-boxSize/2 * s + boxSize/2 * c)}
        };
        SDL_RenderDrawLines(renderer, points, 5);
        
        // Draw box labels
        draw_box_labels(renderer, font, centerX, centerY, boxSize, box.angle);
        
        // Draw particles
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (int i = 0; i < NUM_PARTICLES; i++) {
            SDL_Rect rect = {
                (int)(particles[i].x - particles[i].radius),
                (int)(particles[i].y - particles[i].radius),
                (int)(particles[i].radius * 2),
                (int)(particles[i].radius * 2)
            };
            SDL_RenderFillRect(renderer, &rect);
        }
        
        // Draw time counter
        draw_time_counter(renderer, font, simTime);
        
        SDL_RenderPresent(renderer);
    }
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}
