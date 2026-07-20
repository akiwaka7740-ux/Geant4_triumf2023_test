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
        "/home/hashizume/Geant4/install/include/Geant4"
      ],
      "compilerPath": "/usr/bin/g++",
      "cppStandard": "c++17",
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

#### `compilerPath`

- 使うコンパイラの実体を指定します。
- ここでは `g++` を使う前提で設定しました。

#### `cppStandard`

- C++ の標準規格を指定します。
- Geant4 を扱う場合、`c++17` などがよく使われます。

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
