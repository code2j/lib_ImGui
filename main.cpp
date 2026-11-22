#include "ui.hpp"
#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <limits>
#include <cmath>

#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <urdf_parser/urdf_parser.h>
#include <filesystem>
#include <cstdlib>
#include <unordered_map>
#include <vector>




namespace {
namespace fs = std::filesystem;
urdf::ModelInterfaceSharedPtr robot;
std::unordered_map<std::string, Model> meshes;
struct JointControl {
    urdf::JointConstSharedPtr joint;
    float value = 0.0f;
    float lower = 0.0f;
    float upper = 0.0f;
};
std::vector<JointControl> joint_controls;
std::unordered_map<std::string, size_t> joint_indices;
struct PrincipledMaterial {
    float base[4] = {1, 1, 1, 1};
    float metallic = 0, roughness = 0.5f, ior = 1.5f, specular_ior_level = 0.5f;
};
std::unordered_map<std::string, PrincipledMaterial> principled_materials;
bool use_principled = true;
Texture2D environment_texture{}, agx_texture{};
Shader toon_shader{}, outline_shader{}, principled_shader{}, geometry_shader{};
RenderTexture2D position_buffer{}, shadow_buffer{};
int geometry_pass = 0;
Matrix shadow_view_projection{};
Model box_model{}, sphere_model{}, cylinder_model{};

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
        uniform sampler2D environmentMap;
        uniform sampler2D agxLut;
        uniform sampler2D positionMap;
        uniform sampler2D shadowMap;
        uniform mat4 lightVP;
        uniform vec2 viewportSize;
        uniform int hasGeometry;
        uniform int hasEnvironment;
        uniform int hasAgx;
        uniform vec4 baseColor;
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
        float shadowVisibility(vec3 n) {
            vec4 lightClip = lightVP * vec4(worldPosition, 1.0);
            vec3 projected = lightClip.xyz / lightClip.w * 0.5 + 0.5;
            if (any(lessThan(projected, vec3(0))) || any(greaterThan(projected, vec3(1)))) return 1.0;
            float bias = max(0.0005, 0.0018 * (1.0 - max(dot(n, normalize(vec3(0.75, 0.4, -0.5))), 0.0)));
            vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
            float visible = 0.0;
            for (int y = -1; y <= 1; ++y)
                for (int x = -1; x <= 1; ++x) {
                    vec4 stored = texture(shadowMap, projected.xy + vec2(x, y)*texel);
                    visible += (stored.a < 0.5 || projected.z - bias <= stored.r) ? 1.0 : 0.0;
                }
            return visible / 9.0;
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
            float r = clamp(roughness, 0.04, 1.0);
            float dielectric = pow((ior-1.0)/(ior+1.0), 2.0) * (2.0 * specularIorLevel);
            vec3 texel = pow(texture(texture0, uv).rgb, vec3(2.2));
            vec3 albedo = baseColor.rgb * texel * vertexTint.rgb;
            vec3 f0 = mix(vec3(dielectric), albedo, metallic);
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
                float shadow = hasGeometry != 0 ? shadowVisibility(n) : 1.0;
                // The HDRI is an area environment, so only part of its light comes
                // from the bright region represented by this shadow map.
                float lightingVisibility = ao * mix(0.72, 1.0, shadow);
                linearColor = irradiance * albedo * (1.0-metallic) * (1.0-fresnel) * lightingVisibility
                            + reflected * mix(ao, 1.0, r * 0.5) * mix(0.8, 1.0, shadow);
            } else {
                vec3 l = normalize(vec3(0.45, 0.85, 0.55));
                linearColor = albedo * (0.35 + max(dot(n, l), 0.0) * 0.8);
            }
            vec3 mapped = linearColor / (vec3(1.0) + linearColor);
            vec3 displayColor = hasAgx != 0 ? applyAgx(linearColor) : pow(max(mapped, vec3(0.0)), vec3(1.0/2.2));
            finalColor = vec4(displayColor, baseColor.a * vertexTint.a);
        })";
    principled_shader = LoadShaderFromMemory(principled_vertex, principled_fragment);
    principled_shader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(principled_shader, "environmentMap");
    principled_shader.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(principled_shader, "agxLut");
    principled_shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(principled_shader, "positionMap");
    principled_shader.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(principled_shader, "shadowMap");
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
    box_model = LoadModelFromMesh(GenMeshCube(1, 1, 1));
    sphere_model = LoadModelFromMesh(GenMeshSphere(1, 20, 24));
    cylinder_model = LoadModelFromMesh(GenMeshCylinder(1, 1, 24));
}

