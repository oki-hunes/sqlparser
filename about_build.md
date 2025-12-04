# ビルドに関するメモ (about_build.md)

## ビルド環境
- **OS**: Windows
- **Compiler**: Visual C++ 2026 (MSVC 14.50.x)
- **Build System**: CMake + NMake

## ビルドコマンド
Visual Studio の環境変数が正しく引き継がれない場合があるため、以下のコマンドを使用して `vcvars64.bat` を明示的に呼び出してから `nmake` を実行することを推奨します。

```powershell
cmd /c "call ""D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat"" && nmake"
```

## 再構築について
- **ソースコードの変更のみ (.cpp, .hpp の修正)**:
  上記の `nmake` コマンドを実行するだけで十分です。CMake の再実行（Makefile の再生成）は不要です。

- **ファイルの追加・削除 / プロジェクト構成の変更**:
  `CMakeLists.txt` を変更した場合や、新しいソースファイルを追加した場合は、CMake を再実行して Makefile を更新する必要があります。

## トラブルシューティング
- **`nmake` や `cmake` が見つからないエラー**:
  PowerShell やコマンドプロンプトのセッションで環境変数が正しく設定されていない可能性があります。上記の「ビルドコマンド」のように、コマンド実行時に毎回 `vcvars64.bat` を通すことで確実にビルドできます。
