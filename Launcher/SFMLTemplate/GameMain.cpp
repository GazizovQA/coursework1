#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <windows.h> 
#include <tlhelp32.h>

const std::string RES_PATH = "Resources/";
const std::string ACCOUNTS_FILE = RES_PATH + "accounts.txt";
const std::string BEST_SCORE_PATH = RES_PATH + "arcanoid/best_score.txt";
const std::string UNIQUE_USERS_FILE = RES_PATH + "unique_users.txt";
const int WIDTH = 1600;
const int HEIGHT = 800;
const char DELIMITER = '|';
const float AVATAR_RIGHT_MARGIN = 20.0f;
const float AVATAR_TOP_MARGIN = 20.0f;

// Реализация списков
struct NotificationNode;
struct UserNode;

// Узел списка уведомлений
struct NotificationNode {
    NotificationNode* next = nullptr;

    struct Notification {
        sf::Text text;
        float timer = 0.0f;
        float alpha = 255.0f;
    } data;

    NotificationNode(const Notification& note) : data(note) {}
    ~NotificationNode() { delete next; }
};

// Список уведомлений
class NotificationList {
public:
    NotificationNode* head = nullptr;

    ~NotificationList() { delete head; }

    void add(const NotificationNode::Notification& note) {
        NotificationNode* newNode = new NotificationNode(note);
        if (!head) {
            head = newNode;
        }
        else {
            NotificationNode* current = head;
            while (current->next) current = current->next;
            current->next = newNode;
        }
    }

    void removeOld() {
        NotificationNode** current = &head;
        while (*current) {
            if ((*current)->data.timer >= 2.0f) {
                NotificationNode* toDelete = *current;
                *current = toDelete->next;
                toDelete->next = nullptr;
                delete toDelete;
            }
            else {
                current = &(*current)->next;
            }
        }
    }
};

// Узел списка пользователей
struct UserNode {
    UserNode* next = nullptr;

    struct User {
        std::string username;
        std::string login;
        std::string password;
        std::string secret_word;
        std::string avatar;

        User() = default;
        User(const std::string& un, const std::string& lg,
            const std::string& pw, const std::string& sw,
            const std::string& av) : username(un), login(lg),
            password(pw), secret_word(sw), avatar(av) {}
    } data;

    UserNode(const User& user) : data(user) {}
    ~UserNode() { delete next; }
};

// Список пользователей
class UserList {
public:
    UserNode* head = nullptr;

    ~UserList() { delete head; }

    void add(const UserNode::User& user) {
        UserNode* newNode = new UserNode(user);
        if (!head) {
            head = newNode;
        }
        else {
            UserNode* current = head;
            while (current->next) current = current->next;
            current->next = newNode;
        }
    }

   
    bool loginExists(const std::string& login) const {
        UserNode* current = head;
        while (current) {
            if (current->data.login == login) return true;
            current = current->next;
        }
        return false;
    }

    UserNode::User* findUser(const std::string& login, const std::string& password) {
        UserNode* current = head;
        while (current) {
            if (current->data.login == login && current->data.password == password) {
                return &current->data;
            }
            current = current->next;
        }
        return nullptr;
    }
};

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string join(const std::vector<std::string>& parts, char delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i != 0) oss << delimiter;
        oss << parts[i];
    }
    return oss.str();
}

enum class AppState {
    Welcome,
    Login,
    Register,
    Profile,
    ProfileSettings,
    Games,
    Leaderboards,
    ExitConfirm,
    LogoutConfirm
};

struct Dialog {
    sf::RectangleShape background;
    sf::Text question;
    sf::RectangleShape yesButton;
    sf::Text yesText;
    sf::RectangleShape noButton;
    sf::Text noText;
    bool active = false;
};


class TextInput {
public:

    TextInput() = default;

    TextInput(sf::Vector2f size, sf::Vector2f pos, sf::Font& font,
        const std::string& labelText, float outlineThickness = 2.0f)
        : label(labelText)
    {
        box.setSize(size);
        box.setPosition(pos);
        box.setFillColor(sf::Color(60, 60, 60));
        box.setOutlineThickness(outlineThickness);
        box.setOutlineColor(sf::Color(200, 200, 200));

        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color(150, 150, 150));
        text.setPosition(pos.x + 15, pos.y + 10);
        text.setString(label);

        errorText.setFont(font);
        errorText.setCharacterSize(20);
        errorText.setFillColor(sf::Color(255, 50, 50));
        errorText.setPosition(pos.x + size.x + 10, pos.y + 5);

        cursor.setSize(sf::Vector2f(2, 30));
        cursor.setFillColor(sf::Color::White);
    }

    void handleEvent(sf::Event event, sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed) {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            bool wasActive = active;
            active = box.getGlobalBounds().contains(mousePos);

            if (active && !wasActive) {
                cursorClock.restart();
                cursorVisible = true;
                if (content.empty()) {
                    text.setString("");
                    text.setFillColor(sf::Color::White);
                }
            }

            if (!active && wasActive) {
                if (content.empty()) {
                    text.setString(label);
                    text.setFillColor(sf::Color(150, 150, 150));
                }
            }
        }

        if (active && event.type == sf::Event::TextEntered) {
            if (event.text.unicode == '\b') {
                if (!content.empty()) content.pop_back();
            }
            else if (event.text.unicode < 128) {
                content += static_cast<char>(event.text.unicode);
            }
            updateText();
            cursorClock.restart();
            cursorVisible = true;
        }
    }

    void updateText() {
        if (active || !content.empty()) {
            text.setString(content);
            text.setFillColor(sf::Color::White);
        }
        else {
            text.setString(label);
            text.setFillColor(sf::Color(150, 150, 150));
        }

        sf::FloatRect textBounds = text.getLocalBounds();
        while (textBounds.width > box.getSize().x - 30) {
            content.pop_back();
            text.setString(content);
            textBounds = text.getLocalBounds();
        }

        cursor.setPosition(text.getPosition().x + textBounds.width + 2, text.getPosition().y + 5);
    }

    void updateCursor(float dt) {
        if (active) {
            if (cursorClock.getElapsedTime().asSeconds() > 0.5) {
                cursorVisible = !cursorVisible;
                cursorClock.restart();
            }
        }
    }

    void draw(sf::RenderWindow& window) {
        window.draw(box);
        window.draw(text);
        if (active && cursorVisible) window.draw(cursor);
        if (hasError) window.draw(errorText);
    }

    const std::string& getContent() const { return content; }
    void setError(bool error, const std::string& message = "") {
        hasError = error;
        errorText.setString(message);
    }

