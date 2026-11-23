#include "ui_3d.h"
#include "rlgl.h"
#include "raymath.h"
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace ImGui3D {
namespace fs = std::filesystem;
namespace {
Texture2D environment_texture{}, agx_texture{};
Shader toon_shader{}, outline_shader{}, principled_shader{}, geometry_shader{};
RenderTexture2D position_buffer{};
bool default_textures_initialized = false;
int geometry_pass = 0;
PrincipledMaterial current_material{};
struct MeshCommand {
    Mesh mesh;
    Material material;
    std::array<MaterialMap, 12> maps;
    Matrix transform;
    PrincipledMaterial principled;
};
std::vector<MeshCommand> mesh_commands;

bool CaptureMesh(Mesh mesh, Material material, Matrix transform)
{
    if (!material.maps) return false;
    MeshCommand command{};
    command.mesh = mesh;
    command.material = material;
    for (size_t i = 0; i < command.maps.size(); ++i) command.maps[i] = material.maps[i];
    command.transform = MatrixMultiply(transform, rlGetMatrixTransform());
    command.principled = current_material;
    mesh_commands.push_back(command);
    return true;
}
void InitToonRendering()
{
    if (toon_shader.id) return;
    const char* vertex = R"(#version 330
        in vec3 vertexPosition;
        in vec2 vertexTexCoord;
        in vec4 vertexColor;
        uniform mat4 mvp;
        uniform mat4 matModel;
        out vec3 worldPosition;
        out vec2 uv;
        out vec4 vertexTint;
        void main() {
            worldPosition = (matModel * vec4(vertexPosition, 1.0)).xyz;
            uv = vertexTexCoord;
            vertexTint = vertexColor;
            gl_Position = mvp * vec4(vertexPosition, 1.0);
        })";
    const char* fragment = R"(#version 330
        in vec3 worldPosition;
        in vec2 uv;
        in vec4 vertexTint;
        uniform sampler2D texture0;
        uniform vec4 colDiffuse;
        out vec4 finalColor;
        void main() {
            vec3 normal = normalize(cross(dFdx(worldPosition), dFdy(worldPosition)));
            if (!gl_FrontFacing) normal = -normal;
            float light = max(dot(normal, normalize(vec3(0.45, 0.85, 0.55))), 0.0);
            float shade = light > 0.75 ? 1.0 : light > 0.38 ? 0.78 : light > 0.12 ? 0.58 : 0.42;
            vec4 color = texture(texture0, uv) * colDiffuse * vertexTint;
            finalColor = vec4(color.rgb * shade, color.a);
        })";
    toon_shader = LoadShaderFromMemory(vertex, fragment);
    const char* principled_vertex = R"(#version 330
        in vec3 vertexPosition;
        in vec3 vertexNormal;
        in vec2 vertexTexCoord;
        in vec4 vertexColor;
        uniform mat4 mvp;
        uniform mat4 matModel;
        uniform mat4 matNormal;
        out vec3 worldPosition;
        out vec3 worldNormal;
        out vec2 uv;
        out vec4 vertexTint;
        void main() {
            worldPosition = (matModel * vec4(vertexPosition, 1.0)).xyz;
            worldNormal = mat3(matNormal) * vertexNormal;
            uv = vertexTexCoord;
            vertexTint = vertexColor;
            gl_Position = mvp * vec4(vertexPosition, 1.0);
        })";
    const char* principled_fragment = R"(#version 330
        in vec3 worldPosition;
        in vec3 worldNormal;
        in vec2 uv;
        in vec4 vertexTint;
        uniform sampler2D texture0;
        uniform sampler2D metallicMap;
        uniform sampler2D roughnessMap;
        uniform int hasMetallicMap;
        uniform int hasRoughnessMap;
        uniform sampler2D environmentMap;
        uniform sampler2D agxLut;
        uniform sampler2D positionMap;
        uniform vec2 viewportSize;
        uniform int hasGeometry;
        uniform int hasEnvironment;
        uniform int hasAgx;
        uniform vec4 baseColor;
        uniform vec4 colDiffuse;
        uniform float metallic;
        uniform float roughness;
        uniform float ior;
        uniform float specularIorLevel;
        uniform vec3 cameraPosition;
        out vec4 finalColor;
        const float PI = 3.14159265359;
        vec2 environmentUv(vec3 direction) {
            // Convert viewport Y-up to Blender's Z-up before sampling the panorama.
            vec3 d = normalize(vec3(direction.x, -direction.z, direction.y));
            return vec2(atan(d.y, d.x) / (2.0*PI) + 0.5, asin(clamp(d.z, -1.0, 1.0)) / PI + 0.5);
        }
        vec3 environment(vec3 d, float lod) {
            return max(textureLod(environmentMap, environmentUv(d), lod).rgb, vec3(0.0));
        }
        vec3 applyAgx(vec3 linearColor) {
            const float n = 48.0;
            vec3 p = clamp((log2(max(linearColor, vec3(0.000244140625))) + 12.0) / 18.0, 0.0, 1.0) * (n - 1.0);
            float b0 = floor(p.b), b1 = min(b0 + 1.0, n - 1.0);
            vec2 uv0 = vec2((b0*n + p.r + 0.5)/(n*n), (p.g + 0.5)/n);
            vec2 uv1 = vec2((b1*n + p.r + 0.5)/(n*n), (p.g + 0.5)/n);
            return mix(texture(agxLut, uv0).rgb, texture(agxLut, uv1).rgb, p.b-b0);
        }
        float ambientVisibility(vec3 n) {
            vec2 center = gl_FragCoord.xy / viewportSize;
            vec2 pixel = 1.0 / vec2(textureSize(positionMap, 0));
            float blocked = 0.0;
            for (int i = 0; i < 16; ++i) {
                float angle = float(i) * 2.39996323;
                float radius = 2.0 + 12.0 * sqrt((float(i)+0.5)/16.0);
                vec2 coord = center + vec2(cos(angle), sin(angle)) * pixel * radius;
                if (any(lessThan(coord, vec2(0))) || any(greaterThan(coord, vec2(1)))) continue;
                vec4 samplePos = texture(positionMap, coord);
                if (samplePos.a < 0.5) continue;
                vec3 delta = samplePos.xyz - worldPosition;
                float distanceToSample = length(delta);
                if (distanceToSample < 0.002 || distanceToSample > 0.18) continue;
                blocked += smoothstep(0.05, 0.5, dot(n, delta) / distanceToSample) *
                           (1.0 - smoothstep(0.015, 0.18, distanceToSample));
            }
            return clamp(1.0 - blocked * 0.12, 0.38, 1.0);
        }
        float radicalInverse(int bits) {
            float result = 0.0, factor = 0.5;
            for (int j=0; j<8; ++j) { result += float(bits & 1)*factor; bits >>= 1; factor *= 0.5; }
            return result;
        }
        vec3 ggxReflection(vec3 n, vec3 v, float rough, vec3 f0, vec3 tangent, vec3 bitangent) {
            vec3 total = vec3(0.0);
            float alpha = rough*rough, a2 = alpha*alpha;
            const int count = 24;
            for (int i=0; i<count; ++i) {
                float u = (float(i)+0.5)/float(count), phi = 2.0*PI*radicalInverse(i);
                float cosTheta = sqrt((1.0-u)/(1.0+(a2-1.0)*u));
                float sinTheta = sqrt(max(0.0, 1.0-cosTheta*cosTheta));
                vec3 h = normalize(tangent*cos(phi)*sinTheta + bitangent*sin(phi)*sinTheta + n*cosTheta);
                vec3 l = reflect(-v, h); float ndl = max(dot(n,l),0.0);
                if (ndl > 0.0) {
                    float vdh = max(dot(v,h),0.0);
                    vec3 f = f0 + (1.0-f0)*pow(1.0-vdh,5.0);
                    float ndv = max(dot(n, v), 0.001);
                    float ndh = max(dot(n, h), 0.001);
                    float gv = 2.0*ndv/(ndv + sqrt(a2 + (1.0-a2)*ndv*ndv));
                    float gl = 2.0*ndl/(ndl + sqrt(a2 + (1.0-a2)*ndl*ndl));
                    total += environment(l, 3.0) * f * gv * gl * vdh / (ndv * ndh);
                }
            }
            return total / float(count);
        }
        void main() {
            vec3 geometric = normalize(cross(dFdx(worldPosition), dFdy(worldPosition)));
            vec3 n = length(worldNormal) > 0.001 ? normalize(worldNormal) : geometric;
            if (!gl_FrontFacing) n = -n;
            vec3 v = normalize(cameraPosition - worldPosition);
            float ndv = max(dot(n, v), 0.001);
            float metal = metallic * (hasMetallicMap != 0 ? texture(metallicMap, uv).r : 1.0);
            float r = clamp(roughness * (hasRoughnessMap != 0 ? texture(roughnessMap, uv).r : 1.0), 0.04, 1.0);
            float dielectric = pow((ior-1.0)/(ior+1.0), 2.0) * (2.0 * specularIorLevel);
            vec3 texel = pow(texture(texture0, uv).rgb, vec3(2.2));
            vec3 albedo = baseColor.rgb * texel * vertexTint.rgb * colDiffuse.rgb;
            vec3 f0 = mix(vec3(dielectric), albedo, metal);
            vec3 fresnel = f0 + (max(vec3(1.0-r), f0)-f0)*pow(1.0-ndv, 5.0);
            vec3 linearColor;
            if (hasEnvironment != 0) {
                // Cosine-weighted hemisphere samples approximate diffuse studio lighting.
                vec3 tangent = normalize(cross(abs(n.y) < 0.99 ? vec3(0,1,0) : vec3(1,0,0), n));
                vec3 bitangent = cross(n, tangent);
                vec3 irradiance = vec3(0.0);
                const int count = 24;
                for (int i = 0; i < count; ++i) {
                    float u = (float(i) + 0.5) / float(count);
                    float phi = float(i) * 2.39996323;
                    float radial = sqrt(u);
                    vec3 direction = tangent*(radial*cos(phi)) + bitangent*(radial*sin(phi)) + n*sqrt(1.0-u);
                    irradiance += environment(direction, 5.0);
                }
                irradiance /= float(count);
                vec3 reflected = ggxReflection(n, v, r, f0, tangent, bitangent);
                float ao = hasGeometry != 0 ? ambientVisibility(n) : 1.0;
                // The preview environment is not a directional lamp: a single
                // shadow map produces false bands across HDRI-lit surfaces.
                linearColor = irradiance * albedo * (1.0-metal) * (1.0-fresnel) * ao
                            + reflected * mix(ao, 1.0, r * 0.5);
            } else {
                vec3 l = normalize(vec3(0.45, 0.85, 0.55));
                linearColor = albedo * (0.35 + max(dot(n, l), 0.0) * 0.8);
            }
            vec3 mapped = linearColor / (vec3(1.0) + linearColor);
            vec3 displayColor = hasAgx != 0 ? applyAgx(linearColor) : pow(max(mapped, vec3(0.0)), vec3(1.0/2.2));
            finalColor = vec4(displayColor, baseColor.a * vertexTint.a * colDiffuse.a * texture(texture0, uv).a);
        })";
    principled_shader = LoadShaderFromMemory(principled_vertex, principled_fragment);
    principled_shader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(principled_shader, "environmentMap");
    principled_shader.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(principled_shader, "agxLut");
    principled_shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(principled_shader, "positionMap");
    principled_shader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(principled_shader, "metallicMap");
    principled_shader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(principled_shader, "roughnessMap");
    const char* geometry_vertex = R"(#version 330
        in vec3 vertexPosition;
        uniform mat4 mvp;
        uniform mat4 matModel;
        out vec3 worldPosition;
        void main() {
            worldPosition = (matModel * vec4(vertexPosition, 1.0)).xyz;
            gl_Position = mvp * vec4(vertexPosition, 1.0);
        })";
    const char* geometry_fragment = R"(#version 330
        in vec3 worldPosition;
        uniform int depthOnly;
        out vec4 finalColor;
        void main() { finalColor = depthOnly != 0 ? vec4(gl_FragCoord.z, 0, 0, 1) : vec4(worldPosition, 1); }
        )";
    geometry_shader = LoadShaderFromMemory(geometry_vertex, geometry_fragment);
    const char* outline_vertex = R"(#version 330
        in vec3 vertexPosition;
        in vec3 vertexNormal;
        uniform mat4 mvp;
        void main() {
            vec4 position = mvp * vec4(vertexPosition, 1.0);
            vec3 normal = length(vertexNormal) > 0.001 ? normalize(vertexNormal) : normalize(vertexPosition + vec3(0.0001));
            vec2 direction = (mvp * vec4(normal, 0.0)).xy;
            direction = length(direction) > 0.0001 ? normalize(direction) : vec2(0.0);
            position.xy += direction * vec2(2.0 / 1280.0, 2.0 / 720.0) * 2.5 * position.w;
            gl_Position = position;
        })";
    const char* outline_fragment = R"(#version 330
        out vec4 finalColor;
        void main() { finalColor = vec4(0.035, 0.045, 0.065, 1.0); })";
    outline_shader = LoadShaderFromMemory(outline_vertex, outline_fragment);
}