void DrawToonModel(Model& model, Vector3 position, Vector3 axis, float angle, Vector3 scale, Color color,
                   const PrincipledMaterial* principled = nullptr)
{
    if (geometry_pass) {
        std::vector<Shader> original;
        for (int i = 0; i < model.materialCount; ++i) {
            original.push_back(model.materials[i].shader);
            model.materials[i].shader = geometry_shader;
        }
        const int depth = geometry_pass == 2;
        SetShaderValue(geometry_shader, GetShaderLocation(geometry_shader, "depthOnly"), &depth, SHADER_UNIFORM_INT);
        DrawModelEx(model, position, axis, angle, scale, WHITE);
        for (int i = 0; i < model.materialCount; ++i) model.materials[i].shader = original[i];
        return;
    }
    if (use_principled && principled && principled_shader.id) {
        std::vector<Shader> original;
        std::vector<Texture2D> original_environment, original_occlusion, original_position, original_shadow;
        original_occlusion.reserve(model.materialCount);
        original.reserve(model.materialCount);
        original_environment.reserve(model.materialCount);
        for (int i = 0; i < model.materialCount; ++i) {
            original.push_back(model.materials[i].shader);
            original_position.push_back(model.materials[i].maps[MATERIAL_MAP_NORMAL].texture);
            original_shadow.push_back(model.materials[i].maps[MATERIAL_MAP_ROUGHNESS].texture);
            model.materials[i].maps[MATERIAL_MAP_NORMAL].texture = position_buffer.texture;
            model.materials[i].maps[MATERIAL_MAP_ROUGHNESS].texture = shadow_buffer.texture;
            original_environment.push_back(model.materials[i].maps[MATERIAL_MAP_EMISSION].texture);
            original_occlusion.push_back(model.materials[i].maps[MATERIAL_MAP_OCCLUSION].texture);
            model.materials[i].maps[MATERIAL_MAP_OCCLUSION].texture = agx_texture;
            model.materials[i].shader = principled_shader;
            model.materials[i].maps[MATERIAL_MAP_EMISSION].texture = environment_texture;
        }
        const int has_geometry = position_buffer.id && shadow_buffer.id;
        SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "hasGeometry"), &has_geometry, SHADER_UNIFORM_INT);
        const float viewport_size[] = {(float)rlGetFramebufferWidth(), (float)rlGetFramebufferHeight()};
        SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "viewportSize"), viewport_size, SHADER_UNIFORM_VEC2);
        SetShaderValueMatrix(principled_shader, GetShaderLocation(principled_shader, "lightVP"), shadow_view_projection);
        const int has_agx = agx_texture.id ? 1 : 0;
        SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "hasAgx"), &has_agx, SHADER_UNIFORM_INT);
        const int has_environment = environment_texture.id ? 1 : 0;
        SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "hasEnvironment"), &has_environment, SHADER_UNIFORM_INT);
        const Vector3 view = ImGui3D::camera.position;
        SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "cameraPosition"), &view, SHADER_UNIFORM_VEC3);
        SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "baseColor"), principled->base, SHADER_UNIFORM_VEC4);
        SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "metallic"), &principled->metallic, SHADER_UNIFORM_FLOAT);
        SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "roughness"), &principled->roughness, SHADER_UNIFORM_FLOAT);
        SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "ior"), &principled->ior, SHADER_UNIFORM_FLOAT);
        SetShaderValue(principled_shader, GetShaderLocation(principled_shader, "specularIorLevel"), &principled->specular_ior_level, SHADER_UNIFORM_FLOAT);
        DrawModelEx(model, position, axis, angle, scale, WHITE);
        for (int i = 0; i < model.materialCount; ++i) {
            model.materials[i].shader = original[i];
            model.materials[i].maps[MATERIAL_MAP_NORMAL].texture = original_position[i];
            model.materials[i].maps[MATERIAL_MAP_ROUGHNESS].texture = original_shadow[i];
            model.materials[i].maps[MATERIAL_MAP_EMISSION].texture = original_environment[i];
            model.materials[i].maps[MATERIAL_MAP_OCCLUSION].texture = original_occlusion[i];
        }
        return;
    }
    // DrawModelEx uses each material's shader. Restore it afterwards so the
    // model retains ownership of its original material resources.
    std::vector<Shader> original;
    original.reserve(model.materialCount);
    for (int i = 0; i < model.materialCount; ++i) {
        original.push_back(model.materials[i].shader);
        model.materials[i].shader = outline_shader;
    }
    // 먼저 뒤쪽 면을 조금 확장해 실루엣을 그리고 그 위에 본체를 그린다.
    rlDrawRenderBatchActive();
    rlEnableBackfaceCulling();
    rlSetCullFace(RL_CULL_FACE_FRONT);
    DrawModelEx(model, position, axis, angle, scale, color);
    rlSetCullFace(RL_CULL_FACE_BACK);
    for (int i = 0; i < model.materialCount; ++i) model.materials[i].shader = toon_shader;
    DrawModelEx(model, position, axis, angle, scale, color);
    for (int i = 0; i < model.materialCount; ++i) model.materials[i].shader = original[i];
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

