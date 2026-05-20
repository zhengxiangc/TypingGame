# TypingGame — 项目总说明（classexam/Coding）

本文档为 **本工程总说明**：对应路径 **`classexam/Coding`**（源码与 CMake 根目录）。上级目录 **`classexam`** 为工作区根（本机示例：**`D:\work\c++train\classexam`**）：**`build_and_test.bat` / `test.bat`** 会先切换到该层再执行 `cmake` / `ctest`，生成目录为 **`classexam/build`**。根目录 **`classexam/.gitignore`** 已忽略 **`build/`**、运行期 **`logs/`**、**`telemetry/`**。

---

## 1. 功能概要

### 1.1 主菜单（`mainwindow`）

- 入口：**Save Apples**、**Space War**、**Exit**。
- 实现类 **`KCTMainWindow`**；选择子游戏后隐藏主窗口；子窗口关闭后回到主菜单。

### 1.2 拯救苹果（`applegame` + `KCTGameController`）

- 打字消除下落的苹果，计分、失败线、关卡与难度设置；**QSettings** 持久化（组织名 `TypingGame`，应用名 `SaveApples`）。
- 规则与数据在 **`model/`**（`KCTGameConfig`、`KCTGameStateData`、`KCTAppleEntity` 等）、**`controller/gamecontroller`**；界面与定时器、键盘、音频/日志在 **`KCTAppleGame`**（`applegame.cpp` / `applegame.h`）。
- 窗口为固定 **800×600**（`setFixedSize`），**未**使用 **`KCTGameViewport`**。

### 1.3 太空大战（`spacewargame` + `KCTSpaceWarController`）

- 敌机带 **A–Z** 字母（同屏不重复），轨迹下落 + 摆动；约 **28%** 为陨石（`SPACE_ENEMY_4`），其余为敌机（`SPACE_ENEMY_0`）。玩家飞船默认左右移动；按对应键发射追踪弹（`SPACE_BOMB`）、爆炸序列帧；触底或与玩家碰撞扣 **HP**（初始 **18**，为 0 结束）。
- **画面资源**：由 **`KCTSpaceWarAssets`**（`spacewar/spacewarassets.*`）从 **`assets/images/spacewar/`** 加载 PNG，在 **`paintEvent`** 中绘制背景、星空条（`SPACE_STARS` 1×5 切分）、飞船、敌机/陨石、字母框、子弹、爆炸、顶栏 HUD（生命条/得分/时间标签）。资源缺失时回退为简单几何绘制，游戏仍可运行。
- **音频**：由 **`KCTSpaceWarAudioService`**（`spacewar/spacewaraudio.*`）在独立线程播放，**不**共用拯救苹果的 **`KCTGameAudioService`**。BGM 为 **`assets/music/spacewar/SPACE_BG.wav`**；音效见下文「太空大战资源」。
- **QSettings**（组织名 `TypingGame`，应用名 **`SpaceWar`**）可调：敌机数量、速度等级、升级间隔、**双倍得分（Bonus mode）**、**奖励模式（Reward mode）**、生成率等。顶栏 **Settings** 打开 **360×360** 设置面板（`onSettingsClicked`，`spacewargame.cpp`）。
- **设置界面（内嵌面板，非 `QDialog`）**：为避免 Windows 上独立对话框先以系统默认**白底**显示再套用样式导致的闪屏，太空大战**不使用** `QDialog::exec()`，而在 **`KCTSpaceWarGame` 客户区内**叠加透明点击遮罩 **`KCTSettingsOverlay`**（文件内匿名类，只拦截鼠标、不绘制）与居中子控件 **`panel`**。面板背景为课程资源 **`APPLE_SETUP.png`**（与拯救苹果设置弹窗同源）；**奖励模式**复选框使用 **`CHECKBOX_BUTTON.png`**（左半=未选、右半=选中，以 `QIcon` 切换）。资源缺失时面板回退 **`KCTResourceConfig::dialogStyle()`** 纯色底。模态行为由 **`QEventLoop`** 实现（Save → 写 **QSettings** 并 `quit`，Cancel → `quit`）。进行中打开设置时仅**停止 `QTimer`、暂停 BGM**，**不**切到 `PausedGameState`，避免整屏重绘 “PAUSED” 叠层。拯救苹果（`applegame`）仍使用 **`QDialog`**，未改。
- **奖励模式（Reward mode）**（设置中勾选并保存后生效）：
  - 在 **无在飞奖励词** 且 **无未完成请求** 时，约每 **10 秒** 异步请求一个英文奖励词（长度约 4–8，领域在若干主题间轮换）。
  - 调用 **Deepseek** Chat Completions API（`KCTDeepSeekWordClient`，`services/deepseekwordclient.*`）；失败或无效响应时 **自动降级** 为本地词库，不中断游戏。
  - 奖励词 **横向飞过** 屏幕；玩家按顺序敲完全词后 **生命值恢复为 18**。
  - **API 密钥**：勿写入仓库。仅从系统/用户**环境变量** **`Deepseek_Api`** 读取（`qgetenv("Deepseek_Api")`，首尾空白会去掉）；不设界面输入。修改环境变量后需**重新启动**游戏进程才会生效。
