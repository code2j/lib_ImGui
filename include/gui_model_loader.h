#pragma once
#include "raylib.h"

// 모델과 애니메이션 데이터를 하나로 묶어주는 구조체
typedef struct {
    Model model;
    ModelAnimation *animations;
    int animCount;
} GltfData;

// ==================================================================
// 커스텀 함수: GLTF 파일 로드
// ==================================================================
GltfData load_gltf(const char* fileName)
{
    GltfData data = { 0 };

    // 모델 로드
    data.model = LoadModel(fileName);

    // 애니메이션 로드
    data.animCount = 0;
    data.animations = LoadModelAnimations(fileName, &data.animCount);

    return data;
}

// ==================================================================
// 커스텀 함수: GLTF 데이터 메모리 해제
// ==================================================================
void unload_gltf(GltfData data)
{
    UnloadModelAnimations(data.animations, data.animCount);
    UnloadModel(data.model);
}