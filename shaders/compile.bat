glslc vertexTextureCSM.vert -o vertexTextureCSM.spv -g
glslc fragmentTextureCSM.frag -o fragmentTextureCSM.spv -g
glslc vertexCSM.vert -o vertexCSM.spv -g
glslc fragmentCSM.frag -o fragmentCSM.spv -g
glslc vertexPBR.vert -o vertexPBR.spv -g
glslc fragmentPBR.frag -o fragmentPBR.spv -g
glslc fragmentDepthPrePass.frag -o fragmentDepthPrePass.spv -g
glslc --target-spv=spv1.5 meshPBR.mesh -o meshPBR.spv -g
glslc --target-spv=spv1.5 taskShell.task -o taskShell.spv -g
glslc --target-spv=spv1.5 CSM.mesh -o meshCSM.spv -g
glslc --target-spv=spv1.5 taskShadow.task -o taskShadow.spv -g