#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm> 
#include <random>    

enum class GameState { MainMenu, Playing, Leaderboard, Settings, News, Achievements, GameOver };
enum Suit { SPADES = 0, CLUBS, HEARTS, DIAMONDS };
enum Rank { SIX = 6, SEVEN, EIGHT, NINE, TEN, JACK, QUEEN, KING, ACE = 14 };

sf::Texture cardSheetTexture;
sf::Texture cardBackTexture;

// --- КЛАС КАРТИ ---
class CardVisual {
public:
    Suit suit; Rank rank; sf::Sprite sprite;

    CardVisual(Suit s, Rank r) : suit(s), rank(r) { showBack(); }

    void showFace() {
        sprite.setTexture(cardSheetTexture);
        int cardW = cardSheetTexture.getSize().x / 13;
        int cardH = cardSheetTexture.getSize().y / 4;
        int rowIndex = static_cast<int>(suit);
        int colIndex = 0;
        switch (rank) {
        case ACE: colIndex = 0; break; case KING: colIndex = 1; break; case QUEEN: colIndex = 2; break;
        case JACK: colIndex = 3; break; case TEN: colIndex = 4; break; case NINE: colIndex = 5; break;
        case EIGHT: colIndex = 6; break; case SEVEN: colIndex = 7; break; case SIX: colIndex = 8; break;
        }
        sprite.setTextureRect(sf::IntRect(colIndex * cardW, rowIndex * cardH, cardW, cardH));
        sf::FloatRect bounds = sprite.getLocalBounds();
        sprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        float targetHeight = 120.f;
        float scale = targetHeight / cardH;
        sprite.setScale(scale, scale);
    }

    void showBack() {
        sprite.setTexture(cardBackTexture);
        float targetHeight = 120.f;
        float scale = targetHeight / cardBackTexture.getSize().y;
        sprite.setOrigin(cardBackTexture.getSize().x / 2.f, cardBackTexture.getSize().y / 2.f);
        sprite.setScale(scale, scale);
    }
};

// --- МЕНЮ ТА КНОПКИ (ПОВЕРНУТО НА АНГЛІЙСЬКУ) ---
struct MenuResources {
    sf::Texture background, title;
    sf::Font font;
};

class SimpleButton {
public:
    sf::RectangleShape rect; sf::Text text; bool isHovered = false;

    // Повернули std::string для надійності
    SimpleButton(sf::Vector2f position, std::string labelText, sf::Font& font, sf::Vector2f size = sf::Vector2f(250.f, 50.f)) {
        rect.setSize(size); rect.setOrigin(size.x / 2.f, size.y / 2.f); rect.setPosition(position);
        rect.setFillColor(sf::Color(50, 50, 50, 200)); rect.setOutlineThickness(3.f); rect.setOutlineColor(sf::Color(200, 150, 50));
        text.setFont(font); text.setString(labelText); text.setCharacterSize(24); text.setFillColor(sf::Color::White);
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top + textBounds.height / 2.0f);
        text.setPosition(position);
    }
    void update(sf::Vector2i mousePos) {
        if (rect.getGlobalBounds().contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
            if (!isHovered) { isHovered = true; rect.setFillColor(sf::Color(100, 100, 100, 255)); }
        }
        else {
            if (isHovered) { isHovered = false; rect.setFillColor(sf::Color(50, 50, 50, 200)); }
        }
    }
    bool isClicked(sf::Event event) { return (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left && isHovered); }
    void draw(sf::RenderWindow& window) { window.draw(rect); window.draw(text); }
};

bool loadMenuResources(MenuResources& res) {
    res.background.loadFromFile("C:/SFML/menu_bg.png");
    res.title.loadFromFile("C:/SFML/title_durak.png");
    res.font.loadFromFile("C:/SFML/arial.ttf");
    return true;
}

// --- ГОЛОВНИЙ КЛАС ГРИ ---
class GameEngine {
public:
    std::vector<CardVisual> deck;
    std::vector<CardVisual> playerHand;
    std::vector<CardVisual> oppHand;
    std::vector<CardVisual> tableCards;

