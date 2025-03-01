# 📡 Biblioteca RF

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

- [Recepción RF](#-recepción-rf)
- [RF Auxiliar](#%EF%B8%8F-librería-auxiliar-de-recepción-rf)
---

# 🧮 Recepción RF

## 📌 Índice

- [¿Qué hace esta librería?](#-qué-hace-esta-librería)
- [Introducción](#-introducción)
  - [Formato de la trama de datos](#-formato-de-la-trama-de-datos)
- [Configuración de la Librería](#%EF%B8%8F-configuraci%C3%B3n-de-la-librer%C3%ADa)
  - [Selección del Temporizador](#-selecci%C3%B3n-del-temporizador)
  - [Tiempo de Mantenimiento de Señal](#%EF%B8%8F-tiempo-de-mantenimiento-de-señal)
  - [Versión del Algoritmo de Decodificación](#-versión-del-algoritmo-de-decodificación)
- [Simulación de Pulsación de Botón](#%EF%B8%8F-simulación-de-pulsación-de-botón)
- [Variables](#-variables)
- [Funciones](#-funciones)
  - [Inicialización y Control](#-inicialización-y-control)
  - [Estado y Datos](#-estado-y-datos)
- [Uso de la Librería](#-uso-de-la-librería)
- [Recursos Utilizados](#%EF%B8%8F-recursos-utilizados)

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

---

## ⚙️ Configuración de la Librería

### ⏳ Selección del Temporizador
- Por defecto, la librería funciona con **Timer 1**.
- Para usar **Timer 0**, definir `RF_RX_TIMER0` antes de incluir la librería:
  ```c
  #define RF_RX_TIMER0
  ```

### ⏱️ Tiempo de Mantenimiento de Señal
- La señal de recepción espera un máximo de **200 ms** antes de considerar que el botón ha sido liberado.
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

## 🎛️ Simulación de Pulsación de Botón

- En los mandos a distancia RF, mientras la señal sea recibida, el sistema interpreta que el botón sigue presionado. Una vez que la señal desaparece, se asume que el botón ha sido soltado.
- Debido a interferencias, pueden ocurrir pequeñas interrupciones en la recepción. Para evitar falsas detecciones, se aplica un retardo antes de considerar que el botón ha sido liberado, funcionando como un "antirrebote RF".
- De forma predeterminada, el sistema espera hasta **200 ms** sin señal válida antes de asumir que el botón ha sido liberado.
- Para modificar este tiempo, definir `RF_MANTENIDO_TIME_OUT` con el valor deseado en milisegundos:
  ```c
  #define RF_MANTENIDO_TIME_OUT 500
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

**Nota:** Si no se usa la librería `rf_rx_aux.c`, la variable `RFmantenido` no se activa automáticamente. Debe activarse manualmente cuando se reciba una señal.

---

## 🖥️ Recursos Utilizados
- Utiliza **Timer 1** por defecto, o **Timer 0** si se define `RF_RX_TIMER0`.
- Utiliza la **interrupción externa** para la captura de datos.

---

# 🛠️ Librería Auxiliar de Recepción RF

## Índice

- [Que hace esta libreria?](#-qué-hace-esta-librería-1)
- [Introduccion](#-introducción-1)
- [Como se almacenan los mandos](#-métodos-de-almacenamiento)
- [Configuracion de la libreria](#%EF%B8%8F-configuración-de-la-librería-1)
- [Variables](#-variables)
- [Funciones](#-funciones)
  - [Comparacion y analisis](#-comparación-y-análisis)
  - [Almacenamiento](#-almacenamiento)
  - [Recuperacion y gestion de memoria](#-recuperación-y-gestión-de-memoria)

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

### 💾 Almacenamiento

- `GrabarMando()`: Almacena un mando en la primera posición de EEPROM, desplazando los existentes (el más antiguo se elimina).
- `GrabarMando(rfRemote)`: Igual que la anterior, pero permite definir manualmente los datos a almacenar.
- `GrabarBloqueMandos()`: Guarda todos los canales de un mando a la vez (`MandoVirtual`).
- `GrabarBloqueMandos(rfRemote)`: Variante que permite especificar los datos a grabar.

### 📖 Recuperación y gestión de memoria

- `LeerMandos()`: Carga todos los mandos almacenados en `MemRF` desde EEPROM.
- `BorrarMandos()`: Elimina todos los registros almacenados en EEPROM y limpia `MemRF`.

---
