#!/bin/bash
glslc vertexTexture.vert -o vertexTexture.spv
glslc fragmentTexture.frag -o fragmentTexture.spv
glslc vertexDebugNormals.vert -o vertexDebugNormals.spv
glslc fragmentDebugNormals.frag -o fragmentDebugNormals.spv
glslc vertexDebugTextureCoords.vert -o vertexDebugTextureCoords.spv
glslc fragmentDebugTextureCoords.frag -o fragmentDebugTextureCoords.spv
glslc vertexDebugTextureCoords.vert -o vertexDebugTextureCoords.spv
glslc fragmentDebugTextureCoords.frag -o fragmentDebugTextureCoords.spv
glslc vertexTextureNoLight.vert -o vertexTextureNoLight.spv
glslc fragmentTextureNoLight.frag -o fragmentTextureNoLight.spv
glslc fragmentDepth.frag -o fragmentDepth.spv