    CardVisual* trumpCardPtr = nullptr;
    Suit currentTrumpSuit;

    bool isPlayerAttacking = true;
    bool isPlayerAction = true;
    std::string gameOverMsg = "";

    GameEngine() {
        cardSheetTexture.loadFromFile("C:/SFML/card_sheet.png");
        cardBackTexture.loadFromFile("C:/SFML/card_back.png");
    }

    void prepareNewGame() {
        deck.clear(); playerHand.clear(); oppHand.clear(); tableCards.clear();
        gameOverMsg = "";
        isPlayerAttacking = true;
        isPlayerAction = true;

        for (int s = SPADES; s <= DIAMONDS; ++s) {
            for (int r = SIX; r <= ACE; ++r) {
                deck.emplace_back(static_cast<Suit>(s), static_cast<Rank>(r));
            }
        }
        std::random_device rd; std::mt19937 g(rd());
        std::shuffle(deck.begin(), deck.end(), g);

        for (int i = 0; i < 6; ++i) {
            CardVisual pCard = deck.back(); deck.pop_back(); pCard.showFace(); playerHand.push_back(pCard);
            CardVisual oCard = deck.back(); deck.pop_back(); oCard.showBack(); oppHand.push_back(oCard);
        }

        trumpCardPtr = &deck.front();
        currentTrumpSuit = trumpCardPtr->suit;
        trumpCardPtr->showFace();
    }

    bool canBeat(const CardVisual& attackCard, const CardVisual& defenseCard) {
        if (attackCard.suit == defenseCard.suit) return defenseCard.rank > attackCard.rank;
        else if (defenseCard.suit == currentTrumpSuit) return true;
        return false;
    }

    bool canAddCard(const CardVisual& card) {
        if (tableCards.empty()) return true;
        for (const auto& c : tableCards) {
            if (c.rank == card.rank) return true;
        }
        return false;
    }

    bool isBetterToPlay(const CardVisual& c1, const CardVisual& c2) {
        bool c1Trump = (c1.suit == currentTrumpSuit);
        bool c2Trump = (c2.suit == currentTrumpSuit);
        if (c1Trump && !c2Trump) return false;
        if (!c1Trump && c2Trump) return true;
        return c1.rank < c2.rank;
    }

    void endRound(bool defenderTook) {
        if (defenderTook) {
            if (isPlayerAttacking) {
                for (auto& c : tableCards) { c.showBack(); oppHand.push_back(c); }
            }
            else {
                for (auto& c : tableCards) { c.showFace(); playerHand.push_back(c); }
            }
        }
        tableCards.clear();

        auto drawToSix = [&](std::vector<CardVisual>& hand, bool isPlayer) {
            while (hand.size() < 6 && !deck.empty()) {
                CardVisual c = deck.back(); deck.pop_back();
                if (isPlayer) c.showFace(); else c.showBack();
                hand.push_back(c);
            }
            };

        if (isPlayerAttacking) {
            drawToSix(playerHand, true); drawToSix(oppHand, false);
        }
        else {
            drawToSix(oppHand, false); drawToSix(playerHand, true);
        }

        if (defenderTook) {
            isPlayerAction = isPlayerAttacking;
        }
        else {
            isPlayerAttacking = !isPlayerAttacking;
            isPlayerAction = isPlayerAttacking;
        }

        checkWinCondition();
    }

    void checkWinCondition() {
        if (deck.empty()) {
            if (playerHand.empty() && oppHand.empty()) gameOverMsg = "DRAW!";
            else if (playerHand.empty()) gameOverMsg = "YOU WIN!";
            else if (oppHand.empty()) gameOverMsg = "YOU LOSE!";
        }
    }

