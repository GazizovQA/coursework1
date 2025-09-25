
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <fstream> 

const float WIDTH = 600.f;
const float HEIGHT = 800.f;
const float speedMULTIPLIER = 2750.0f;
const float ballSize = 20.f;
const float MAX_BALL_SPEED = 0.15f;
int currentLevel = 1;
int score = 0;
int bestScore = 0;
float levelTransitionTimer = 0.0f;
const float LEVEL_TRANSITION_TIME = 2.0f;
const int rows = 5;
const int cols = 10;
const float blockWidth = 16.f * 3.125f;
const float blockHeight = 9.f * 3.333f;
const float spacing = 10.f;
const float RESPAWN_TIME = 2.f;


enum class GameState {
    Menu,
    Playing,
    Paused,
    GameOver,
    LevelTransition
};


struct Vector2f {
    float x, y;
    Vector2f() : x(0), y(0) {}
    Vector2f(float x, float y) : x(x), y(y) {}

    Vector2f operator*(float scalar) const {
        return Vector2f(x * scalar, y * scalar);
    }

    Vector2f& operator+=(const Vector2f& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    float length() const {
        return std::sqrt(x * x + y * y);
    }

    Vector2f normalized() const {
        float len = length();
        return len != 0 ? Vector2f(x / len, y / len) : *this;
    }
};


struct Block {
    sf::Sprite sprite;
    bool isDestroyed;
    int currentFrame;
    float timeSinceLastFrame;

    Block(float x, float y, const sf::Texture& texture) {
        sprite.setTexture(texture);
        sprite.setPosition(x, y);
        isDestroyed = false;
        currentFrame = 0;
        timeSinceLastFrame = 0.f;
        sprite.setTextureRect(sf::IntRect(0, 0, 16, 9));
        sprite.setScale(3.125f, 2.222f);
    }

    void UpdateAnimation(float deltaTime) {
        if (isDestroyed) return;
        timeSinceLastFrame += deltaTime;
        const float animationSpeed = 0.1f;
        if (timeSinceLastFrame >= animationSpeed) {
            timeSinceLastFrame = 0.f;
            currentFrame = (currentFrame + 1) % 6;
            sprite.setTextureRect(sf::IntRect(currentFrame * 16, 0, 16, 9));
        }
    }
};

// Типы бонусов
enum class BonusType {
    ExtraLife,
    ExtraBall
};

// Структура бонуса
struct Bonus {
    BonusType type;
    sf::Sprite sprite;
    bool isActive;
    Vector2f velocity;

    Bonus(float x, float y, const sf::Texture& texture, BonusType type)
        : type(type), isActive(true) {
        sprite.setTexture(texture);
        sprite.setPosition(x, y);
        sprite.setScale(0.05f, 0.05f);
        velocity = Vector2f(0, 0.05f);
    }
};

// Структура дополнительного мяча
struct ExtraBall {
    sf::CircleShape shape;
    Vector2f velocity;
    float lifetime;

    ExtraBall(float x, float y) : lifetime(10.0f) {
        shape.setRadius(ballSize / 2);
        shape.setFillColor(sf::Color(255, 165, 0));
        shape.setPosition(x, y);
        velocity = Vector2f((rand() % 100 - 50) / 100.0f, -0.1f);
    }
};

// Структура взрыва
struct Explosion {
    sf::Sprite sprite;
    int currentFrame;
    float timeSinceLastFrame;
    bool isActive;

    Explosion(float x, float y, const sf::Texture& texture) {
        sprite.setTexture(texture);
        sprite.setPosition(x, y);
        currentFrame = 0;
        timeSinceLastFrame = 0.f;
        isActive = true;
        sprite.setTextureRect(sf::IntRect(0, 0, 16, 16));
        sprite.setScale(2.0f, 2.0f);
    }

