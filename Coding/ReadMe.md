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

### 1.3 太空大战（`spacewargame` + `KCTSpaceWarController`）

- 敌机带 **A–Z** 字母（同屏不重复），轨迹下落 + 摆动；玩家飞船默认左右移动；按对应键发射追踪弹、爆炸效果；触底或与玩家碰撞扣 **HP**（初始 **18**，为 0 结束）。
- **QSettings**（组织名 `TypingGame`，应用名 **`SpaceWar`**）可调：敌机数量、速度等级、升级间隔、**双倍得分（Bonus mode）**、**奖励模式（Reward mode）**、生成率等。
- **奖励模式（Reward mode）**（设置中勾选并保存后生效）：
  - 在 **无在飞奖励词** 且 **无未完成请求** 时，约每 **10 秒** 异步请求一个英文奖励词（长度约 4–8，领域在若干主题间轮换）。
  - 调用 **Deepseek** Chat Completions API（`KCTDeepSeekWordClient`，`services/deepseekwordclient.*`）；失败或无效响应时 **自动降级** 为本地词库，不中断游戏。
  - 奖励词 **横向飞过** 屏幕；玩家按顺序敲完全词后 **生命值恢复为 18**。
  - **API 密钥**：勿写入仓库。仅从系统/用户**环境变量** **`Deepseek_Api`** 读取（`qgetenv("Deepseek_Api")`，首尾空白会去掉）；不设界面输入。修改环境变量后需**重新启动**游戏进程才会生效。
- 源码均在 **`src/spacewar/`**，经目标 **`spacewar_embed`** 链入 **`TypingGame`**。
- **`spacewar/spacewar_main.cpp`** 供 **独立 CMake 工程**（例如课程或其它仓库中的 SpaceWar 子工程）单独生成 **SpaceWar.exe** 时使用，**不参与** `TypingGame` 目标编译。

---

## 2. 架构（MVC 与服务层）

| 层 | 位置 | 职责 |
|----|------|------|
| **Model** | `src/model/`、`src/spacewar/model/` | 实体与状态、配置结构体；无 UI。 |
| **Controller** | `src/controller/`、`src/spacewar/controller/` | 游戏规则、`tick`、输入处理；不创建界面控件。 |
| **View** | `mainwindow`、`applegame`、`spacewargame` | 窗口、绘制、`QTimer` 驱动 `tick`、键盘事件、与 **`KCTResourceConfig`** 样式配合。 |
| **配置** | `src/config/resourceconfig` | 样式与部分界面文案集中管理。 |
| **基础设施** | `src/services/` | 异步日志、JSON Lines 埋点、多线程音频（**`KCTGameAudioService`** 等）、**Deepseek 异步取词（`KCTDeepSeekWordClient`）**；规则层不直接写盘，由 View 在事件点调用。 |

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
    │   │   ├── resourceconfig.cpp / .h
    │   ├── services/               # 日志、埋点、音频、Deepseek 等
    │   │   ├── asynclogger.* / eventtracker.*
    │   │   ├── gameaudioservice.* / sfxplayer.* / bgmplayer.*
    │   │   ├── deepseekwordclient.*
    │   │   ├── sfxid.h / threadsafequeue.h
    │   └── spacewar/               # 太空大战（全部在此，无其他目录副本）
    │       ├── spacewar_main.cpp   # 仅独立 SpaceWar 工程入口
    │       ├── spacewargame.cpp / .h
    │       ├── controller/
    │       │   └── spacewarcontroller.cpp / .h
    │       └── model/
    │           ├── spacewarconfig.h
    │           ├── spacewarentities.h
    │           └── spacewarstatedata.h
    ├── res/                        # 构建时复制到 exe 旁 assets/（见 res/README.txt）
    │   ├── sounds/
    │   └── music/
    ├── mui/
    │   └── TypingGame_zh_CN.ts
    ├── tests/
    │   └── test_gamecontroller.cpp
    ├── script/
    │   ├── build_and_test.bat
    │   ├── test.bat
    │   └── cppcheck.bat
    └── skills/
```

---

## 4. CMake 目标

| 目标 | 说明 |
|------|------|
| **`typinggame_logic`** | 静态库：拯救苹果 **`KCTGameController`** 及对应 model；Qt **Core**。 |
| **`typinggame_infra`** | 静态库：**`KCTResourceConfig`**、**`services/`**（含 **`KCTDeepSeekWordClient`**）；Qt **Core + Network + Multimedia**。 |
| **`spacewar_embed`** | 静态库：太空大战 **`KCTSpaceWarController`**、**`KCTSpaceWarGame`**（`spacewargame`）；依赖 **`typinggame_infra`**。 |
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

---

## 6. 资源、日志与埋点（运行期）

- **资源**：相对 **`QCoreApplication::applicationDirPath()`**：**`assets/sounds/`**、**`assets/music/`**（由 **`res/`** 在构建后复制）。详见 **`res/README.txt`** 及各子目录说明；缺失音频时程序仍可运行。
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

- **Qt 6.5+**（以 **`CMakeLists.txt` 中 `find_package`** 为准；当前使用 **Core、Network、Widgets、Multimedia、LinguistTools、Test**）。