    void executeBotAction() {
        if (gameOverMsg != "") return;

        if (!isPlayerAttacking) {
            if (tableCards.empty() || tableCards.size() % 2 == 0) {
                int bestIdx = -1;
                for (size_t i = 0; i < oppHand.size(); ++i) {
                    if (canAddCard(oppHand[i])) {
                        if (bestIdx == -1 || isBetterToPlay(oppHand[i], oppHand[bestIdx])) {
                            bestIdx = i;
                        }
                    }
                }
                if (bestIdx != -1) {
                    // Бот перевертає карту перед тим, як покласти на стіл!
                    CardVisual playedCard = oppHand[bestIdx];
                    playedCard.showFace();

                    tableCards.push_back(playedCard);
                    oppHand.erase(oppHand.begin() + bestIdx);
                    isPlayerAction = true;
                }
                else {
                    endRound(false);
                }
            }
        }
        else {
            if (tableCards.size() % 2 != 0) {
                CardVisual cardToBeat = tableCards.back();
                int bestIdx = -1;
                for (size_t i = 0; i < oppHand.size(); ++i) {
                    if (canBeat(cardToBeat, oppHand[i])) {
                        if (bestIdx == -1 || isBetterToPlay(oppHand[i], oppHand[bestIdx])) {
                            bestIdx = i;
                        }
                    }
                }
                if (bestIdx != -1) {
                    // Бот перевертає карту захисту!
                    CardVisual playedCard = oppHand[bestIdx];
                    playedCard.showFace();

                    tableCards.push_back(playedCard);
                    oppHand.erase(oppHand.begin() + bestIdx);
                    isPlayerAction = true;
                }
                else {
                    endRound(true);
                }
            }
        }
    }
};