- 源码均在 **`src/spacewar/`**，经目标 **`spacewar_embed`** 链入 **`TypingGame`**。
- **`spacewar/spacewar_main.cpp`** 供 **独立 CMake 工程**（例如课程或其它仓库中的 SpaceWar 子工程）单独生成 **SpaceWar.exe** 时使用，**不参与** `TypingGame` 目标编译。

#### 多分辨率 / DPI 与可缩放窗口（仅太空大战，已实现）

**状态**：`src/config/gameviewport.*` 与 `KCTSpaceWarGame` 视口逻辑已编入 **`typinggame_infra`** / **`spacewar_embed`**；主菜单与拯救苹果仍为固定 **800×600**，未接入视口。

**设计分辨率**为 **800×600**（`KCTGameViewport::designWidth()` / `designHeight()`）。窗口客户区可拖拽改变大小（`setMinimumSize(400, 300)`，无 `setFixedSize`）。游戏区缩放因子：

```text
scale = min(客户区宽 / 800, 客户区高 / 600)
gameWidth  = round(800 × scale)
gameHeight = round(600 × scale)
offsetX/Y  = 客户区与 gameWidth/gameHeight 居中差（letterbox 黑边）
```

非 4:3 客户区时，**widget 全区域涂黑**，仅在 `(offsetX, offsetY)` 处绘制 **gameWidth×gameHeight** 游戏区，避免拉伸变形。

**坐标约定**：

| 空间 | 说明 |
|------|------|
| **设计坐标** | 800×600 下的像素（按钮 `mapDesignX/Y`、常量 `s_design*`） |
| **游戏区坐标** | 当前缩放后的 `gameWidth×gameHeight`；**`KCTSpaceWarController::tick`**、敌机/子弹/玩家位置均在此空间 |
| **控件坐标** | 顶栏 Exit/Settings/Pause 为 **widget 客户区坐标**（设计坐标经 `mapDesignX/Y`）；敌机在 **`paintEvent`** 中于游戏区坐标绘制 |

**`KCTGameViewport`**（`config/gameviewport.h`、`gameviewport.cpp`，库 **`typinggame_infra`**）：

| API | 作用 |
|-----|------|
| `updateFromClientSize(w, h)` | 根据客户区重算 scale、gameWidth/Height、offset |
| `scale()` | 当前统一缩放比 |
| `gameWidth()` / `gameHeight()` | 缩放后游戏区尺寸 |
| `offsetX()` / `offsetY()` | letterbox 偏移 |
| `scaled(designPixels)` | 设计像素 → 游戏区像素（int/double 重载） |
| `mapDesignX/Y(design)` | 设计坐标 → widget 坐标（含 offset） |
| `defaultWindowSize()` | 首次显示：约 **85%** 主屏可用区域，且不低于 **800×600** 与 **DPI×设计分辨率** 的较大值 |

**View（`KCTSpaceWarGame`）**：

