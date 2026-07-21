# MC Simulator# MC Simulator - Entrenador de Oratoria e Inteligencia Artificial en VR (UE 5.7.1)

Este proyecto es una plataforma de simulación inmersiva en Realidad Virtual para **Meta Quest** desarrollada en **Unreal Engine 5.7.1** con **C++** y **Python/Gemini Flash**.

---

## 🏛️ Sistema Multi-Escenario y Menú Lobby Elegante

El simulador cuenta con un flujo inmersivo donde el usuario inicia en un **Menú 3D en el Lobby**, selecciona su entorno de práctica y entra a un escenario personalizado con iluminación temática y evaluación adaptativa de Inteligencia Artificial.

---

### 🎨 Guía Paso a Paso: Creación del Menú Elegante (`W_ScenarioMenu`)

#### 1. Crear el Widget de Selección de Escenario (`W_ScenarioMenu`)
1. En tu **Content Drawer**, navega a tu carpeta de Blueprints.
2. Haz clic derecho en un área vacía -> **User Interface** -> **Widget Blueprint**. Nómbralo **`W_ScenarioMenu`** y ábrelo con doble clic.
3. En el panel **Palette** a la izquierda:
   * Arrastra un **Canvas Panel** a la raíz.
   * Añade un **Border** central (Fondo oscuro semi-transparente `#0E1015`, opacidad 0.85).
   * Añade un **TextBlock** superior: *"SELECCIONA TU ESCENARIO DE PRACTICA"*, fuente en tono dorado (`#E2B04A`).
4. Diseña 3 Tarjetas Visuales (usando **Vertical Box** y **Horizontal Box** con 3 componentes **Border** oscuros con esquinas redondeadas):

##### 🏛️ Tarjeta 1: Auditorio Principal
* **Título:** `Auditorio de Gran Impacto`
* **Descripción:** `Presentación oral masiva ante jurados y público.`
* **Botón (`Btn_Auditorio`):** Texto *"Entrar a Auditorio"*.
* **Evento de Clic en `Btn_Auditorio`:**
  1. `Get Game Instance` -> `Cast To MCSIMULATORGameInstance`.
  2. Llama a `Set Current Scenario` con:
     * **Scenario Id:** `"Auditorio"`
     * **Scenario Name:** `"Auditorio Principal"`
     * **AI Context:** `"Presentación oral masiva frente a jurados y audiencia en un auditorio principal."`
  3. Llama a `Open Level by Name` escribiendo `AuditorioMap`.

##### 🏫 Tarjeta 2: Sala de Clases
* **Título:** `Aula Académica`
* **Descripción:** `Exposición docente y control de grupo frente a estudiantes.`
* **Botón (`Btn_Classroom`):** Texto *"Entrar a Aula"*.
* **Evento de Clic en `Btn_Classroom`:**
  1. `Get Game Instance` -> `Cast To MCSIMULATORGameInstance`.
  2. Llama a `Set Current Scenario` con:
     * **Scenario Id:** `"Classroom"`
     * **Scenario Name:** `"Sala de Clases"`
     * **AI Context:** `"Exposición docente y académica frente a estudiantes en un aula de clases."`
  3. Llama a `Open Level by Name` escribiendo `ClassroomMap`.

##### 💼 Tarjeta 3: Escritorio de Entrevista
* **Título:** `Entrevista Ejecutiva 1-a-1`
* **Descripción:** `Reunión cercana de trabajo y evaluación de contacto visual.`
* **Botón (`Btn_Interview`):** Texto *"Entrar a Entrevista"*.
* **Evento de Clic en `Btn_Interview`:**
  1. `Get Game Instance` -> `Cast To MCSIMULATORGameInstance`.
  2. Llama a `Set Current Scenario` con:
     * **Scenario Id:** `"Interview"`
     * **Scenario Name:** `"Escritorio de Entrevista"`
     * **AI Context:** `"Entrevista de trabajo y reunión ejecutiva 1 a 1 frente a un entrevistador."`
  3. Llama a `Open Level by Name` escribiendo `InterviewMap`.

5. Compila y guarda `W_ScenarioMenu`.

---

### 🌐 Paso 2: Crear el Actor 3D para el Menú (`BP_LobbyMenuActor`)