void UnloadMeshes(std::unordered_map<std::string, Model>& models)
{
    for (auto& entry : models) UnloadModel(entry.second);
    models.clear();
}

fs::path MeshPath(const std::string& filename, const fs::path& directory)
{
    if (filename.rfind("package://", 0) == 0) {
        const std::string relative = filename.substr(10);
        const auto slash = relative.find('/');
        if (slash == std::string::npos) return {};
        const char* env = std::getenv("AMENT_PREFIX_PATH");
        const std::string prefixes = env ? env : "";
        size_t start = 0;
        while (start <= prefixes.size()) {
            const auto end = prefixes.find(':', start);
            const std::string prefix = prefixes.substr(start, end == std::string::npos ? end : end - start);
            if (!prefix.empty()) {
                fs::path result = fs::path(prefix) / "share" / relative;
                if (fs::is_regular_file(result)) return fs::absolute(result).lexically_normal();
            }
            if (end == std::string::npos) break;
            start = end + 1;
        }
        return {};
    }
    if (filename.rfind("file://", 0) == 0) return fs::path(filename.substr(7)).lexically_normal();
    if (filename.find("://") != std::string::npos) return {};
    return fs::absolute(directory / filename).lexically_normal();
}

void ApplyPose(const urdf::Pose& pose)
{
    rlTranslatef((float)pose.position.x, (float)pose.position.y, (float)pose.position.z);
    const Quaternion q = {(float)pose.rotation.x, (float)pose.rotation.y, (float)pose.rotation.z, (float)pose.rotation.w};
    Vector3 axis; float angle;
    QuaternionToAxisAngle(q, &axis, &angle);
    if (std::isfinite(angle) && angle != 0.0f) rlRotatef(angle * RAD2DEG, axis.x, axis.y, axis.z);
}

Color VisualColor(const urdf::Visual& visual)
{
    if (!visual.material) return WHITE;
    const auto& c = visual.material->color;
    auto component = [](double value) { return (unsigned char)(std::clamp(value, 0.0, 1.0) * 255.0 + 0.5); };
    return {component(c.r), component(c.g), component(c.b), component(c.a)};
}

void CollectJointControls(const urdf::LinkConstSharedPtr& link)
{
    for (const auto& child : link->child_links) {
        if (!child || !child->parent_joint) continue;
        const auto& joint = child->parent_joint;
        if (joint->type == urdf::Joint::REVOLUTE || joint->type == urdf::Joint::CONTINUOUS ||
            joint->type == urdf::Joint::PRISMATIC) {
            float lower = joint->type == urdf::Joint::PRISMATIC ? -1.0f : -PI;
            float upper = joint->type == urdf::Joint::PRISMATIC ? 1.0f : PI;
            if (joint->type != urdf::Joint::CONTINUOUS && joint->limits) {
                lower = (float)joint->limits->lower;
                upper = (float)joint->limits->upper;
            }
            if (!std::isfinite(lower) || !std::isfinite(upper) || lower > upper) {
                lower = joint->type == urdf::Joint::PRISMATIC ? -1.0f : -PI;
                upper = joint->type == urdf::Joint::PRISMATIC ? 1.0f : PI;
            }
            joint_indices[joint->name] = joint_controls.size();
            joint_controls.push_back({joint, std::clamp(0.0f, lower, upper), lower, upper});
        }
        CollectJointControls(child);
    }
}

void ApplyJointMotion(const urdf::Joint& joint)
{
    const auto it = joint_indices.find(joint.name);
    if (it == joint_indices.end()) return;
    const float value = joint_controls[it->second].value;
    Vector3 axis = {(float)joint.axis.x, (float)joint.axis.y, (float)joint.axis.z};
    if (Vector3LengthSqr(axis) < 1e-12f) return;
    axis = Vector3Normalize(axis);
    if (joint.type == urdf::Joint::PRISMATIC)
        rlTranslatef(axis.x * value, axis.y * value, axis.z * value);
    else
        rlRotatef(value * RAD2DEG, axis.x, axis.y, axis.z);
}

