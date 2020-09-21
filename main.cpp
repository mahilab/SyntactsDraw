#include <syntacts>
#include <Mahi/Gui.hpp>

using namespace tact;
using namespace mahi::gui;
using namespace mahi::util;

constexpr int WIDTH  = 300;
constexpr int HEIGHT = 900;

using std::vector;

class SyntactsDraw : public Application {
public:
    SyntactsDraw() : Application(WIDTH,HEIGHT,"SyntactsDraw",false) { 
        ImGui::StyleColorsMahiDark3();
        ImGui::DisableViewports();
        // initialize our device
        // m_session.open("MOTU 24Ao", API::ASIO);
    }

    void update() override {
        ImGui::BeginFixed("SyntactsDraw", {0,0}, {WIDTH,HEIGHT}, ImGuiWindowFlags_NoTitleBar);
        if (ImGui::Button("Play", ImVec2(-1,0))) {

        }
        if (ImGui::Button("Clear", ImVec2(-1,0))) {
            m_paths.clear();
        }
        ImGui::ToggleButton("Realtime Mode",&m_mouseMode,ImVec2(-1,0));


        if (showSyntactsArray()) {
            if (ImGui::IsMouseClicked(0)) {
                m_paths.emplace_back();
            }
            if (ImGui::IsMouseDown(0)) {
                ImVec2 pos = ImGui::GetMousePos();
                if (m_paths.back().empty())
                    m_paths.back().push_back(pos);  
                else if (pos != m_paths.back().back())                    
                    m_paths.back().push_back(pos);  
            }
        }    

        drawPath(Oranges::Orange, 5);

        ImGui::End();
    }

    

    bool showSyntactsArray() {
        Vec2 pad = ImGui::GetStyle().WindowPadding;
        ImDrawList& dl = *ImGui::GetWindowDrawList();
        Vec2 cp = ImGui::GetCursorPos();
        float w = WIDTH - pad.x * 2;
        float h = w * 8.0f / 3.0f;
        dl.AddRectFilled(cp, cp + Vec2(w,h),IM_COL32(255,255,255,10), 5);
        float spacing = w / 3;
        float radius  = spacing * 0.5f / 2;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 3; ++c) {
                float x = cp.x + spacing / 2 + c * spacing;
                float y = cp.y + spacing / 2 + r * spacing;
                dl.AddCircleFilled(Vec2(x,y),radius,IM_COL32(255,255,255,128),32);
            }
        }
        Rect rect(cp, cp+Vec2(w,h));
        return rect.contains(ImGui::GetMousePos());
    }

    void drawPath(Color col, float thickness) {
        ImDrawList& dl = *ImGui::GetWindowDrawList();
        auto col32 = ImGui::ColorConvertFloat4ToU32(col);
        for (auto& path : m_paths) {
            if (path.size() > 0) {
                dl.AddCircleFilled(path[0], 2*thickness, col32,32);
                dl.AddPolyline(&path[0],(int)path.size(),col32,false,thickness);
            }
        }
    }

    vector<vector<ImVec2>> m_paths;     
    bool m_mouseMode = false;
    // Session m_session;
};

int main(int argc, char const *argv[])
{
    SyntactsDraw app;
    app.run();
    return 0;
}
