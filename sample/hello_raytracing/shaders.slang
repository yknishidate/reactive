struct PushConstants
{
    float4x4 invView;
    float4x4 invProj;
};

[vk::push_constant]
ConstantBuffer<PushConstants> gPush;

RaytracingAccelerationStructure gTopLevelAS;

RWTexture2D<float4> gOutputImage;

struct RayPayload
{
    float3 color;
};

float2 PixelToScreen(uint2 launchID, uint2 launchSize)
{
    float2 screen = ((float2(launchID.xy) + float2(0.5f)) / float2(launchSize.xy)) * 2.0f - 1.0f;
    return { screen.x, -screen.y };
}

[shader("raygeneration")]
void rayGenMain()
{
    uint3 launchID   = DispatchRaysIndex();
    uint3 launchSize = DispatchRaysDimensions();

    // 画面上のUV座標を [(-1) .. (+1)] にマッピング
    float2 screen = PixelToScreen(launchID.xy, launchSize.xy);

    // 逆行列を使ってワールド座標系でのレイを生成
    float4 origin = mul(gPush.invView, float4(0, 0, 0, 1));
    float4 target = mul(gPush.invProj, float4(screen.xy, 1.0f, 1.0f));
    float4 direction = mul(gPush.invView, float4(normalize(target.xyz), 0));

    // Ray Payload 初期化
    RayPayload payload;
    payload.color = float3(0, 0, 0);

    RayDesc ray;
    ray.Origin = origin.xyz;
    ray.TMin = 0.001f;
    ray.Direction = direction.xyz;
    ray.TMax = 10000.0f;

    TraceRay(gTopLevelAS,
             RAY_FLAG_NONE,  // rayFlags
             0xFF,           // instanceInclusionMask
             0,              // rayContributionToHitGroupIndex
             0,              // multiplierForGeometryContributionToHitGroupIndex
             0,              // missShaderIndex
             ray,            // RayDesc
             payload         // inout RayPayload
    );

    gOutputImage[launchID.xy] = float4(payload.color, 1.0f);
}

[shader("closesthit")]
void closestHitMain(inout RayPayload payload,
                    in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.color = float3(attribs.barycentrics.xy, 0.0f);
}

[shader("miss")]
void missMain(inout RayPayload payload)
{
    payload.color = float3(0.0f, 0.0f, 0.5f);
}
