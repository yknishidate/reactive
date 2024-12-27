// shaders.slang

//
// This file provides a simple vertex and fragment shader that can be compiled
// using Slang. This code should also be valid as HLSL, and thus it does not
// use any of the new language features supported by Slang.
//

struct VertexStageInput
{
    uint index : SV_VertexID;
};

struct VertexStageOutput
{
    float3 color;
    float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexStageOutput vertexMain(VertexStageInput input)
{
    VertexStageOutput output;

    float3 positions[] = { float3(-1, -1, 0), float3(0, 1, 0), float3(1, -1, 0) };
    float3 colors[] = { float3(0), float3(1, 0, 0), float3(0, 1, 0) };

    output.color = colors[input.index];
    output.sv_position = float4(positions[input.index], 1.0);

    return output;
}

[shader("fragment")]
float4 fragmentMain(float3 color) : SV_Target
{
    return float4(color, 1.0);
}