- **`showEvent`**：首次 `resize(defaultWindowSize())`，再 **`applyViewport()`**。
- **`resizeEvent`**：调用 **`applyViewport()`**。
- **`applyViewport()`**：`updateFromClientSize` → **`layoutTopButtons()`**（顶栏 Exit/Settings/Pause 的 geometry 与字号样式）→ 若游戏区尺寸变化则 **`relayoutControllerForViewport`** + **`regenerateStars()`**。
- **`paintEvent`**：`translate(offset)` 后在游戏区内按层绘制背景 → 星空 → 飞船 → 敌机/陨石（含字母框）→ 子弹 → 爆炸 → HUD（进行中/暂停）→ 奖励词 → 状态文案；尺寸用 **`scaled()`** 换算。敌机与 HUD 分别由 **`drawEnemies()`**、**`drawHud()`** 实现。
- **`onSettingsClicked`**：见上文「设置界面（内嵌面板）」。
- **`detectGameplayAudioEvents()`**：在 **`updateGame`** 中根据敌机数、爆炸数、`upgradeTier` 变化播放 **PLANEOUT / BLAST / UPGRADE** 音效。
- **`tick` / 输入**：传入 **`gameWidth()`、`gameHeight()`、`enemySize()`、`playerWidth()`、`playerHeight()`、`anchorPlayerY()`**（均由视口从 `s_design*` 换算）。

**Controller（`KCTSpaceWarController`）**：

| 方法 | 作用 |
|------|------|
| `startGame(config, gameWidth, playerWidth)` | 开局并将玩家水平居中 |
| `rescaleEntitiesForGameSize(oldW, oldH, newW, newH)` | **Playing / Paused** 下窗口缩放时，按比例缩放敌机 x/y/anchorX、子弹、爆炸、奖励词与玩家 x |
| `centerPlayer(gameWidth, playerWidth)` | **Idle / End**（及非进行中缩放）下保持飞船水平居中 |

**Model（`spacewarentities.h`）**：`KCTEnemyEntity` 含 **`isMeteor`**；`trySpawnEnemy` 约 **28%** 概率生成陨石，其余为敌机。

**样式与资源类**：

- **`KCTResourceConfig`**：`exitButtonStyle` / `settingsButtonStyle` / `pauseButtonStyle`（顶栏按钮）；`dialogStyle()` 供拯救苹果 **`QDialog`** 及太空大战设置面板**无图回退**时使用。
- **`KCTSpaceWarAssets`**：加载 **`assets/images/spacewar/*.png`**，提供背景、精灵、HUD、设置图、复选框半图、爆炸帧切片等。
- **`KCTSpaceWarAudioService`**：加载 **`assets/sounds/spacewar/`**、**`assets/music/spacewar/`**，见 §6。

**范围说明**：主菜单（**`mainwindow`**）与 **拯救苹果**（`setFixedSize(800,600)`）**未**做视口适配；后续可将 **`KCTGameViewport`** 复用到其它子游戏。

#### 太空大战资源（课程素材 → 工程 `res/` → 运行期 `assets/`）

课程原始包路径：**`res/游戏相关资源/Space/`**（`Images/`、`Sounds/`）。维护或替换素材时可从该处复制到下列目录后重新编译（POST_BUILD 会将整个 **`res/`** 复制为 **`assets/`**）。

| 类型 | 仓库路径（`Coding/res/`） | 运行期路径（exe 旁） | 文件名与用途 |
|------|---------------------------|----------------------|--------------|
| 图片 | `images/spacewar/` | `assets/images/spacewar/` | `SPACE_BACKGROUND` 背景；`SPACE_SHIP` 玩家；`SPACE_BOMB` 子弹；`SPACE_ENEMY_0` 敌机；`SPACE_ENEMY_4` 陨石；`SPACE_CAPTION_BACK` 字母框；`SPACE_EXPLOSION_0` 爆炸（3×3 序列帧）；`SPACE_LIFE` / `SPACE_LIFE_OVER` 生命条；`SPACE_LABEL_LIFE` / `SCORE` / `TIME` HUD；`SPACE_STARS` 星空 1×5 色条；`APPLE_SETUP` 设置背景；`CHECKBOX_BUTTON` 奖励模式复选框 |
| 音效 | `sounds/spacewar/` | `assets/sounds/spacewar/` | `SPACE_BLAST` 击毁；`SPACE_PLANEOUT` 敌机出现；`SPACE_SHOOT` 正确按键发射；`SPACE_WORDOUT` 奖励词出现；`UPGRADE` 难度档位提升 |
| BGM | `music/spacewar/` | `assets/music/spacewar/` | `SPACE_BG.wav` 循环背景音乐 |

各子目录另有 **`README.txt`**（`res/images/spacewar/README.txt` 等）列出完整文件名。

---

## 2. 架构（MVC 与服务层）

