struct VertexOutput
{
    float4 position : SV_Position;
    float4 color    : COLOR0;
};

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(4, 1, 1)]
void meshMain(uint gtid: SV_GroupThreadID,
              uint gid: SV_GroupID,
              out indices uint3 triangles[128],
              out vertices VertexOutput vertices[64])
{
    float2 positions[4] = {
        float2(-0.5, -0.5),
        float2( 0.5, -0.5),
        float2( 0.5,  0.5),
        float2(-0.5,  0.5)
    };
    
    uint3 indices[2] = {
        uint3(0, 1, 2),
        uint3(2, 3, 0)
    };

    uint vertexCount = 4;
    uint triangleCount = 2;
    SetMeshOutputCounts(vertexCount, triangleCount);

    if (gtid < vertexCount) {
        vertices[gtid].position.xy = positions[gtid];
        vertices[gtid].color.xy = positions[gtid] + 0.5;
    }
    if (gtid < triangleCount) {
        triangles[gtid] = indices[gtid];
    }
}

[shader("fragment")]
float4 fragmentMain(float4 color: COLOR0) : SV_Target
{
    return color;
}