    void UpdateAnimation(float deltaTime) {
        timeSinceLastFrame += deltaTime;
        const float animationSpeed = 0.2f;
        if (timeSinceLastFrame >= animationSpeed) {
            timeSinceLastFrame = 0.f;
            currentFrame++;
            if (currentFrame >= 4) {
                isActive = false;
            }
            else {
                sprite.setTextureRect(sf::IntRect(currentFrame * 16, 0, 16, 16));
            }
        }
    }
};

// Пути к ресурсам
const std::string RESOURCES_PATH = "Resources/";



// Функция для обработки столкновения мяча с блоком
void HandleBlockCollision(sf::FloatRect ballBounds, sf::FloatRect blockBounds, float& ballSpeedX, float& ballSpeedY) {
    float overlapLeft = ballBounds.left + ballBounds.width - blockBounds.left;
    float overlapRight = blockBounds.left + blockBounds.width - ballBounds.left;
    float overlapTop = ballBounds.top + ballBounds.height - blockBounds.top;
    float overlapBottom = blockBounds.top + blockBounds.height - ballBounds.top;

    bool fromLeft = std::abs(overlapLeft) < std::abs(overlapRight);
    bool fromTop = std::abs(overlapTop) < std::abs(overlapBottom);

    float minOverlapX = fromLeft ? overlapLeft : overlapRight;
    float minOverlapY = fromTop ? overlapTop : overlapBottom;

    if (std::abs(minOverlapX) < std::abs(minOverlapY)) {
        ballSpeedX *= -1;
    }
    else {
        ballSpeedY *= -1;
    }
}



int main() {
    // Инициализация окна
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Arkanoid");

    // Загружаем текстуры
    sf::Texture blockTexture;
    if (!blockTexture.loadFromFile(RESOURCES_PATH + "Sprites/NES - Arkanoid - Blocks & Backgrounds.png")) {
        std::cerr << "Failed to load block texture!" << std::endl;
        return EXIT_FAILURE;
    }

    sf::Texture explosionTexture;
    if (!explosionTexture.loadFromFile(RESOURCES_PATH + "Sprites/boom.png")) {
        std::cerr << "Failed to load explosion texture!" << std::endl;
        return EXIT_FAILURE;
    }

    sf::Texture lifeBonusTexture;
    if (!lifeBonusTexture.loadFromFile(RESOURCES_PATH + "Sprites/life_bonus.png")) {
        std::cerr << "Failed to load life bonus texture!" << std::endl;
        return EXIT_FAILURE;
    }

    sf::Texture ballBonusTexture;
    if (!ballBonusTexture.loadFromFile(RESOURCES_PATH + "Sprites/ball_bonus.png")) {
        std::cerr << "Failed to load ball bonus texture!" << std::endl;
        return EXIT_FAILURE;
    }

    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile(RESOURCES_PATH + "Sprites/background.jpg")) {
        std::cerr << "Failed to load background texture!" << std::endl;
        return EXIT_FAILURE;
    }

    sf::Font font;
    if (!font.loadFromFile(RESOURCES_PATH + "PixelizerBold.ttf")) {
        std::cerr << "Failed to load font!" << std::endl;
        return EXIT_FAILURE;
    }

    // Остальная инициализация
    GameState currentState = GameState::Menu;
    int lives = 3;
    bool isRespawning = false;
    float respawnTimer = 0.0f;
    const float RESPAWN_TIME = 2.0f;
    float ballSpeedX = 0.1f;
    float ballSpeedY = 0.1f;

    // Платформа
    sf::RectangleShape platform;
    platform.setSize(sf::Vector2f(100, 20));
    platform.setFillColor(sf::Color::Cyan);
    platform.setPosition(350, 650);

    // Мяч
    sf::CircleShape ball(ballSize / 2);
    ball.setFillColor(sf::Color::Red);

    // списки игровых обьектов
    std::vector<Block> blocks;
    std::vector<Explosion> explosions;
    std::vector<Bonus> activeBonuses;
    std::vector<ExtraBall> extraBalls;

    // Фон
    sf::Sprite backgroundSprite(backgroundTexture);
    backgroundSprite.setScale(
        WIDTH / backgroundSprite.getLocalBounds().width,
        HEIGHT / backgroundSprite.getLocalBounds().height
    );

    // Текст
    sf::Text livesText;
    livesText.setFont(font);
    livesText.setCharacterSize(30);
    livesText.setFillColor(sf::Color::White);
    livesText.setPosition(10, HEIGHT - 40);

