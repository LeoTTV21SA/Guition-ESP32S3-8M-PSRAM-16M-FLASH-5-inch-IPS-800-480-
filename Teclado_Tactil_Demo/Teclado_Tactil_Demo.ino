

/*******************************************************************************
 ******************************************************************************/
/*******************************************************************************
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <TAMC_GT911.h>


// Configuración de visualización
#define GFX_BL 2
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40, 41, 39, 42, // DE, VSYNC, HSYNC, PCLK
    45, 48, 47, 21, 14, // R0-R4
    5, 6, 7, 15, 16, 4, // G0-G5
    8, 3, 46, 9, 1, // B0-B4
    0, 8, 4, 8, // hsync
    0, 8, 4, 8, // vsync
    1, 15000000 // pclk
);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(800, 480, rgbpanel);

// Configuración de la pantalla táctil
#define TOUCH_SDA 19
#define TOUCH_SCL 20
#define TOUCH_INT 18
#define TOUCH_RST 38
TAMC_GT911 ts = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, 800, 480);

// Definición de colores para la interfaz
#define BG_COLOR BLACK
#define KEY_COLOR BLUE
#define PRESSED_COLOR GREEN
#define TEXT_COLOR WHITE

// Teclado
const uint16_t keys[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

// Estructura para almacenar múltiples toques
struct TouchPoint {
    int16_t x;
    int16_t y;
    int16_t keyX;
    int16_t keyY;
};

// Variables para almacenar el estado
TouchPoint lastTouches[5]; // Soporta hasta 5 toques (ajusta según TAMC_GT911)
TouchPoint currentTouches[5];
int lastTouchCount = 0;
int currentTouchCount = 0;

void setup() {
    Serial.begin(115200);
    
    // Inicialización de pantalla
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
    gfx->begin();
    gfx->fillScreen(BG_COLOR);
    
    // Inicialización de la pantalla táctil
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    ts.begin();
    ts.setRotation(ROTATION_INVERTED);
    
    // Presentación inicial del teclado 
    drawKeyboard();
}

void loop() {
    // Verifica eventos táctiles cada 40 ms
    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate >= 40) { 
        lastUpdate = millis();
        processTouch();
    }
}

// Dibuja un teclado virtual de 4 filas por 3 columnas
void drawKeyboard() {
    int keyWidth = 200;
    int keyHeight = 100;
    int startX = 100;
    int startY = 100;

    gfx->startWrite();
    
    // Dibuja todos los botones 
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            drawKey(startX + col*keyWidth, startY + row*keyHeight, 
                   keyWidth-10, keyHeight-10, keys[row][col], KEY_COLOR);
        }
    }
    
    gfx->endWrite();
}

// Procesa eventos táctiles para detectar teclas presionadas (con soporte multitáctil)
void processTouch() {
    ts.read();
    currentTouchCount = 0;

    if (ts.isTouched) {
        int keyWidth = 200;
        int keyHeight = 100;
        int startX = 100;
        int startY = 100;

        // Procesa hasta ts.touches (máximo de puntos táctiles soportados)
        for (int i = 0; i < ts.touches && i < 5; i++) {
            int16_t touchX = ts.points[i].x;
            int16_t touchY = ts.points[i].y;
            
            // Ignora toques inválidos
            if (touchX < 0 || touchY < 0) continue;

            Serial.printf("Touch %d: X=%d, Y=%d\n", i, touchX, touchY);

            // Identifica la tecla presionada según las coordenadas táctiles
            int16_t keyX = -1, keyY = -1;
            for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 3; col++) {
                    int x = startX + col * keyWidth;
                    int y = startY + row * keyHeight;
                    
                    if (touchX >= x && touchX < x + keyWidth - 10 && 
                        touchY >= y && touchY < y + keyHeight - 10) {
                        keyX = x;
                        keyY = y;
                        break;
                    }
                }
                if (keyX != -1) break;
            }

            // Almacena el toque válido
            if (keyX != -1) {
                currentTouches[currentTouchCount].x = touchX;
                currentTouches[currentTouchCount].y = touchY;
                currentTouches[currentTouchCount].keyX = keyX;
                currentTouches[currentTouchCount].keyY = keyY;
                currentTouchCount++;
            }
        }
    }

    // Actualiza el teclado si el estado cambió
    if (currentTouchCount != lastTouchCount || 
        memcmp(currentTouches, lastTouches, currentTouchCount * sizeof(TouchPoint)) != 0) {
        updateKeyboard();
        memcpy(lastTouches, currentTouches, currentTouchCount * sizeof(TouchPoint));
        lastTouchCount = currentTouchCount;
    }
}

// Actualiza el teclado redibujando las teclas según los eventos táctiles
void updateKeyboard() {
    gfx->startWrite();

    // Restaurar todas las teclas a su estado normal
    int keyWidth = 200;
    int keyHeight = 100;
    int startX = 100;
    int startY = 100;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            int x = startX + col * keyWidth;
            int y = startY + row * keyHeight;
            drawKey(x, y, keyWidth - 10, keyHeight - 10, keys[row][col], KEY_COLOR);
        }
    }

    // Dibujar las teclas presionadas
    for (int i = 0; i < currentTouchCount; i++) {
        int row = (currentTouches[i].keyY - 100) / 100;
        int col = (currentTouches[i].keyX - 100) / 200;
        drawKey(currentTouches[i].keyX, currentTouches[i].keyY, 190, 90, keys[row][col], PRESSED_COLOR);
    }

    gfx->endWrite();
}

// Dibuja una tecla rectangular con un símbolo en el centro
void drawKey(int x, int y, int w, int h, char symbol, uint16_t color) {
    gfx->fillRoundRect(x, y, w, h, 10, color);
    gfx->setTextColor(TEXT_COLOR);
    gfx->setTextSize(3);
    gfx->setCursor(x + w/2 - 15, y + h/2 - 15);
    gfx->print(symbol);
}

/*******************************************************************************
 ******************************************************************************/
