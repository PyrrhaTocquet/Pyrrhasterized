#!/bin/bash
#glslc fragmentTextureCSM.frag -o fragmentTextureCSM.spv -g
#glslc fragmentCSM.frag -o fragmentCSM.spv -g
#glslc fragmentPBR.frag -o fragmentPBR.spv -g
#glslc --target-spv=spv1.5 meshPBR.mesh -o meshPBR.spv -g
#glslc --target-spv=spv1.5 taskShell.task -o taskShell.spv -g
#glslc --target-spv=spv1.5 CSM.mesh -o meshCSM.spv -g
#glslc --target-spv=spv1.5 taskShadow.task -o taskShadow.spv -g
../slang/bin/slangc.exe CSM.mesh.slang -profile spirv_1_4 -o meshCSM.spv -entry meshMain
../slang/bin/slangc.exe CSM.amp.slang -profile spirv_1_4 -o ampCSM.spv -entry amplificationMain
../slang/bin/slangc.exe mainPBRMaterial.mesh.slang -profile spirv_1_4 -o meshPBR.spv -entry meshMain
../slang/bin/slangc.exe mainPBRMaterial.frag.slang -profile spirv_1_4 -o fragmentPBR.spv -entry fragMain
../slang/bin/slangc.exe mainPBRMaterial.amp.slang -profile spirv_1_4 -o amplificationPBR.spv -entry amplificationMain
../slang/bin/slangc.exe DepthOnlyPass.frag.slang -profile spirv_1_4 -o fragDepthOnly.spv -entry fragMain
echo "done"
read 