void DrawLink(const urdf::LinkConstSharedPtr& link, const fs::path& directory)
{
    for (const auto& visual : link->visual_array) {
        if (!visual || !visual->geometry) continue;
        rlPushMatrix();
        ApplyPose(visual->origin);
        const Color color = VisualColor(*visual);
        const auto material_it = principled_materials.find(visual->material_name);
        const PrincipledMaterial* principled = material_it != principled_materials.end() ? &material_it->second : nullptr;
        switch (visual->geometry->type) {
        case urdf::Geometry::BOX: {
            const auto& box = static_cast<const urdf::Box&>(*visual->geometry);
            DrawToonModel(box_model, {0, 0, 0}, {0, 1, 0}, 0,
                          {(float)box.dim.x, (float)box.dim.y, (float)box.dim.z}, color, principled);
            break;
        }
        case urdf::Geometry::SPHERE: {
            const auto& sphere = static_cast<const urdf::Sphere&>(*visual->geometry);
            DrawToonModel(sphere_model, {0, 0, 0}, {0, 1, 0}, 0, Vector3Scale({1, 1, 1}, (float)sphere.radius), color, principled);
            break;
        }
        case urdf::Geometry::CYLINDER: {
            const auto& cylinder = static_cast<const urdf::Cylinder&>(*visual->geometry);
            DrawToonModel(cylinder_model, {0, 0, (float)-cylinder.length * 0.5f}, {1, 0, 0}, 90,
                          {(float)cylinder.radius, (float)cylinder.length, (float)cylinder.radius}, color, principled);
            break;
        }
        case urdf::Geometry::MESH: {
            const auto& mesh = static_cast<const urdf::Mesh&>(*visual->geometry);
            auto it = meshes.find(MeshPath(mesh.filename, directory).string());
            if (it != meshes.end()) DrawToonModel(it->second, {0, 0, 0}, {0, 1, 0}, 0,
                {(float)mesh.scale.x, (float)mesh.scale.y, (float)mesh.scale.z}, color, principled);
            break;
        }
        }
        rlPopMatrix();
    }
    for (const auto& child : link->child_links) {
        if (!child || !child->parent_joint) continue;
        rlPushMatrix();
        ApplyPose(child->parent_joint->parent_to_joint_origin_transform);
        ApplyJointMotion(*child->parent_joint);
        DrawLink(child, directory);
        rlPopMatrix();
    }
}
fs::path robot_directory;
}