private:
    sf::RectangleShape box;
    sf::Text text;
    std::string content;
    bool active = false;
    std::string label;
    sf::Text errorText;
    bool hasError = false;
    sf::RectangleShape cursor;
    sf::Clock cursorClock;
    bool cursorVisible = false;
};

class AuthManager {
private:

    std::string xorEncryptDecrypt(const std::string& data, const std::string& key) {
        std::string encryptedData(data.size(), '\0');

        for (size_t i = 0; i < data.size(); ++i) {
            encryptedData[i] = data[i] ^ key[i % key.size()];
        }

        return encryptedData;
    }

    UserList users;
    std::string encryptionKey = "my_secret_key";

    void loadUsers() {
        delete users.head;
        users.head = nullptr;

        std::ifstream file(ACCOUNTS_FILE);
        if (!file) return;

        std::string line;
        while (std::getline(file, line)) {
            std::vector<std::string> parts = split(line, DELIMITER);
            if (parts.size() == 5) {
                std::string decryptedPassword = xorEncryptDecrypt(parts[2], encryptionKey);
                users.add(UserNode::User(parts[0], parts[1], decryptedPassword, parts[3], parts[4]));
            }
        }
    }

    void saveUsers() {
        std::ofstream file(ACCOUNTS_FILE);
        UserNode* current = users.head;
        while (current) {
            std::string encryptedPassword = xorEncryptDecrypt(current->data.password, encryptionKey);
            file << current->data.username << DELIMITER
                << current->data.login << DELIMITER
                << encryptedPassword << DELIMITER
                << current->data.secret_word << DELIMITER
                << current->data.avatar << "\n";
            current = current->next;
        }
    }

public:
    AuthManager() { loadUsers(); }

    bool loginExists(const std::string& login) const {
        return users.loginExists(login);
    }

    bool registerUser(const UserNode::User& newUser) {
        if (loginExists(newUser.login)) return false;
        users.add(newUser);
        saveUsers();
        return true;
    }

    UserNode::User* loginUser(const std::string& login, const std::string& password) {
        return users.findUser(login, password);
    }
};

class GameLauncher {
private:
    sf::RenderWindow window;
    AppState currentState = AppState::Welcome;
    sf::Font mainFont;
    AuthManager authManager;
    UserNode::User* currentUser = nullptr;

    sf::Texture avatarTexture;
    sf::Sprite avatarSprite;
    bool isAvatarHovered = false;

    sf::Texture backgroundTex;
    sf::Sprite background;

    
    TextInput inputs[4];
    int inputCount = 0;

    struct Button {
        sf::RectangleShape shape;
        sf::Text text;
        bool isHovered = false;
    };
    Button buttons[4];
    int buttonCount = 0;

    Button profileButtons[4];
    int profileButtonCount = 0;

    struct GameStat {
        sf::RectangleShape background;
        sf::Text gameTitle;
        sf::Text bestScore;
        sf::Text playersCount;
        sf::Text wipText;
    };

    GameStat leaderboardGames[5];
    int gameStatCount = 0;
    sf::Text statsTitle;

    sf::Text welcomeText;
    float welcomeAlpha = 255.0f;
    bool fading = true;

    NotificationList notifications;

    sf::RectangleShape profileHeader;

    sf::Text titleText;
    sf::Clock pulsationClock;
    sf::Text copyrightText;

    sf::RectangleShape settingsBackButton;
    sf::Text settingsBackText;
    bool isSettingsBackHovered = false;

    sf::RectangleShape arcanoidButton;
    sf::Text arcanoidButtonText;

    sf::Text gamesTitle;
    sf::Text leaderboardsTitle;
    sf::RectangleShape backButton;
    sf::Text backButtonText;

    Dialog exitDialog;
    Dialog logoutDialog;

    float pulsateScale = 1.0f;

    void createButton(int index, const std::string& label,
        sf::Vector2f position, sf::Font& font, unsigned characterSize = 30) {
        buttons[index].text.setString(label);
        buttons[index].text.setFont(font);
        buttons[index].text.setCharacterSize(characterSize);
        buttons[index].text.setFillColor(sf::Color::White);

        sf::FloatRect textBounds = buttons[index].text.getLocalBounds();
        buttons[index].shape.setSize(sf::Vector2f(textBounds.width + 40, textBounds.height + 20));
        buttons[index].shape.setPosition(position);

        buttons[index].shape.setFillColor(sf::Color::Transparent);
        buttons[index].shape.setOutlineThickness(2);
        buttons[index].shape.setOutlineColor(sf::Color(200, 200, 200));

        buttons[index].text.setPosition(
            position.x + (buttons[index].shape.getSize().x - textBounds.width) / 2 - textBounds.left,
            position.y + (buttons[index].shape.getSize().y - textBounds.height) / 2 - textBounds.top
        );
    }

