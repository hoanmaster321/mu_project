#version 450
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

struct ClothParticle {
    vec4 pos;     // xyz = current pos, w = invMass (0.0 = fixed, 1.0 = movable)
    vec4 vel;     // xyz = velocity, w = state (PVS_FIXEDPOS = 1)
    vec4 force;   // xyz = force, w = oneTimeMoveCount
    vec4 oneTime; // xyz = oneTimeMove accumulated, w = pad
};

struct ClothLink {
    uint v0;
    uint v1;
    float distMin;
    float distMax;
    uint style;   // 1 = PLS_LOOSEDISTANCE, 2 = PLS_SPRING, 4 = PLS_STRICTDISTANCE
    uint pad0;
    uint pad1;
    uint pad2;
};

struct ClothSphereCol {
    vec4 centerRadius; // xyz = center, w = radius
};

layout(std430, set = 0, binding = 0) buffer ParticleBuffer {
    ClothParticle particles[];
};

layout(std430, set = 0, binding = 1) readonly buffer LinkBuffer {
    ClothLink links[];
};

layout(std430, set = 0, binding = 2) readonly buffer SphereBuffer {
    ClothSphereCol spheres[];
};

layout(push_constant) uniform ClothParams {
    uint numParticles;
    uint numLinks;
    uint numSpheres;
    uint subSteps;
    float fTime;
    float windX;
    float windY;
    float gravity;
    uint dwType;
} uParams;

void main()
{
    uint id = gl_GlobalInvocationID.x;
    if (id >= uParams.numParticles) return;

    // Fixed / pinned vertex: do not move via physics
    if (particles[id].pos.w <= 0.0) return;

    vec3 p = particles[id].pos.xyz;
    vec3 v = particles[id].vel.xyz;
    vec3 f = particles[id].force.xyz;

    float dt = uParams.fTime;
    float invMass = particles[id].pos.w;

    // Apply wind and gravity
    f.x += uParams.windX;
    f.y += uParams.windY;
    f.z += uParams.gravity;

    // Velocity Verlet / Euler integration
    v += f * (invMass * dt);
    // Damping
    v *= 0.985;
    p += v * dt;

    // Sphere collision detection & response
    for (uint s = 0u; s < uParams.numSpheres; ++s)
    {
        vec3 center = spheres[s].centerRadius.xyz;
        float radius = spheres[s].centerRadius.w;
        if (radius <= 0.0) continue;

        vec3 toVert = p - center;
        float dist = length(toVert);
        if (dist < radius)
        {
            vec3 norm = (dist > 0.001) ? (toVert / dist) : vec3(0.0, 0.0, 1.0);
            p = center + norm * radius;
            // Damp normal velocity on collision
            v -= norm * max(0.0, dot(v, norm));
        }
    }

    particles[id].pos.xyz = p;
    particles[id].vel.xyz = v;
}
