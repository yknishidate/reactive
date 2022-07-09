
layout(binding =  12, rg32ui) uniform uimage2D resevSampleImage; // (x_z, m)
layout(binding =  13, rg16f) uniform image2D resevWeightImage;  // (w_sum, p_z)

struct Reservoir
{
    float w_sum; // sum of all weights
    float p_z;   // probability of target
    int x_z;     // sample
    int m;       // number of streaming
};

float calcReservoirWeight(in Reservoir reservoir)
{
    return reservoir.w_sum / reservoir.m / reservoir.p_z;
}

Reservoir createReservoir()
{
    Reservoir reservoir;
    reservoir.w_sum = 0.0;
    reservoir.x_z = 0;
    reservoir.p_z = 0.0;
    reservoir.m = 0;
    return reservoir;
}

Reservoir loadReservoir(in ivec2 id)
{
    uvec2 resevSample_ = imageLoad(resevSampleImage, id).xy;
    vec2 resevWeight_ = imageLoad(resevWeightImage, id).xy;
    Reservoir reservoir;
    reservoir.x_z = int(resevSample_.x);
    reservoir.m = int(resevSample_.y);
    reservoir.w_sum = resevWeight_.x;
    reservoir.p_z = resevWeight_.y;
    return reservoir;
}
