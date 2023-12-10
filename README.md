# ShiftFlamework
## 必要なソフトウェア
- git
- google depot tools
- ninja (wasm向けのビルドをする場合)
- emscripten (wasm向けのビルドをする場合)
- cmake
## クローン
```
git clone https://github.com/UweSilver/ShiftFlamework.git
```
最近のgitのGUIクライアント(GitHub Desktop, GitKraken など)は`clone`するときに `--recurse` オプションを勝手につけるようです。これが利用しているdawnのクローン時に問題を起こすため、`--recurse`でない手段でクローンしてください。

次にリポジトリのディレクトリ直下で
```
git pull --force
git submodule init
git submodule update
```
lib\dawn内で、
```
cp scripts/standalone.gclient .gclient
gclient sync
```	

## ビルド、実行
### windows
リポジトリのディレクトリ直下で
```
.\build\win.bat
.\out\win\Debug\game.exe
```
### web
リポジトリのディレクトリ直下で
```
emcmake cmake -B out\web
cmake --build .\out\web
cp .\resource\index.html .\out\web\index.html
cd .\out\web
python -m http.server 8080
```
ブラウザで http://localhost:8080/index.html 

## 必要ツール類のインストール（参考）
### windows
#### google depot tools

https://www.chromium.org/developers/how-tos/depottools/#installing

に従ってdepot toolをインストールし、Pathを通しておく。

#### emscripten

https://emscripten.org/docs/getting_started/downloads.html

に従ってインストールし、Pathを通す。

#### ninja

https://ninja-build.org/

のGetting Startedに従ってダウンロードし、Pathを通す。depot toolsをインストールしている場合、depot tools内のninjaが使われてしまう場合があるため、Path内でdepot toolsより前に記述して、こちらが使われるようにしておく。

#### cmake
https://cmake.org/download/#latest のWindows x64 Installerをダウンロードして実行する。

#### VisualStudio

https://visualstudio.microsoft.com/ja/

からVisual Studioをインストールする。Visual Studio Installer内のワークロードを選択する画面で"C++によるデスクトップ開発"にチェックマークを入れる。

## コード規約
### 命名
- 関数、変数名はスネークケース (例: snake_case)

- 名前空間名・クラス名はパスカルケース(アッパーキャメルケース) (例: PascalCase)

で命名する

### メモリ管理
new/destroy及び生ポインタによるメモリの管理は禁止。`std::shared_ptr, std::unique_ptr, std::weak_ptr`などのスマートポインタを使う。

### Gitのコミットログ・プルリクエスト
日本語の命令形で変更内容を簡潔に記述する。

例: READMEにコード規約を追加する

### 最適化について
1. 最適化するな
2. まだ早い
3. 本当にそこが原因で遅いのかちゃんと調べる
4. コードの可読性を損なわないように、広範囲に変更の影響が出ないように最適化してもよい


### DLL export\import
#### エンジン・スクリプトのどちらに実装するか
スクリプトのコンパイル時間が増大するのを防ぎたいので、できるだけエンジンに実装する。
#### メンバ関数の名前
`ネームスペース_クラス_クラステンプレート引数_関数_関数テンプレート引数_引数`

## 使い方
### テスクチャの追加
1. GIMPで.hにエクスポートする
2. `static char* header_data = ...` を`static uint8_t header_data[] = ...`に書き換える
3. 
```
#include "テクスチャの名前.h"

std::vector<uint8_t> pixels(
    4 * texture_desc.size.width * texture_desc.size.height, 0);

{
    uint32_t idx = 0;
    for (uint32_t i = 0; i < width; i++) {
        for (uint32_t j = 0; j < height; j++) {
            std::array<uint8_t, 4> data = {};
            data.at(0) = (uint8_t)header_data[idx];
            data.at(1) = (uint8_t)header_data[idx + 1];
            data.at(2) = (uint8_t)header_data[idx + 2];
            data.at(3) = (uint8_t)header_data[idx + 3];
            idx += 4;

            pixels.at(4 * ((width - 1 - i) * height + j) + 0) =
                (((data.at(0) - 33) << 2) | ((data.at(1) - 33) >> 4));
            pixels.at(4 * ((width - 1 - i) * height + j) + 1) =
                ((((data.at(1) - 33) & 0xF) << 4) | ((data.at(2) - 33) >> 2));
            pixels.at(4 * ((width - 1 - i) * height + j) + 2) =
                ((((data.at(2) - 33) & 0x3) << 6) | ((data.at(3) - 33)));
        }
    }
}
```