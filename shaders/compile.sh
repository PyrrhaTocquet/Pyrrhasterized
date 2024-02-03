#!/bin/bash
glslc vertexTextureCSM.vert -o vertexTextureCSM.spv
glslc fragmentTextureCSM.frag -o fragmentTextureCSM.spv
glslc vertexCSM.vert -o vertexCSM.spv
glslc fragmentCSM.frag -o fragmentCSM.spv
glslc vertexPBR.vert -o vertexPBR.spv
glslc fragmentPBR.frag -o fragmentPBR.spv
glslc --target-spv=spv1.5 meshPBR.mesh -o meshPBR.spv
glslc --target-spv=spv1.5 taskShell.task -o taskShell.spv
glslc --target-spv=spv1.5 CSM.mesh -o meshCSM.spv
glslc --target-spv=spv1.5 taskShadow.task -o taskShadow.spv