| 层 | 位置 | 职责 |
|----|------|------|
| **Model** | `src/model/`、`src/spacewar/model/` | 实体与状态、配置结构体；无 UI。 |
| **Controller** | `src/controller/`、`src/spacewar/controller/` | 游戏规则、`tick`、输入处理；不创建界面控件。 |
| **View** | `mainwindow`、`applegame`、`spacewargame` | 窗口、绘制、`QTimer` 驱动 `tick`、键盘事件；拯救苹果用 **`KCTResourceConfig`** + **`KCTGameAudioService`**；太空大战用 **`KCTSpaceWarAssets`** + **`KCTSpaceWarAudioService`**。 |
| **配置** | `src/config/resourceconfig`、`src/config/gameviewport` | 样式与部分界面文案；**`KCTGameViewport`** 负责设计分辨率下的等比缩放与 letterbox。 |
| **基础设施** | `src/services/`、`src/spacewar/spacewaraudio.*` | 异步日志、JSON Lines 埋点；拯救苹果多线程音频（**`KCTGameAudioService`**）；**Deepseek 取词（`KCTDeepSeekWordClient`）**；太空大战专用 **`KCTSpaceWarAudioService`**。 |

---

## 3. 目录结构

### 3.1 工作区（`classexam`）与 `Coding` 的关系

```
classexam/                          # 工作区根（build_and_test.bat / test.bat 的隐含 cwd）
├── .gitignore                      # 忽略 build/、logs/、telemetry/ 等
├── build/                          # CMake 生成目录（本地生成，勿提交）
│   └── Release/ 或 Debug/         # 多配置生成器在首次成功 build 后出现；见下文「构建」
│
└── Coding/                         # 本 ReadMe 所在目录 = 工程源码根
    ├── CMakeLists.txt
    ├── CMakePresets.json           # 可选：default 预设，binaryDir → ../build，VS 2022 x64
    ├── ReadMe.md                   # 本总说明
    ├── src/
    │   ├── main.cpp
    │   ├── mainwindow.cpp / .h / mainwindow.ui
    │   ├── applegame.cpp / .h
    │   ├── model/                  # 拯救苹果
    │   │   ├── appleentity.h
    │   │   ├── gameconfig.h
    │   │   └── gamestatedata.h
    │   ├── controller/
    │   │   ├── gamecontroller.cpp / .h
    │   ├── config/
    │   │   ├── gameviewport.cpp / .h      # 视口缩放（太空大战 DPI / 可拖拽窗口）
    │   │   ├── resourceconfig.cpp / .h
    │   ├── services/               # 日志、埋点、音频、Deepseek 等
    │   │   ├── asynclogger.* / eventtracker.*
    │   │   ├── gameaudioservice.* / sfxplayer.* / bgmplayer.*
    │   │   ├── deepseekwordclient.*
    │   │   ├── sfxid.h / threadsafequeue.h
    │   └── spacewar/               # 太空大战（全部在此，无其他目录副本）
    │       ├── spacewar_main.cpp   # 仅独立 SpaceWar 工程入口
    │       ├── spacewargame.cpp / .h
    │       ├── spacewarassets.cpp / .h   # 图片资源加载
    │       ├── spacewaraudio.cpp / .h    # 太空大战专用 SFX/BGM
    │       ├── controller/
    │       │   └── spacewarcontroller.cpp / .h
    │       └── model/
    │           ├── spacewarconfig.h
    │           ├── spacewarentities.h
    │           └── spacewarstatedata.h
    ├── res/                        # 构建时复制到 exe 旁 assets/（见 res/README.txt）
    │   ├── 游戏相关资源/           # 课程原始素材（Apple/Space/Common 等，勿删）
    │   ├── images/spacewar/        # 太空大战运行时图片（由 Space/Images 整理）
    │   ├── sounds/                 # 拯救苹果 SFX + sounds/spacewar/
    │   └── music/                  # 拯救苹果 BGM + music/spacewar/
    ├── mui/
    │   └── TypingGame_zh_CN.ts
    ├── tests/
    │   └── test_gamecontroller.cpp
    ├── script/
    │   ├── build_and_test.bat
    │   ├── test.bat
    │   ├── where_is_game.bat
    │   └── cppcheck.bat
    └── skills/
```

---

## 4. CMake 目标