    void createProfileButton(int index, const std::string& label,
        sf::Vector2f position, sf::Font& font, unsigned characterSize = 30) {
        profileButtons[index].text.setString(label);
        profileButtons[index].text.setFont(font);
        profileButtons[index].text.setCharacterSize(characterSize);
        profileButtons[index].text.setFillColor(sf::Color::White);

        sf::FloatRect textBounds = profileButtons[index].text.getLocalBounds();
        profileButtons[index].shape.setSize(sf::Vector2f(textBounds.width + 40, textBounds.height + 20));
        profileButtons[index].shape.setPosition(position);

        profileButtons[index].shape.setFillColor(sf::Color::Transparent);
        profileButtons[index].shape.setOutlineThickness(2);
        profileButtons[index].shape.setOutlineColor(sf::Color(200, 200, 200));

        profileButtons[index].text.setPosition(
            position.x + (profileButtons[index].shape.getSize().x - textBounds.width) / 2 - textBounds.left,
            position.y + (profileButtons[index].shape.getSize().y - textBounds.height) / 2 - textBounds.top
        );
    }

    void initProfileSettingsScreen() {
        settingsBackButton.setSize(sf::Vector2f(120, 40));
        settingsBackButton.setPosition(50, 50);
        settingsBackButton.setFillColor(sf::Color(80, 80, 80));
        settingsBackButton.setOutlineThickness(2);
        settingsBackButton.setOutlineColor(sf::Color(200, 200, 200));

        settingsBackText.setString("Back");
        settingsBackText.setFont(mainFont);
        settingsBackText.setCharacterSize(24);
        settingsBackText.setFillColor(sf::Color::White);
        settingsBackText.setPosition(
            settingsBackButton.getPosition().x + 25,
            settingsBackButton.getPosition().y + 5
        );
    }

    void initGamesScreen() {
        gamesTitle.setString("My Games");
        gamesTitle.setFont(mainFont);
        gamesTitle.setCharacterSize(60);
        gamesTitle.setFillColor(sf::Color::White);
        gamesTitle.setPosition(WIDTH / 2 - 150, 100);

        backButton.setSize(sf::Vector2f(120, 40));
        backButton.setPosition(50, 50);
        backButton.setFillColor(sf::Color(80, 80, 80));
        backButton.setOutlineThickness(2);
        backButton.setOutlineColor(sf::Color(200, 200, 200));

        backButtonText.setString("Back");
        backButtonText.setFont(mainFont);
        backButtonText.setCharacterSize(24);
        backButtonText.setFillColor(sf::Color::White);
        backButtonText.setPosition(60, 55);

        arcanoidButton.setSize(sf::Vector2f(200, 60));
        arcanoidButton.setPosition(WIDTH / 2 - 100, 300);
        arcanoidButton.setFillColor(sf::Color(80, 80, 80));
        arcanoidButton.setOutlineThickness(2);
        arcanoidButton.setOutlineColor(sf::Color(200, 200, 200));

        arcanoidButtonText.setString("Arcanoid");
        arcanoidButtonText.setFont(mainFont);
        arcanoidButtonText.setCharacterSize(30);
        arcanoidButtonText.setFillColor(sf::Color::White);
        arcanoidButtonText.setPosition(
            arcanoidButton.getPosition().x + 30,
            arcanoidButton.getPosition().y + 10
        );
    }

    void createGameStat(GameStat& stat, const std::string& name,
        float xPos, float yPos, float width, float height)
    {
        stat.background.setSize(sf::Vector2f(width, height));
        stat.background.setPosition(xPos, yPos);
        stat.background.setFillColor(sf::Color(80, 80, 80));


        stat.gameTitle.setString(name);
        stat.gameTitle.setFont(mainFont);
        stat.gameTitle.setCharacterSize(28);
        sf::FloatRect titleBounds = stat.gameTitle.getLocalBounds();
        stat.gameTitle.setOrigin(titleBounds.width / 2, 0);
        stat.gameTitle.setPosition(
            xPos + width / 2,
            yPos + 15
        );


        if (name == "Arkanoid") {

            stat.bestScore.setString("Best: 0");
            stat.bestScore.setFont(mainFont);
            stat.bestScore.setCharacterSize(24);
            stat.bestScore.setPosition(
                xPos + 20,
                yPos + 60
            );


            stat.playersCount.setString("Players: 0");
            stat.playersCount.setFont(mainFont);
            stat.playersCount.setCharacterSize(24);
            stat.playersCount.setPosition(
                xPos + 20,
                yPos + 100
            );
        }

        else {
            stat.wipText.setString("WIP");
            stat.wipText.setFont(mainFont);
            stat.wipText.setCharacterSize(42);
            sf::FloatRect wipBounds = stat.wipText.getLocalBounds();
            stat.wipText.setOrigin(wipBounds.width / 2, wipBounds.height / 2);
            stat.wipText.setPosition(
                xPos + width / 2,
                yPos + height / 2
            );
        }
    }

