#version 450

layout(local_size_x = 16, local_size_y = 16) in;

// Binding 0: Old wave height page (read-only)
layout(std430, set = 0, binding = 0) readonly buffer OldPage {
    int oldHeight[];
};

// Binding 1: New wave height page (read-write)
layout(std430, set = 0, binding = 1) buffer NewPage {
    int newHeight[];
};

// Binding 2: Base wave page 2 (read-write)
layout(std430, set = 0, binding = 2) buffer BaseWave2 {
    int baseWave2[];
};

// Binding 3: Base wave page 3 (read-write)
layout(std430, set = 0, binding = 3) buffer BaseWave3 {
    int baseWave3[];
};

layout(push_constant) uniform WaterParams {
    int gridSize;       // 256
    int heroX;
    int heroY;
    float worldTime;
    int runBaseWave;
} uParams;

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    uint N = uint(uParams.gridSize);

    if (x >= N || y >= N) return;

    uint idx = y * N + x;

    // 1. Water Ripple PDE Stencil (Skip boundary 1 pixel)
    if (x > 0u && x < N - 1u && y > 0u && y < N - 1u) {
        int up    = oldHeight[(y + 1u) * N + x];
        int down  = oldHeight[(y - 1u) * N + x];
        int right = oldHeight[y * N + (x + 1u)];
        int left  = oldHeight[y * N + (x - 1u)];

        int newh = ((up + down + right + left) >> 1) - newHeight[idx];
        newHeight[idx] = newh - (newh >> 4);
    }

    // 2. Base Wave Sine Calculation (around hero viewport or full grid)
    if (uParams.runBaseWave != 0) {
        float fx = float(x);
        float fy = float(y);
        float t = uParams.worldTime;
        int maxH1 = int(sin(t * 0.005 + fy * 0.1 + fx * 0.1) * 50.0);
        baseWave2[idx] = int(float(maxH1) - sin(t * 0.003 + fx * 0.1 + fy * 0.5) * 50.0);
        int maxH2 = int(sin(t * 0.001 + fy * 0.5 + fx * 0.5) * 25.0);
        baseWave3[idx] = int(float(maxH2) - sin(t * 0.002 + fx * 1.0 + fy * 0.3) * 25.0);
    }
}