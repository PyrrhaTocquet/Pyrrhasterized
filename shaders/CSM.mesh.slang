/*
author: Pyrrha Tocquet
date: 31/03/25
desc: Cascaded shadow maps mesh shader
*/
import "modules/global";
import "modules/geometry";
import "modules/shadows";

[[vk::binding(0, 0)]] StructuredBuffer<Geometry::MeshletInfo>           meshletInfosBuffer;
[[vk::binding(1, 0)]] StructuredBuffer<Geometry::Triangle>              primitivesBuffer;
[[vk::binding(2, 0)]] StructuredBuffer<uint>                            indexBuffer; // TODO remove, useless for now
[[vk::binding(3, 0)]] StructuredBuffer<Geometry::InputVertex>           vertexBuffer;

[[vk::binding(0, 1)]] ConstantBuffer<Shadows::CascadeUniformObject>     ubo;

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void meshMain(
    uint3 threadId: SV_DispatchThreadID,
    in payload uint inMeshletOffset,
    in uniform Global::PushConstant inPushConstant,
    out indices uint3 outSharedIndices[128],
    out vertices Geometry::MinimalOutputVertex outSharedVertices[128]
    )
{
    Geometry::MeshletInfo currentMeshlet = meshletInfosBuffer[inPushConstant.meshletId + inMeshletOffset];
    SetMeshOutputCounts(currentMeshlet.vertexCount, currentMeshlet.primitiveCount);

    // Vertices
    if (threadId.x < currentMeshlet.vertexCount)
    {
        uint vertexIndex = currentMeshlet.vertexOffset + threadId.x;
        Geometry::InputVertex vertex = vertexBuffer[vertexIndex];

        float4 positionWorld = mul(inPushConstant.model, float4(vertex.pos, 1.0));
        float4x4 cascadeViewMatrix = ubo.cascadeViewProj[inPushConstant.cascadeId];
        outSharedVertices[threadId.x].pos = mul(cascadeViewMatrix, positionWorld);
    }

    // Primitives
    if (threadId.x < currentMeshlet.primitiveCount)
    {
        Geometry::Triangle tri = primitivesBuffer[currentMeshlet.primitiveOffset + threadId.x];
        outSharedIndices[threadId.x] = uint3(tri.i0, tri.i1, tri.i2);
    }
}