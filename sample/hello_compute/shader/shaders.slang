struct MandelbrotParams
{
    float2 lowerLeft;
    float2 upperRight;
    int    maxIterations;
};

RWTexture2D<float4> outputImage;

ConstantBuffer<MandelbrotParams> mandelbrotParams;

[numthreads(16, 16, 1)]
[shader("compute")]
[require(spvImageQuery)]
[require(spvSparseResidency)]
void computeMain(uint3 threadID: SV_DispatchThreadID)
{
    uint2 storePos = threadID.xy;

    uint width, height;
    outputImage.GetDimensions(width, height);

    float2 normCoordinates = float2(
        float(storePos.x) / float(width),
        float(storePos.y) / float(height)
    );

    float2 c = lerp(mandelbrotParams.lowerLeft, mandelbrotParams.upperRight, normCoordinates);

    float2 z = float2(0.0, 0.0);

    int i = 0;
    for (; i < mandelbrotParams.maxIterations; i++)
    {
        float x = (z.x * z.x - z.y * z.y) + c.x;
        float y = (z.x * z.y + z.y * z.x) + c.y;

        if ((x*x + y*y) > 4.0f)
            break;

        z.x = x;
        z.y = y;
    }

    float color = float(i) / float(mandelbrotParams.maxIterations);
    outputImage[storePos] = float4(color, color, color, 1.0f);
}
