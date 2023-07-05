#!/bin/bash
glslc vertexTextureCSM.vert -o vertexTextureCSM.spv
glslc fragmentTextureCSM.frag -o fragmentTextureCSM.spv
glslc vertexCSM.vert -o vertexCSM.spv
glslc fragmentCSM.frag -o fragmentCSM.spv
glslc vertexPBR.vert -o vertexPBR.spv
glslc fragmentPBR.frag -o fragmentPBR.spv