    auto InitGame = [&]() {
        currentLevel = 1;
        score = 0;
        int lives = 3;
        bool isRespawning = false;
        float respawnTimer = RESPAWN_TIME;
        float ballSpeedX = 0.1f;
        float ballSpeedY = 0.1f;

        platform.setPosition(350, 650);
        ball.setPosition(WIDTH / 2 - ballSize / 2, HEIGHT / 2 - ballSize / 2);

        // Читаем лучший счёт из файла
        std::ifstream inputFile("best_score.txt");
        if (inputFile) {
            inputFile >> bestScore;
            inputFile.close();
        }

        // Инициализация блоков для первого уровня
        blocks.clear();
        const int rows = 5;
        const int cols = 10;
        const float blockWidth = 16.f * 3.125f;
        const float blockHeight = 9.f * 3.333f;
        const float spacing = 10.f;

        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                float x = j * (blockWidth + spacing) + spacing;
                float y = i * (blockHeight + spacing) + 50;
                blocks.emplace_back(x, y, blockTexture);
            }
        }
        };

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    sf::Clock gameClock;
    float lastTime = gameClock.getElapsedTime().asSeconds();

    while (window.isOpen()) {
        float currentTime = gameClock.getElapsedTime().asSeconds();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
                if (currentState == GameState::Menu) {
                    if (event.key.code == sf::Keyboard::Enter) {
                        InitGame();
                        currentState = GameState::Playing;
                    }
                }
                else if (currentState == GameState::Playing) {
                    if (event.key.code == sf::Keyboard::P) {
                        currentState = GameState::Paused;
                    }
                }
                else if (currentState == GameState::Paused) {
                    if (event.key.code == sf::Keyboard::P) {
                        currentState = GameState::Playing;
                    }
                    if (event.key.code == sf::Keyboard::R) {
                        currentState = GameState::Menu;
                    }
                }
                else if (currentState == GameState::GameOver) {
                    if (event.key.code == sf::Keyboard::R) {
                        InitGame();
                        currentState = GameState::Playing;
                    }
                }
            }
        }

        if (currentState == GameState::Playing) {
            if (!isRespawning) {
                // Управление платформой
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                    platform.move(-0.2f * deltaTime * speedMULTIPLIER, 0.0f);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                    platform.move(0.2f * deltaTime * speedMULTIPLIER, 0.0f);
                }

                // Проверка положения платформы
                sf::Vector2f platformPos = platform.getPosition();
                if (platformPos.x < 0) platform.setPosition(0, platformPos.y);
                if (platformPos.x + platform.getSize().x > WIDTH)
                    platform.setPosition(WIDTH - platform.getSize().x, platformPos.y);

                // Движение мяча
                ball.move(ballSpeedX * deltaTime * speedMULTIPLIER, ballSpeedY * deltaTime * speedMULTIPLIER);

                // Проверка нижней границы
                sf::Vector2f ballPos = ball.getPosition();
                if (ballPos.y + ballSize * 2 > HEIGHT) {
                    lives--;
                    if (lives <= 0) {
                        currentState = GameState::GameOver;
                    }
                    else {
                        isRespawning = true;
                        respawnTimer = RESPAWN_TIME;
                        ball.setPosition(WIDTH / 2 - ballSize / 2, HEIGHT / 2 - ballSize / 2);
                    }
                }

                // Проверка границ окна
                if (ballPos.x <= 0) {
                    ballSpeedX = -ballSpeedX;
                    ball.setPosition(0, ballPos.y);
                }
                if (ballPos.x >= WIDTH - ballSize) {
                    ballSpeedX = -ballSpeedX;
                    ball.setPosition(WIDTH - ballSize, ballPos.y);
                }
                if (ballPos.y <= 0) {
                    ballSpeedY = -ballSpeedY;
                    ball.setPosition(ballPos.x, 0);
                }

                // Столкновение с платформой
                if (ball.getGlobalBounds().intersects(platform.getGlobalBounds())) {
                    float ballCenterX = ball.getPosition().x + ballSize / 2;
                    float platformCenterX = platform.getPosition().x + platform.getSize().x / 2;
                    float hitPosition = (ballCenterX - platformCenterX) / (platform.getSize().x / 2);
                    Vector2f direction(hitPosition, -1.0f);
                    direction = direction.normalized();
                    ballSpeedX = direction.x * MAX_BALL_SPEED;
                    ballSpeedY = direction.y * MAX_BALL_SPEED;
                    ball.setPosition(ball.getPosition().x, platform.getPosition().y - ballSize);
                }

                // Столкновение с блоками
                for (auto& block : blocks) {
                    if (!block.isDestroyed && ball.getGlobalBounds().intersects(block.sprite.getGlobalBounds())) {
                        block.isDestroyed = true;
                        explosions.emplace_back(
                            block.sprite.getPosition().x,
                            block.sprite.getPosition().y,
                            explosionTexture
                        );

                        // Генерируем бонусы с вероятностью 10%
                        if (rand() % 100 < 10) {
                            BonusType type = rand() % 2 == 0 ?
                                BonusType::ExtraLife :
                                BonusType::ExtraBall;
                            const sf::Texture& tex =
                                type == BonusType::ExtraLife ?
                                lifeBonusTexture :
                                ballBonusTexture;
                            activeBonuses.emplace_back(
                                block.sprite.getPosition().x,
                                block.sprite.getPosition().y,
                                tex,
                                type
                            );
                        }

                        score += 10;

                        HandleBlockCollision(
                            ball.getGlobalBounds(),
                            block.sprite.getGlobalBounds(),
                            ballSpeedX,
                            ballSpeedY
                        );
                    }
                }

                // Обработка бонусов
                for (auto& bonus : activeBonuses) {
                    bonus.sprite.move(bonus.velocity.x * deltaTime * speedMULTIPLIER,
                        bonus.velocity.y * deltaTime * speedMULTIPLIER);

                    if (bonus.sprite.getGlobalBounds().intersects(platform.getGlobalBounds())) {
                        bonus.isActive = false;
                        if (bonus.type == BonusType::ExtraLife) {
                            lives++;
                        }
                        else if (bonus.type == BonusType::ExtraBall) {
                            extraBalls.emplace_back(
                                platform.getPosition().x + platform.getSize().x / 2,
                                platform.getPosition().y - ballSize
                            );
                        }
                    }

                    if (bonus.sprite.getPosition().y > HEIGHT) {
                        bonus.isActive = false;
                    }
                }

                // Обработка дополнительных мячей
                for (auto& extraBall : extraBalls) {
                    extraBall.shape.move(
                        extraBall.velocity.x * deltaTime * speedMULTIPLIER,
                        extraBall.velocity.y * deltaTime * speedMULTIPLIER
                    );
                    extraBall.lifetime -= deltaTime;

                    sf::Vector2f pos = extraBall.shape.getPosition();
                    if (pos.x <= 0 || pos.x >= WIDTH - ballSize) {
                        extraBall.velocity.x *= -1;
                    }
                    if (pos.y <= 0) {
                        extraBall.velocity.y *= -1;
                    }
                    if (pos.y > HEIGHT) {
                        extraBall.lifetime = 0;
                    }

                    for (auto& block : blocks) {
                        if (!block.isDestroyed && extraBall.shape.getGlobalBounds().intersects(block.sprite.getGlobalBounds())) {
                            block.isDestroyed = true;
                            explosions.emplace_back(
                                block.sprite.getPosition().x,
                                block.sprite.getPosition().y,
                                explosionTexture
                            );

                            HandleBlockCollision(
                                extraBall.shape.getGlobalBounds(),
                                block.sprite.getGlobalBounds(),
                                extraBall.velocity.x,
                                extraBall.velocity.y
                            );

                            score += 10;
                        }
                    }
                }

                // Удаление неактивных бонусов и мертвых мячей
                activeBonuses.erase(
                    std::remove_if(activeBonuses.begin(), activeBonuses.end(),
                        [](const Bonus& b) { return !b.isActive; }),
                    activeBonuses.end()
                );

                extraBalls.erase(
                    std::remove_if(extraBalls.begin(), extraBalls.end(),
                        [](const ExtraBall& eb) { return eb.lifetime <= 0; }),
                    extraBalls.end()
                );

                // Ограничение максимальной скорости
                ballSpeedX = std::min(std::max(ballSpeedX, -MAX_BALL_SPEED), MAX_BALL_SPEED);
                ballSpeedY = std::min(std::max(ballSpeedY, -MAX_BALL_SPEED), MAX_BALL_SPEED);

                // Проверка завершения уровня
                bool allDestroyed = std::all_of(blocks.begin(), blocks.end(),
                    [](const Block& b) { return b.isDestroyed; });

                if (allDestroyed) {
                    currentLevel++;
                    score += 100;
                    currentState = GameState::LevelTransition;
                    levelTransitionTimer = LEVEL_TRANSITION_TIME;

                    // Переходим на следующий уровень
                    blocks.clear();
                    const int rows = 5 + currentLevel;
                    const int cols = 10;
                    const float blockWidth = 16.f * 3.125f;
                    const float blockHeight = 9.f * 3.333f;
                    const float spacing = 10.f;

                    for (int i = 0; i < rows; ++i) {
                        for (int j = 0; j < cols; ++j) {
                            float x = j * (blockWidth + spacing) + spacing;
                            float y = i * (blockHeight + spacing) + 50;
                            blocks.emplace_back(x, y, blockTexture);
                        }
                    }

                    // Возвращаемся в центр
                    ball.setPosition(WIDTH / 2, HEIGHT / 2);
                    platform.setPosition(350, 650);
                }
            }
            else {
                respawnTimer -= deltaTime;
                if (respawnTimer <= 0) {
                    isRespawning = false;
                    ballSpeedX = 0.1f;
                    ballSpeedY = 0.1f;
                }
            }

            // Обновляем анимацию блоков
            for (auto& block : blocks) {
                if (!block.isDestroyed) {
                    block.UpdateAnimation(deltaTime);
                }
            }

            // Обновляем анимацию взрывов
            for (auto& explosion : explosions) {
                if (explosion.isActive) {
                    explosion.UpdateAnimation(deltaTime);
                }
            }

            // Удаляем законченные взрывы
            explosions.erase(
                std::remove_if(explosions.begin(), explosions.end(),
                    [](const Explosion& e) { return !e.isActive; }),
                explosions.end()
            );

            // Обновляем текст количества жизней
            std::stringstream ss;
            ss << "Lives: " << lives;
            livesText.setString(ss.str());
        }

        // Обработка состояния перехода уровня
        if (currentState == GameState::LevelTransition) {
            levelTransitionTimer -= deltaTime;
            if (levelTransitionTimer <= 0) {
                currentState = GameState::Playing;
            }
        }

        // Обработка конца игры
        if (lives <= 0) {
            // Сохраняем рекорд
            if (score > bestScore) {
                bestScore = score;
                std::ofstream outputFile("best_score.txt");
                if (outputFile) {
                    outputFile << bestScore;
                    outputFile.close();
                }
            }
            currentState = GameState::GameOver;
            if (event.key.code == sf::Keyboard::R) {
                currentState = GameState::Menu;
                lives = 3;
            }
        }

        // Рендеринг
        window.clear();
        window.draw(backgroundSprite);

        switch (currentState) {
        case GameState::Menu: {
            sf::Text menuText;
            menuText.setFont(font);
            menuText.setString("Press Enter to Start");
            menuText.setCharacterSize(50);
            menuText.setFillColor(sf::Color::White);
            menuText.setPosition(WIDTH / 2 - 200, HEIGHT / 2);
            window.draw(menuText);
            break;
        }
        case GameState::Playing: {
            window.draw(platform);

            for (const auto& block : blocks) {
                if (!block.isDestroyed) {
                    window.draw(block.sprite);
                }
            }

            for (const auto& explosion : explosions) {
                if (explosion.isActive) {
                    window.draw(explosion.sprite);
                }
            }

            if (!isRespawning ||
                static_cast<int>(respawnTimer * 10) % 2 == 0) {
                window.draw(ball);
            }

            // Рисуем активные бонусы
            for (const auto& bonus : activeBonuses) {
                window.draw(bonus.sprite);
            }

            // Рисуем дополнительные шары
            for (const auto& extraBall : extraBalls) {
                window.draw(extraBall.shape);
            }

            window.draw(livesText);
            break;
        }
        case GameState::Paused: {
            sf::Text pauseText;
            pauseText.setFont(font);
            pauseText.setString("Paused\nPress P to Resume\nPress R to Return to Menu");
            pauseText.setCharacterSize(50);
            pauseText.setFillColor(sf::Color::White);
            pauseText.setPosition(WIDTH / 2 - 200, HEIGHT / 2 - 50);
            window.draw(pauseText);
            break;
        }
        case GameState::LevelTransition: {
            sf::Text transitionText;
            transitionText.setFont(font);
            transitionText.setString("Stage " + std::to_string(currentLevel));
            transitionText.setCharacterSize(50);
            transitionText.setFillColor(sf::Color::Yellow);
            transitionText.setPosition(WIDTH / 2 - 100, HEIGHT / 2 - 50);
            window.draw(transitionText);
            break;
        }
        case GameState::GameOver: {
            sf::Text gameOverText;
            gameOverText.setFont(font);
            gameOverText.setString("Game Over\nScore: " + std::to_string(score) +
                "\nBest Score: " + std::to_string(bestScore) +
                (score > bestScore ? "\nNew Record!" : ""));
            gameOverText.setCharacterSize(50);
            gameOverText.setFillColor(sf::Color::Red);
            gameOverText.setPosition(WIDTH / 2 - 200, HEIGHT / 2 - 50);
            window.draw(gameOverText);
            break;
        }
        }

        window.display();
    }

    return 0;
}