    void initLeaderboardsScreen() {
        leaderboardsTitle.setString("Leaderboards");
        leaderboardsTitle.setFont(mainFont);
        leaderboardsTitle.setCharacterSize(60);
        leaderboardsTitle.setFillColor(sf::Color::White);
        leaderboardsTitle.setPosition(WIDTH / 2 - 200, 100);
        leaderboardsTitle.setPosition(WIDTH / 2 - leaderboardsTitle.getLocalBounds().width / 2, 50);

        backButton.setSize(sf::Vector2f(120, 40));
        backButton.setPosition(50, 50);
        backButton.setFillColor(sf::Color(80, 80, 80));
        backButton.setOutlineThickness(2);
        backButton.setOutlineColor(sf::Color(200, 200, 200));

        backButtonText.setString("Back");
        backButtonText.setFont(mainFont);
        backButtonText.setCharacterSize(24);
        backButtonText.setFillColor(sf::Color::White);
        backButtonText.setPosition(60, 55);


        // Параметры сетки
        const float blockWidth = 350;
        const float blockHeight = 160;
        const float horizontalSpacing = 40;
        const float verticalSpacing = 25;
        const float startY = 150;
        const int columns = 2;

        // Центрирование сетки
        float gridWidth = columns * blockWidth + (columns - 1) * 50;
        float startX = (WIDTH - gridWidth) / 2;

        gameStatCount = 5;
        for (int i = 0; i < gameStatCount; ++i) {
            int row = i / columns;
            int col = i % columns;

            float x = startX + col * (blockWidth + 50);
            float y = startY + row * (blockHeight + verticalSpacing);

            if (y + blockHeight > HEIGHT - 50) break;

            createGameStat(leaderboardGames[i],
                (i == 0) ? "Arkanoid" : "Coming Soon",
                x,
                y,
                blockWidth,
                blockHeight
            );
        }
    }


    void initDialogs() {

        exitDialog.background.setSize(sf::Vector2f(500, 200));
        exitDialog.background.setFillColor(sf::Color(70, 70, 70));
        exitDialog.background.setOutlineThickness(2);
        exitDialog.background.setOutlineColor(sf::Color(200, 200, 200));
        exitDialog.background.setPosition(WIDTH / 2 - 250, HEIGHT / 2 - 100);

        exitDialog.question.setString("Are you sure you want to exit?");
        exitDialog.question.setFont(mainFont);
        exitDialog.question.setCharacterSize(30);
        exitDialog.question.setFillColor(sf::Color::White);
        sf::FloatRect questionBounds = exitDialog.question.getLocalBounds();
        exitDialog.question.setPosition(
            exitDialog.background.getPosition().x + (500 - questionBounds.width) / 2,
            exitDialog.background.getPosition().y + 30
        );

        exitDialog.yesButton.setSize(sf::Vector2f(100, 40));
        exitDialog.yesButton.setPosition(exitDialog.background.getPosition().x + 100, HEIGHT / 2 + 20);
        exitDialog.yesButton.setFillColor(sf::Color(80, 80, 80));
        exitDialog.yesButton.setOutlineThickness(2);
        exitDialog.yesButton.setOutlineColor(sf::Color(200, 200, 200));

        exitDialog.yesText.setString("Yes");
        exitDialog.yesText.setFont(mainFont);
        exitDialog.yesText.setCharacterSize(24);
        exitDialog.yesText.setFillColor(sf::Color::White);
        exitDialog.yesText.setPosition(exitDialog.yesButton.getPosition().x + 35, HEIGHT / 2 + 30);

        exitDialog.noButton.setSize(sf::Vector2f(100, 40));
        exitDialog.noButton.setPosition(exitDialog.background.getPosition().x + 300, HEIGHT / 2 + 20);
        exitDialog.noButton.setFillColor(sf::Color(80, 80, 80));
        exitDialog.noButton.setOutlineThickness(2);
        exitDialog.noButton.setOutlineColor(sf::Color(200, 200, 200));

        exitDialog.noText.setString("No");
        exitDialog.noText.setFont(mainFont);
        exitDialog.noText.setCharacterSize(24);
        exitDialog.noText.setFillColor(sf::Color::White);
        exitDialog.noText.setPosition(exitDialog.noButton.getPosition().x + 35, HEIGHT / 2 + 30);


        logoutDialog.background.setSize(sf::Vector2f(500, 200));
        logoutDialog.background.setFillColor(sf::Color(70, 70, 70));
        logoutDialog.background.setOutlineThickness(2);
        logoutDialog.background.setOutlineColor(sf::Color(200, 200, 200));
        logoutDialog.background.setPosition(WIDTH / 2 - 250, HEIGHT / 2 - 100);

        logoutDialog.question.setString("Are you sure you want to logout?");
        logoutDialog.question.setFont(mainFont);
        logoutDialog.question.setCharacterSize(30);
        logoutDialog.question.setFillColor(sf::Color::White);
        questionBounds = logoutDialog.question.getLocalBounds();
        logoutDialog.question.setPosition(
            logoutDialog.background.getPosition().x + (500 - questionBounds.width) / 2,
            logoutDialog.background.getPosition().y + 30
        );

        logoutDialog.yesButton.setSize(sf::Vector2f(100, 40));
        logoutDialog.yesButton.setPosition(logoutDialog.background.getPosition().x + 100, HEIGHT / 2 + 20);
        logoutDialog.yesButton.setFillColor(sf::Color(80, 80, 80));
        logoutDialog.yesButton.setOutlineThickness(2);
        logoutDialog.yesButton.setOutlineColor(sf::Color(200, 200, 200));

        logoutDialog.yesText.setString("Yes");
        logoutDialog.yesText.setFont(mainFont);
        logoutDialog.yesText.setCharacterSize(24);
        logoutDialog.yesText.setFillColor(sf::Color::White);
        logoutDialog.yesText.setPosition(logoutDialog.yesButton.getPosition().x + 35, HEIGHT / 2 + 30);

        logoutDialog.noButton.setSize(sf::Vector2f(100, 40));
        logoutDialog.noButton.setPosition(logoutDialog.background.getPosition().x + 300, HEIGHT / 2 + 20);
        logoutDialog.noButton.setFillColor(sf::Color(80, 80, 80));
        logoutDialog.noButton.setOutlineThickness(2);
        logoutDialog.noButton.setOutlineColor(sf::Color(200, 200, 200));

        logoutDialog.noText.setString("No");
        logoutDialog.noText.setFont(mainFont);
        logoutDialog.noText.setCharacterSize(24);
        logoutDialog.noText.setFillColor(sf::Color::White);
        logoutDialog.noText.setPosition(logoutDialog.noButton.getPosition().x + 35, HEIGHT / 2 + 30);
    }
    void launchGame() {
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        std::wstring command = L"Resources\\arcanoid\\arcanoid.exe";
        std::wstring workingDir = L"Resources\\arcanoid";

        if (CreateProcessW(
            NULL,                   // Имя исполняемого файла (уже указано в command)
            &command[0],            // Командная строка
            NULL,                   // Атрибуты безопасности процесса
            NULL,                   // Атрибуты безопасности потока
            FALSE,                  // Наследование дескрипторов
            CREATE_NEW_CONSOLE,     // Флаги создания
            NULL,                   // Окружение
            workingDir.c_str(),     // Текущий каталог
            &si,                    // STARTUPINFO
            &pi                     // PROCESS_INFORMATION
        )) {

            // Ожидание завершения игры
            WaitForSingleObject(pi.hProcess, INFINITE);

            updateUserGameStats(0);

            // Получение кода завершения
            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);

            // Уведомление пользователя
            addNotification("Игра завершена. Код: " + std::to_string(exitCode));

            // Освобождение ресурсов
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
        else {
            // Обработка ошибки запуска
            DWORD errorCode = GetLastError();
            addNotification("Ошибка запуска: " + std::to_string(errorCode));
        }
    }
public:
    GameLauncher() : window(sf::VideoMode(WIDTH, HEIGHT), "Pixel Launcher") {
        if (!mainFont.loadFromFile(RES_PATH + "Fonts/PixelizerBold.ttf")) {
            std::cerr << "Error loading font!\n";
            exit(1);
        }

        if (!backgroundTex.loadFromFile(RES_PATH + "background.jpg")) {
            std::cerr << "Error loading background!\n";
            exit(2);
        }
        background.setTexture(backgroundTex);

        titleText.setString("Pixel Launcher");
        titleText.setFont(mainFont);
        titleText.setCharacterSize(80);
        titleText.setFillColor(sf::Color(200, 200, 200));
        sf::FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setOrigin(titleBounds.left + titleBounds.width / 2.0f,
            titleBounds.top + titleBounds.height / 2.0f);
        titleText.setPosition(WIDTH / 2, 150);

        copyrightText.setString("Test");
        copyrightText.setFont(mainFont);
        copyrightText.setCharacterSize(20);
        copyrightText.setFillColor(sf::Color(150, 150, 150, 200));
        copyrightText.setPosition(20, HEIGHT - 40);

        initWelcomeScreen();
        initLoginScreen();
        initProfileScreen();
        initProfileSettingsScreen();
        initGamesScreen();
        initLeaderboardsScreen();
        initDialogs();
    }

