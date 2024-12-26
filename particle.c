#include <math.h>
#include <stdio.h>
#include "particle.h"

void transform_to_box_space(float* x, float* y, float centerX, float centerY, float angle) {
    // Translate to origin
    *x -= centerX;
    *y -= centerY;
    
    // Rotate backwards
    float c = cos(-angle);
    float s = sin(-angle);
    float newX = *x * c - *y * s;
    float newY = *x * s + *y * c;
    
    *x = newX;
    *y = newY;
}

void transform_from_box_space(float* x, float* y, float centerX, float centerY, float angle) {
    // Rotate forwards
    float c = cos(angle);
    float s = sin(angle);
    float newX = *x * c - *y * s;
    float newY = *x * s + *y * c;
    
    // Translate back
    *x = newX + centerX;
    *y = newY + centerY;
}

void rotate_vector(float* vx, float* vy, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    float newVx = *vx * c - *vy * s;
    float newVy = *vx * s + *vy * c;
    *vx = newVx;
    *vy = newVy;
}

void update_particles(Particle* particles, int num_particles, float dt, int width, int height, BoxState* box) {
    float centerX = width / 2.0f;
    float centerY = height / 2.0f;
    float minDimension = (width < height) ? width : height;
    float boxSize = (minDimension - 40) / sqrtf(2);  // 20px border on each side
    float halfBoxSize = boxSize / 2.0f;

    // Constant downward gravity
    float gx = 0;
    float gy = GRAVITY;  // Always points down in world space

    for (int i = 0; i < num_particles; i++) {
        // Update velocity with gravity in world space first
        particles[i].vx += gx * dt;
        particles[i].vy += gy * dt;
        
        // Transform to box space for boundary collisions
        float x = particles[i].x;
        float y = particles[i].y;
        float vx = particles[i].vx;
        float vy = particles[i].vy;
        transform_to_box_space(&x, &y, centerX, centerY, box->angle);
        rotate_vector(&vx, &vy, -box->angle);

        // Apply velocity in box space
        x += vx * dt;
        y += vy * dt;

        // Box boundary collisions in local space
        if (x < -halfBoxSize) {
            x = -halfBoxSize;
            vx = -vx * BOUNCE_DAMPENING;
        }
        if (x > halfBoxSize) {
            x = halfBoxSize;
            vx = -vx * BOUNCE_DAMPENING;
        }
        if (y < -halfBoxSize) {
            y = -halfBoxSize;
            vy = -vy * BOUNCE_DAMPENING;
        }
        if (y > halfBoxSize) {
            y = halfBoxSize;
            vy = -vy * BOUNCE_DAMPENING;
        }

        // Transform back to world space
        rotate_vector(&vx, &vy, box->angle);
        transform_from_box_space(&x, &y, centerX, centerY, box->angle);

        // Check for boundary wrap-around
        if (fabs(x - particles[i].x) > halfBoxSize || fabs(y - particles[i].y) > halfBoxSize) {
            printf("Particle %d made large jump: (%f,%f) -> (%f,%f) at angle %f\n", 
                   i, particles[i].x, particles[i].y, x, y, box->angle);
        }

        // Update particle
        particles[i].x = x;
        particles[i].y = y;
        particles[i].vx = vx;
        particles[i].vy = vy;
        
        // Particle-particle collisions in world space
        for (int j = i + 1; j < num_particles; j++) {
            float dx = particles[j].x - particles[i].x;
            float dy = particles[j].y - particles[i].y;
            float distance = dx * dx + dy * dy;
            float minDistance = (particles[i].radius + particles[j].radius) * 
                              (particles[i].radius + particles[j].radius);
            
            if (distance < minDistance) {
                float dist = sqrt(distance);
                float nx = dx / dist;
                float ny = dy / dist;
                
                float rvx = particles[j].vx - particles[i].vx;
                float rvy = particles[j].vy - particles[i].vy;
                float velAlongNormal = rvx * nx + rvy * ny;
                
                if (velAlongNormal > 0) continue;
                
                float impulse = -(1.0f + BOUNCE_DAMPENING) * velAlongNormal;
                impulse /= 2.0f;
                
                float impulsex = impulse * nx;
                float impulsey = impulse * ny;
                
                particles[i].vx -= impulsex;
                particles[i].vy -= impulsey;
                particles[j].vx += impulsex;
                particles[j].vy += impulsey;
                
                // Positional correction
                const float percent = 0.2f;
                const float slop = 0.01f;
                float correction = (dist - sqrt(minDistance) + slop) / dist * percent;
                float correctionX = correction * nx;
                float correctionY = correction * ny;
                
                particles[i].x += correctionX;
                particles[i].y += correctionY;
                particles[j].x -= correctionX;
                particles[j].y -= correctionY;
            }
        }
    }
}
