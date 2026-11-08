#include <iostream>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
using namespace std;

const int SIZE = 10;
const int TARGET = 5;       // n: цель по очкам.
const int STEP_MS = 400;    // Время между шагами, мс. Больше — медленнее.
struct Point { int x, y; };
Point snake[2][100];        // snake[игрок][часть тела], элемент 0 — голова.
Point direction[2] = {{1, 0}, {1, 0}};
Point food;
int length[2] = {1, 1}, score[2] = {0, 0};
int players = 1, active = 0, movesLeft = 10;
string result;
termios oldTerminal;

// Служебный код Linux: восстановить терминал при выходе.
void restoreTerminal() {
    tcsetattr(0, TCSANOW, &oldTerminal);
    cout << "\033[0m\033[?25h\033[?1049l" << flush;
}

// Ждём клавишу не дольше timeout мс. -1 — ждать без ограничения.
int readKey(int timeout = -1) {
    pollfd input = {0, POLLIN, 0};
    if (poll(&input, 1, timeout) == 0) return 0;
    unsigned char key;
    if (read(0, &key, 1) != 1 || key == 3 || key == 4) return 'q';
    if (key >= 'A' && key <= 'Z') key += 'a' - 'A';
    return key;
}

bool same(Point a, Point b) { return a.x == b.x && a.y == b.y; }

bool onSnake(int player, Point cell) {
    for (int i = 0; i < length[player]; i++)
        if (same(snake[player][i], cell)) return true;
    return false;
}

// Собираем свободные клетки в массив и выбираем случайную.
bool newFood() {
    Point freeCells[100];
    int count = 0;
    for (int y = 0; y < SIZE; y++) {
        for (int x = 0; x < SIZE; x++) {
            Point cell = {x, y};
            if (!onSnake(0, cell) && (players == 1 || !onSnake(1, cell)))
                freeCells[count++] = cell;
        }
    }
    if (count == 0) return false;
    food = freeCells[rand() % count];
    return true;
}

void draw(bool moving) {
    cout << "\033[H\033[J\n  \033[1;32m"
         << (players == 1 ? "Режим одиночной игры" : "Два игрока") << "\033[0m\n\n";
    if (players == 1) cout << "  Счёт: " << score[0];
    else {
        cout << "  Игрок 1: " << score[0] << "    Игрок 2: " << score[1] << "    Цель: " << TARGET;
        cout << "\n  Ходит игрок " << active + 1 << "    Осталось шагов: " << movesLeft;
    }
    cout << "\n\n  +--------------------+\n";
    for (int y = 0; y < SIZE; y++) {
        cout << "  |";
        for (int x = 0; x < SIZE; x++) {
            Point cell = {x, y};
            bool first = onSnake(0, cell);
            bool second = players == 2 && onSnake(1, cell);
            if (first && second) cout << "\033[1;35mX \033[0m";
            else if (first || second) {
                int p = first ? 0 : 1;
                char symbol = 'o';
                if (same(snake[p][length[p] - 1], cell)) symbol = '+';
                if (same(snake[p][0], cell)) symbol = '@'; // При длине 1 показываем голову.
                cout << (p == 0 ? "\033[1;32m" : "\033[1;36m")
                     << symbol << " \033[0m";
            } else if (same(cell, food)) cout << "\033[1;31m* \033[0m";
            else cout << "\033[90m. \033[0m";
        }
        cout << "|\n";
    }
    cout << "  +--------------------+\n\n"
         << "  @ — голова   o — тело   + — конец хвоста   * — еда\n"
         << "  WASD — направление   H — справка   P — пауза   Q — выход\n"
         << (moving ? "  Змейка движется сама.\n" : "  Нажмите WASD, чтобы начать движение.\n") << flush;
}

void help() {
    cout << "\033[H\033[J\n  "
         << (players == 1 ? "Режим одиночной игры" : "Два игрока")
         << "\n\n  ПРАВИЛА\n\n"
         << "  Поле 10x10. Старт в центре, длина 1.\n"
         << "  Ешьте *: +1 очко и +1 к длине.\n"
         << "  @ — голова, o — тело, + — последняя клетка хвоста.\n"
         << "  Стена или свой хвост заканчивают игру.\n"
         << "  WASD задаёт направление, дальше змейка ползёт сама.\n"
         << "  При длине больше 1 нельзя развернуться назад.\n\n";
    if (players == 1) {
        cout << "  Цель — собрать как можно больше еды и набрать больше очков.\n"
             << "  Ограничения по очкам и смены игроков нет.\n"
             << "  Игра заканчивается при столкновении или заполнении поля.\n"
             << "  В конце выводится ваш итоговый счёт.\n\n";
    } else {
        cout << "  У каждого игрока своя змейка и свой счёт, поле и еда общие.\n"
         << "  Один ход — перемещение змейки на следующую соседнюю клетку.\n"
         << "  Каждый игрок делает 10 автоматических шагов,\n"
         << "  затем очередь переходит другому. Передача хода — с остановкой.\n"
         << "  Следующий игрок нажимает WASD, когда готов.\n"
         << "  Чужую змейку пересекать можно, пересечение отмечено X.\n"
         << "  Игра заканчивается при столкновении или " << TARGET << " очках.\n"
         << "  Если свободных клеток для еды нет, игра тоже заканчивается.\n"
         << "  Побеждает больший счёт; равный счёт — ничья.\n\n";
    }
    cout << "  Английская раскладка: W вверх, A влево, S вниз, D вправо.\n"
         << "  H — справка, P — пауза, Q / Ctrl+C — выход.\n"
         << "  Справка и пауза останавливают игру.\n\n"
         << "  Enter — продолжить, Q — выйти.\n" << flush;
}