namespace Urdf {
void ClearUrdf();
bool LoadUrdf(const std::string& path, std::string& error)
{
    error.clear();
    try {
        const fs::path file = fs::absolute(path).lexically_normal();
        if (!fs::is_regular_file(file)) { error = "URDF 파일을 찾을 수 없습니다: " + file.string(); return false; }
        auto parsed = urdf::parseURDFFile(file.string());
        if (!parsed || !parsed->getRoot()) { error = "URDF를 해석할 수 없습니다: " + file.string(); return false; }
        std::vector<urdf::LinkSharedPtr> links;
        parsed->getLinks(links);
        std::unordered_map<std::string, Model> loaded;
        for (const auto& link : links) for (const auto& visual : link->visual_array) {
            if (!visual || !visual->geometry || visual->geometry->type != urdf::Geometry::MESH) continue;
            const auto& mesh = static_cast<const urdf::Mesh&>(*visual->geometry);
            const fs::path mesh_file = MeshPath(mesh.filename, file.parent_path());
            if (mesh_file.empty() || !fs::is_regular_file(mesh_file)) {
                error = "메시 파일을 찾을 수 없습니다: " + mesh.filename;
                UnloadMeshes(loaded); return false;
            }
            const std::string extension = mesh_file.extension().string();
            if (extension != ".obj" && extension != ".gltf" && extension != ".glb") {
                error = "지원하지 않는 메시 형식입니다 (OBJ, glTF, GLB 지원): " + mesh_file.string();
                UnloadMeshes(loaded); return false;
            }
            const std::string key = mesh_file.string();
            if (!loaded.count(key)) {
                Model model = LoadModel(key.c_str());
                if (!IsModelValid(model)) { UnloadModel(model); UnloadMeshes(loaded); error = "메시 로드 실패: " + key; return false; }
                loaded.emplace(key, model);
            }
        }
        ClearUrdf();
        robot = std::move(parsed);
        robot_directory = file.parent_path();
        meshes = std::move(loaded);
        // URDF's standard material only stores color; optional sidecar stores Blender BSDF values.
        fs::path material_file = file;
        material_file.replace_extension(".principled");
        std::ifstream material_stream(material_file);
        std::string line;
        while (std::getline(material_stream, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream input(line);
            std::string name;
            input >> name;
            if (name == "environment" || name == "agx") {
                std::string relative_path;
                if (input >> relative_path) {
                    Texture2D& texture = name == "environment" ? environment_texture : agx_texture;
                    if (texture.id) UnloadTexture(texture);
                    texture = {};
                    LoadFloatTexture(file.parent_path() / relative_path, texture, name == "environment");
                }
                continue;
            }
            input.clear();
            input.str(line);
            PrincipledMaterial material;
            if (input >> name >> material.base[0] >> material.base[1] >> material.base[2] >> material.base[3]
                      >> material.metallic >> material.roughness >> material.ior >> material.specular_ior_level)
                principled_materials[name] = material;
        }
        CollectJointControls(robot->getRoot());
        return true;
    } catch (const std::exception& ex) { error = std::string("URDF 로드 실패: ") + ex.what(); return false; }
}
void ClearUrdf() {
    UnloadMeshes(meshes);
    joint_controls.clear();
    joint_indices.clear();
    principled_materials.clear();
    if (environment_texture.id) UnloadTexture(environment_texture);
    if (agx_texture.id) UnloadTexture(agx_texture);
    environment_texture = {}; agx_texture = {};
    robot.reset();
    robot_directory.clear();
}
void ShowJointControls()
{
    if (!robot || joint_controls.empty()) return;
    ImGui::SetNextWindowSize(ImVec2(410, 360), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("관절 조작")) {
        if (ImGui::Button("초기 위치"))
            for (auto& control : joint_controls)
                control.value = std::clamp(0.0f, control.lower, control.upper);
        ImGui::Separator();
        for (auto& control : joint_controls) {
            ImGui::PushID(control.joint->name.c_str());
            ImGui::TextUnformatted(control.joint->name.c_str());
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (control.joint->type == urdf::Joint::PRISMATIC)
                ImGui::SliderFloat("##value", &control.value, control.lower, control.upper, "%.3f m");
            else
                ImGui::SliderAngle("##value", &control.value, control.lower * RAD2DEG, control.upper * RAD2DEG, "%.1f deg");
            ImGui::PopID();
        }
    }
    ImGui::End();
}
void DestroyUrdfRenderer() {
    ClearUrdf();
    if (box_model.meshCount) UnloadModel(box_model);
    if (sphere_model.meshCount) UnloadModel(sphere_model);
    if (cylinder_model.meshCount) UnloadModel(cylinder_model);
    box_model = {}; sphere_model = {}; cylinder_model = {};
    if (toon_shader.id) UnloadShader(toon_shader);
    if (outline_shader.id) UnloadShader(outline_shader);
    if (principled_shader.id) UnloadShader(principled_shader);
    if (geometry_shader.id) UnloadShader(geometry_shader);
    if (position_buffer.id) UnloadRenderTexture(position_buffer);
    if (shadow_buffer.id) UnloadRenderTexture(shadow_buffer);
    geometry_shader = {}; position_buffer = {}; shadow_buffer = {};
    toon_shader = {}; outline_shader = {}; principled_shader = {};
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

void DrawUrdf()
{
    if (!robot || !robot->getRoot()) return;
    InitToonRendering();
    if (use_principled && !principled_materials.empty() && geometry_shader.id) {
        rlDrawRenderBatchActive();
        const unsigned int old_framebuffer = rlGetActiveFramebuffer();
        const int width = rlGetFramebufferWidth(), height = rlGetFramebufferHeight();
        const Matrix view = rlGetMatrixModelview(), projection = rlGetMatrixProjection();
        if (!position_buffer.id) position_buffer = LoadGeometryBuffer(640, 360);
        if (!shadow_buffer.id) shadow_buffer = LoadGeometryBuffer(1024, 1024);
        if (position_buffer.id && shadow_buffer.id) {
            const Vector3 light_dir = Vector3Normalize({0.75f, 0.4f, -0.5f});
            const Vector3 light_target = {0.0f, 0.45f, 0.0f};
            const Matrix light_view = MatrixLookAt(Vector3Add(light_target, Vector3Scale(light_dir, 4.0f)), light_target, {0, 1, 0});
            const Matrix light_projection = MatrixOrtho(-2, 2, -2, 2, 0.1, 10.0);
            shadow_view_projection = MatrixMultiply(light_view, light_projection);
            for (int pass = 1; pass <= 2; ++pass) {
                const auto& target = pass == 1 ? position_buffer : shadow_buffer;
                rlEnableFramebuffer(target.id);
                rlViewport(0, 0, target.texture.width, target.texture.height);
                rlSetMatrixModelview(pass == 1 ? view : light_view);
                rlSetMatrixProjection(pass == 1 ? projection : light_projection);
                ClearBackground(BLANK);
                geometry_pass = pass;
                rlPushMatrix();
                rlRotatef(-90.0f, 1, 0, 0);
                DrawLink(robot->getRoot(), robot_directory);
                rlPopMatrix();
                rlDrawRenderBatchActive();
            }
            geometry_pass = 0;

        }
        rlEnableFramebuffer(old_framebuffer);
        rlViewport(0, 0, width, height);
        rlSetMatrixModelview(view);
        rlSetMatrixProjection(projection);
    }
    rlPushMatrix();
    rlRotatef(-90.0f, 1, 0, 0); // URDF Z-up -> viewport Y-up
    DrawLink(robot->getRoot(), robot_directory);
    rlPopMatrix();
}
}


int main(int argc, char **argv) {
    // ImGui 초기화
    ImGui::Init("테스트 프로그램", 1280, 720);
    ImGui::LoadConfig("../imgui.ini");

    ImGui::AddTab(ICON_MD_HOME "  메인 ");
    ImGui::AddTab(ICON_MD_BUILD " 테스트 ");

    char urdf_path[2048];
    std::snprintf(urdf_path, sizeof(urdf_path), "%s/data/urdf/uon_7dof.urdf", ROOT_PATH);
    std::string urdf_message;
    const bool sample_loaded = Urdf::LoadUrdf(urdf_path, urdf_message);
    if (sample_loaded) {
        urdf_message = "UON 7-DOF 로봇을 불러왔습니다.";
        ImGui3D::camera.position = {1.5f, 1.2f, 2.0f};
        ImGui3D::camera.target = {0.0f, 0.45f, 0.0f};
    }
    ImGui::show_3d_viewport = true;


    while (!ImGui::ShouldClose()) {
        ImGui::Context([&]()
        {
            ImGui::SetNextWindowSize(ImVec2(520, 180), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(360, 175), ImVec2(FLT_MAX, FLT_MAX));
            ImGui::Begin("URDF Loader");
            ImGui::InputText("URDF 경로", urdf_path, sizeof(urdf_path));
            if (ImGui::Button("URDF 로드")) {
                if (Urdf::LoadUrdf(urdf_path, urdf_message)) {
                    urdf_message = "URDF를 불러왔습니다.";
                    ImGui::show_3d_viewport = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("모델 지우기")) {
                Urdf::ClearUrdf();
                urdf_message = "모델을 지웠습니다.";
            }
            ImGui::TextWrapped("%s", urdf_message.c_str());
            if (!principled_materials.empty())
                ImGui::Checkbox("Blender 재질", &use_principled);
            ImGui::End();
            Urdf::ShowJointControls();

            ImGui::Begin(" " ICON_MD_WIDGETS " Wedgets ");
            const float availableHeight = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing();
            const float widgetsHeight = availableHeight > 1.0f ? availableHeight : 1.0f;
            if (ImGui::BeginCollapsingHeader(ICON_MD_WIDGETS " Widgets Example ")) {
                ImGui::BeginChild("Widgets_Child", ImVec2(0, widgetsHeight), true);

                // ==========================================
                // 1. 기본 컨트롤 (Basic Controls)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_CHECK_BOX " Basic Controls");
                ImGui::Separator();

                static bool is_c = false;
                if (ImGui::Check("체크박스", &is_c)) {}

                static int radio_idx = 0;
                if (ImGui::Radio("라디오 1", &radio_idx, 0)) {}
                if (ImGui::Radio("라디오 2", &radio_idx, 1)) {}

                static bool is_t = false;
                if (ImGui::Toggle("토글", &is_t)) {}

                float btn_width = ImGui::GetContentRegionAvail().x / 4.15f;
                ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(100, 141, 222, 255)); // 기본
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(120, 161, 242, 255)); // 호버 (밝게)
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32( 80, 121, 202, 255)); // 클릭 (어둡게)
                if (ImGui::Button("알림\n정보", ImVec2(btn_width, 0))) {
                    ImGui::NotifyInfo("정보 메세지 표시");
                }
                ImGui::PopStyleColor(3);
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32( 46, 160,  67, 255)); // 기본
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32( 66, 180,  87, 255)); // 호버 (밝게)
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32( 26, 140,  47, 255)); // 클릭 (어둡게)
                if (ImGui::Button("알림\n성공", ImVec2(btn_width, 0))) {
                    ImGui::NotifySucc("성공 메세지 표시");
                }
                ImGui::PopStyleColor(3);
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(227, 179,  65, 255)); // 기본
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(247, 199,  85, 255)); // 호버 (밝게)
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(207, 159,  45, 255)); // 클릭 (어둡게)
                if (ImGui::Button("알림\n경고", ImVec2(btn_width, 0))) {
                    ImGui::NotifyWarn("경고 메세지 표시");
                }
                ImGui::PopStyleColor(3);
                ImGui::SameLine();

                ImGui::PushStyleColor(ImGuiCol_Button,        IM_COL32(224, 108, 117, 255)); // 기본
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(244, 128, 137, 255)); // 호버 (밝게)
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  IM_COL32(204,  88,  97, 255)); // 클릭 (어둡게)
                if (ImGui::Button("알림\n에러", ImVec2(btn_width, 0))) {
                    ImGui::NotifyError("에러 메세지 표시");
                }
                ImGui::PopStyleColor(3);
                ImGui::Dummy(ImVec2(0, 20.0f));


                // ==========================================
                // 2. 텍스트 입력 (Text Inputs)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_EDIT " Text Inputs");
                ImGui::Separator();

                static char text_buf[128] = "Hello, Custom ImGui!";
                ImGui::InputTextLine("텍스트 입력", text_buf, IM_COUNTOF(text_buf));

                static char pw_buf[64] = "";
                ImGui::InputTextLine("비밀번호", pw_buf, IM_COUNTOF(pw_buf), ImGuiInputTextFlags_Password);

                static char multiline_buf[256] = "여기에\n여러 줄의\n텍스트를 입력하세요.";
                ImGui::InputTextLines("메모", multiline_buf, IM_COUNTOF(multiline_buf), ImVec2(0, 80));
                ImGui::Dummy(ImVec2(0, 20.0f));


                // ==========================================
                // 3. 슬라이더 및 드래그 (Sliders & Drags)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_TUNE " Sliders & Drags");
                ImGui::Separator();

                static double drag_value = 50.0f;
                ImGui::Drag("Drag", &drag_value, 1.0f, 0.0f, 100.0f, "%.1f");

                static float slider_value = 30.0f;
                ImGui::Slider("Slider2", &slider_value, 0.0f, 100.0f);

                static float range_min = 20.0f, range_max = 80.0f;
                ImGui::SliderRange("Slider Range2", &range_min, &range_max, 0.0f, 100.0f);
                ImGui::Dummy(ImVec2(0, 20.0f));


                // ==========================================
                // Input Number
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_NUMBERS " Input Number");
                ImGui::Separator();

                static int input_scalar_value = 50;
                ImGui::InputNum("Input Int", &input_scalar_value);

                static double input_double_value = 50.0;
                ImGui::InputNum("Input Double", &input_double_value, 0.01, 0, "%0.2f");
                ImGui::Dummy(ImVec2(0, 20.0f));


                // ==========================================
                // 4. 리스트 및 드롭다운 (Lists & Dropdowns)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_LIST " Lists & Combos");
                ImGui::Separator();

                static int current_theme = 1;
                ImGui::DropDown("DropDown", &current_theme, "선택 1\0선택 2\0선택 3\0", 3);

                static int listbox_item_current = 1;
                const char* listbox_items[] = { "Apple", "Banana", "Cherry", "Kiwi", "Mango", "Orange" };
                ImGui::List("과일 목록", &listbox_item_current, listbox_items, IM_COUNTOF(listbox_items), 4);
                ImGui::Dummy(ImVec2(0, 20.0f));


                // ==========================================
                // 5. 색상 선택기 (Color Pickers)
                // ==========================================
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_COLOR_LENS " Color Pickers");
                ImGui::Separator();

                static float color_rgb[3] = { 0.36f, 0.41f, 0.94f };
                ImGui::ColorEdit3X("컬러3", color_rgb);

                static float color_rgba[4] = { 0.8f, 0.2f, 0.3f, 0.5f };
                ImGui::ColorEdit4X("컬러4", color_rgba, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
                ImGui::Dummy(ImVec2(0, 20.0f));

                // 탭 전환과 각 탭의 독립적인 상태
                ImGui::TextColored(ImVec4(0.36f, 0.41f, 0.94f, 1.0f), ICON_MD_TAB " Tabs");
                ImGui::Separator();
                static int selectedTab = 0;
                const char* tabs[] = { "일반", "설정", "정보" };
                ImGui::BubbleTabs("WidgetExampleTabs", tabs, IM_ARRAYSIZE(tabs), &selectedTab);
                ImGui::Spacing();
                if (selectedTab == 0) {
                    static bool enabled = true;
                    ImGui::Checkbox("활성화", &enabled);
                    ImGui::TextUnformatted("일반 설정을 표시합니다.");
                } else if (selectedTab == 1) {
                    static float value = 0.5f;
                    ImGui::SliderFloat("값", &value, 0.0f, 1.0f);
                } else {
                    ImGui::TextUnformatted("버블을 눌러 탭을 전환합니다.");
                }

                ImGui::EndChild();
                ImGui::EndCollapsingHeader();
            }
            ImGui::End();
        }, [] { Urdf::DrawUrdf(); });
    }



    Urdf::DestroyUrdfRenderer();
    ImGui::Destroy();
    return 0;
}



