# Soporte de Frecuencias

La librería soporta las siguientes frecuencias de reloj:
- 4MHz, 8MHz, 16MHz, 32MHz: El timer incrementa cada 1μs
- 24MHz, 48MHz: El timer incrementa cada 0.666μs

Para 24MHz y 48MHz se utilizan prescalers especiales:
- 24MHz: DIV_4 -> 1.5MHz (0.666μs por tick)
- 48MHz: DIV_8 -> 1.5MHz (0.666μs por tick)

La librería convierte automáticamente todos los valores de tiempo a microsegundos reales usando la macro `RF_TICKS_TO_US()`, por lo que todas las mediciones y comparaciones de tiempo se hacen SIEMPRE en microsegundos, independientemente de la frecuencia del reloj.# 📡 Biblioteca RF

## 👤 Autor
Martin Andersen
[IDEAA Lab](http://www.ideaalab.com)

---

## 📜 Licencia

Este proyecto está licenciado bajo la **Licencia MIT**.  
Puedes leer los términos completos en el siguiente enlace:  

[Licencia MIT](https://opensource.org/licenses/MIT)

---

## Contenido

- Recepción RF
- RF Auxiliar
---

# 🧮 Recepción RF

## 📌 Índice

- ¿Qué hace esta librería?
- Introducción
  - Formato de la trama de datos
- Configuración de la Librería
  - Selección del Temporizador
  - Antirebote de Señal
  - Versión del Algoritmo de Decodificación
- Variables
- Funciones
  - Inicialización y Control
  - Estado y Datos
- Uso de la Librería
- Recursos Utilizados

---

## ❓ ¿Qué hace esta librería?
Esta librería permite decodificar señales RF codificadas con encoders como:
- PT2240B
- PT2260
- PT2262
- PT2264

---

## 📖 Introducción
Esta librería utiliza un temporizador interno para contar los pulsos recibidos. Estos pulsos se detectan a través del pin de interrupción externa.

### 🧩 Formato de la trama de datos
Cada trama de datos consta de **24 bits + 1 bit de sincronización**. Cada bit se representa mediante un pulso con el siguiente formato:
- **CERO:** 25% duty del pulso
- **UNO:** 75% duty del pulso
- **SYNC:** 3.125% duty del pulso

Si lo vemos como unidades de tiempo, una trama completa en total suma 128 unidades de tiempo (ut) de la siguiente manera:
Cada bit de datos son 4 ut (24 bits * 4 = 96 ut) y un sync son 32 ut.
- **bit 0:** 1 ut HIGH + 3 ut LOW      |¯|___
- **bit 1:** 3 ut HIGH + 1 ut LOW      |¯¯¯|_
- **bit sync:** 1 ut HIGH + 31 ut LOW  |¯|_______________________________

Cuanto dura una unidad de tiempo depende de: el encoder, el voltaja y la resistencia de oscilación del emisor.
Valores típicos para 1 unidad de tiempo:
- **500uS** nos da una trama de 64mS
- **250uS** nos da una trama de 32mS
- **125uS** nos da una trama de 16mS

Este valor es importante ya que la libreria desecha pulsos de menos de RF_MIN_PULSE para filtrar ruido. Poner un valor muy bajo nos permite recibir tramas mas rapidas, pero tambien capta más ruido. Por el contrario valores mas altos recibe tramas mas lentas pero filtra más ruido.
---

## ⚙️ Configuración de la Librería

### ⏳ Selección del Temporizador
- Por defecto, la librería funciona con **Timer 1**.
- Para usar **Timer 0**, definir `RF_RX_TIMER0` antes de incluir la librería:
  ```c
  #define RF_RX_TIMER0
  ```

### ⏱️ Antirebote de Señal
```diff
- ATENCIÓN! Es la funcion `AnalizarRF()` cuando detecta una coincidencia la que establece `RFmantenido = true`. Si no se usa la librería `rf_rx_aux.c`, la variable `RFmantenido` no se activa automáticamente. Si se quiere usar se debe activarse manualmente cuando se reciba una señal válida.
```

Un mando a distancia o emisor envía la trama de datos repetidas veces, una a continuación de la siguiente. Para evitar una activación por cada trama de datos recibida, la variable `RFmantenido` cambia a `true`, y cuando se deja de recibir esa trama, el valor cambia a `false`. Debido a posibles errores en la transmisión o recepción, a veces se pierde una trama intermedia, pero no indica que la señal se haya dejado de recibir. Al igual que con un pulsador, podemos aplicar un "antirebote" que contempla pequeños vacíos de señal sin que `RFmantenido` cambie a `false`.

Si queremos una respuesta rápida pero propensa al ruido, establecer el tiempo de antirebote bajo (duración de 2-3 tramas completas). Esto puede provocar que una única trama enviada pueda ser interpretada como varias si se pierde alguna trama intermedia.

Si preferimos una respuesta más fiable pero menos ágil, podemos establecer este tiempo más alto (duración de 5-10 tramas, por ejemplo). Pequeños cortes en la trama no afectarían a la variable `RFmantenido`, pero esta variable tardaría más en volver a `false` cuando la señal recibida se apague.

- Para modificar este tiempo, definir `RF_MANTENIDO_TIME_OUT` con el valor deseado en milisegundos:
  ```c
  #define RF_MANTENIDO_TIME_OUT 500
  ```

### 🔄 Versión del Algoritmo de Decodificación
- La función `DataFrameComplete()` ha sido optimizada en varias versiones.
- Para utilizar versiones anteriores, definir una de las siguientes opciones antes de compilar:
  ```c
  #define RF_DECODE_V0
  #define RF_DECODE_V1
  ```

---

## 📊 Variables
- `rfBuffer`: Buffer de recepción que almacena los valores recibidos.
- Comprobar `rfBuffer` solo cuando `DataReady()` devuelva `TRUE`.

---

## 🔧 Funciones

### 🚀 Inicialización y Control
- `EncenderRF()`: Activa la recepción RF y las interrupciones.
- `ApagarRF()`: Desactiva la recepción RF y las interrupciones.
- `RestartRFmantenido()`: Reinicia temporizador que cuenta el tiempo sin haber recibido señal. Normalmente solo de uso interno, pero si estamos en una funcion que bloquea el programa durante un tiempo sin recibir señal y no queremos que se reinicie RFmantenido podemos llamarla para que reinicie el contador.
- `LimpiarRF()`: Reinicia las variables de tiempos y buffer. Normalmente solo de uso interno.

### 📡 Estado y Datos
- `DataReady()`: Devuelve `TRUE` cuando se ha recibido una trama completa.
- `GetRFTime()`: Devuelve el tiempo (`int32`) en milisegundos de la última trama completa.

---

## 📝 Uso de la Librería

1. Inicializar la librería llamando a `EncenderRF()`.
2. En el bucle principal, comprobar constantemente `DataReady()`. Cuando devuelva `TRUE`, se puede leer `rfBuffer`.

```c
if(DataReady() == TRUE){
    // Datos recibidos disponibles en rfBuffer
    ... ejecutar acciones
}
```

---

## 🖥️ Recursos Utilizados
- Utiliza **Timer 1** por defecto, o **Timer 0** si se define `RF_RX_TIMER0`.
- Utiliza la **interrupción externa** para la captura de datos.

---

# 🛠️ Librería Auxiliar de Recepción RF

## Índice

- Que hace esta libreria?
- Introduccion
- Como se almacenan los mandos
- Configuracion de la libreria
- Variables
- Funciones
  - Comparacion y analisis
  - Almacenamiento
  - Recuperacion y gestion de memoria
- Simulación de Pulsación de Botón

## ❓ ¿Qué hace esta librería?

Esta librería complementa `rf_rx.c`, proporcionando funciones para gestionar y almacenar mandos RF en memoria EEPROM. Su propósito principal es permitir el emparejamiento de un mando con el receptor, almacenando su dirección en memoria y comparando las señales recibidas para activar únicamente los mandos previamente almacenados.

### Características principales:

- 📥 Leer y almacenar mandos en la EEPROM.
- ✅ Verificar si una trama recibida corresponde a un mando almacenado.
- 🔄 Gestionar bloques de direcciones en EEPROM para redundancia o respaldo.
- 🖨️ Mostrar información de datos recibidos o almacenados a través del puerto serie.

---

## 📖 Introducción

Los mandos a distancia envían una trama de datos de **24 bits + 1 bit de sincronización**, que es decodificada por la librería `rf_rx.c`. La trama contiene:

- **Bits de dirección:** Parte fija que identifica el mando.
- **Bits de datos:** Parte variable que indica el estado de los botones.

Dependiendo del tipo de mando y su codificación, los bits de dirección y datos pueden tener distintas longitudes.

📌 **Opciones de almacenamiento:**

1. **Almacenar solo la dirección** del mando ya que los datos los podemos deducir de la trama recibida. Esto solo es posible si sabemos el tipo de mando que está emitiendo la señal.
2. **Almacenar la trama completa para cada canal**, si entendemos **mando** como **contenedor de canales** creamos un "mando virtual" donde cada canal almacena la trama completa. Esto nos permite no saber el formato de los datos recibidos, el conjunto de 24 bits de un pulsador será diferente al conjunto de 24 bits de otro pulsador del mismo mando, o incluso de un mando diferente.

Ejemplo:

```c
MemRF[Mando]        // Almacena solo la dirección (2 bytes en total)
MemRF[Mando][Canal] // Almacena la trama completa (3 bytes por canal)
```

---

## 📂 Métodos de almacenamiento

### 🔑 Dirección del mando

Se almacena solo la dirección, y los canales se deducen dinámicamente.

```c
MemRF[3]  // Tres mandos almacenados
MemRF[0]: [M1]
MemRF[1]: [M2]
MemRF[2]: [M3]
```

### 🗄️ Trama completa

Cada canal se almacena como una trama de datos completa.

```c
MemRF[3][4]  // Tres mandos con 4 canales cada uno
MemRF	 [x][0]		 [x][1]		 [x][2]		 [x][3]
[0][y]:	[M1/Ch1]	[M1/Ch2]	[M1/Ch3]	[M1/Ch4]
[1][y]:	[M2/Ch1]	[M2/Ch2]	[M2/Ch3]	[M2/Ch4]
[2][y]:	[M3/Ch1]	[M3/Ch2]	[M3/Ch3]	[M3/Ch4]

MemRF[5][1]  // Cinco mandos de 1 canal cada uno
MemRF	[x][0]
[0][0]: [M1/Ch1]
[1][0]: [M2/Ch1]
[2][0]: [M3/Ch1]
[3][0]: [M4/Ch1]
[4][0]: [M5/Ch1]
```

---

## ⚙️ Configuración de la librería

- 📌 **Dependencia:** Requiere `rf_rx.c`.
- 📍 **Dirección de inicio en EEPROM:**
  ```c
  #define POS_MEM_MANDOS_START_RF 10
  ```
- 📦 **Cantidad máxima de mandos:**
  ```c
  #define NUM_MANDOS_RF 3
  ```
- 🔢 **Número de canales por mando** (si se almacenan individualmente):
  ```c
  #define NUM_CANALES_RF 4
  ```

---

## 📊 Variables

- `Recibido`: Última trama de datos recibida.
- `RecibAnterior`: Trama previa, utilizada para comparación y sincronización.
- `MemRF[x] / MemRF[x][y]`: Lista de mandos emparejados almacenados en EEPROM.
- `ButtonMatch[x]`: Indica qué botones fueron activados en cada mando (8 bits, 1 por pulsador).
- `flagSync`: Señala si el sistema está en modo sincronización.
- `SyncStep`: Define el paso actual en el proceso de emparejamiento de múltiples canales.
- `MandoVirtual[x]`: Mando virtual que agrupa varios canales simultáneamente.

---

## 🔧 Funciones

### 🔍 Comparación y análisis

- `AnalizarRF(rfRemote)`: Compara la trama recibida con `MemRF`, guarda coincidencias en `ButtonMatch[]` y devuelve `TRUE` si hay coincidencias.
- `FlancoMantenido()`: Devuelve `MANTENIDO_RISING`, `MANTENIDO_FALLING` o `FALSE` para detectar el inicio y fin de una pulsación de un mando a distancia. Comprobar su valor constantemente en el bucle principal para poder reaccionar a los cambios.

### 💾 Almacenamiento

- `GrabarMando()`: Almacena un mando en la primera posición de EEPROM, desplazando los existentes (el más antiguo se elimina).
- `GrabarMando(rfRemote)`: Igual que la anterior, pero permite definir manualmente los datos a almacenar.
- `GrabarBloqueMandos()`: Guarda todos los canales de un mando a la vez (`MandoVirtual`).
- `GrabarBloqueMandos(rfRemote)`: Variante que permite especificar los datos a grabar.

### 📖 Recuperación y gestión de memoria

- `LeerMandos()`: Carga todos los mandos almacenados en `MemRF` desde EEPROM.
- `BorrarMandos()`: Elimina todos los registros almacenados en EEPROM y limpia `MemRF`.

---

## 🎛️ Detectar la Pulsación de un Botón del Mando

En los mandos a distancia RF, mientras la señal sea recibida, el sistema interpreta que el botón sigue presionado. Una vez que la señal desaparece, se asume que el botón ha sido soltado.

Ver sección **Antirebote de Señal** para ajustar el valor de antirebote y velocidad de respuesta.

Usar este codigo para detectar el inicio y fin de una pulsacion o trama de datos:

```c
switch(FlancoMantenido()){
    case(MANTENIDO_RISING):
        //acaba de empezar a recibirse señal
        ...
        break;
    case(MANTENIDO_FALLING):
        //acaba de dejar de recibirse señal
        ...
        break;
    case(FALSE):
        //no hubo cambio
        ...
        break;
}
```
