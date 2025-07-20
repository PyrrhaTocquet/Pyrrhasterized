/*
author: Pyrrha Tocquet
date: 07/04/25
desc: Amplification Shader for main materials
*/
import "modules/global";
import "modules/geometry";

[[vk::binding(0, 0)]] StructuredBuffer<Geometry::MeshletInfo> meshletInfosBuffer; // TODO unused remove ?
[[vk::binding(0, 1)]] ConstantBuffer<Global::GeneralUBO> generalUbo;

struct PayloadData
{
    uint shellId;
    uint shellCount;
    uint meshletOffset;
};

[shader("amplification")]
[numthreads(1, 1, 1)]
void amplificationMain(
    uint3 globalThreadId: SV_DispatchThreadID,
    in uniform Global::PushConstant inPushConstant,
)
{
    PayloadData payload;
#ifdef SHELL_TEXTURING
    payload.shellId = gl_WorkGroupID.z;
    payload.shellCount = gl_NumWorkGroups.z;
    payload.meshletOffset = gl_GlobalInvocationID.x;
#else
    payload.shellId = 0u;
    payload.shellCount = 1u;
    payload.meshletOffset = globalThreadId.x;
#endif
    DispatchMesh(1, 1, 1, payload);
}

// MeshletInfo meshlet = meshletInfosBuffer.meshletInfos[PushConstants.meshletId + gl_GlobalInvocationID.x];

/*vec3 viewSpaceCenter = vec3(ubo.view * PushConstants.model * vec4(meshlet.boundingSphere.xyz, 1.0));

float viewSpaceRadius = meshlet.boundingSphere.w * max(max(abs(PushConstants.model[0][0]), abs(PushConstants.model[1][1])), abs(PushConstants.model[2][2])) * max(max(abs(ubo.view[0][0]), abs(ubo.view[1][1])), abs(ubo.view[2][2]));

float fovy = 3.14/4.0;
float fovx = 3.14/2.0;

float xf = tan(fovx) * (-viewSpaceCenter.z);


if(viewSpaceCenter.x - viewSpaceRadius <= xf)
{
    EmitMeshTasksEXT(1, 1, 1);
}*/
