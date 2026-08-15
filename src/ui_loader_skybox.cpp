#include "ui_loader.h"
namespace ImGui
{
    Model load_skybox(const char* cubemapFileName)
    {
        const char* vsFileName = IMGUI_ROOT "/data/shaders/skybox.vs";
        const char* fsFileName = IMGUI_ROOT "/data/shaders/skybox.fs";


        //  스카이박스용 큐브 메쉬 생성
        Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
        Model skybox = LoadModelFromMesh(cube);

        //  셰이더 로드
        skybox.materials[0].shader = LoadShader(vsFileName, fsFileName);

        // 셰이더 내부 변수(Uniform) 설정
        int mapValue = MATERIAL_MAP_CUBEMAP;
        SetShaderValue(skybox.materials[0].shader, GetShaderLocation(skybox.materials[0].shader, "environmentMap"), &mapValue, SHADER_UNIFORM_INT);

        int gammaValue = 0;
        SetShaderValue(skybox.materials[0].shader, GetShaderLocation(skybox.materials[0].shader, "doGamma"), &gammaValue, SHADER_UNIFORM_INT);

        int vflippedValue = 0;
        SetShaderValue(skybox.materials[0].shader, GetShaderLocation(skybox.materials[0].shader, "vflipped"), &vflippedValue, SHADER_UNIFORM_INT);

        // 스카이박스 텍스처(Cubemap) 로드 및 모델에 적용
        ::Image image = LoadImage(cubemapFileName);
        skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(image, CUBEMAP_LAYOUT_AUTO_DETECT);
        UnloadImage(image); // GPU에 텍스처를 올렸으므로 RAM에 있는 이미지 데이터는 해제

        return skybox;
    }
}
