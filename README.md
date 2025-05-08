## Guition-ESP32S3-8M-PSRAM-16M-FLASH-5-inch-IPS-800-480-

1.Teclado Táctil Virtual para Pantalla TFT 800x480
Este proyecto escolar implementa un teclado táctil virtual de 4x3 teclas en una pantalla TFT RGB de 800x480 (JC8048W550C_I) usando un ESP32 y la biblioteca Arduino_GFX_Library. Fue un prototipo para aprender a configurar la pantalla y el controlador táctil GT911, y para identificar parámetros estables que eliminaron el parpadeo (rebote) en proyectos posteriores.
Descripción
El proyecto muestra un teclado virtual con teclas numéricas (0-9, *, #) que cambian de color (azul a verde) al ser presionadas. Soporta hasta 5 toques simultáneos gracias al controlador GT911. Fue un paso clave para entender la sincronización de la pantalla (pclk, hsync, vsync) y optimizar la fuente de alimentación, logrando estabilidad en pruebas de 20 minutos con cables USB largos , logrando estabilidad por un dia completo con un cablen USB con una longitud estandar y con el mismo cable se probo la estabilidad por 8h conectado a la computadora durante el tiempo escolar.

Características

Teclado virtual: 4 filas x 3 columnas con teclas 1-9, 0, *, #.
Soporte multitáctil: Detecta hasta 5 toques simultáneos.
Interfaz gráfica: Teclas rectangulares con esquinas redondeadas, color azul (reposo) y verde (presionado).
Estabilidad: Sin parpadeo gracias a pclk = 15 MHz y fuente USB estable (≥2A).
Frecuencia: Actualización táctil cada 40 ms (~25 Hz).

Requisitos

Hardware
Placa: ESP32 (por ejemplo, ESP32-S3 Dev Module).
Pantalla: TFT RGB 800x480 (JC8048W550C_I, controlador probable: ILI9488).
Controlador táctil: TAMC_GT911.
Fuente de alimentación: USB (5V, ≥2A), probado con cables cortos y largos.
Cables: USB de calidad para minimizar interferencias.

Software

Arduino IDE: (Versión 2.3.6).
Bibliotecas:
Arduino_GFX_Library.
TAMC_GT911.


Archivo: Teclado_tactil_Demo.ino.

Instalación

Configurar Arduino IDE:

Instala Arduino IDE desde arduino.cc.
Agrega la placa ESP32: en Preferencias, añade https://raw.githubusercontent.com/espressif/arduino-esp32/master/package_esp32_index.json.
Instala el núcleo ESP32 en Herramientas > Placa > Gestor de Placas.


Instalar bibliotecas:

En Herramientas > Gestionar Bibliotecas, instala:
Arduino_GFX_Library.
TAMC_GT911.


Descargar el código:

Copia Teclado_tactil_Demo.ino en una carpeta con el mismo nombre.


Conectar el hardware:

Conecta la pantalla TFT y el controlador táctil al ESP32 según la tabla de pines.
Usa un cable USB de calidad (5V, ≥2A).


Cargar el código:

Abre Teclado_tactil_Demo.ino en Arduino IDE.
Selecciona la placa ESP32 y el puerto COM.
Compila y carga (Ctrl+U).



Conexiones



Pin ESP32
Función
Conexión en Pantalla

40 -> DE ->  Data Enable

41 -> VSYNC -> Vertical Sync

39 -> HSYNC -> Horizontal Sync

42 -> PCLK -> Pixel Clock

45,48,47,21,14 -> R0-R4 -> Rojo (5 bits)

5,6,7,15,16,4 -> G0-G5 -> Verde (6 bits)

8,3,46,9,1 -> B0-B4 -> Azul (5 bits)

2 -> GFX_BL -> Retroiluminación

19 -> TOUCH_SDA -> I2C SDA (táctil)

20 -> TOUCH_SCL -> I2C SCL (táctil)

18 -> TOUCH_INT -> Interrupción táctil

38 -> TOUCH_RST -> Reset táctil


Alimentación: Conecta el ESP32 a un puerto USB (PC o cargador, 5V, ≥2A).
Nota: Cables largos probados con éxito por 20 minutos. (recordar modificar cuando termine la prueba de cable largo en 2 dias)

Uso

Iniciar el proyecto:

Conecta el ESP32 a un puerto USB o cargador.
La pantalla muestra un teclado virtual de 4x3 teclas.


Interacción:

Toca una tecla (por ejemplo, 5): cambia de azul a verde.
Suelta la tecla: vuelve a azul.
Prueba múltiples toques (hasta 5) para verificar multitáctil.


Monitor serie:

Abre el monitor serie (115200 baudios) para ver las coordenadas táctiles:Touch 0: X=320, Y=240



Pruebas de estabilidad:

Probado por 20 minutos con cable USB largo, sin parpadeo.
Configuración (pclk = 15 MHz) aplicada al proyecto final.


Problemas y Soluciones

Problema: Parpadeo aleatorio (rebote) en la pantalla.
Causa: Frecuencia de píxeles (pclk) alta o fuente inestable.
Solución:
Ajustar pclk a 15 MHz (1, 15000000) para estabilidad (~31.4 Hz).
Usar fuente USB de calidad (≥2A).
Configurar sincronización: hsync = 0, 8, 4, 8, vsync = 0, 8, 4, 8.


Licencia
Licencia MIT: Este proyecto es de código abierto y puede ser usado o modificado libremente, con atribución al autor.
Créditos

Autor: LeoTTV21SA (estudiante, proyecto escolar).
Bibliotecas:
Arduino_GFX_Library.
TAMC_GT911.




2. Gráfico Lineal Simple con LVGL para Pantalla TFT 800x480
Este proyecto escolar implementa un gráfico de líneas simple en una pantalla TFT RGB de 800x480 (JC8048W550C_I) usando un ESP32 y la biblioteca LVGL. Muestra valores simulados de oscilación (5.00-20.00) y temperatura (20.00-30.00) actualizados cada segundo. Fue un prototipo para aprender a usar LVGL y optimizar la configuración de la pantalla, logrando estabilidad sin parpadeo.
Descripción
El proyecto crea un gráfico de líneas con dos series (oscilación en rojo, temperatura en azul) en una pantalla táctil de 800x480. Fue un paso intermedio para entender la integración de LVGL con Arduino_GFX_Library y el controlador táctil GT911, y para probar configuraciones que eliminaron el parpadeo (rebote) en el proyecto final. Estable por 20 minutos con cable USB largo, logrando estabilidad por un dia completo con un cablen USB con una longitud estandar y con el mismo cable se probo la estabilidad por 8h conectado a la computadora durante el tiempo escolar.

Características

Gráfico de líneas: Dos series (oscilación, temperatura) con 30 puntos visibles.
Valores simulados: Oscilación (5.00-20.00), temperatura (20.00-30.00) con decimales.
Soporte táctil: Detecta toques, aunque no se usa en el gráfico.
Estabilidad: Sin parpadeo con pclk = 15 MHz y fuente USB estable (≥2A).
Frecuencia: Loop a ~200 Hz (delay(5)), pantalla a ~31.4 Hz. Si el grafico le parpadea (rebote) tenga en cuenta la opcion de modificar el (delay(5)) a [(delay(16)) o (delay(17))]

Requisitos
Hardware

Placa: ESP32 (por ejemplo, ESP32-S3 Dev Module).
Pantalla: TFT RGB 800x480 (JC8048W550C_I, controlador probable: ILI9488).
Controlador táctil: TAMC_GT911.
Fuente de alimentación: USB (5V, ≥2A), probado con cables cortos y largos (cargador y PC).
Cables: USB de calidad para minimizar interferencias.

Software

Arduino IDE: (Versión 2.3.6).
Bibliotecas:
Arduino_GFX_Library.
LVGL (versión 8.3.3).
TAMC_GT911.


Archivo: grafico_lineal_Simple.ino.

Instalación

Configurar Arduino IDE:

Instala Arduino IDE desde arduino.cc.
Agrega la placa ESP32: en Preferencias, añade https://raw.githubusercontent.com/espressif/arduino-esp32/master/package_esp32_index.json.
Instala el núcleo ESP32 en Herramientas > Placa > Gestor de Placas.


Instalar bibliotecas:

En Herramientas > Gestionar Bibliotecas, instala:
Arduino_GFX_Library.
LVGL (versión 8.3.3).
TAMC_GT911.


Configura lv_conf.h para resolución 800x480 y buffer pequeño.


Descargar el código:

Copia grafico_lineal_Simple.ino en una carpeta con el mismo nombre.


Conectar el hardware:

Conecta la pantalla TFT y el controlador táctil al ESP32 según la tabla de pines.
Usa un cable USB de calidad (5V, ≥2A).


Cargar el código:

Abre grafico_lineal_Simple.ino en Arduino IDE.
Selecciona la placa ESP32 y el puerto COM.
Compila y carga (Ctrl+U).


Conexiones

Pin ESP32 -> Función -> Conexión en Pantalla

40 -> DE -> Data Enable

41 -> VSYNC -> Vertical Sync

39 -> HSYNC -> Horizontal Sync

42 -> PCLK -> Pixel Clock

45,48,47,21,14 -> R0-R4 -> Rojo (5 bits)

5,6,7,15,16,4 -> G0-G5 -> Verde (6 bits)

8,3,46,9,1 -> B0-B4 -> Azul (5 bits)

2 -> GFX_BL -> Retroiluminación

19 -> TOUCH_SDA -> I2C SDA (táctil)

20 -> TOUCH_SCL -> I2C SCL (táctil)

18 -> TOUCH_INT -> Interrupción táctil

38 -> TOUCH_RST -> Reset táctil


Alimentación: Conecta el ESP32 a un puerto USB (PC o cargador, 5V, ≥2A).
Nota: Cables largos probados con éxito por 20 minutos.(en proceso de prueba de 2 dias)

Uso

Iniciar el proyecto:

Conecta el ESP32 a un puerto USB o cargador.
La pantalla muestra un gráfico de líneas centrado (700x300 píxeles).


Interfaz:

Gráfico: Muestra oscilación (rojo, 5.00-20.00) y temperatura (azul, 20.00-30.00) actualizadas cada segundo.
Desplazamiento: Los últimos 30 puntos son visibles, con modo de actualización por desplazamiento.
Táctil: Detecta toques, pero no interactúa con el gráfico.


Monitor serie:

Abre el monitor serie (115200 baudios) para depuración:Pantalla no inicializada (si falla)


Pruebas de estabilidad:

Probado por 20 minutes con cable USB largo, sin parpadeo.(en proseso de prueba por 2 dias)
Configuración (pclk = 15 MHz) aplicada al proyecto final.


Problemas y Soluciones

Problema: Parpadeo aleatorio (rebote) en la pantalla.

Causa: Frecuencia de píxeles (pclk) alta o fuente inestable.
Solución:
Ajustar pclk a 15 MHz (1, 15000000) para estabilidad (~31.4 Hz).
Usar fuente USB de calidad (≥2A).
Configurar sincronización: hsync = 0, 8, 4, 8, vsync = 0, 8, 4, 8.


Problema: Gráfico simple sin etiquetas ni interacción.

Solución: Este prototipo llevó al proyecto final con etiquetas dinámicas (temperature_display, switch_labels) y botones.

Licencia
Licencia MIT: Este proyecto es de código abierto y puede ser usado o modificado libremente, con atribución al autor.
Créditos

Autor: LeoTTV21SA (estudiante, proyecto escolar).
Bibliotecas:
Arduino_GFX_Library.
LVGL.
TAMC_GT911.




