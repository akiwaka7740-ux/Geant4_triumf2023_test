# VS Code で C/C++ の include パスエラーを解消する方法

## 目的

このメモは、VS Code で C/C++ のヘッダが見つからない、または include パスが解決できないときに、どのような設定を行ったかを整理したものです。

Geant4 プロジェクトでは、外部ライブラリのヘッダや自分のプロジェクト内の `include/` ディレクトリを正しく認識させないと、IntelliSense が「include file not found」や「path not found」と表示します。

## まず理解しておくこと

VS Code の C/C++ 機能は、単にファイルを開くだけではヘッダの場所を自動で完璧に知りません。特に次のような場合に誤判定しやすいです。

- プロジェクトが CMake で構成されている
- Geant4 のヘッダが外部インストール先にある
- 自分のコードが `include/` 配下に分かれている
- 以前の IntelliSense キャッシュが残っている

そのため、VS Code に対して「どのディレクトリを include すべきか」を明示的に伝える必要があります。

## まず行ったこと

### 1. CMake で `compile_commands.json` を生成した

CMake では、次のように実行してコンパイル情報を出力しました。

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

この操作の意味は、C/C++ のコンパイラが実際にどの include パスでソースをコンパイルしたかを記録することです。

#### `compile_commands.json` の役割

- CMake が生成する、コンパイルコマンドの一覧です。
- VS Code の C/C++ 拡張が、各ソースファイルごとの include パスを理解するための重要な情報源です。
- これがあると、Geant4 やプロジェクト内のヘッダを正しく解決しやすくなります。

#### なぜ必要か

ヘッダの場所は、単純にファイル名だけでは分かりません。コンパイラに渡された `-I` オプションやインクルードディレクトリの情報が必要です。`compile_commands.json` は、その情報を機械が読み取りやすい形でまとめたものです。

## 2. VS Code の設定ファイル `.vscode/c_cpp_properties.json` を作成・編集した

このファイルは、VS Code の C/C++ IntelliSense に対して「このワークスペースではどういう設定で解析するか」を伝えるための設定ファイルです。

### 典型的な内容

```json
{
  "configurations": [
    {
      "name": "Linux",
      "compileCommands": "${workspaceFolder}/build/compile_commands.json",
      "includePath": [
        "${workspaceFolder}/include",
        "${workspaceFolder}/incase",
        "/home/hashizume/Geant4/install/include",
        "/home/hashizume/Geant4/install/include/Geant4",
        "/usr/local/root_6.36.06/include"
      ],
      "browse": {
        "path": [
          "${workspaceFolder}/include",
          "/home/hashizume/Geant4/install/include",
          "/home/hashizume/Geant4/install/include/Geant4",
          "/usr/local/root_6.36.06/include",
          "/usr/local/root_6.36.06/include/**"
        ]
      },
      "compilerPath": "/usr/bin/g++",
      "cppStandard": "c++20",
      "intelliSenseMode": "linux-gcc-x64"
    }
  ],
  "version": 4
}
```

### 各項目の意味

#### `compileCommands`

- `build/compile_commands.json` を参照するように指定します。
- これにより、VS Code が CMake のコンパイル情報を読み取るようになります。

#### `includePath`

- プロジェクトや外部ライブラリのヘッダが置かれているディレクトリを指定します。
- ここで `include/` や Geant4 のインクルードパスを追加することで、`#include <...>` や `#include "..."` を解決しやすくなります。
- ROOT のヘッダを使う場合は、ROOT の include ディレクトリを追加します。
- 例えば `#include <TFile.h>` や `#include <ROOT/RDataFrame.hxx>` を使う場合、次の親ディレクトリを指定します。

```json
"/usr/local/root_6.36.06/include"
```

- `#include <ROOT/RDataFrame.hxx>` のために `/usr/local/root_6.36.06/include/ROOT` を指定するのではなく、その親である `/usr/local/root_6.36.06/include` を指定します。

#### `browse.path`

- IntelliSense のシンボル検索やヘッダ探索に使われるパスです。
- `includePath` と同じ階層、つまり `"name"`, `"compileCommands"`, `"includePath"` と同じ `{ ... }` の中に書きます。
- `includePath` の中に `browse` を入れてはいけません。
- ROOT のようにサブディレクトリ配下にも多くのヘッダがある場合は、次のように `/**` 付きのパスも入れると索引されやすくなります。

```json
"browse": {
  "path": [
    "${workspaceFolder}/include",
    "/home/hashizume/Geant4/install/include/Geant4",
    "/usr/local/root_6.36.06/include",
    "/usr/local/root_6.36.06/include/**"
  ]
}
```

#### `compilerPath`

- 使うコンパイラの実体を指定します。
- ここでは `g++` を使う前提で設定しました。

#### `cppStandard`

- C++ の標準規格を指定します。
- Geant4 を扱う場合、`c++17` などがよく使われます。
- ただし、ROOT を一緒に使う場合は、ROOT がどの C++ 標準でビルドされているかに合わせる必要があります。
- 今回の環境では `/usr/local/root_6.36.06/bin/root-config --cflags` が `-std=c++20` を返すため、VS Code 側も `"cppStandard": "c++20"` に合わせます。
- `TFile.h` のような基本的なヘッダは `c++17` 設定でも読めることがありますが、`ROOT/RDataFrame.hxx` はより新しい C++ 機能や ROOT 内部ヘッダを多く使うため、C++ 標準が合っていないと IntelliSense が失敗しやすくなります。

