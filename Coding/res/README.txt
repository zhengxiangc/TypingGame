TypingGame runtime assets (copied next to TypingGame.exe on each build)

Relative to the executable directory (QCoreApplication::applicationDirPath()):

  assets/sounds/   Short WAV files for QSoundEffect (see assets/sounds/README.txt)
  assets/music/    Background music (see assets/music/README.txt)

If audio files are missing, the game still runs; SFX/BGM are skipped until you add files.

After building, files are copied to:
  <build>/<Debug|Release>/assets/...
Absolute example (replace with your tree):
  D:\work\c++train\chenzhengxiang\ClassExamProject\TypingGame\build\Release\assets\

If copy did not run, manually copy the whole "assets" folder to sit beside TypingGame.exe.
