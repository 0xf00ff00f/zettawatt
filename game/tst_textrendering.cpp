#include <fontcache.h>
#include <glwindow.h>
#include <pixmap.h>
#include <shaderprogram.h>
#include <spritebatcher.h>
#include <texture.h>

#include <stb_truetype.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <ctime>
#include <iostream>
#include <numeric>

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
    std::unique_ptr<GX::TextureAtlas> m_textureAtlas;
    std::unique_ptr<GX::FontCache> m_fontCache;
    std::unique_ptr<GX::SpriteBatcher> m_spriteBatcher;
    double m_angle = 0.0;
};

void TestWindow::initializeGL()
{
    using namespace std::string_literals;

    m_textureAtlas = std::make_unique<GX::TextureAtlas>(256, 256, GX::PixelType::Grayscale);
    m_fontCache = std::make_unique<GX::FontCache>(m_textureAtlas.get());

    const auto font =
#ifdef _WIN32
            "/Windows/Fonts/comic.ttf"s;
#else
            "/usr/share/fonts/truetype/takao-gothic/TakaoPGothic.ttf"s;
#endif
    if (!m_fontCache->load(font, 50)) {
        spdlog::warn("failed to load {}", font.c_str());
    }

    m_shaderManager = std::make_unique<GX::ShaderManager>();
    m_spriteBatcher = std::make_unique<GX::SpriteBatcher>(m_shaderManager.get());

    m_spriteBatcher->setBatchProgram(GX::ShaderManager::Program::Text);
}

void TestWindow::paintGL()
{
    glViewport(0, 0, width(), height());
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const auto projectionMatrix =
            glm::ortho(0.0f, static_cast<float>(width()), static_cast<float>(height()), 0.0f, -1.0f, 1.0f);
    const auto viewMatrix = glm::mat4(1);
    const auto angle = static_cast<float>(0.1 * std::sin(m_angle));
    const auto modelMatrix = glm::rotate(glm::mat4(1), angle, glm::vec3(0, 0, 1));

    m_spriteBatcher->setTransformMatrix(projectionMatrix * viewMatrix * modelMatrix);
    m_spriteBatcher->startBatch();

    using namespace std::string_literals;

    static const std::vector<std::u32string> lines = {
        U"Lorem ipsum dolor sit amet, consectetur"s,
        U"adipiscing elit, sed do eiusmod tempor incididunt"s,
        U"ut labore et dolore magna aliqua. Ut enim ad"s,
        U"minim veniam, quis nostrud exercitation ullamco"s,
        U"laboris nisi ut aliquip ex ea commodo consequat."s,
        U"Duis aute irure dolor in reprehenderit in"s,
        U"voluptate velit esse cillum dolore eu fugiat nulla"s,
        U"pariatur. Excepteur sint occaecat cupidatat non"s,
        U"proident, sunt in culpa qui officia deserunt mollit"s,
        U"anim id est laborum."s,
        U"しかし時には、参考文献に掲載されている文章をそのまま転載"s,
        U"し、読者に読ませることによって、記事が説明しようとする事"s,
        U"項に対する読者の理解が著しく向上することがあります。たと"s,
        U"えば、作家を主題とする記事において、その作家の作風が色濃"s,
        U"く反映された作品の一部を掲載したり、政治家を主題とする記"s,
        U"事において、その政治家の重要演説の一部を掲載すれば、理解"s,
        U"の助けとなるでしょう。このような執筆方法は、ウィキペディ"s,
        U"アが検証可能性の担保を重要方針に掲げる趣旨に、決して反す"s,
        U"るものではありません。"s,
    };

    const auto color = glm::vec4(.5 + .5 * std::sin(20. * m_angle), 1, 0, 1);

    float y = 100;
    for (const auto &text : lines) {
        float x = 100;
        for (char32_t ch : text) {
            const auto glyph = m_fontCache->getGlyph(ch);
            if (!glyph) {
                continue;
            }

            const auto p0 = glm::vec2(x, y) + glm::vec2(glyph->boundingBox.min);
            const auto p1 = p0 + glm::vec2(glyph->boundingBox.max - glyph->boundingBox.min);

            m_spriteBatcher->addSprite(glyph->pixmap, p0, p1, color, 0);
            x += glyph->advanceWidth;
        }
        y += 50;
    }

    m_spriteBatcher->renderBatch();
}

void TestWindow::update(double elapsed)
{
    m_angle += .1 * elapsed;
}

int main()
{
    TestWindow w;
    w.initialize(1280, 1024, "test");
    w.renderLoop();
}
