TypingGame runtime assets (copied next to TypingGame.exe on each build)

Relative to the executable directory (QCoreApplication::applicationDirPath()):

  assets/sounds/           Save Apples SFX (hit.wav, miss.wav, level.wav)
  assets/sounds/spacewar/  Space War SFX (SPACE_*.wav, UPGRADE.wav)
  assets/music/            Save Apples BGM (bgm.mp3)
  assets/music/spacewar/   Space War BGM (SPACE_BG.wav)
  assets/images/spacewar/  Space War sprites and HUD (see images/spacewar/README.txt)
  assets/images/menu/      Main menu logo, header, game thumbnails (see images/menu/README.txt)

If audio files are missing, the game still runs; SFX/BGM are skipped until you add files.

After building, files are copied to:
  <build>/<Debug|Release>/assets/...
Absolute example (replace with your tree):
  D:\work\c++train\chenzhengxiang\ClassExamProject\TypingGame\build\Release\assets\

If copy did not run, manually copy the whole "assets" folder to sit beside TypingGame.exe.
