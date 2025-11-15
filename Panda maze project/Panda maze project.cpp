
// Panda Bamboo Maze


#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <stack>
#include <random>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <cmath>

// ----------------------------- Types & Enums -----------------------------
enum class GameState { Menu, Playing, Win };
struct Cell { int x, y; };

// ----------------------------- Maze generator -----------------------------
class Maze {
public:
    int cols, rows;
    std::vector<std::vector<int>> grid; // 0 = wall, 1 = path

    Maze(int cols_, int rows_) : cols(cols_), rows(rows_) {
        if (cols % 2 == 0) cols++;
        if (rows % 2 == 0) rows++;
        grid.assign(rows, std::vector<int>(cols, 0));
        generate();
    }

    void generate() {
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                grid[r][c] = 0;

        std::mt19937 rng((unsigned)std::time(nullptr));
        std::stack<Cell> st;
        int sx = 1, sy = 1;
        grid[sy][sx] = 1;
        st.push({ sx, sy });

        auto neighbors = [&](int x, int y) {
            std::vector<Cell> n;
            const int dx[4] = { 0, 2, 0, -2 };
            const int dy[4] = { -2, 0, 2, 0 };
            for (int i = 0; i < 4; ++i) {
                int nx = x + dx[i], ny = y + dy[i];
                if (nx > 0 && nx < cols - 1 && ny > 0 && ny < rows - 1) {
                    if (grid[ny][nx] == 0) n.push_back({ nx, ny });
                }
            }
            return n;
            };

        while (!st.empty()) {
            Cell c = st.top();
            auto n = neighbors(c.x, c.y);
            if (!n.empty()) {
                std::uniform_int_distribution<int> pick(0, (int)n.size() - 1);
                Cell chosen = n[pick(rng)];
                int wx = (c.x + chosen.x) / 2;
                int wy = (c.y + chosen.y) / 2;
                grid[wy][wx] = 1;
                grid[chosen.y][chosen.x] = 1;
                st.push(chosen);
            }
            else st.pop();
        }

        // entry and exit openings
        grid[1][0] = 1;
        grid[rows - 2][cols - 1] = 1;
    }

    std::vector<sf::Vector2i> allPathCells() const {
        std::vector<sf::Vector2i> out;
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                if (grid[r][c] == 1) out.push_back({ c, r });
        return out;
    }
};

// ----------------------------- AnimatedSprite -----------------------------
class AnimatedSprite {
public:
    sf::Sprite sprite;
    sf::Texture texture;
    int frameW = 0, frameH = 0;
    int cols = 1, rows = 1;
    float frameTime = 0.12f;
    float timer = 0.f;
    int currentFrame = 0;
    int currentRow = 0;

    bool load(const std::string& path, int fw, int fh, int columns = 1, int rows_ = 1, float ftime = 0.12f) {
        if (!texture.loadFromFile(path)) return false;
        sprite.setTexture(texture);
        frameW = fw; frameH = fh;
        cols = columns; rows = rows_;
        frameTime = ftime;
        currentFrame = 0; currentRow = 0; timer = 0.f;
        updateRect();
        return true;
    }

    void setScale(float sx, float sy) { sprite.setScale(sx, sy); }
    void setPosition(float x, float y) { sprite.setPosition(x, y); }
    void setPosition(const sf::Vector2f& p) { sprite.setPosition(p); }
    sf::Vector2f getPosition() const { return sprite.getPosition(); }
    sf::FloatRect getGlobalBounds() const { return sprite.getGlobalBounds(); }

    void setRow(int row) {
        if (row < 0) row = 0;
        if (row >= rows) row = rows - 1;
        currentRow = row;
        currentFrame = 0;
        timer = 0.f;
        updateRect();
    }

    void update(float dt) {
        timer += dt;
        if (timer >= frameTime) {
            timer -= frameTime;
            currentFrame = (currentFrame + 1) % cols;
            updateRect();
        }
    }

private:
    void updateRect() {
        int left = currentFrame * frameW;
        int top = currentRow * frameH;
        sprite.setTextureRect(sf::IntRect(left, top, frameW, frameH));
    }
};