| 目标 | 说明 |
|------|------|
| **`typinggame_logic`** | 静态库：拯救苹果 **`KCTGameController`** 及对应 model；Qt **Core**。 |
| **`typinggame_infra`** | 静态库：**`KCTResourceConfig`**、**`KCTGameViewport`**、**`services/`**（含 **`KCTDeepSeekWordClient`**）；Qt **Core + Gui + Network + Multimedia**。 |
| **`spacewar_embed`** | 静态库：太空大战 **`KCTSpaceWarController`**、**`KCTSpaceWarGame`**、**`KCTSpaceWarAssets`**、**`KCTSpaceWarAudioService`**；依赖 **`typinggame_infra`**。 |
| **`TypingGame`** | **主可执行文件**：主菜单 + 拯救苹果 + 太空大战；`qt6_add_translations`；**`PRIVATE` 增加 `src` 包含目录**；Windows 下 **`windeployqt`** + **`res` → `assets`**。 |
| **`TypingGameLogicTests`** | Qt Test：拯救苹果控制器（**`KCTGameController`**）；`QTEST_GUILESS_MAIN`。输出在 **`build/<Config>/tests/`**，与主程序 **`TypingGame.exe`** 分开，避免误运行单测程序。 |

多配置生成器下：**`TypingGame.exe`** 在 **`${CMAKE_BINARY_DIR}/$<CONFIG>/`**；**`TypingGameLogicTests.exe`** 在 **`${CMAKE_BINARY_DIR}/$<CONFIG>/tests/`**（仅用于 **`ctest`** 或自测，不是游戏）。

---

## 5. 构建、运行与常见问题

**构建与测试脚本**（`Coding\script\build_and_test.bat`、`test.bat`）会自行切换到 **`classexam` 根目录**；你也可在资源管理器中于该根目录打开终端后调用 **`Coding\script\...`**。未传参时 **`build_and_test.bat` / `test.bat` 默认使用 `Debug`**；需要 Release 时显式传入 **`Release`**。

一键配置 + 编译 + 测试：

```bat
Coding\script\build_and_test.bat
Coding\script\build_and_test.bat Release
```

**`build_and_test.bat` 会按顺序探测 Qt 路径**（如 `D:\work\ST\QT\6.11.0\msvc2022_64`、`C:\Qt\6.5.0\msvc2019_64` 等）；若均未命中，请设置环境变量 **`QT6_DIR`**（指向 Qt 安装根，脚本会去掉 `\lib\cmake\Qt6` 后缀以得到 `CMAKE_PREFIX_PATH`），或改用下方手动 **`cmake`** 并自行指定 **`-DCMAKE_PREFIX_PATH`**。

使用 **CMake Presets**（在 **`Coding`** 目录）：**`CMakePresets.json`** 的 **`default`** 将 **`binaryDir`** 设为 **`${sourceDir}/../build`**（与在 **`classexam` 根**执行 **`cmake -S Coding -B build`** 等价），生成器为 **Visual Studio 17 2022**、**x64**。

```bat
cd Coding
cmake --preset default
cmake --build ..\build --config Release
ctest --test-dir ..\build -C Release --output-on-failure
```

手动示例（**当前目录为 `classexam` 根**）：

