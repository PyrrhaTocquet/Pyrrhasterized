/*
author: Pyrrha Tocquet
date: 01/04/25
desc: Cascaded shadow maps amplification/task shader
*/
import "modules/global";

[shader("amplification")]
[numthreads(1, 1, 1)]
void amplificationMain(
    uint3 globalThreadId: SV_GroupThreadID,
    in uniform Global::PushConstant inPushConstant,
)
{
    DispatchMesh(1, 1, 1, globalThreadId.x); // pretty useless shader right :' )
}