bool LoadFloatTexture(const fs::path& path, Texture2D& target, bool mipmaps)
{
    std::ifstream input(path, std::ios::binary);
    uint32_t width = 0, height = 0;
    if (!input.read(reinterpret_cast<char*>(&width), sizeof(width)) ||
        !input.read(reinterpret_cast<char*>(&height), sizeof(height)) ||
        !width || !height || width > 4096 || height > 4096) return false;
    std::vector<float> pixels((size_t)width * height * 4);
    if (!input.read(reinterpret_cast<char*>(pixels.data()), pixels.size() * sizeof(float))) return false;
    Image image = {pixels.data(), (int)width, (int)height, 1, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32};
    target = LoadTextureFromImage(image);
    if (!target.id) return false;
    if (mipmaps) GenTextureMipmaps(&target);
    SetTextureFilter(target, mipmaps ? TEXTURE_FILTER_TRILINEAR : TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(target, mipmaps ? TEXTURE_WRAP_REPEAT : TEXTURE_WRAP_CLAMP);
    return true;
}

RenderTexture2D LoadGeometryBuffer(int width, int height)
{
    RenderTexture2D buffer{};
    buffer.id = rlLoadFramebuffer();
    if (!buffer.id) return {};
    buffer.texture = {rlLoadTexture(nullptr, width, height, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32, 1),
                      width, height, 1, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32};
    buffer.depth = {rlLoadTextureDepth(width, height, true), width, height, 1, 19};
    rlFramebufferAttach(buffer.id, buffer.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    rlFramebufferAttach(buffer.id, buffer.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);
    if (!rlFramebufferComplete(buffer.id)) {
        UnloadRenderTexture(buffer);
        return {};
    }
    SetTextureFilter(buffer.texture, TEXTURE_FILTER_POINT);
    SetTextureWrap(buffer.texture, TEXTURE_WRAP_CLAMP);
    return buffer;
}

 }
void SetMaterial(const PrincipledMaterial* material)
{
    current_material = material ? *material : PrincipledMaterial{};
}

static void ApplyPreviewMaterial(Material* material)
{
    if (geometry_pass) {
        material->shader = geometry_shader;
        const int depth = 0;
        SetShaderValue(geometry_shader, GetShaderLocation(geometry_shader, "depthOnly"), &depth, SHADER_UNIFORM_INT);
        return;
    }
    if (!principled_shader.id) return;
    material->shader = principled_shader;
    PrincipledMaterial effective = current_material;
    const bool gltf = material->params[3] == 1.0f;
    if (gltf) {
        effective.base[0] = effective.base[1] = effective.base[2] = effective.base[3] = 1.0f;
        effective.metallic = material->maps[MATERIAL_MAP_METALNESS].value;
        effective.roughness = material->maps[MATERIAL_MAP_ROUGHNESS].value;
    }
    const int has_metallic_map = gltf && material->maps[MATERIAL_MAP_METALNESS].texture.id != 0;
    const int has_roughness_map = gltf && material->maps[MATERIAL_MAP_ROUGHNESS].texture.id != 0;
    SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "hasMetallicMap"), &has_metallic_map, SHADER_UNIFORM_INT);
    SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "hasRoughnessMap"), &has_roughness_map, SHADER_UNIFORM_INT);
    material->maps[MATERIAL_MAP_NORMAL].texture = position_buffer.texture;
    material->maps[MATERIAL_MAP_OCCLUSION].texture = agx_texture;
    material->maps[MATERIAL_MAP_EMISSION].texture = environment_texture;
    const int has_geometry = position_buffer.id != 0;
    const int has_agx = agx_texture.id != 0;
    const int has_environment = environment_texture.id != 0;
    const float viewport_size[] = {(float)rlGetFramebufferWidth(), (float)rlGetFramebufferHeight()};
    SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "hasGeometry"), &has_geometry, SHADER_UNIFORM_INT);
    SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "viewportSize"), viewport_size, SHADER_UNIFORM_VEC2);
    SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "hasAgx"), &has_agx, SHADER_UNIFORM_INT);
    SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "hasEnvironment"), &has_environment, SHADER_UNIFORM_INT);
    SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "cameraPosition"), &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "baseColor"), effective.base, SHADER_UNIFORM_VEC4);
    SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "metallic"), &effective.metallic, SHADER_UNIFORM_FLOAT);
    SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "roughness"), &effective.roughness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "ior"), &effective.ior, SHADER_UNIFORM_FLOAT);
    SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "specularIorLevel"), &effective.specular_ior_level, SHADER_UNIFORM_FLOAT);
}