    void run() {
        sf::Clock clock;
        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                handleEvent(event);
            }

            float dt = clock.restart().asSeconds();
            update(dt);
            render();
        }
    }
private:
    void initWelcomeScreen() {
        welcomeText.setString("Welcome");
        welcomeText.setFont(mainFont);
        welcomeText.setCharacterSize(100);
        welcomeText.setFillColor(sf::Color(255, 255, 255, 255));
        sf::FloatRect bounds = welcomeText.getLocalBounds();
        welcomeText.setPosition(WIDTH / 2 - bounds.width / 2, HEIGHT / 3 - bounds.height / 2);
    }

    void initLoginScreen() {
        inputCount = 2;
        inputs[0] = TextInput(sf::Vector2f(400, 50), sf::Vector2f(WIDTH / 2 - 200, 300),
            mainFont, "Login");
        inputs[1] = TextInput(sf::Vector2f(400, 50), sf::Vector2f(WIDTH / 2 - 200, 380),
            mainFont, "Password");

        buttonCount = 2;
        createButton(0, "Login", sf::Vector2f(WIDTH / 2 - 100, 480), mainFont);
        createButton(1, "Register", sf::Vector2f(WIDTH / 2 - 100, 550), mainFont, 24);
    }

    void initRegisterScreen() {
        inputCount = 4;
        inputs[0] = TextInput(sf::Vector2f(400, 50), sf::Vector2f(WIDTH / 2 - 200, 250),
            mainFont, "Username");
        inputs[1] = TextInput(sf::Vector2f(400, 50), sf::Vector2f(WIDTH / 2 - 200, 330),
            mainFont, "Login");
        inputs[2] = TextInput(sf::Vector2f(400, 50), sf::Vector2f(WIDTH / 2 - 200, 410),
            mainFont, "Password");
        inputs[3] = TextInput(sf::Vector2f(400, 50), sf::Vector2f(WIDTH / 2 - 200, 490),
            mainFont, "Secret Word");

        buttonCount = 2;
        createButton(0, "< Back", sf::Vector2f(50, 50), mainFont, 24);
        createButton(1, "Register", sf::Vector2f(WIDTH / 2 - 100, 580), mainFont);
    }

    void initProfileScreen() {
        sf::Text tempText;
        tempText.setFont(mainFont);
        tempText.setCharacterSize(40);

        profileButtonCount = 4;

        tempText.setString("Games");
        sf::FloatRect textBounds = tempText.getLocalBounds();
        float buttonWidth = textBounds.width + 40;
        float xPos = (WIDTH - buttonWidth) / 2.0f;
        createProfileButton(0, "Games", sf::Vector2f(xPos, 200), mainFont, 40);

        tempText.setString("Leaderboards");
        textBounds = tempText.getLocalBounds();
        buttonWidth = textBounds.width + 40;
        xPos = (WIDTH - buttonWidth) / 2.0f;
        createProfileButton(1, "Leaderboards", sf::Vector2f(xPos, 300), mainFont, 40);

        tempText.setString("Exit");
        textBounds = tempText.getLocalBounds();
        buttonWidth = textBounds.width + 40;
        xPos = (WIDTH - buttonWidth) / 2.0f;
        createProfileButton(2, "Exit", sf::Vector2f(xPos, 400), mainFont, 40);

        tempText.setString("Logout");
        textBounds = tempText.getLocalBounds();
        buttonWidth = textBounds.width + 40;
        xPos = (WIDTH - buttonWidth) / 2.0f;
        createProfileButton(3, "Logout", sf::Vector2f(xPos, 500), mainFont, 40);
    }

    void handleEvent(sf::Event& event) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }

        for (int i = 0; i < inputCount; i++) {
            inputs[i].handleEvent(event, window);
        }

        if (currentState == AppState::Profile) {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            isAvatarHovered = avatarSprite.getGlobalBounds().contains(mousePos);

            if (event.type == sf::Event::MouseButtonReleased && isAvatarHovered) {
                currentState = AppState::ProfileSettings;
            }
        }

        if (currentState == AppState::ProfileSettings) {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            isSettingsBackHovered = settingsBackButton.getGlobalBounds().contains(mousePos);

            if (event.type == sf::Event::MouseButtonReleased &&
                settingsBackButton.getGlobalBounds().contains(mousePos)) {
                currentState = AppState::Profile;
            }
        }

        if (event.type == sf::Event::MouseButtonReleased) {
            sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

            for (int i = 0; i < buttonCount; i++) {
                if (buttons[i].shape.getGlobalBounds().contains(mousePos)) {
                    handleButtonClick(buttons[i].text.getString());
                }
            }

            if (currentState == AppState::Profile) {
                for (int i = 0; i < profileButtonCount; i++) {
                    if (profileButtons[i].shape.getGlobalBounds().contains(mousePos)) {
                        handleButtonClick(profileButtons[i].text.getString());
                    }
                }
            }

            if (currentState == AppState::Games) {
                if (arcanoidButton.getGlobalBounds().contains(mousePos)) {
                    addNotification("Arcanoid it's starting...");
                    launchGame();
                }
            }

            if ((currentState == AppState::Games || currentState == AppState::Leaderboards) &&
                backButton.getGlobalBounds().contains(mousePos)) {
                currentState = AppState::Profile;
            }

            if (currentState == AppState::ExitConfirm) {
                if (exitDialog.yesButton.getGlobalBounds().contains(mousePos)) {
                    window.close();
                }
                else if (exitDialog.noButton.getGlobalBounds().contains(mousePos)) {
                    currentState = AppState::Profile;
                }
            }
            else if (currentState == AppState::LogoutConfirm) {
                if (logoutDialog.yesButton.getGlobalBounds().contains(mousePos)) {
                    currentUser = nullptr;
                    currentState = AppState::Login;
                }
                else if (logoutDialog.noButton.getGlobalBounds().contains(mousePos)) {
                    currentState = AppState::Profile;
                }
            }
        }
    }

    void handleButtonClick(const sf::String& btnText) {
        if (currentState == AppState::Login) {
            if (btnText == "Login") {
                attemptLogin();
            }
            else if (btnText == "Register") {
                currentState = AppState::Register;
                initRegisterScreen();
            }
        }
        else if (currentState == AppState::Register) {
            if (btnText == "< Back") {
                currentState = AppState::Login;
                initLoginScreen();
            }
            else if (btnText == "Register") {
                attemptRegister();
            }
        }
        else if (currentState == AppState::Profile) {
            if (btnText == "Games") {
                currentState = AppState::Games;
            }
            else if (btnText == "Leaderboards") {
                currentState = AppState::Leaderboards;
            }
            else if (btnText == "Exit") {
                currentState = AppState::ExitConfirm;
            }
            else if (btnText == "Logout") {
                currentState = AppState::LogoutConfirm;
            }
        }
    }

    void addNotification(const std::string& message) {
        NotificationNode::Notification note;
        note.text.setString(message);
        note.text.setFont(mainFont);
        note.text.setCharacterSize(24);
        note.text.setFillColor(sf::Color(50, 200, 50));
        note.text.setPosition(20, 20);
        notifications.add(note);
    }

    void updateNotifications(float dt) {
        NotificationNode* current = notifications.head;
        while (current) {
            current->data.timer += dt;
            current->data.alpha = 255 * (1.0f - std::min(current->data.timer / 2.0f, 1.0f));
            current->data.text.setFillColor(sf::Color(50, 200, 50, static_cast<sf::Uint8>(current->data.alpha)));
            current = current->next;
        }
        notifications.removeOld();
    }

    void attemptLogin() {
        std::string login = inputs[0].getContent();
        std::string password = inputs[1].getContent();

        if (login.empty() || password.empty()) {
            inputs[0].setError(true, "Fill all fields!");
            return;
        }

        UserNode::User* user = authManager.loginUser(login, password);
        if (user) {
            currentUser = user;
            currentState = AppState::Profile;

            if (!avatarTexture.loadFromFile(RES_PATH + "Avatars/" + user->avatar)) {
                avatarTexture.loadFromFile(RES_PATH + "Avatars/default.png");
            }
            avatarSprite.setTexture(avatarTexture);
            avatarSprite.setScale(2.0f, 2.0f);
            avatarSprite.setPosition(
                WIDTH - AVATAR_RIGHT_MARGIN - avatarSprite.getGlobalBounds().width,
                AVATAR_TOP_MARGIN
            );

            addNotification("Login successful!");
        }
        else {
            inputs[0].setError(true, "Invalid credentials!");
        }
    }

    void attemptRegister() {
        for (int i = 0; i < inputCount; i++) {
            inputs[i].setError(false);
            if (inputs[i].getContent().empty()) {
                inputs[i].setError(true, "Field required!");
                return;
            }
        }

        if (authManager.loginExists(inputs[1].getContent())) {
            inputs[1].setError(true, "Login exists!");
            return;
        }

        UserNode::User newUser(
            inputs[0].getContent(),
            inputs[1].getContent(),
            inputs[2].getContent(),
            inputs[3].getContent(),
            "Icon1.png"
        );

        if (authManager.registerUser(newUser)) {
            addNotification("Registration success!");
            currentState = AppState::Login;
            initLoginScreen();
        }
    }

    void update(float dt) {
        if (currentState == AppState::Welcome) {
            if (fading) {
                welcomeAlpha -= 80 * dt;
                if (welcomeAlpha <= 0) {
                    fading = false;
                    currentState = AppState::Login;
                }
            }
            welcomeText.setFillColor(sf::Color(255, 255, 255,
                static_cast<sf::Uint8>(welcomeAlpha)));
        }

        float time = pulsationClock.getElapsedTime().asSeconds();
        pulsateScale = 1.0f + 0.1f * std::sin(time * 2.0f);
        float colorValue = 150 + 105 * std::sin(time * 3.0f);

        if (currentState == AppState::Login || currentState == AppState::Register || currentState == AppState::Profile) {
            titleText.setScale(pulsateScale, pulsateScale);
            titleText.setFillColor(sf::Color(colorValue, colorValue, colorValue));

            if (currentState == AppState::Profile) {
                avatarSprite.setScale(2.0f * pulsateScale, 2.0f * pulsateScale);
                avatarSprite.setPosition(
                    WIDTH - AVATAR_RIGHT_MARGIN - avatarSprite.getGlobalBounds().width,
                    AVATAR_TOP_MARGIN
                );
            }
        }
        else {
            titleText.setScale(1.0f, 1.0f);
            titleText.setFillColor(sf::Color(200, 200, 200));
        }

        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        for (int i = 0; i < buttonCount; i++) {
            buttons[i].isHovered = buttons[i].shape.getGlobalBounds().contains(mousePos);
            buttons[i].shape.setOutlineColor(buttons[i].isHovered ? sf::Color::White : sf::Color(200, 200, 200));
            buttons[i].text.setFillColor(buttons[i].isHovered ? sf::Color(50, 50, 50) : sf::Color::White);
            buttons[i].shape.setFillColor(buttons[i].isHovered ? sf::Color(200, 200, 200, 50) : sf::Color::Transparent);
        }

        if (currentState == AppState::Profile) {
            for (int i = 0; i < profileButtonCount; i++) {
                profileButtons[i].isHovered = profileButtons[i].shape.getGlobalBounds().contains(mousePos);
                profileButtons[i].shape.setOutlineColor(profileButtons[i].isHovered ? sf::Color::White : sf::Color(200, 200, 200));
                profileButtons[i].text.setFillColor(profileButtons[i].isHovered ? sf::Color(50, 50, 50) : sf::Color::White);
                profileButtons[i].shape.setFillColor(profileButtons[i].isHovered ? sf::Color(200, 200, 200, 50) : sf::Color::Transparent);
            }
        }

        if (currentState == AppState::Games || currentState == AppState::Leaderboards) {
            bool backHovered = backButton.getGlobalBounds().contains(mousePos);
            backButton.setOutlineColor(backHovered ? sf::Color::White : sf::Color(200, 200, 200));
            backButton.setFillColor(backHovered ? sf::Color(200, 200, 200, 50) : sf::Color(80, 80, 80));
        }

        if (currentState == AppState::Games) {
            bool arcanoidHovered = arcanoidButton.getGlobalBounds().contains(mousePos);
            arcanoidButton.setOutlineColor(arcanoidHovered ? sf::Color::White : sf::Color(200, 200, 200));
            arcanoidButton.setFillColor(arcanoidHovered ? sf::Color(200, 200, 200, 50) : sf::Color(80, 80, 80));
        }

        if (currentState == AppState::ExitConfirm || currentState == AppState::LogoutConfirm) {
            Dialog& currentDialog = (currentState == AppState::ExitConfirm) ? exitDialog : logoutDialog;
            bool yesHovered = currentDialog.yesButton.getGlobalBounds().contains(mousePos);
            bool noHovered = currentDialog.noButton.getGlobalBounds().contains(mousePos);

            currentDialog.yesButton.setOutlineColor(yesHovered ? sf::Color::White : sf::Color(200, 200, 200));
            currentDialog.yesButton.setFillColor(yesHovered ? sf::Color(200, 200, 200, 50) : sf::Color(80, 80, 80));
            currentDialog.noButton.setOutlineColor(noHovered ? sf::Color::White : sf::Color(200, 200, 200));
            currentDialog.noButton.setFillColor(noHovered ? sf::Color(200, 200, 200, 50) : sf::Color(80, 80, 80));
        }

        for (int i = 0; i < inputCount; i++) {
            inputs[i].updateCursor(dt);
        }

        updateNotifications(dt);
    }

    void updateUserGameStats(int gameIndex) {
        if (!currentUser) return;

        std::vector<std::string> lines;
        std::ifstream inFile(UNIQUE_USERS_FILE);
        std::string line;
        bool found = false;

        // Чтение и обновление данных
        while (std::getline(inFile, line)) {
            if (line.find(currentUser->login + DELIMITER) == 0) {
                std::vector<std::string> parts = split(line, DELIMITER);
                if (parts.size() == 6) {
                    parts[gameIndex + 1] = "1";
                    line = join(parts, DELIMITER);
                    found = true;
                }
            }
            lines.push_back(line);
        }

        // Если пользователь новый
        if (!found) {
            std::string newEntry = currentUser->login + "|0|0|0|0|0";
            size_t pos = newEntry.find('|', 0);
            for (int i = 0; i < gameIndex; i++) {
                pos = newEntry.find('|', pos + 1);
            }
            newEntry[pos + 1] = '1';
            lines.push_back(newEntry);
        }

        // Запись обратно в файл
        std::ofstream outFile(UNIQUE_USERS_FILE);
        for (auto& l : lines) {
            outFile << l << "\n";
        }
    }

    void updateLeaderboardsStats() {
        // Обновление лучшего счета для арканоида
        std::ifstream scoreFile(BEST_SCORE_PATH);
        if (scoreFile) {
            std::string bestScore;
            std::getline(scoreFile, bestScore);
            leaderboardGames[0].bestScore.setString("Best Score: " + bestScore);
        }
        else {
            leaderboardGames[0].bestScore.setString("Best Score: N/A");
        }

        // Подсчет уникальных игроков для арканоида
        std::ifstream usersFile(UNIQUE_USERS_FILE);
        if (!usersFile) return;

        int arkanoidPlayers = 0;
        std::string line;
        while (std::getline(usersFile, line)) {
            std::vector<std::string> parts = split(line, DELIMITER);
            if (parts.size() == 6 && parts[1] == "1") {
                arkanoidPlayers++;
            }
        }
        leaderboardGames[0].playersCount.setString("Players: " + std::to_string(arkanoidPlayers));
    }

    void render() {
        window.clear();
        window.draw(background);
        window.draw(copyrightText);

        switch (currentState) {
        case AppState::Welcome:
            window.draw(welcomeText);
            break;

        case AppState::Login:
        case AppState::Register:
            titleText.setPosition(WIDTH / 2, 100);
            window.draw(titleText);
            for (int i = 0; i < inputCount; i++) {
                inputs[i].draw(window);
            }
            for (int i = 0; i < buttonCount; i++) {
                window.draw(buttons[i].shape);
                window.draw(buttons[i].text);
            }
            break;

        case AppState::Profile:
            titleText.setPosition(WIDTH / 2, 80);
            window.draw(titleText);
            window.draw(avatarSprite);

            if (isAvatarHovered) {
                sf::FloatRect avatarBounds = avatarSprite.getGlobalBounds();
                sf::RectangleShape hoverEffect(sf::Vector2f(
                    avatarBounds.width,
                    avatarBounds.height
                ));
                hoverEffect.setPosition(avatarBounds.left, avatarBounds.top);
                hoverEffect.setFillColor(sf::Color::Transparent);
                hoverEffect.setOutlineColor(sf::Color::White);
                hoverEffect.setOutlineThickness(2 * pulsateScale);
                window.draw(hoverEffect);
            }

            for (int i = 0; i < profileButtonCount; i++) {
                window.draw(profileButtons[i].shape);
                window.draw(profileButtons[i].text);
            }
            break;

        case AppState::ProfileSettings:
            window.draw(settingsBackButton);
            window.draw(settingsBackText);
            break;

        case AppState::Games:
            window.draw(gamesTitle);
            window.draw(backButton);
            window.draw(backButtonText);
            window.draw(arcanoidButton);
            window.draw(arcanoidButtonText);
            break;

        case AppState::Leaderboards:
            window.draw(leaderboardsTitle);
            window.draw(backButton);
            window.draw(backButtonText);

            updateLeaderboardsStats();       // Обновляем данные статистики

            for (int i = 0; i < gameStatCount; i++) {
                auto& gameStat = leaderboardGames[i];
                window.draw(gameStat.background);
                window.draw(gameStat.gameTitle);
                if (gameStat.gameTitle.getString() == "Arkanoid") {
                    window.draw(gameStat.bestScore);
                    window.draw(gameStat.playersCount);
                }
                else {
                    window.draw(gameStat.wipText);
                }
            }
            break;

        case AppState::ExitConfirm:
        case AppState::LogoutConfirm:
            Dialog& currentDialog = (currentState == AppState::ExitConfirm) ? exitDialog : logoutDialog;
            window.draw(currentDialog.background);
            window.draw(currentDialog.question);
            window.draw(currentDialog.yesButton);
            window.draw(currentDialog.yesText);
            window.draw(currentDialog.noButton);
            window.draw(currentDialog.noText);
            break;
        }

        NotificationNode* current = notifications.head;
        while (current) {
            window.draw(current->data.text);
            current = current->next;
        }

        window.display();
    }
};

int main() {
    GameLauncher launcher;
    launcher.run();
    return 0;
}