```bat
cmake -S Coding -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

仅运行已有构建树中的测试：

```bat
Coding\script\test.bat
Coding\script\test.bat Release
```

指定 Qt 安装路径（示例）：

```bat
cmake -S Coding -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.5.0\msvc2019_64
```

**主程序路径（Visual Studio 等多配置生成器）**：  
`build\Release\TypingGame.exe` 或 `build\Debug\TypingGame.exe`。

**不要与单元测试混淆**：同一次构建还会在 **`build\Release\tests\TypingGameLogicTests.exe`**（或 **`Debug\tests\...`**）生成 **Qt Test** 可执行文件；它没有主菜单，运行后会执行测试用例并退出，**不是游戏**。若需要玩游戏，请只运行 **`TypingGame.exe`**。构建成功后 **`Coding\script\build_and_test.bat`** 会在结尾打印上述两条路径。

### 配置后没有 `Release` 或 `Debug` 子目录？

- 仅执行 **`cmake -S Coding -B build` 而未编译** 时，往往还没有包含 **`TypingGame.exe`** 的配置子目录；请先 **`cmake --build build --config Release`**（或运行 **`build_and_test.bat Release`**）。
- 若使用 **Ninja** 等**单配置**生成器，可执行文件可能在 **`build/`** 根目录，无 **`Release`/`Debug`** 子目录，由 **`CMAKE_BUILD_TYPE`** 决定。

### `build\Release` 里没有 `TypingGame.exe`？

按当前 **Visual Studio 多配置** 工程，主游戏**应当**在 **`build\Release\TypingGame.exe`**（Debug 则在 **`build\Debug\`**）。若目录里**只有** `tests\` 或其它文件、**没有** `TypingGame.exe`，常见原因：

1. **没有成功编译 `TypingGame` 目标**（链接/编译报错但被忽略）：请在 **`classexam` 根**执行  
   `cmake --build build --config Release --target TypingGame`  
   查看完整输出是否报错。
2. **在 IDE 里只生成/运行了 `TypingGameLogicTests`**：需在解决方案中生成 **`ALL_BUILD`** 或项目 **`TypingGame`**；启动游戏请选 **`TypingGame`** 为启动项目，不要选测试项目。
3. **看错配置**：找的是 **`Release`** 目录，但实际只编过 **Debug**，请查看 **`build\Debug\TypingGame.exe`**。
4. **单配置生成器**：可执行文件可能在 **`build\TypingGame.exe`**，不在 **`build\Release\`**。

成功执行 **`Coding\script\build_and_test.bat Release`** 时，若缺少主程序，脚本会报错并提示上述排查方向。

**本机仍找不到 exe 时**：在 **`classexam` 根目录**双击或运行 **`Coding\script\where_is_game.bat`**，会在窗口里列出是否存在于 `build\Release`、`build\Debug`、`build\` 根目录，并递归搜索 **`TypingGame.exe`**；若都没有，脚本会提示先执行完整编译。

### 太空大战奖励模式与网络

- 使用 Deepseek 时需本机可访问 **`https://api.deepseek.com`**，并在环境中设置 **`Deepseek_Api`**。
- 未设置 **`Deepseek_Api`** 时仍可通过 **本地词库** 体验奖励词飞过与回血逻辑。

### 编译错误：找不到 `config/gameviewport.h`（C1083）

多出现在**仅拉取/修改了 `spacewargame.h` 却未同步 `gameviewport.*`**，或 **CMake 缓存未刷新** 时。处理步骤：

1. 确认存在 **`Coding/src/config/gameviewport.h`** 与 **`gameviewport.cpp`**。
2. 确认 **`CMakeLists.txt`** 中 **`typinggame_infra`** 已列出上述两个文件并链接 **`Qt::Gui`**。
3. 在 **`classexam` 根**重新配置并编译：
   ```bat
   cmake -S Coding -B build -DCMAKE_PREFIX_PATH=<Qt安装根>
   cmake --build build --config Debug --target TypingGame
   ```

### 太空大战 DPI / 可拖拽窗口自测

