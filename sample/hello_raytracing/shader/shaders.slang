// --------------------------------------------------------------
// Slang レイトレーシング シェーダ (RayGen, ClosestHit, Miss)
// --------------------------------------------------------------
struct PushConstants
{
    float4x4 invView;
    float4x4 invProj;
};

// GLSL の layout(push_constant) uniform 相当 → Slang/HLSL だと cbuffer などで受ける
ConstantBuffer<PushConstants> gPush;

// GLSL の layout(binding=0) uniform accelerationStructureEXT → Slang だと:
RaytracingAccelerationStructure gTopLevelAS;

// GLSL の layout(binding=1, rgba8) uniform image2D outputImage → Slang だと:
RWTexture2D<float4> gOutputImage;

// Ray Payload 相当 (GLSL の rayPayloadEXT / rayPayloadInEXT vec3 payload)
struct RayPayload
{
    float3 color;
};

// Closest Hit の hitAttributeEXT 相当
struct HitAttributes
{
    float3 attribs;
};

// --------------------------------------------------------------
// [1] Ray Generation Shader
// --------------------------------------------------------------
[shader("raygeneration")]
void rayGenMain()
{
    // Vulkan GLSL: gl_LaunchIDEXT, gl_LaunchSizeEXT
    // Slang/HLSL: DispatchRaysIndex(), DispatchRaysDimensions()
    uint3 launchID   = DispatchRaysIndex();
    uint3 launchSize = DispatchRaysDimensions();

    // 画面上のUV座標を [(-1) .. (+1)] にマッピング
    float2 uv = (float2(launchID.xy) / float2(launchSize.xy)) * 2.0f - 1.0f;

    // 逆行列を使ってワールド座標系でのレイを生成
    float4 origin    = mul(gPush.invView, float4(0, 0, 0, 1));
    float4 target    = mul(gPush.invProj, float4(uv.x, uv.y, 1.0f, 1.0f));
    float4 direction = mul(gPush.invView, float4(normalize(target.xyz), 0));

    // Ray Payload 初期化 (色を返すためのレイペイロード)
    RayPayload payload;
    payload.color = float3(0, 0, 0);

    // traceRayEXT の代わりに、Slang/HLSL の TraceRay を呼び出す
    // - フラグや cullMask は GLSL のものに相当する値を使ってください
    //   (ここでは簡単に RAY_FLAG_NONE, 0xFF に置き換え)
    TraceRay(
        gTopLevelAS,
        RAY_FLAG_NONE,         // gl_RayFlagsOpaqueEXT 相当
        0xFF,                  // cullMask
        0,                     // sbtRecordOffset
        0,                     // sbtRecordStride
        0,                     // missIndex
        origin.xyz,  0.001f,   // ray origin / tmin
        direction.xyz, 10000.f,// ray direction / tmax
        0,                     // payload location (スロット)
        payload                // inout RayPayload
    );

    // 最終的な結果を imageStore → RWTexture2D への書き込みに対応
    gOutputImage[launchID.xy] = float4(payload.color, 1.0f);
}

// --------------------------------------------------------------
// [2] Closest Hit Shader
// --------------------------------------------------------------
[shader("closesthit")]
void closestHitMain(
    inout RayPayload     payload,
    inout HitAttributes  hitAttribs
)
{
    // GLSL: payload = vec3(attribs.xy, 0);
    payload.color = float3(hitAttribs.attribs.xy, 0.0f);
}

// --------------------------------------------------------------
// [3] Miss Shader
// --------------------------------------------------------------
[shader("miss")]
void missMain(inout RayPayload payload)
{
    // GLSL: payload = vec3(0.0, 0.0, 0.5);
    payload.color = float3(0.0f, 0.0f, 0.5f);
}