bool waitEnter() {
    while (true) {
        int key = readKey();
        if (key == 'q') return false;
        if (key == '\n' || key == '\r') return true;
    }
}

// Один автоматический шаг. false — партия окончена.
bool moveSnake(Point nextDirection) {
    int p = active;
    Point head = {snake[p][0].x + nextDirection.x, snake[p][0].y + nextDirection.y};
    if (head.x < 0 || head.x >= SIZE || head.y < 0 || head.y >= SIZE) {
        result = "Столкновение со стеной.";
        return false;
    }
    bool eats = same(head, food);
    // Без еды последняя клетка хвоста одновременно освобождается.
    for (int i = 0; i < length[p] - (eats ? 0 : 1); i++) {
        if (same(head, snake[p][i])) {
            result = "Столкновение со своим хвостом.";
            return false;
        }
    }
    // Другую змейку не проверяем: вариант 11 разрешает пересечения.
    if (eats) { length[p]++; score[p]++; }
    for (int i = length[p] - 1; i > 0; i--) snake[p][i] = snake[p][i - 1];
    snake[p][0] = head;
    direction[p] = nextDirection;
    if (eats) {
        if (players == 2 && score[p] >= TARGET) {
            result = "Набрана цель по очкам.";
            return false;
        }
        if (!newFood()) {
            result = "Поле заполнено.";
            return false;
        }
    }
    if (players == 2 && --movesLeft == 0) {
        active = 1 - active;
        movesLeft = 10;
    }
    return true;
}

int main() {
    if (tcgetattr(0, &oldTerminal) != 0) {
        cerr << "Запустите игру в терминале WSL.\n";
        return 1;
    }
    termios settings = oldTerminal;
    settings.c_lflag &= ~(ICANON | ECHO | ISIG);
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &settings) != 0) return 1;
    atexit(restoreTerminal);
    cout << "\033[?1049h\033[?25l\033[H\033[J\n  ЗМЕЙКА\n\n"
         << "  1 — Режим одиночной игры\n  2 — Два игрока\n  Q — выход\n" << flush;
    int key;
    do { key = readKey(); } while (key != '1' && key != '2' && key != 'q');
    if (key == 'q') return 0;
    players = key - '0';
    help();
    if (!waitEnter()) return 0;
    srand(time(nullptr));
    for (int p = 0; p < players; p++) snake[p][0] = {5, 5};
    newFood();

    bool moving = false;
    Point nextDirection = direction[active];
    using Clock = chrono::steady_clock;
    auto nextStep = Clock::now();
    draw(moving);
    while (true) {
        // Клавиша меняет направление, таймер двигает змейку каждые STEP_MS.
        int wait = moving ? max(0, int(chrono::duration_cast<chrono::milliseconds>(nextStep - Clock::now()).count())) : -1;
        key = readKey(wait);
        if (key == 'q') return 0;
        if (key == 'h' || key == 'p') {
            if (key == 'h') help();
            else cout << "\n  ПАУЗА. Enter — продолжить, Q — выйти.\n" << flush;
            if (!waitEnter()) return 0;
            nextStep = Clock::now() + chrono::milliseconds(STEP_MS);
            draw(moving);
            continue;
        }
        Point wanted = {0, 0};
        if (key == 'w') wanted = {0, -1};
        if (key == 'a') wanted = {-1, 0};
        if (key == 's') wanted = {0, 1};
        if (key == 'd') wanted = {1, 0};
        if ((wanted.x || wanted.y) && (length[active] == 1 ||
            !same(wanted, {-direction[active].x, -direction[active].y}))) {
            nextDirection = wanted;
            if (!moving) nextStep = Clock::now() + chrono::milliseconds(STEP_MS);
            moving = true;
        }
        if (moving && Clock::now() >= nextStep) {
            int previous = active;
            bool alive = moveSnake(nextDirection);
            if (previous != active) moving = false; // Новый игрок начинает сам.
            draw(moving);
            if (!alive) break;
            nextStep = Clock::now() + chrono::milliseconds(STEP_MS);
        }
    }
    cout << "\n  " << result << "\n  Итоговый счёт: " << score[0];
    if (players == 2) {
        cout << " : " << score[1];
        if (score[0] == score[1]) cout << " — ничья!";
        else cout << " — победил игрок " << (score[0] > score[1] ? 1 : 2) << '!';
    }
    cout << "\n  Enter — закрыть игру.\n" << flush;
    waitEnter();
    return 0;
}
