struct VertexStageInput
{
    uint index : SV_VertexID;
};

struct VertexStageOutput
{
    float4 position : SV_Position;
    float3 color;
};

[shader("vertex")]
VertexStageOutput vertexMain(VertexStageInput input)
{
    float3 positions[] = { float3(-1, -1, 0), float3(0, 1, 0), float3(1, -1, 0) };
    float3 colors[] = { float3(0), float3(1, 0, 0), float3(0, 1, 0) };
    
    VertexStageOutput output;
    output.position = float4(positions[input.index], 1.0);
    output.color = colors[input.index];
    return output;
}

[shader("fragment")]
float4 fragmentMain(float3 color) : SV_Target
{
    return float4(color, 1.0);
}
