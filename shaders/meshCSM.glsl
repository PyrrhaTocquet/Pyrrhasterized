#version 460
#extension GL_EXT_mesh_shader : require
layout(row_major) uniform;
layout(row_major) buffer;

#line 16 0
struct MeshletInfo_std430_0
{
    vec4 boundingSphere_0;
    uint vertexCount_0;
    uint vertexOffset_0;
    uint primitiveCount_0;
    uint primitiveOffset_0;
    uint meshletId_0;
};


#line 34
layout(std430, binding = 0) readonly buffer StructuredBuffer_MeshletInfo_std430_t_0 {
    MeshletInfo_std430_0 _data[];
} meshletInfosBuffer_0;

#line 6
struct Vertex_std430_0
{
    vec3 pos_0;
    vec2 texCoord_0;
    vec3 normal_0;
    vec4 tangent_0;
};


#line 37
layout(std430, binding = 3) readonly buffer StructuredBuffer_Vertex_std430_t_0 {
    Vertex_std430_0 _data[];
} vertexBuffer_0;


struct _MatrixStorage_float4x4_ColMajorstd140_0
{
    vec4  data_0[4];
};


#line 42
struct _Array_std140_matrixx3Cfloatx2C4x2C4x3E4_0
{
    _MatrixStorage_float4x4_ColMajorstd140_0  data_1[4];
};


#line 43
struct CascadeUniformObject_std140_0
{
    _Array_std140_matrixx3Cfloatx2C4x2C4x3E4_0 cascadeViewProj_0;
    vec4 cascadeSplits_0;
};


#line 45
layout(binding = 0, set = 1)
layout(std140) uniform block_CascadeUniformObject_std140_0
{
    _Array_std140_matrixx3Cfloatx2C4x2C4x3E4_0 cascadeViewProj_0;
    vec4 cascadeSplits_0;
}ubo_0;

#line 23
struct Triangle_std430_0
{
    uint i0_0;
    uint i1_0;
    uint i2_0;
};


#line 35
layout(std430, binding = 1) readonly buffer StructuredBuffer_Triangle_std430_t_0 {
    Triangle_std430_0 _data[];
} primitivesBuffer_0;

#line 31
struct _MatrixStorage_float4x4_ColMajorstd430_0
{
    vec4  data_2[4];
};


#line 31
struct PushConstant_std430_0
{
    _MatrixStorage_float4x4_ColMajorstd430_0 model_0;
    int materialId_0;
    uint cascadeId_0;
    uint meshletId_1;
};


#line 31
struct EntryPointParams_std430_0
{
    PushConstant_std430_0 pushConstant_0;
};


#line 54
layout(push_constant)
layout(std430) uniform block_EntryPointParams_std430_0
{
    PushConstant_std430_0 pushConstant_0;
}entryPointParams_0;

#line 54
out gl_MeshPerVertexEXT
{
    vec4 gl_Position;
} gl_MeshVerticesEXT[128];


out uvec3  gl_PrimitiveTriangleIndicesEXT[128];


#line 47
struct PayloadData_0
{
    uint meshletOffset_0;
};


#line 47
taskPayloadSharedEXT PayloadData_0 payloadData_0;


#line 60
layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
layout(max_vertices = 128) out;
layout(max_primitives = 128) out;
layout(triangles) out;
void main()
{

#line 60
    uint _S1 = meshletInfosBuffer_0._data[uint(entryPointParams_0.pushConstant_0.meshletId_1 + payloadData_0.meshletOffset_0)].primitiveCount_0;

#line 69
    SetMeshOutputsEXT(meshletInfosBuffer_0._data[uint(entryPointParams_0.pushConstant_0.meshletId_1 + payloadData_0.meshletOffset_0)].vertexCount_0, meshletInfosBuffer_0._data[uint(entryPointParams_0.pushConstant_0.meshletId_1 + payloadData_0.meshletOffset_0)].primitiveCount_0);

    uint _S2 = gl_GlobalInvocationID.x;

#line 71
    if(_S2 < (meshletInfosBuffer_0._data[uint(entryPointParams_0.pushConstant_0.meshletId_1 + payloadData_0.meshletOffset_0)].vertexCount_0))
    {

#line 78
        gl_MeshVerticesEXT[_S2].gl_Position = (((mat4x4(ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[0][0], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[1][0], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[2][0], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[3][0], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[0][1], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[1][1], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[2][1], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[3][1], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[0][2], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[1][2], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[2][2], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[3][2], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[0][3], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[1][3], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[2][3], ubo_0.cascadeViewProj_0.data_1[entryPointParams_0.pushConstant_0.cascadeId_0].data_0[3][3])) * ((((mat4x4(entryPointParams_0.pushConstant_0.model_0.data_2[0][0], entryPointParams_0.pushConstant_0.model_0.data_2[1][0], entryPointParams_0.pushConstant_0.model_0.data_2[2][0], entryPointParams_0.pushConstant_0.model_0.data_2[3][0], entryPointParams_0.pushConstant_0.model_0.data_2[0][1], entryPointParams_0.pushConstant_0.model_0.data_2[1][1], entryPointParams_0.pushConstant_0.model_0.data_2[2][1], entryPointParams_0.pushConstant_0.model_0.data_2[3][1], entryPointParams_0.pushConstant_0.model_0.data_2[0][2], entryPointParams_0.pushConstant_0.model_0.data_2[1][2], entryPointParams_0.pushConstant_0.model_0.data_2[2][2], entryPointParams_0.pushConstant_0.model_0.data_2[3][2], entryPointParams_0.pushConstant_0.model_0.data_2[0][3], entryPointParams_0.pushConstant_0.model_0.data_2[1][3], entryPointParams_0.pushConstant_0.model_0.data_2[2][3], entryPointParams_0.pushConstant_0.model_0.data_2[3][3])) * (vec4(vertexBuffer_0._data[uint(meshletInfosBuffer_0._data[uint(entryPointParams_0.pushConstant_0.meshletId_1 + payloadData_0.meshletOffset_0)].vertexOffset_0 + _S2)].pos_0, 1.0)))))));

#line 71
    }

#line 81
    if(_S2 < _S1)
    {

        gl_PrimitiveTriangleIndicesEXT[_S2] = uvec3(primitivesBuffer_0._data[uint(meshletInfosBuffer_0._data[uint(entryPointParams_0.pushConstant_0.meshletId_1 + payloadData_0.meshletOffset_0)].primitiveOffset_0 + _S2)].i0_0, primitivesBuffer_0._data[uint(meshletInfosBuffer_0._data[uint(entryPointParams_0.pushConstant_0.meshletId_1 + payloadData_0.meshletOffset_0)].primitiveOffset_0 + _S2)].i1_0, primitivesBuffer_0._data[uint(meshletInfosBuffer_0._data[uint(entryPointParams_0.pushConstant_0.meshletId_1 + payloadData_0.meshletOffset_0)].primitiveOffset_0 + _S2)].i2_0);

#line 81
    }

#line 86
    return;
}