void BeginPreviewCapture()
{
    InitToonRendering();
    if (!default_textures_initialized) {
        const fs::path assets = fs::path(IMGUI_ROOT) / "resources";
        if (!environment_texture.id) LoadFloatTexture(assets / "forest_env.rgba32f", environment_texture, true);
        if (!agx_texture.id) LoadFloatTexture(assets / "agx_view.rgba32f", agx_texture, false);
        default_textures_initialized = true;
    }
    mesh_commands.clear();
    SetMaterial();
    SetDrawMeshCaptureHook(CaptureMesh);
}

void EndPreviewCapture()
{
    SetDrawMeshCaptureHook(nullptr);
    rlDrawRenderBatchActive();
    SetDrawMeshMaterialHook(ApplyPreviewMaterial);
    auto replay = [] {
        for (auto& command : mesh_commands) {
            command.material.maps = command.maps.data();
            current_material = command.principled;
            DrawMesh(command.mesh, command.material, command.transform);
        }
    };
    if (geometry_shader.id) {
        const unsigned int old_framebuffer = rlGetActiveFramebuffer();
        const int width = rlGetFramebufferWidth(), height = rlGetFramebufferHeight();
        const Matrix view = rlGetMatrixModelview(), projection = rlGetMatrixProjection();
        if (!position_buffer.id) position_buffer = LoadGeometryBuffer(640, 360);
        if (position_buffer.id) {
            rlEnableFramebuffer(position_buffer.id);
            rlViewport(0, 0, position_buffer.texture.width, position_buffer.texture.height);
            rlSetMatrixModelview(view);
            rlSetMatrixProjection(projection);
            ClearBackground(BLANK);
            geometry_pass = 1;
            replay();
            geometry_pass = 0;
        }
        rlEnableFramebuffer(old_framebuffer);
        rlViewport(0, 0, width, height);
        rlSetMatrixModelview(view);
        rlSetMatrixProjection(projection);
    }
    replay();
    SetDrawMeshMaterialHook(nullptr);
    SetMaterial();
    mesh_commands.clear();
}
void ResetPreviewTextures()
{
    if (environment_texture.id) UnloadTexture(environment_texture);
    if (agx_texture.id) UnloadTexture(agx_texture);
    environment_texture = {}; agx_texture = {};
    default_textures_initialized = false;
}
void DestroyPreviewRenderer()
{
    ResetPreviewTextures();
    if (toon_shader.id) UnloadShader(toon_shader);
    if (outline_shader.id) UnloadShader(outline_shader);
    if (principled_shader.id) UnloadShader(principled_shader);
    if (geometry_shader.id) UnloadShader(geometry_shader);
    if (position_buffer.id) UnloadRenderTexture(position_buffer);
    toon_shader = {}; outline_shader = {}; principled_shader = {}; geometry_shader = {}; position_buffer = {};
}
}
