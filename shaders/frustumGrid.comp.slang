/*
author: Pyrrha Tocquet
date: 26/04/2026
desc: Forward+ frustum grid for light culling compute shader
*/

import "modules/global";
import "modules/frustum";

#define BLOCK_SIZE 16

struct FrustumGridDispatchParams
{
    uint2 threadGroupsCount;
    uint2 executedThreadsCount; // <= executed threads, depends on screen res 
}

float4 ClipToView(float4 clip)
{
    // view space pos
    float4 view = mul(generalUBO.invProj, clip);

    // Perspective proj
    view = view / view.w;

    return view;
}

float4 ScreenToView(float4 screen)
{
    // to NDC
    float2 texCoord = screen.xy / generalUBO.screenRes;

    // to clip space
    float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);

    return ClipToView(clip);
}

[[vk::binding(0, 0)]] ConstantBuffer<Global::GeneralUBO> generalUBO;
[[vk::binding(1, 0)]] ConstantBuffer<FrustumGridDispatchParams> dispatchParamsCb;
[[vk::binding(2, 0)]] RWStructuredBuffer<Frustum::Frustum> outFrustumSb;

[shader("compute")]
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(uint3 groupId: SV_GroupID,
          uint3 groupThreadId: SV_GroupThreadID,
          uint3 dispatchThreadId: SV_DispatchThreadID,
          float groupIndex: SV_GroupIndex)
{
    // view space eye pos
    const float3 eye = float3(0.f, 0.f, 0.f);

    // far plane corner points
    float4 screenSpaceCornerPoints[4];

    // Top Left, Top Right, Bottom Left, Bottom Right
    screenSpaceCornerPoints[0] = float4(dispatchThreadId.xy * BLOCK_SIZE, -1.0f, 1.0f);
    screenSpaceCornerPoints[1] = float4(float2(dispatchThreadId.x + 1, dispatchThreadId.y) * BLOCK_SIZE, -1.0f, 1.0f);
    screenSpaceCornerPoints[2] = float4(float2(dispatchThreadId.x, dispatchThreadId.y + 1) * BLOCK_SIZE, -1.0f, 1.0f);
    screenSpaceCornerPoints[3] = float4(float2(dispatchThreadId.x + 1, dispatchThreadId.y + 1) * BLOCK_SIZE, -1.0f, 1.0f);

    // to view space
    float3 viewSpaceCorners[4];
    for (int i = 0; i < 4; i++)
    {
        viewSpaceCorners[i] = ScreenToView(screenSpaceCornerPoints[i]).xyz;
    }

    // build frustum
    Frustum::Frustum frustum;
    // Left, Right, Top, Bottom
    frustum.planes[0] = Frustum::PlaneFromPoints(eye, viewSpaceCorners[2], viewSpaceCorners[0]);
    frustum.planes[1] = Frustum::PlaneFromPoints(eye, viewSpaceCorners[1], viewSpaceCorners[3]);
    frustum.planes[2] = Frustum::PlaneFromPoints(eye, viewSpaceCorners[0], viewSpaceCorners[1]);
    frustum.planes[3] = Frustum::PlaneFromPoints(eye, viewSpaceCorners[3], viewSpaceCorners[2]);

    // Store frustum
    if (dispatchThreadId.x < dispatchParamsCb.executedThreadsCount.x && dispatchThreadId.y < dispatchParamsCb.executedThreadsCount.y)
    {
        uint index = dispatchThreadId.x + (dispatchThreadId.y * dispatchParamsCb.executedThreadsCount.x);
        outFrustumSb[index] = frustum;
    }
}
