/*
author: Pyrrha Tocquet
date: 01/04/25
desc: main material mesh shader. Forms the meshlets and constructs the vertex data
*/
import "modules/global";
import "modules/geometry";

[[vk::binding(0, 0)]] StructuredBuffer<Geometry::MeshletInfo>   meshletInfosBuffer;
[[vk::binding(1, 0)]] StructuredBuffer<Geometry::Triangle>      primitivesBuffer;
[[vk::binding(2, 0)]] StructuredBuffer<uint>                    indexBuffer;
[[vk::binding(3, 0)]] StructuredBuffer<Geometry::InputVertex>   vertexBuffer;

[[vk::binding(0, 1)]] ConstantBuffer<Global::GeneralUBO>        generalUBO;

struct PayloadData
{
    uint shellId;
    uint shellCount;
    uint meshletOffset;
};

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(128, 1, 1)]
void meshMain(
    uint3 threadId: SV_GroupIndex,
    in payload PayloadData inPayloadData,
    in uniform Global::PushConstant inPushConstant,
    out indices uint3 outSharedIndices[128],
    out vertices Geometry::MeshOutputVertex outSharedVertices[128]
)
{
    uint meshletId = inPushConstant.meshletId + inPayloadData.meshletOffset;
    Geometry::MeshletInfo currentMeshlet = meshletInfosBuffer[meshletId];

    SetMeshOutputCounts(currentMeshlet.vertexCount, currentMeshlet.primitiveCount);

    if(threadId.x < currentMeshlet.vertexCount)
    {
        outSharedVertices[threadId.x].meshletId = meshletId;

        uint vertexIndex = currentMeshlet.vertexOffset + threadId.x; //indexBuffer.indices[currentMeshlet.vertexOffset + threadId.x];
        Geometry::InputVertex vertex = vertexBuffer[vertexIndex];

#ifdef SHELL_TEXTURING
        uint shellId = PayloadData.shellId;
        vec4 positionWorld =  inPushConstant.model * vec4(ubo.hairLength*(taskData.shellCount - shellId - 1) /taskData.shellCount * normalize(vertex.normal) + vertex.pos, 1.0);
        positionWorld -= ubo.gravityFactor*vec4(0.0, 1.0, 0.0, 0.0) * max(0, 0.5 + dot(vec3(0.0, 1.0, 0.0), mat3(inPushConstant.model) * normalize(vertex.normal))) * (taskData.shellCount - shellId - 1)* (taskData.shellCount - shellId - 1)/(taskData.shellCount* taskData.shellCount);
#else
        float4 positionWorld = mul(inPushConstant.model, float4(vertex.pos, 1.f));
#endif
        outSharedVertices[threadId.x].texCoord = vertex.texCoord;
        outSharedVertices[threadId.x].normal = mul(float3x3(inPushConstant.model), normalize(vertex.normal));
        outSharedVertices[threadId.x].posWorld = positionWorld.xyz;
        outSharedVertices[threadId.x].posView = mul(generalUBO.view, positionWorld).xyz;
        outSharedVertices[threadId.x].tangent = float4(normalize(mul(float3x3(inPushConstant.model), vertex.tangent.xyz)), vertex.tangent.w);
#ifdef SHELL_TEXTURING
        outSharedVertices[threadId.x].shellId = payloadData.shellCount - shellId - 1;
        outSharedVertices[threadId.x].shellCount = payloadData.shellCount;
#endif
        outSharedVertices[threadId.x].pos = mul(mul(generalUBO.proj, generalUBO.view), positionWorld); // invert this ? 
  }

  if(threadId.x < currentMeshlet.primitiveCount)
  {
    Geometry::Triangle tri = primitivesBuffer[currentMeshlet.primitiveOffset + threadId.x];
    outSharedIndices[threadId.x] =  uint3(tri.i0, tri.i1, tri.i2);
  }
}
  
  