// namespace ImGui {
// enum class HudInputType { Key, MouseLeft, MouseRight, MouseLeftHold, MouseRightHold };
// struct HudHint { const char* key; const char* description; HudInputType type; };
//
// void ControlsHud(const ImVec2& origin, const ImVec2& size, const HudHint* hints, int count)
// {
//     if (!hints || count <= 0 || size.x <= 100.0f || size.y <= 50.0f) return;
//     ImDrawList* draw = ImGui::GetWindowDrawList();
//     const float padding = 12.0f, line = ImGui::GetTextLineHeight();
//     const float row_height = line + 10.0f, header_height = 0.0f;
//     float key_width = 0.0f, description_width = 0.0f;
//     for (int i = 0; i < count; ++i) {
//         const auto& hint = hints[i];
//         key_width = std::max(key_width, ImGui::CalcTextSize(hint.key).x);
//         if (hint.type == HudInputType::MouseLeftHold || hint.type == HudInputType::MouseRightHold)
//             key_width = std::max(key_width, 18.0f + 2.0f * (2.0f + ImGui::CalcTextSize("꾸욲~").x * 0.7f));
//         description_width = std::max(description_width, ImGui::CalcTextSize(hint.description).x);
//     }
//     const float badge_width = key_width + 16.0f;
//     const float width = std::min(size.x - 16.0f, std::max(185.0f, padding * 3 + badge_width + description_width));
//     const float height = std::min(size.y - 16.0f, header_height + padding + row_height * count);
//     const ImVec2 pos(origin.x + 8.0f, origin.y + size.y - height - 8.0f);
//     const ImVec2 end(pos.x + width, pos.y + height);
//     draw->PushClipRect(pos, end, true);
//     draw->AddRectFilled(pos, end, ImGui::GetColorU32(ImGuiCol_ChildBg, 0.88f), 8.0f);
//     draw->AddRect(pos, end, ImGui::GetColorU32(ImGuiCol_Border), 8.0f);
//     for (int i = 0; i < count; ++i) {
//         const float y = pos.y + header_height + 6.0f + i * row_height;
//         const ImVec2 badge_min(pos.x + padding, y);
//         const ImVec2 badge_max(badge_min.x + badge_width, y + line + 4.0f);
//         if (hints[i].type != HudInputType::Key) {
//             const bool hold = hints[i].type == HudInputType::MouseLeftHold || hints[i].type == HudInputType::MouseRightHold;
//             const float mouse_width = 18.0f;
//             const float mx = badge_min.x + (badge_width - mouse_width) * 0.5f;
//             const float my = y, mouse_height = line + 4.0f;
//             draw->AddRectFilled(ImVec2(mx, my), ImVec2(mx + mouse_width, my + mouse_height), ImGui::GetColorU32(ImGuiCol_FrameBg), 7.0f);
//             const bool left = hints[i].type == HudInputType::MouseLeft || hints[i].type == HudInputType::MouseLeftHold;
//             draw->AddRectFilled(ImVec2(mx + (left ? 1.0f : mouse_width * 0.5f), my + 1.0f),
//                 ImVec2(mx + (left ? mouse_width * 0.5f : mouse_width - 1.0f), my + mouse_height * 0.48f),
//                 ImGui::GetColorU32(ImGuiCol_Button), 6.0f, left ? ImDrawFlags_RoundCornersTopLeft : ImDrawFlags_RoundCornersTopRight);
//             draw->AddRect(ImVec2(mx, my), ImVec2(mx + mouse_width, my + mouse_height), ImGui::GetColorU32(ImGuiCol_TextDisabled), 7.0f);
//             draw->AddLine(ImVec2(mx, my + mouse_height * 0.48f), ImVec2(mx + mouse_width, my + mouse_height * 0.48f), ImGui::GetColorU32(ImGuiCol_TextDisabled));
//             draw->AddLine(ImVec2(mx + mouse_width * 0.5f, my + 1.0f), ImVec2(mx + mouse_width * 0.5f, my + mouse_height * 0.48f), ImGui::GetColorU32(ImGuiCol_TextDisabled));
//             if (hold) {
//                 const float font_size = ImGui::GetFontSize() * 0.7f;
//                 draw->AddText(ImGui::GetFont(), font_size, ImVec2(mx + mouse_width + 2.0f, my), IM_COL32(255, 215, 80, 255), "꾸욲~");
//             }
//         } else {
//             const float rounding = 5.0f;
//             const float text_width = ImGui::CalcTextSize(hints[i].key).x;
//             const float cap_width = std::max(line + 4.0f, text_width + 16.0f);
//             const float cap_x = badge_min.x + (badge_width - cap_width) * 0.5f;
//             const ImVec2 cap_min(cap_x, badge_min.y);
//             const ImVec2 cap_max(cap_x + cap_width, badge_max.y);
//             const ImVec2 face_max(cap_max.x, cap_max.y - 3.0f);
//             draw->AddRectFilled(cap_min, cap_max, ImGui::GetColorU32(ImGuiCol_Border), rounding);
//             draw->AddRectFilled(cap_min, face_max, ImGui::GetColorU32(ImGuiCol_FrameBg), rounding);
//             draw->AddRect(cap_min, face_max, ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.6f), rounding);
//             draw->AddText(ImVec2(cap_x + (cap_width - text_width) * 0.5f, y + 0.5f), ImGui::GetColorU32(ImGuiCol_Text), hints[i].key);
//         }
//         draw->AddText(ImVec2(badge_max.x + padding, y + 2.0f), ImGui::GetColorU32(ImGuiCol_Text), hints[i].description);
//     }
//     draw->PopClipRect();
// }
// }