/*******************************************************************************
 ******************************************************************************/
/*#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <TAMC_GT911.h>

// Configuracion de visualizacion
// Esta sección define la configuración de la pantalla RGB, incluyendo el pin de retroiluminación y los pines para señales RGB y sincronización
// Guition ESP32S3 8M PSRAM 
#define GFX_BL 2
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40, 41, 39, 42, // DE, VSYNC, HSYNC, PCLK
    8, 3, 46, 9, 1, // R0-R4
    5, 6, 7, 15, 16, 4, // G0-G5
    45, 48, 47, 21, 14, // B0-B4
    0, 8, 4, 8, // hsync
    0, 8, 4, 8, // vsync
    1, 16000000 // pclk
);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(800, 480, rgbpanel);

// Configuracion de la pantalla tactil
#define TOUCH_SDA 19
#define TOUCH_SCL 20
#define TOUCH_INT 18
#define TOUCH_RST 38
TAMC_GT911 ts = TAMC_GT911(TOUCH_SDA, TOUCH_SCL, TOUCH_INT, TOUCH_RST, 800, 480);

// Definición de colores para la interfaz
#define BG_COLOR BLACK
#define KEY_COLOR BLUE
#define PRESSED_COLOR GREEN
#define TEXT_COLOR WHITE

// Teclado
const uint16_t keys[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

// Variable para almacenar el estado
int16_t lastKeyX = -1, lastKeyY = -1;

void setup() {
    Serial.begin(115200);
    
    // Inicializacion de pantalla
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, HIGH);
    gfx->begin();
    gfx->fillScreen(BG_COLOR);
    
    // Inicialización de la pantalla táctil
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    ts.begin();
    ts.setRotation(ROTATION_INVERTED);
    
    // Presentacion inicial del teclado 
    drawKeyboard();
}

void loop() {
    // Verifica eventos táctiles cada 40 ms
    static uint32_t lastUpdate = 0;
    if (millis() - lastUpdate >= 40) { 
        lastUpdate = millis();
        processTouch();
    }
}
// Dibuja un teclado virtual de 4 filas por 3 columnas"
void drawKeyboard() {
    int keyWidth = 200;
    int keyHeight = 100;
    int startX = 100;
    int startY = 100;

    gfx->startWrite(); // Bloueamos la pantalla
    
    // Dibuja todos los botones 
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            drawKey(startX + col*keyWidth, startY + row*keyHeight, 
                   keyWidth-10, keyHeight-10, keys[row][col], KEY_COLOR);
        }
    }
    
    gfx->endWrite(); // Desbloquear la pantalla 
}

// Procesa eventos táctiles para detectar teclas presionadas
void processTouch() {
    ts.read();
    int16_t currentKeyX = -1, currentKeyY = -1;
    
    if (ts.isTouched) {
        int16_t touchX = ts.points[0].x;
        int16_t touchY = ts.points[0].y;
        
        // Identifica la tecla presionada según las coordenadas táctiles
        int keyWidth = 200;
        int keyHeight = 100;
        int startX = 100;
        int startY = 100;
        
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 3; col++) {
                int x = startX + col*keyWidth;
                int y = startY + row*keyHeight;
                
                if (touchX >= x && touchX < x + keyWidth-10 && 
                    touchY >= y && touchY < y + keyHeight-10) {
                    currentKeyX = x;
                    currentKeyY = y;
                    break;
                }
            }
            if (currentKeyX != -1) break;
        }
    }
    
    // si el estado cambio, redibujamos 
    if (currentKeyX != lastKeyX || currentKeyY != lastKeyY) {
        updateKeyboard(lastKeyX, lastKeyY, currentKeyX, currentKeyY);
        lastKeyX = currentKeyX;
        lastKeyY = currentKeyY;
    }
}
// Actualiza el teclado redibujando las teclas según los eventos táctiles
void updateKeyboard(int16_t oldX, int16_t oldY, int16_t newX, int16_t newY) {
    gfx->startWrite();
    
    // Restaurar el boton anterior
    if (oldX != -1) {
        int row = (oldY - 100) / 100;
        int col = (oldX - 100) / 200;
        drawKey(oldX, oldY, 190, 90, keys[row][col], KEY_COLOR);
    }
    
    // Dibujar el nuevo boton presionado
    if (newX != -1) {
        int row = (newY - 100) / 100;
        int col = (newX - 100) / 200;
        drawKey(newX, newY, 190, 90, keys[row][col], PRESSED_COLOR);
    }
    
    gfx->endWrite();
}
//Dibuja una tecla rectangular con un símbolo en el centro
void drawKey(int x, int y, int w, int h, char symbol, uint16_t color) {
    gfx->fillRoundRect(x, y, w, h, 10, color);
    gfx->setTextColor(TEXT_COLOR);
    gfx->setTextSize(3);
    gfx->setCursor(x + w/2 - 15, y + h/2 - 15);
    gfx->print(symbol);
}*/
