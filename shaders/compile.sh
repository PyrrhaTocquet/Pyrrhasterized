#!/bin/bash
rm *.spv
rm meshCSM.mesh
../slang/bin/slangc.exe CSM.mesh.slang -profile glsl_460 -o meshCSM.mesh -target glsl -entry meshMain -g
#glslc fragmentTextureCSM.frag -o fragmentTextureCSM.spv -g
#glslc fragmentCSM.frag -o fragmentCSM.spv -g
#glslc fragmentPBR.frag -o fragmentPBR.spv -g
glslc --target-spv=spv1.5 legacy/meshPBR.mesh -o meshPBR.spv -g
#glslc --target-spv=spv1.5 taskShell.task -o taskShell.spv -g
glslc --target-spv=spv1.5 meshCSM.mesh -o meshCSM.spv -g
#glslc --target-spv=spv1.5 taskShadow.task -o taskShadow.spv -g

../slang/bin/slangc.exe CSM.mesh.slang -profile spirv_1_5 -o meshCSM.spv -entry meshMain -g -target spirv #not compiling somehow ????
../slang/bin/slangc.exe mainPBRMaterial.mesh.slang -profile spirv_1_5 -o meshPBR.spv -entry meshMain -g -target spirv #not compiling somehow ????



../slang/bin/slangc.exe CSM.amp.slang -profile spirv_1_4 -o ampCSM.spv -entry amplificationMain -g
../slang/bin/slangc.exe mainPBRMaterial.frag.slang -profile spirv_1_4 -o fragmentPBR.spv -entry fragMain -g
../slang/bin/slangc.exe mainPBRMaterial.amp.slang -profile spirv_1_4 -o amplificationPBR.spv -entry amplificationMain -g
../slang/bin/slangc.exe DepthOnlyPass.frag.slang -profile spirv_1_4 -o fragDepthOnly.spv -entry fragMain -g
../slang/bin/slangc.exe frustumGrid.comp.slang -profile spirv_1_4 -o frustumGrid.spv -entry main -g
echo "done"
read 