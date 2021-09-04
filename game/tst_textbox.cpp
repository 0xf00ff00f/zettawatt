#include "uipainter.h"

#include <glwindow.h>
#include <shadermanager.h>
#include <util.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <ctime>
#include <iostream>
#include <numeric>

constexpr const char *FontName = "IBMPlexSans-Regular.ttf";

class TestWindow : public GX::GLWindow
{
public:
    TestWindow() = default;
    ~TestWindow() override = default;

private:
    void initializeGL() override;
    void paintGL() override;
    void update(double elapsed) override;

    std::unique_ptr<GX::ShaderManager> m_shaderManager;
    std::unique_ptr<UIPainter> m_painter;
    double m_angle = 0.0;
    double m_time = 0;
};

void TestWindow::initializeGL()
{
    m_shaderManager = std::make_unique<GX::ShaderManager>();
    m_painter = std::make_unique<UIPainter>(m_shaderManager.get());
    m_painter->resize(width(), height());
}

void TestWindow::paintGL()
{
    using namespace std::string_literals;

    glViewport(0, 0, width(), height());
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_painter->startPainting();

    constexpr auto FontSize = 40;
    static const auto LabelFont = UIPainter::Font { FontName, FontSize };
    m_painter->setFont(LabelFont);
    m_painter->setVerticalAlign(UIPainter::VerticalAlign::Middle);
    m_painter->setHorizontalAlign(UIPainter::HorizontalAlign::Left);

    const auto boxWidth = 400.0 + 300.0 * sin(2.0 * m_time);
    const auto boxHeight = 200.0f;
    const auto radius = 20.0f;
    const auto left = m_painter->sceneBox().min.x + radius;
    const auto textBox = GX::BoxF { glm::vec2(left, -0.5f * boxHeight), glm::vec2(left + boxWidth, 0.5f * boxHeight) };
    const auto outerBox = GX::BoxF { textBox.min - glm::vec2(radius), textBox.max + glm::vec2(radius) };

    const auto text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nullam imperdiet nisi nulla. Integer dictum arcu a felis lobortis semper. Sphinx of black quartz, judge my vow."s;

    m_painter->drawRoundedRect(outerBox, radius, glm::vec4(0, 1, 1, 0.5), -1);
    m_painter->drawTextBox(textBox, glm::vec4(1), 0, text);
    m_painter->donePainting();
}

void TestWindow::update(double elapsed)
{
    m_time += elapsed;
}

int main()
{
    TestWindow w;
    w.initialize(1280, 720, "test");
    w.renderLoop();
}
