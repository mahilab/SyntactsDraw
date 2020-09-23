#define MAHI_GUI_NO_CONSOLE

#include <syntacts>
#include <Mahi/Gui.hpp>
#include <Mahi/Util.hpp>

using namespace tact;
using namespace mahi::gui;
using namespace mahi::util;

constexpr int WIDTH  = 300; // window width
constexpr int HEIGHT = 865; // window height

constexpr int COLS   = 3;   // array columns
constexpr int ROWS   = 8;   // array rows

/// Syntacts Array Drawing Application
class SyntactsDraw : public Application {
public:

    /// Constructor
    SyntactsDraw() : Application(WIDTH,HEIGHT,"SyntactsDraw",false) {
        // set ImGui theme and disable viewports (multi-window)
        ImGui::StyleColorsMahiDark3();
        ImGui::DisableViewports();
    }

private:

    /// Called once per frame
    void update() override {
        // setup ImGui window
        ImGui::SetNextWindowPos({0,0}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({WIDTH,HEIGHT}, ImGuiCond_Always);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
        ImGui::Begin("Syntacts Draw", nullptr, flags);
        // if Session not open, offer initialization button
        if (!m_session.isOpen()) {
            if (ImGui::Button("Initialize Syntacts", ImVec2(-1,-1)))
                initialize(); 
        }
        else {
            // controls to play and clear path
            ImGui::BeginDisabled(m_followMode);
            if (ImGui::Button("Play Paths", ImVec2(-1,0)) && !m_playing)
                start_coroutine(playPaths());
            if (ImGui::Button("Clear Paths", ImVec2(-1,0)))
                m_paths.clear();
            ImGui::EndDisabled();
            // spatializer radius control via slide and mouse scroll
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat("##Radius",&m_radius, 0.1f, 2.0f, "Radius = %.1f"))
                m_spat.setRadius(m_radius);
            float scroll = ImGui::GetIO().MouseWheel;
            if (scroll != 0) {
                m_radius += scroll * 0.1f;
                m_radius = clamp(m_radius, 0.1f, 2.0f);
                m_spat.setRadius(m_radius);
            }
            // follow mode (i.e. target = mouse pos)
            if (ImGui::Checkbox("Follow Mouse",&m_followMode)) {
                if (m_followMode)
                    m_spat.play(m_sig);
                else
                    m_spat.stop();
            }
            // get current mouse pos
            Vec2 mouse = ImGui::GetMousePos();
            // draw array graphics
            drawArray();
            // playback mode, allow user to draw paths
            if (!m_followMode) {
                if (m_arrayRect.contains(mouse) && !m_playing) {
                    if (ImGui::IsMouseClicked(0))
                        m_paths.emplace_back();
                    if (ImGui::IsMouseDown(0))
                        m_paths.back().push_back(mouse);
                }
                drawPaths(Oranges::Orange, 5);
            }
            // follow mode, allow user to set target pos with mouse cursor
            else {
                m_target_px = mouse;
                auto target = pixelToSpatial(mouse);
                m_spat.setTarget(target.x, target.y);
            }
            // drag target circle
            if (m_followMode || m_playing)
                drawTarget();
        }
        // end ImGui window
        ImGui::End();
    }

    /// Draws the array background graphics
    void drawArray() {
        auto& dl     = *ImGui::GetWindowDrawList();
        Vec2 cp      = ImGui::GetCursorPos();
        Vec2 pad     = ImGui::GetStyle().WindowPadding;
        float w      = WIDTH - pad.x * 2;
        float h      = ROWS * w / COLS;
        float space  = w / COLS;
        float radius = space * 0.25f;
        m_arrayRect  = Rect(cp, Vec2(w,h));
        m_tactorRect = Rect(cp + Vec2(space,space)/2, Vec2(w,h) - Vec2(space,space));
        dl.AddRectFilled(m_arrayRect.tl(), m_arrayRect.br(), IM_COL32(255,255,255,10), 5);
        dl.AddRect(m_arrayRect.tl(), m_arrayRect.br(), IM_COL32(255,255,255,128));
        dl.AddRect(m_tactorRect.tl(), m_tactorRect.br(), IM_COL32(255,255,255, 32));
        for (int c = 0; c < COLS; ++c) {
            for (int r = 0; r < ROWS; ++r) {
                int ch      = c*ROWS + r;
                float x     = cp.x + space / 2 + c * space;
                float y     = cp.y + h - space / 2 - r * space;
                float level = (float)m_session.getLevel(ch);
                Color col   = Tween::Linear(Grays::Gray70, Pinks::HotPink, level);
                dl.AddCircleFilled(Vec2(x,y),radius,ImGui::ColorConvertFloat4ToU32(col),32);
            }
        };
    }

