#ifndef PARTICLE_H
#define PARTICLE_H

#include <stdbool.h>

//#define GRAVITY 9.81f
#define GRAVITY 20.0f
#define BOUNCE_DAMPENING 0.7f
#define METERS_TO_PIXELS(m) ((m) * boxSize / BOX_SIZE_METERS)
#define PIXELS_TO_METERS(p) ((p) * BOX_SIZE_METERS / boxSize)

typedef struct {
    float x, y;       // Position in pixels
    float vx, vy;     // Velocity in pixels/second
    float radius;     // Radius in pixels
} Particle;

typedef struct {
    float angle;              // Current rotation angle in radians
    float targetAngle;        // Target angle for smooth rotation
    bool isRotating;          // Whether we're currently in rotation
    float timeSinceLastMove;  // Time since significant particle movement
} BoxState;

void update_particles(Particle* particles, int num_particles, float dt, int width, int height, BoxState* box);

#endif