## ROOT 解析マクロで追加した設定

今回、Geant4 の出力 ROOT ファイルを解析する ROOT マクロを書くために、VS Code に ROOT のヘッダを認識させました。

確認した ROOT の場所は次の通りです。

```bash
/usr/local/root_6.36.06/bin/root-config --incdir
```

結果として、ROOT の include ディレクトリは次の場所でした。

```text
/usr/local/root_6.36.06/include
```

`TFile.h`, `TTree.h`, `TH1.h` はこの直下にあります。

```text
/usr/local/root_6.36.06/include/TFile.h
/usr/local/root_6.36.06/include/TTree.h
/usr/local/root_6.36.06/include/TH1.h
```

一方、`RDataFrame` は `ROOT/` サブディレクトリの下にあります。

```text
/usr/local/root_6.36.06/include/ROOT/RDataFrame.hxx
```

そのため、マクロでは次のように include します。

```cpp
#include <TFile.h>
#include <TTree.h>
#include <ROOT/RDataFrame.hxx>
```

このとき `.vscode/c_cpp_properties.json` では、次のように ROOT include の親ディレクトリを指定します。

```json
"includePath": [
  "${workspaceFolder}/include",
  "/home/hashizume/Geant4/install/include/Geant4",
  "/usr/local/root_6.36.06/include"
],
"browse": {
  "path": [
    "${workspaceFolder}/include",
    "/home/hashizume/Geant4/install/include/Geant4",
    "/usr/local/root_6.36.06/include",
    "/usr/local/root_6.36.06/include/**"
  ]
},
"cppStandard": "c++20"
```

### `TFile.h` は読めるのに `ROOT/RDataFrame.hxx` が読めない理由

`TFile.h` は ROOT include ディレクトリ直下にある古くからの基本ヘッダです。

```text
/usr/local/root_6.36.06/include/TFile.h
```

一方、`RDataFrame.hxx` は次のように `ROOT/` サブディレクトリの下にあります。

```text
/usr/local/root_6.36.06/include/ROOT/RDataFrame.hxx
```

さらに `RDataFrame.hxx` は内部で多くの ROOT RDF ヘッダや比較的新しい C++ 機能を使います。そのため、単に `TFile.h` が読めているだけでは、`RDataFrame` まで正しく IntelliSense できるとは限りません。

今回の ROOT 6.36.06 は `root-config --cflags` で `-std=c++20` を返すため、VS Code 側の `cppStandard` も `c++20` に合わせる必要があります。

## ROOT マクロが `compile_commands.json` に載らないことがある

このプロジェクトの `CMakeLists.txt` では、主に `sim.cc` と `src/*.cc` がビルド対象になっています。

ROOT 解析マクロを `analysis/*.C` に置いた場合、そのマクロは Geant4 シミュレーション本体のビルド対象ではないため、`build/compile_commands.json` に載らないことがあります。

その場合、VS Code は `compile_commands.json` から解析情報を取れないため、`.vscode/c_cpp_properties.json` の `includePath` や `browse.path` がより重要になります。

ROOT マクロを VS Code で開いたときに include エラーが出る場合は、次も確認します。

- VS Code で開いているフォルダがプロジェクトルートになっているか
- `.vscode/c_cpp_properties.json` がそのワークスペースで読まれているか
- 右下の言語モードが `C++` になっているか
- `C/C++: Log Diagnostics` で、そのファイルに使われている include path を確認する

## 3. IntelliSense のキャッシュをリセットした

設定を変えたあと、VS Code の IntelliSense が古い情報を使い続けてしまうことがあります。そのため、コマンドパレットで次のコマンドを実行しました。

- `C/C++: Reset IntelliSense Database`

これにより、VS Code が新しい設定で再解析し直します。

## それぞれのファイルが何のために必要だったか

### `compile_commands.json`

- 実際のコンパイルコマンドをまとめたファイルです。
- VS Code に「どの include パスでコンパイルされるか」を伝えるために使います。
- これがないと、IntelliSense はヘッダの場所を推測しにくくなります。

### `.vscode/c_cpp_properties.json`

- VS Code に対して、C/C++ の解析方法を指示する設定ファイルです。
- `compileCommands` や `includePath` を指定して、IntelliSense が正しく動くようにします。
- このファイルがないと、VS Code はヘッダの位置を見つけられません。

### `CMakeLists.txt`

- プロジェクトのビルド方法を定義するファイルです。
- ここで Geant4 や include パスが正しく設定されていないと、`compile_commands.json` も不正確になります。
- つまり、CMake がビルドに必要な情報を持っているため、`compile_commands.json` の生成元になっています。

### `build/`

- CMake の実行結果を置くディレクトリです。
- `compile_commands.json` は通常ここに出力されます。
- VS Code はこの場所を見て、コンパイル情報を取得します。

## 重要なポイント

- `compile_commands.json` を生成することと、VS Code にその場所を設定することは別作業です。
- どちらかが欠けると、IntelliSense はまだヘッダを見つけられません。
- Geant4 のような外部ライブラリを使うプロジェクトでは、include パスを明示することが重要です。

## 迷ったときの基本手順

1. CMake で `compile_commands.json` を生成する
2. `.vscode/c_cpp_properties.json` でそのパスを指定する
3. `includePath` に必要なディレクトリを追加する
4. IntelliSense をリセットする

この流れを覚えておくと、C/C++ プロジェクト全体でかなり応用できます。