1. 运行 **`TypingGame.exe`** → 进入 **Space War**。
2. **首次打开**：窗口大小应接近当前显示器可用区域（约 **85%**）且不低于 **DPI×800×600** 与 **800×600** 的较大值；**100% 缩放** 的 1080p 屏上可接近 **800×600** 客户区。
3. **拖拽窗口边缘**：游戏区保持 **4:3**，两侧或上下出现**黑边**；敌机、碰撞、顶栏按钮与字体随游戏区同步缩放；**Playing / Paused** 过程中敌机位置应连续缩放，不应被“挤”到异常坐标。
4. **Idle（按 ENTER 前）** 与 **Game Over**：玩家飞船应处于游戏区**水平正中**（`centerPlayer`）。
5. 修改 **`config/gameviewport.*`**、**`spacewargame.*`**、**`spacewarassets.*`**、**`spacewaraudio.*`**、**`spacewarcontroller.*`** 或 **`CMakeLists.txt`** 后，需重新 **`cmake`** 并完整编译 **`TypingGame`**（见上文构建说明）。仅替换 **`res/images|sounds|music/spacewar/`** 下文件时，重编 **`TypingGame`** 即可触发 **`res` → `assets`** 复制。
6. **Settings**：弹出设置面板时**不应**在面板区域闪白；背景应为 **`APPLE_SETUP`** 图，奖励模式复选框为 **`CHECKBOX_BUTTON`** 双态图标；若改回 `QDialog`，Windows 上可能再现系统窗口默认白底的一帧闪烁。
7. **画面资源**：`build\<Config>\assets\images\spacewar\` 下应有 `SPACE_BACKGROUND.png`、`SPACE_SHIP.png` 等；进入对局后应看到位图背景/HUD，而非纯色块（缺失文件时才会回退几何绘制）。
8. **音频**：`assets\music\spacewar\SPACE_BG.wav` 对局循环；击毁、敌机出现、发射、奖励词、升级时分别应有对应 WAV（见 §1.3 资源表）。

### 更新太空大战素材（维护说明）

1. 从课程包 **`res/游戏相关资源/Space/Images`**、**`Sounds`** 取文件，复制到 **`res/images/spacewar/`**、**`res/sounds/spacewar/`**、**`res/music/spacewar/`**（文件名与代码一致，见各子目录 **`README.txt`**）。
2. 重新 **`cmake --build build --config <Debug|Release> --target TypingGame`**，POST_BUILD 会将 **`res/`** 同步到 **`build/<Config>/assets/`**。
3. 若只改了 `res/` 未重编，可手动将 **`Coding/res`** 整目录复制到 **`TypingGame.exe`** 同级 **`assets/`**（须保持子目录结构）。

### 太空大战设置面板闪屏（说明）

| 现象 | 原因 |
|------|------|
| 点击 Settings 后，**仅弹出区域**闪一下白 | 独立 **`QDialog`** 在 Windows 上会新建原生窗口，系统常先以**默认白底**显示，随后 Qt 才应用 **`dialogStyle()`** |
| 整屏闪或出现 “PAUSED” 叠层 | 打开设置时切 **`PausedGameState`** 导致整屏重绘（已通过不停表状态、内嵌面板缓解） |

**当前做法**：在游戏窗口内 **`KCTSettingsOverlay` + `panel` + `QEventLoop`**，不创建独立对话框窗口；面板首帧即显示 **`APPLE_SETUP`** 背景图（缺失时回退 **`dialogStyle()`** 深灰底）。`typinggame_infra` 仅 **Qt::Gui**，设置相关 **`QWidget`** 逻辑在 **`spacewar_embed`（Qt Widgets）** 内实现。

---

## 6. 资源、日志与埋点（运行期）

- **资源**：相对 **`QCoreApplication::applicationDirPath()`**，由 **`res/`** 在每次构建后整目录复制为 **`assets/`**：
  - **拯救苹果**：`assets/sounds/`（`hit.wav`、`miss.wav`、`level.wav`）、`assets/music/bgm.mp3`
  - **太空大战**：`assets/images/spacewar/`、`assets/sounds/spacewar/`、`assets/music/spacewar/SPACE_BG.wav`
  - 详见 **`res/README.txt`** 及 `res/images/spacewar/README.txt` 等子目录说明。
- **缺失资源**：拯救苹果/太空大战均在对应文件不存在时**跳过** SFX/BGM 或**回退**简单绘制，主流程不崩溃。
- **课程素材**：`res/游戏相关资源/Space/`、`Apple/` 等为原始包；修改运行时资源时请同步更新 `res/images/spacewar/`（或重新从 `Space/Images` 复制）并重新编译，以便 POST_BUILD 复制到 exe 目录。
- **日志**：**`logs/`** 下按日期滚动的日志文件（由 **`KCTAsyncLogger`** 写入，具体文件名以实现为准）。
- **埋点**：**`telemetry/events.jsonl`**（JSON Lines，**`KCTEventTracker`**）。

上述目录一般在**首次写入**时创建，需在相应操作后于 exe 同级路径查看。

---

## 7. 单元测试与随机数

- 测试通过 **`spawnRate = 0`** 等配置减少对 **`tick`** 内随机生成的依赖；需覆盖 **`ensureMinimumApples` / `tryCreateApple`** 时用确定性字段（如 **`maxApples`**）。
- **Qt 6.11+** 不允许对 **`QRandomGenerator::global()`** 再 **`seed`**，本工程**不在**测试中重置全局 RNG。

---

## 8. 静态分析（cppcheck）

需本机 **`cppcheck`** 在 **`PATH`** 中。在 **`classexam/Coding`** 下：

```bat
script\cppcheck.bat
```

---

## 9. 依赖与许可（简述）

- **Qt 6.5+**（以 **`CMakeLists.txt` 中 `find_package`** 为准；当前使用 **Core、Gui、Network、Widgets、Multimedia、LinguistTools、Test**）。**`KCTGameViewport`**（`defaultWindowSize`）依赖 **Gui**（`QGuiApplication` / `QScreen`），由 **`typinggame_infra`** 链接 **Qt::Gui**。
