#include "mainwindow.h"

// Маштаб затухания
float scaleAttenuationGlass = 0.7;
//Формулы затухания сихнала
double attenuationGlass(double freq) {
    // Формула для стеклопакета
    return (2 + 0.2 * freq)*scaleAttenuationGlass;
}
double attenuationIRRGlass(double freq) {
    // Формула для IRR стекла
    return (23 + 0.3 * freq)*scaleAttenuationGlass;
}
double attenuationConcrete(double freq) {
    // Формула для бетона
    return (5 + 4 * freq)*scaleAttenuationGlass*0.11;// *0.13 чтоб покрасивее было
}
double attenuationWood(double freq) {
    // Формула для дерева
    return (4.85 + 0.12 * freq)*scaleAttenuationGlass;
}


struct Wall {
    int x;
    int y;
    int length;
    int width;
    int materialType;
};


// Функция для определения пересечения с одной стеной
bool isIntersectionWithWall(int x, int y, const Wall& wall) {
    return (x >= wall.x && x <= wall.x + wall.length && y >= wall.y && y <= wall.y + wall.width);
}
// Функция для подсчета пересечений со стенами
double countWallIntersections(int i, int j, int wifi_x, int wifi_y, const Wall walls[], int wallsCount) {
    double attenuation = 0.0;
    double fc = 5;

    int x = i;
    int y = j;
    int dx = abs(wifi_x - i);
    int dy = abs(wifi_y - j);
    int sx = i < wifi_x ? 1 : -1;
    int sy = j < wifi_y ? 1 : -1;
    int err = dx - dy;

    while (true) {
        for (int k = 0; k < wallsCount; ++k) {
            if (isIntersectionWithWall(x, y, walls[k])) {
                switch (walls[k].materialType) {
                case 1:
                    attenuation += attenuationGlass(fc);
                    break;
                case 2:
                    attenuation += attenuationIRRGlass(fc);
                    break;
                case 3:
                    attenuation += attenuationConcrete(fc);
                    break;
                case 4:
                    attenuation += attenuationWood(fc);
                    break;
                default:
                    break;
                }
            }
        }
        if (x == wifi_x && y == wifi_y) {
            break; // Достигнута конечная точка
        }

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
    return attenuation;
}

// Красивая загрузка
void showLoadingBar(int progress) {
    const int barWidth = 50; // Ширина полосы загрузки
    const int blockSize = 100 / barWidth; // Количество блоков на 1%

    std::cout << "[";
    int completedBlocks = progress / blockSize;

    // Для корректного отображения прогресса, увеличиваем completedBlocks
    for (int i = 0; i < barWidth; ++i) {
        if (i < completedBlocks)
            std::cout << "="; // Символ полной загрузки
        else
            std::cout << ".";
    }
    std::cout << "] " << progress << "%\r";
    std::cout.flush();
}

// Формула распространения
float PL(float d, double fc){
    return 28 + 22*log10(d) + 20*log10(fc);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    QGraphicsScene* scene = new QGraphicsScene();

    // Базовые переменные
    QPixmap map(1000, 1000);
    int wifi_x = 500;
    int wifi_y = 500;
    int tx = 23;
    int ant = 10;
    double fc = 5;

    // Маштаб (1 пискель = 1/scale метров)
    float scale = 0.2;

    // Создание 4 стен
    Wall walls[] = {
        {500, 300, 150, 10, 1},   // Стекло
        {200, 250, 10,  150, 2},   // IRR стекло
        {350, 700, 270, 30, 3},   // Бетон
        {800, 600, 5, 100, 4}    // Дерево
    };

    int wallsCount = sizeof(walls) / sizeof(walls[0]); // Количество стен

    QPainter p(&map);

    for (int i = 0; i < 1000; i++) {
        //std::cout << "x=" << i << std::endl;
        showLoadingBar(i/10);
        for (int j = 0; j < 1000; j++) {
            float d = sqrt(pow((wifi_x - i), 2) + pow((wifi_y - j), 2)) / scale;
            float dBm = ant + tx - PL(d, fc);

            // Нормализация dBm от 0 -> 100
            //dBm = ((dBm * -1) - 44) * 2.55; // коэффициент можно менять 1.875=-144blue

            int intersections = countWallIntersections(i, j, wifi_x, wifi_y, walls, wallsCount);

            // Учет влияния стен на увеличение dBm
            dBm -= intersections;

            // Задание цвета на основе какой dBm
            // if(dBm < 0) p.setPen(QColor(255,0,0, 255));
            // else if(dBm < 64 ) p.setPen(QColor(255, dBm*4, 0, 255));
            // else if(dBm < 128) p.setPen(QColor((255 - ((dBm-64)*4)), 255, 0, 255));
            // else if(dBm < 192) p.setPen(QColor(0, 255, ((dBm-128)*4), 255));
            // else if(dBm < 256) p.setPen(QColor(0, 255 - ((dBm-192)*4), 255, 255));
            // else p.setPen(QColor(0,0,150, 255));



            // Задаем цвет в зависимости от мощности сигнала
            if(dBm < -44 && dBm >= -54){
                p.setPen(QColor(255, 0, 0, 255)); // Красный цвет
            } else if (dBm < -54 && dBm >= -64) {
                p.setPen(QColor(255, 69, 0, 255)); // Оранжевый цвет
            } else if (dBm < -64 && dBm >= -74) {
                p.setPen(QColor(255, 140, 0, 255)); // Темный оранжевый
            } else if (dBm < -74 && dBm >= -84) {
                p.setPen(QColor(255, 215, 0, 255)); // Желтый (Золотой)
            } else if (dBm < -84 && dBm >= -94) {
                p.setPen(QColor(255, 255, 0, 255)); // Желтый
            } else if (dBm < -94 && dBm >= -104) {
                p.setPen(QColor(173, 255, 47, 255)); // Желтозеленый
            } else if (dBm < -104 && dBm >= -114) {
                p.setPen(QColor(0, 255, 0, 255)); // Лаймовый
            } else if (dBm < -114 && dBm >= -124) {
                p.setPen(QColor(0, 255, 127, 255)); // Зеленосиний
            } else if (dBm < -124 && dBm >= -134) {
                p.setPen(QColor(0, 255, 255, 255)); // Циановый
            } else if (dBm < -134 && dBm >= -144) {
                p.setPen(QColor(0, 0, 255, 255)); // Голубой
            }


            p.drawPoint(i, j);
        }
    }

    // Отображение на Пиксельной карте
    for (int i = 0; i < wallsCount; ++i) {
        int x = walls[i].x;
        int y = walls[i].y;
        int length = walls[i].length;
        int width = walls[i].width;
        int materialType = walls[i].materialType;

        // Установка цвета в зависимости от типа материала
        switch (materialType) {
        case 1: // Стекло
            p.setPen(QColor(66, 170, 255)); // Голубой цвет для стекла
            break;
        case 2: // IRR стекло
            p.setPen(QColor(186, 85, 211)); // Умеренный цвет орхидеи для IRR стекла
            break;
        case 3: // Бетон
            p.setPen(QColor(128, 128, 128)); // Серый цвет для бетона
            break;
        case 4: // Дерево
            p.setPen(QColor(101, 67, 33)); // Тёмно-коричневый цвет для дерева
            break;
        default:
            break;
        }
        // Рисуем стену пикселями
        for (int px = x; px < x + length; ++px) {
            for (int py = y; py < y + width; ++py) {
                p.drawPoint(px, py);
            }
        }
    }
    scene->addPixmap(map);
    QGraphicsView* view = new QGraphicsView(scene);
    setCentralWidget(view);
}

MainWindow::~MainWindow()
{
}