int main() {
    sf::RenderWindow window(sf::VideoMode(1000, 700), "DURAK Game");
    window.setFramerateLimit(60);

    GameEngine game;
    MenuResources menuRes;
    loadMenuResources(menuRes);

    sf::Sprite bgSprite(menuRes.background);
    sf::Sprite titleSprite(menuRes.title);
    titleSprite.setPosition(150.f, 150.f);

    std::vector<SimpleButton> menuButtons;
    float startX = 250.f; float startY = 300.f;

    // Всі кнопки англійською для безпеки
    menuButtons.emplace_back(sf::Vector2f(startX, startY), "Play", menuRes.font);
    menuButtons.emplace_back(sf::Vector2f(startX, startY + 70.f), "Leaderboard", menuRes.font);
    menuButtons.emplace_back(sf::Vector2f(startX, startY + 140.f), "Settings", menuRes.font);
    menuButtons.emplace_back(sf::Vector2f(startX, startY + 210.f), "News", menuRes.font);
    menuButtons.emplace_back(sf::Vector2f(startX, startY + 280.f), "Achievements", menuRes.font);

    // Ігрові кнопки англійською
    SimpleButton btnPass(sf::Vector2f(850.f, 300.f), "PASS", menuRes.font, sf::Vector2f(180.f, 50.f));
    SimpleButton btnTake(sf::Vector2f(850.f, 400.f), "TAKE", menuRes.font, sf::Vector2f(180.f, 50.f));

    sf::RectangleShape greenTable(sf::Vector2f(800.f, 400.f));
    greenTable.setFillColor(sf::Color(34, 139, 34));
    greenTable.setOutlineThickness(5.f); greenTable.setOutlineColor(sf::Color::White);
    greenTable.setOrigin(400.f, 200.f); greenTable.setPosition(500.f, 350.f);

    GameState currentState = GameState::MainMenu;
    sf::Text escText; escText.setFont(menuRes.font); escText.setString("Press ESC to return to Menu");
    escText.setCharacterSize(20); escText.setFillColor(sf::Color::Yellow); escText.setPosition(20.f, 20.f);

    sf::Text infoText; infoText.setFont(menuRes.font); infoText.setCharacterSize(24); infoText.setFillColor(sf::Color::White);
    infoText.setPosition(500.f, 120.f);

    sf::Text endText; endText.setFont(menuRes.font); endText.setCharacterSize(80); endText.setFillColor(sf::Color::Red);

    sf::Clock botTimer;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                if (currentState != GameState::MainMenu) currentState = GameState::MainMenu;
            }

            if (currentState == GameState::MainMenu) {
                if (menuButtons[0].isClicked(event)) { game.prepareNewGame(); currentState = GameState::Playing; }
                else if (menuButtons[1].isClicked(event)) { currentState = GameState::Leaderboard; }
                else if (menuButtons[2].isClicked(event)) { currentState = GameState::Settings; }
                else if (menuButtons[3].isClicked(event)) { currentState = GameState::News; }
                else if (menuButtons[4].isClicked(event)) { currentState = GameState::Achievements; }
            }
            else if (currentState == GameState::Playing && game.gameOverMsg == "") {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));

                    if (btnPass.isClicked(event) && game.isPlayerAttacking && game.isPlayerAction && game.tableCards.size() % 2 == 0 && !game.tableCards.empty()) {
                        game.endRound(false);
                        botTimer.restart();
                    }
                    if (btnTake.isClicked(event) && !game.isPlayerAttacking && game.isPlayerAction) {
                        game.endRound(true);
                        botTimer.restart();
                    }

                    if (game.isPlayerAction) {
                        for (int i = game.playerHand.size() - 1; i >= 0; --i) {
                            if (game.playerHand[i].sprite.getGlobalBounds().contains(mousePos)) {
                                CardVisual selectedCard = game.playerHand[i];

                                if (game.isPlayerAttacking) {
                                    if (game.canAddCard(selectedCard)) {
                                        game.tableCards.push_back(selectedCard); game.playerHand.erase(game.playerHand.begin() + i);
                                        game.isPlayerAction = false; botTimer.restart();
                                    }
                                }
                                else {
                                    if (game.tableCards.size() % 2 != 0) {
                                        if (game.canBeat(game.tableCards.back(), selectedCard)) {
                                            game.tableCards.push_back(selectedCard); game.playerHand.erase(game.playerHand.begin() + i);
                                            game.isPlayerAction = false; botTimer.restart();
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (game.gameOverMsg != "" && currentState == GameState::Playing) {
            currentState = GameState::GameOver;
        }

        if (currentState == GameState::Playing && !game.isPlayerAction && game.gameOverMsg == "") {
            if (botTimer.getElapsedTime().asSeconds() > 1.0f) {
                game.executeBotAction();
                botTimer.restart();
            }
        }

        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        if (currentState == GameState::MainMenu) {
            for (auto& btn : menuButtons) btn.update(mousePos);
        }
        else if (currentState == GameState::Playing) {
            btnPass.update(mousePos); btnTake.update(mousePos);
            if (game.isPlayerAction) infoText.setString("YOUR TURN!"); else infoText.setString("BOT IS THINKING...");
            sf::FloatRect bounds = infoText.getLocalBounds(); infoText.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        }

        window.clear(sf::Color(30, 30, 30));

        if (currentState == GameState::MainMenu) {
            window.draw(bgSprite); window.draw(titleSprite);
            for (auto& btn : menuButtons) btn.draw(window);
        }
        else if (currentState == GameState::Playing || currentState == GameState::GameOver) {
            window.draw(greenTable);
            window.draw(escText);
            if (currentState == GameState::Playing) window.draw(infoText);

            if (game.isPlayerAction && currentState == GameState::Playing) {
                if (game.isPlayerAttacking && game.tableCards.size() % 2 == 0 && !game.tableCards.empty()) btnPass.draw(window);
                if (!game.isPlayerAttacking) btnTake.draw(window);
            }

            float leftEdgeX = 150.f; float centerY = 350.f;
            if (game.trumpCardPtr && !game.deck.empty()) {
                sf::Sprite& ts = game.trumpCardPtr->sprite; ts.setPosition(leftEdgeX + 30.f, centerY); ts.setRotation(90.f); window.draw(ts);
            }
            if (!game.deck.empty()) {
                sf::Sprite deckSprite(cardBackTexture);
                float scale = 120.f / cardBackTexture.getSize().y;
                deckSprite.setOrigin(cardBackTexture.getSize().x / 2.f, cardBackTexture.getSize().y / 2.f);
                deckSprite.setScale(scale, scale); deckSprite.setPosition(leftEdgeX, centerY); window.draw(deckSprite);

                sf::Text deckSizeText; deckSizeText.setFont(menuRes.font); deckSizeText.setString(std::to_string(game.deck.size()));
                deckSizeText.setCharacterSize(24); deckSizeText.setFillColor(sf::Color::White); deckSizeText.setPosition(leftEdgeX - 10.f, centerY - 80.f);
                window.draw(deckSizeText);
            }

            if (!game.oppHand.empty()) {
                float oppTotalWidth = (game.oppHand.size() - 1) * 60.f; float oppStartX = 500.f - (oppTotalWidth / 2.f);
                for (size_t i = 0; i < game.oppHand.size(); ++i) {
                    sf::Sprite& s = game.oppHand[i].sprite; s.setPosition(oppStartX + (i * 60.f), 200.f); s.setRotation(0.f); window.draw(s);
                }
            }
            if (!game.playerHand.empty()) {
                float playerTotalWidth = (game.playerHand.size() - 1) * 60.f; float playerStartX = 500.f - (playerTotalWidth / 2.f);
                for (size_t i = 0; i < game.playerHand.size(); ++i) {
                    sf::Sprite& s = game.playerHand[i].sprite; s.setPosition(playerStartX + (i * 60.f), 500.f); s.setRotation(0.f); window.draw(s);
                }
            }
            if (!game.tableCards.empty()) {
                int pairsCount = (game.tableCards.size() + 1) / 2; float tableStartX = 500.f - (pairsCount * 60.f);
                for (size_t i = 0; i < game.tableCards.size(); ++i) {
                    sf::Sprite& s = game.tableCards[i].sprite; int pairIndex = i / 2; bool isDefense = (i % 2 != 0);
                    float x = tableStartX + (pairIndex * 120.f); float y = 350.f;
                    if (isDefense) { x += 15.f; y += 15.f; }
                    s.setPosition(x, y); s.setRotation(0.f); window.draw(s);
                }
            }

            if (currentState == GameState::GameOver) {
                sf::RectangleShape overlay(sf::Vector2f(1000.f, 700.f)); overlay.setFillColor(sf::Color(0, 0, 0, 200)); window.draw(overlay);
                endText.setString(game.gameOverMsg);
                sf::FloatRect eb = endText.getLocalBounds(); endText.setOrigin(eb.width / 2.f, eb.height / 2.f); endText.setPosition(500.f, 350.f);
                window.draw(endText); window.draw(escText);
            }
        }
        else {
            window.draw(bgSprite); window.draw(escText);
            sf::Text placeholderText; placeholderText.setFont(menuRes.font); placeholderText.setCharacterSize(40); placeholderText.setFillColor(sf::Color::White);
            std::string screenName = "";
            if (currentState == GameState::Leaderboard) screenName = "LEADERBOARD\n\n1. Player777 - 5000 pts\n2. AnimeFan - 4200 pts\n3. You - 0 pts";
            else if (currentState == GameState::Settings) screenName = "SETTINGS\n\nMusic: ON\nSound: ON\nDifficulty: Normal";
            else if (currentState == GameState::News) screenName = "NEWS\n\nVersion 1.0 Released!\nEnjoy the new anime cards.";
            else if (currentState == GameState::Achievements) screenName = "ACHIEVEMENTS\n\n[Locked] Win 1 game\n[Locked] Beat the AI\n[Locked] Flawless victory";
            placeholderText.setString(screenName);
            sf::FloatRect textBounds = placeholderText.getLocalBounds(); placeholderText.setOrigin(textBounds.width / 2.f, textBounds.height / 2.f);
            placeholderText.setPosition(500.f, 350.f); window.draw(placeholderText);
        }

        window.display();
    }
    return 0;
}