    /// Draws user's paths
    void drawPaths(Color col, float thickness) {
        auto& dl = *ImGui::GetWindowDrawList();
        auto col32 = ImGui::ColorConvertFloat4ToU32(col);
        for (auto& path : m_paths) {
            if (path.size() > 0) {
                dl.AddCircleFilled(path[0], 2*thickness, col32,32);
                dl.AddPolyline(&path[0],(int)path.size(),col32,false,thickness);
            }
        }
    }

    /// Draws the current target position and size
    void drawTarget() {
        auto& dl = *ImGui::GetWindowDrawList();
        float rad = m_radius * m_tactorRect.size().x / 2;
        dl.PushClipRect(m_arrayRect.tl(), m_arrayRect.br());
        dl.AddCircleFilled(m_target_px, rad, IM_COL32(255,0,0,64), 32);
        dl.PopClipRect();
    }

    /// Initializes Syntacts Session and Spatializer
    void initialize() {
        if (m_session.open("MOTU Pro Audio", API::ASIO) != SyntactsError_NoError) {
            LOG(Fatal) << "Failed to open MOTU 24Ao! Ensure that the device is connected and that drivers are installed.";        
            throw std::runtime_error("Failed to open MOTU 24Ao!");
        }
        m_spat.bind(&m_session);
        m_spat.setTarget(0,0);
        m_spat.setRadius(m_radius);
        for (int c = 0; c < COLS; c++) {
            for (int r = 0; r < ROWS; ++r) {
                int ch = c*ROWS + r;
                m_spat.setPosition(ch,(double)c,(double)r);
            }
        }
        m_sig = Sine(175);
    }

    /// Converts a pixel position to Spatializer position
    Vec2 pixelToSpatial(Vec2 in) {
        Vec2 pos;
        pos.x = remap<float>(in.x, m_tactorRect.tl().x, m_tactorRect.br().x, 0., COLS-1);
        pos.y = remap<float>(in.y, m_tactorRect.tl().y, m_tactorRect.br().y, ROWS-1, 0);
        return pos;
    }

    /// Coroutine that plays back user's paths
    Enumerator playPaths() {
        m_playing = true;
        m_spat.play(m_sig);
        for (auto& path : m_paths) {
            for (auto& t : path) {
                m_target_px = t;
                auto target = pixelToSpatial(t);
                m_spat.setTarget(target.x, target.y);
                co_yield nullptr;
            }
        }
        m_spat.stop();
        m_playing = false;
    }

    /// Syntacts Session
    Session m_session;
    /// Syntacts Spatializer
    Spatializer m_spat;
    /// Spatializer radius
    float m_radius = 1;
    /// Syntacts Signal
    Signal m_sig;
    /// User's paths
    std::vector<std::vector<ImVec2>> m_paths;
    /// Are we playing back the user's path?
    bool m_playing = false;
    /// Is mouse following mode enabled?
    bool m_followMode = false;
    /// Pixel rect enclosing outer array
    Rect m_arrayRect;
    /// Pixel rect enclousing inner array
    Rect m_tactorRect;
    /// Pixel position of target
    Vec2 m_target_px;
};

// main, program entry point
int main(int argc, char const *argv[]) {
    SyntactsDraw app;
    app.run();
    return 0;
}