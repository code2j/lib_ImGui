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
Shader toon_shader{}, outline_shader{};
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

void DrawToonModel(Model& model, Vector3 position, Vector3 axis, float angle, Vector3 scale, Color color)
{
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
        switch (visual->geometry->type) {
        case urdf::Geometry::BOX: {
            const auto& box = static_cast<const urdf::Box&>(*visual->geometry);
            DrawToonModel(box_model, {0, 0, 0}, {0, 1, 0}, 0,
                          {(float)box.dim.x, (float)box.dim.y, (float)box.dim.z}, color);
            break;
        }
        case urdf::Geometry::SPHERE: {
            const auto& sphere = static_cast<const urdf::Sphere&>(*visual->geometry);
            DrawToonModel(sphere_model, {0, 0, 0}, {0, 1, 0}, 0, Vector3Scale({1, 1, 1}, (float)sphere.radius), color);
            break;
        }
        case urdf::Geometry::CYLINDER: {
            const auto& cylinder = static_cast<const urdf::Cylinder&>(*visual->geometry);
            DrawToonModel(cylinder_model, {0, 0, (float)-cylinder.length * 0.5f}, {1, 0, 0}, 90,
                          {(float)cylinder.radius, (float)cylinder.length, (float)cylinder.radius}, color);
            break;
        }
        case urdf::Geometry::MESH: {
            const auto& mesh = static_cast<const urdf::Mesh&>(*visual->geometry);
            auto it = meshes.find(MeshPath(mesh.filename, directory).string());
            if (it != meshes.end()) DrawToonModel(it->second, {0, 0, 0}, {0, 1, 0}, 0,
                {(float)mesh.scale.x, (float)mesh.scale.y, (float)mesh.scale.z}, color);
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
        CollectJointControls(robot->getRoot());
        return true;
    } catch (const std::exception& ex) { error = std::string("URDF 로드 실패: ") + ex.what(); return false; }
}
void ClearUrdf() {
    UnloadMeshes(meshes);
    joint_controls.clear();
    joint_indices.clear();
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
    toon_shader = {}; outline_shader = {};
}
void DrawUrdf()
{
    if (!robot || !robot->getRoot()) return;
    InitToonRendering();
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
            ImGui::SetNextWindowSize(ImVec2(520, 145), ImGuiCond_FirstUseEver);
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