1. En el **Content Drawer**, haz clic derecho -> **Blueprint Class** -> **Actor**. Nómbralo **`BP_LobbyMenuActor`**.
2. Ábrelo y haz clic en el botón verde **+ Add** -> añade un **Widget Component**.
3. En el panel **Details** a la derecha:
   * **Widget Class:** Selecciona **`W_ScenarioMenu`**.
   * **Draw Size:** Pon `1200 x 700`.
   * **Geometry:** Selecciona **Cylinder** (esto curvará la pantalla ligeramente para dar una visión panorámica futurista en VR).
4. Compila y guarda.

---

### 🌌 Paso 3: Configurar la Iluminación y Ambiente de los 4 Mapas

Navega a tu carpeta `Content/Maps`:

#### 1. 🌌 `LobbyMap.umap` (Menú Principal Elegante)
* **Creación:** `File` -> `New Level` -> `Basic` -> Guardar como `LobbyMap`.
* **Iluminación Elegante:**
  * Selecciona el **DirectionalLight**: Reduce la intensidad a `1.5` y cambia el tono a azul oscuro de estudio (`#101524`).
  * Añade un **SpotLight** sobre el **Player Start** enfocado hacia el menú con tono dorado suave (`#F4E2BB`).
* **Elementos en escena:**
  * Arrastra el **`BP_LobbyMenuActor`** al mapa frente al `Player Start`.
  * En **World Settings** -> **GameMode Override**, selecciona **`BP_MCSIMULATORGameMode`**.

#### 2. 🏛️ `AuditorioMap.umap` (Escenario de Auditorio)
* **Creación:** `File` -> `New Level` -> `Basic` -> Guardar como `AuditorioMap`.
* **Iluminación Dramática de Escenario:**
  * Selecciona el **DirectionalLight**: Pon su intensidad en `0.2` (simulando la oscuridad/penumbra donde se ubica el público).
  * Añade un **SpotLight (Foco de escenario)** justo arriba del `Player Start` apuntando hacia abajo con intensidad alta (`10.0`), creando la sensación inmersiva de estar parado bajo los reflectores de un teatro/auditorio.
* **Elementos:**
  * Arrastra el **`BP_VRFeedbackActor`** (pantalla de feedback) frente al escenario.
  * En **World Settings** -> **GameMode Override**, selecciona **`BP_MCSIMULATORGameMode`**.

#### 3. 🏫 `ClassroomMap.umap` (Escenario de Aula)
* **Creación:** `File` -> `New Level` -> `Basic` -> Guardar como `ClassroomMap`.
* **Iluminación de Aula:**
  * Selecciona el **DirectionalLight**: Luz blanca limpia de oficina/aula (`#F0F4F8`).
  * Añade luces **PointLight** rectangulares superiores simulando tubos LED de techo.
* **Elementos:**
  * Arrastra una malla oscura en la pared frontal simulando un pizarrón y coloca el **`BP_VRFeedbackActor`** incrustado.
  * En **World Settings** -> **GameMode Override**, selecciona **`BP_MCSIMULATORGameMode`**.

#### 4. 💼 `InterviewMap.umap` (Escenario de Entrevista)
* **Creación:** `File` -> `New Level` -> `Basic` -> Guardar como `InterviewMap`.
* **Iluminación Cálida Ejecutiva:**
  * Selecciona el **DirectionalLight**: Luz cálida de ventana de oficina (`3500K` / `#F8E0B0`).
* **Escala Intima:**
  * Coloca el `Player Start` a corta distancia (**1.2 metros**) del **`BP_VRFeedbackActor`** para dar la sensación inmersiva de estar sentado al otro lado de un escritorio frente al entrevistador.
  * En **World Settings** -> **GameMode Override**, selecciona **`BP_MCSIMULATORGameMode`**.

---

## 🤖 Servidor Backend de IA (`backend/server.py`)

El servidor externo analiza el audio capturado en VR y ajusta los parámetros de evaluación según el contexto enviado por cada mapa.

### Instalación de dependencias:
```bash
pip install google-genai flask
```

### Ejecución:
```bash
set GEMINI_API_KEY=TuClaveDeGeminiAqui
python backend/server.py
```