// ----------------------------- Main -----------------------------
int main() {
    // Config
    const int tileSize = 48;
    const int mazeCols = 21; // odd 
    const int mazeRows = 15; // odd 
    const int windowMargin = 40;

    Maze maze(mazeCols, mazeRows);
    int adjustedCols = maze.cols;
    int adjustedRows = maze.rows;

    int windowW = adjustedCols * tileSize + windowMargin * 2;
    int windowH = adjustedRows * tileSize + windowMargin * 2;

    sf::RenderWindow window(sf::VideoMode(windowW, windowH), "Panda Bamboo Maze");
    window.setFramerateLimit(60);

    sf::Vector2f origin(windowMargin, windowMargin);

    // Assets & anims
    sf::Texture texBamboo;
    sf::Font font;
    AnimatedSprite panda; // expects rows (down,left,right,up)
    AnimatedSprite coin;  // coin frames - THIS IS THE TREASURE!

    sf::SoundBuffer clickBuf, treasureBuf;
    sf::Sound clickSound, treasureSound;

    // File paths (adjust if needed)
    const std::string p_panda = "../assets/textures/Panda.png";
    const std::string p_coin = "../assets/textures/coins.png"; // coin is treasure
    const std::string p_bamboo = "../assets/textures/bamboo.png";
    const std::string p_font = "../assets/fonts/cute_font.ttf";
    const std::string s_click = "../assets/sounds/click.wav";
    const std::string s_treasure = "../assets/sounds/treasure.wav";

    bool okAssets = true;
    if (!font.loadFromFile(p_font)) { std::cerr << "Warning: font not found: " << p_font << "\n"; okAssets = false; }
    if (!texBamboo.loadFromFile(p_bamboo)) { std::cerr << "Note: bamboo texture not found: " << p_bamboo << " (using color fallback)\n"; }

    // Panda animation assumptions
    const int pandaFrameW = 32;
    const int pandaFrameH = 32;
    const int pandaCols = 4; // frames per row
    const int pandaRows = 4; // down, left, right, up
    const float pandaFrameTime = 0.12f;
    if (!panda.load(p_panda, pandaFrameW, pandaFrameH, pandaCols, pandaRows, pandaFrameTime)) {
        std::cerr << "Warning: panda animation not loaded from: " << p_panda << "\n"; okAssets = false;
    }

    // Coin animation (treasure)
    const int coinFrameW = 16;
    const int coinFrameH = 16;
    const int coinCols = 10; // frames in the horizontal strip
    if (!coin.load(p_coin, coinFrameW, coinFrameH, coinCols, 1, 0.08f)) {
        std::cerr << "Warning: coin animation not loaded from: " << p_coin << "\n"; okAssets = false;
    }

    bool haveClick = clickBuf.loadFromFile(s_click);
    if (!haveClick) std::cerr << "Note: click sound missing: " << s_click << "\n";
    bool haveTreasure = treasureBuf.loadFromFile(s_treasure);
    if (!haveTreasure) std::cerr << "Note: treasure sound missing: " << s_treasure << "\n";
    if (haveClick) clickSound.setBuffer(clickBuf);
    if (haveTreasure) treasureSound.setBuffer(treasureBuf);

    // UI
    sf::Text titleText("Panda Bamboo Maze", font, 48);
    titleText.setFillColor(sf::Color(0, 50, 0));
    titleText.setPosition(windowW / 2.f - titleText.getLocalBounds().width / 2.f, 50);

    sf::RectangleShape startBtn(sf::Vector2f(260, 60));
    startBtn.setOrigin(startBtn.getSize() / 2.f);
    startBtn.setPosition(windowW / 2.f, windowH / 2.f + 20);
    startBtn.setOutlineColor(sf::Color(0, 50, 0));
    startBtn.setOutlineThickness(3);
    startBtn.setFillColor(sf::Color(180, 255, 200));

    sf::Text startText("Start Game", font, 32);
    startText.setFillColor(sf::Color::Black);
    startText.setPosition(windowW / 2.f - startText.getLocalBounds().width / 2.f, startBtn.getPosition().y - startText.getLocalBounds().height / 2.f - 5);

    sf::Text hintText("Use arrow keys or WASD to move the panda.", font, 20);
    hintText.setFillColor(sf::Color(0, 50, 0));
    hintText.setPosition(windowMargin, windowH - 30);

    sf::Text winText("YOU FOUND THE TREASURE!", font, 48);
    winText.setFillColor(sf::Color::Black);
    winText.setPosition(windowW / 2.f - winText.getLocalBounds().width / 2.f, windowH / 2.f - 80);

    sf::Text replayText("Press R to play again or Esc to quit.", font, 24);
    replayText.setFillColor(sf::Color::Black);
    replayText.setPosition(windowW / 2.f - replayText.getLocalBounds().width / 2.f, windowH / 2.f + 0);

    // Movement + collision
    const float speed = 180.f;
    sf::Clock clock;

    std::vector<sf::RectangleShape> wallShapes;
    sf::Vector2f playerPos;
    sf::Vector2f treasurePos; // coin position

    float playerDisplaySize = tileSize * 0.75f;
    float pandaScale = playerDisplaySize / (float)pandaFrameW;
    panda.setScale(pandaScale, pandaScale);

    float coinDisplaySize = tileSize * 0.8f;
    float coinScale = coinDisplaySize / (float)coinFrameW;
    coin.setScale(coinScale, coinScale);

    auto collidesWithWalls = [&](sf::FloatRect bounds)->bool {
        int left = std::max(0, (int)((bounds.left - origin.x) / tileSize) - 1);
        int right = std::min(maze.cols - 1, (int)((bounds.left + bounds.width - origin.x) / tileSize) + 1);
        int top = std::max(0, (int)((bounds.top - origin.y) / tileSize) - 1);
        int bottom = std::min(maze.rows - 1, (int)((bounds.top + bounds.height - origin.y) / tileSize) + 1);
        for (int r = top; r <= bottom; ++r) {
            for (int c = left; c <= right; ++c) {
                if (r < 0 || r >= maze.rows || c < 0 || c >= maze.cols) continue;
                if (maze.grid[r][c] == 0) {
                    sf::FloatRect tileRect(origin.x + c * tileSize, origin.y + r * tileSize, (float)tileSize, (float)tileSize);
                    if (bounds.intersects(tileRect)) return true;
                }
            }
        }
        return false;
        };

    // Reset game: generate maze, walls, place player & coin 
    std::mt19937 rng((unsigned)std::time(nullptr) + 12345);
    auto resetGame = [&]() {
        maze.generate();
        wallShapes.clear();
        for (int r = 0; r < maze.rows; ++r) {
            for (int c = 0; c < maze.cols; ++c) {
                if (maze.grid[r][c] == 0) {
                    sf::RectangleShape rect(sf::Vector2f((float)tileSize, (float)tileSize));
                    rect.setPosition(origin.x + c * tileSize, origin.y + r * tileSize);
                    if (texBamboo.getSize().x > 0) rect.setTexture(&texBamboo);
                    else rect.setFillColor(sf::Color(50, 120, 50));
                    wallShapes.push_back(rect);
                }
            }
        }

        // player start cell (1,1)
        playerPos.x = origin.x + 1 * tileSize + (tileSize - playerDisplaySize) / 2.f;
        playerPos.y = origin.y + 1 * tileSize + (tileSize - playerDisplaySize) / 2.f;
        panda.setPosition(playerPos);

        // choose coin (treasure) cell not equal to start
        auto cells = maze.allPathCells();
        std::shuffle(cells.begin(), cells.end(), rng);
        sf::Vector2i tcell = cells.front();
        for (auto& pc : cells) { if (!(pc.x == 1 && pc.y == 1)) { tcell = pc; break; } }
        treasurePos.x = origin.x + tcell.x * tileSize + (tileSize - coinDisplaySize) / 2.f;
        treasurePos.y = origin.y + tcell.y * tileSize + (tileSize - coinDisplaySize) / 2.f;
        coin.setPosition(treasurePos);
        };

    GameState state = GameState::Menu;
    resetGame();
    bool playedTreasureSoundThisWin = false;

    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();

            if (state == GameState::Menu) {
                if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mpos((float)ev.mouseButton.x, (float)ev.mouseButton.y);
                    if (startBtn.getGlobalBounds().contains(mpos)) {
                        if (haveClick) clickSound.play();
                        resetGame();
                        state = GameState::Playing;
                        clock.restart();
                    }
                }
            }
            else if (state == GameState::Win) {
                if (ev.type == sf::Event::KeyPressed) {
                    if (ev.key.code == sf::Keyboard::R) {
                        resetGame();
                        state = GameState::Playing;
                        clock.restart();
                        playedTreasureSoundThisWin = false;
                    }
                    else if (ev.key.code == sf::Keyboard::Escape) {
                        window.close();
                    }
                }
            }
            else if (state == GameState::Playing) {
                if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Escape) {
                    state = GameState::Menu;
                }
            }
        }

        float dt = clock.restart().asSeconds();

        // logic update
        if (state == GameState::Playing) {
            sf::Vector2f vel(0.f, 0.f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::A)) vel.x -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D)) vel.x += 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::W)) vel.y -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S)) vel.y += 1.f;

            if (vel.x == 0.f && vel.y == 0.f) {
                // idle: do nothing special, animation still cycles
            }
            else {
                float len = std::sqrt(vel.x * vel.x + vel.y * vel.y);
                if (len > 0.f) { vel.x /= len; vel.y /= len; }
                sf::Vector2f attempted = playerPos + vel * speed * dt;

                // X axis check
                sf::FloatRect boundsX = panda.getGlobalBounds();
                boundsX.left = attempted.x;
                boundsX.top = playerPos.y;
                if (!collidesWithWalls(boundsX)) playerPos.x = attempted.x;

                // Y axis check
                sf::FloatRect boundsY = panda.getGlobalBounds();
                boundsY.left = playerPos.x;
                boundsY.top = attempted.y;
                if (!collidesWithWalls(boundsY)) playerPos.y = attempted.y;

                // Choose direction row
                if (std::fabs(vel.x) > std::fabs(vel.y)) {
                    if (vel.x < 0) panda.setRow(1); else panda.setRow(2);
                }
                else {
                    if (vel.y < 0) panda.setRow(3); else panda.setRow(0);
                }
            }

            panda.setPosition(playerPos);
            coin.setPosition(treasurePos);

            panda.update(dt);
            coin.update(dt);

            // Win (coin-as-treasure)
            if (panda.getGlobalBounds().intersects(coin.getGlobalBounds())) {
                if (haveTreasure && !playedTreasureSoundThisWin) {
                    treasureSound.play();
                    playedTreasureSoundThisWin = true;
                }
                state = GameState::Win;
            }
        }
        else {
            // Animate idle in menus
            panda.update(dt);
            coin.update(dt);
        }

        // Render
        window.clear(sf::Color(230, 255, 230));

        if (state == GameState::Menu) {
            window.draw(titleText);
            sf::Vector2i mouse = sf::Mouse::getPosition(window);
            if (startBtn.getGlobalBounds().contains((float)mouse.x, (float)mouse.y)) startBtn.setFillColor(sf::Color(160, 235, 180));
            else startBtn.setFillColor(sf::Color(180, 255, 200));
            window.draw(startBtn);
            window.draw(startText);
        }
        else if (state == GameState::Playing || state == GameState::Win) {
            // draw floor path tiles
            for (int r = 0; r < maze.rows; ++r) {
                for (int c = 0; c < maze.cols; ++c) {
                    if (maze.grid[r][c] == 1) {
                        sf::RectangleShape tile(sf::Vector2f((float)tileSize, (float)tileSize));
                        tile.setPosition(origin.x + c * tileSize, origin.y + r * tileSize);
                        tile.setFillColor(sf::Color(245, 255, 245));
                        window.draw(tile);
                    }
                }
            }

            // draw walls
            for (auto& w : wallShapes) window.draw(w);

            // draw coin (treasure) then panda
            window.draw(coin.sprite);
            window.draw(panda.sprite);

            window.draw(hintText);
        }

        if (state == GameState::Win) {
            sf::RectangleShape overlay(sf::Vector2f((float)windowW, (float)windowH));
            overlay.setFillColor(sf::Color(255, 255, 255, 200));
            window.draw(overlay);
            window.draw(winText);
            window.draw(replayText);
        }

        window.display();
    }

    return 0;
}
