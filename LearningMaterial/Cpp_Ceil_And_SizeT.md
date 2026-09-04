# C++ の `std::ceil` と `std::size_t`

## 目的

この資料では、PMTへ到達した光子のうち、指定した割合に対応する光子の到達時刻を求める処理を題材として、次のC++の機能を説明します。

1. `std::ceil`
2. `std::size_t` と `int` の違い

想定する処理は、イベント内の到達光子数を `N`、thresholdを `p` % として、

```text
ceil(N * p / 100)
```

番目に到達した光子の時刻を取得するものです。

## `std::ceil`

### 基本的な意味

`std::ceil` は、与えられた数値以上で最小の整数値へ切り上げる関数です。

使用するには `<cmath>` をincludeします。

```cpp
#include <cmath>
```

例を示します。

```cpp
std::ceil(4.1);   // 5.0
std::ceil(4.9);   // 5.0
std::ceil(5.0);   // 5.0
std::ceil(-4.1);  // -4.0
```

「小数点以下を常に削除する関数」ではありません。負の値では、数直線上で正の方向へ切り上げます。

### 戻り値は整数型ではない

名前に「整数への切り上げ」という意味がありますが、`std::ceil` の戻り値は `int` ではありません。

```cpp
double value = std::ceil(4.1);  // valueは5.0
```

引数が `double` なら、結果も `double` です。そのため、光子番号やvectorの添字として使用するときは、整数型へ明示的に変換します。

```cpp
const std::size_t photonNumber =
    static_cast<std::size_t>(std::ceil(4.1));
```

この結果、`photonNumber` は整数値の `5` になります。

### threshold計算で切り上げる理由

あるイベントで23個の光子が到達し、thresholdを20%に設定した場合を考えます。

```text
23 * 20 / 100 = 4.6
```

20%に到達するためには、少なくとも5個の光子が必要です。

```cpp
const std::size_t numberOfPhotons = 23;
const double thresholdPercent = 20.0;

const std::size_t thresholdPhotonNumber =
    static_cast<std::size_t>(
        std::ceil(
            static_cast<double>(numberOfPhotons) *
            thresholdPercent / 100.0
        )
    );
```

結果は次のようになります。

```text
thresholdPhotonNumber = 5
```

ここで切り捨てて4個にすると、到達割合は次の値にしかなりません。

```text
4 / 23 = 約17.4%
```

したがって、「累積光子数が初めてthreshold以上になる光子」を選ぶ場合は、切り上げが適しています。

### `floor`、`round`、型変換との違い

| 処理 | `4.6` の結果 | 意味 |
|---|---:|---|
| `std::ceil(4.6)` | `5.0` | 切り上げ |
| `std::floor(4.6)` | `4.0` | 切り下げ |
| `std::round(4.6)` | `5.0` | 最も近い整数へ丸める |
| `static_cast<int>(4.6)` | `4` | 小数部分を切り捨てる |

`std::round` は、例えば `4.2` を `4.0` にします。そのため、「threshold以上になった最初の光子」を求める用途には適しません。

### 整数除算に注意する

以下の計算では、すべての値が整数型です。

```cpp
23 * 20 / 100
```

C++の整数同士の除算では小数部分が捨てられるため、結果は `4` になります。その後で `std::ceil` を呼んでも、失われた小数部分は戻りません。

```cpp
std::ceil(23 * 20 / 100);  // ceilへ渡される時点ですでに4
```

少なくとも一つの値を浮動小数点数にします。

```cpp
std::ceil(23 * 20.0 / 100.0);  // 5.0
```

または、光子数を明示的に `double` へ変換します。

```cpp
std::ceil(
    static_cast<double>(numberOfPhotons) *
    thresholdPercent / 100.0
);
```

## `std::size_t` と `int` の違い

### `int`

`int` は、正の値、0、負の値を表現できる符号付き整数型です。

```cpp
int count = 10;
int difference = -3;
```

イベント番号、検出器番号、差分、エラーを表す負の値など、負数を扱う可能性がある量に使用できます。

典型的な環境では `int` は32 bitですが、C++規格が常に32 bitと決めているわけではありません。

### `std::size_t`

`std::size_t` は、オブジェクトの大きさやコンテナの要素数を表すための符号なし整数型です。負の値は表現しません。

使用するときは `<cstddef>` をincludeします。

```cpp
#include <cstddef>
```

`std::vector::size()` の戻り値も `std::size_t` です。

```cpp
std::vector<double> hitTimes = {101.2, 103.5, 104.1};

const std::size_t numberOfPhotons = hitTimes.size();
```

`sizeof` の結果や、`std::vector`、`std::string` などの要素数・添字に関係する型として使われます。

```cpp
const std::size_t bytes = sizeof(double);
const std::size_t index = 2;

double time = hitTimes[index];
```

### 主な違い

| 特徴 | `int` | `std::size_t` |
|---|---|---|
| 負の値 | 表現できる | 表現できない |
| 主な用途 | 一般的な整数、差分、負値を含む値 | サイズ、要素数、添字 |
| `vector::size()`との型の一致 | 一致しない | 一致する |
| 大きさ | 多くの環境で32 bit | 64 bit環境では多くの場合64 bit |

環境によって実際のbit数は異なるため、「必ずこの大きさ」と仮定するべきではありません。

### vectorのループでは `std::size_t` が自然

```cpp
for (std::size_t i = 0; i < hitTimes.size(); ++i) {
    std::cout << hitTimes[i] << '\n';
}
```

`i` と `hitTimes.size()` の型が同じなので、符号付き型と符号なし型の比較による警告が発生しません。

C++11以降では、値を順に使うだけならrange-based forの方がさらに簡潔です。

```cpp
for (double time : hitTimes) {
    std::cout << time << '\n';
}
```

### `std::size_t` の引き算に注意する

`std::size_t` は負の値を表現できません。そのため、0から1を引くと `-1` にはならず、非常に大きな正の値へ回り込みます。

```cpp
std::size_t numberOfPhotons = 0;
std::size_t index = numberOfPhotons - 1;  // 危険
```

vectorが空かどうかを先に確認する必要があります。

```cpp
if (hitTimes.empty()) {
    continue;
}

const std::size_t index = hitTimes.size() - 1;
```

今回のthreshold計算でも、光子が0個のイベントを先に除外することが重要です。

### 符号付き型と符号なし型を不用意に比較しない

次のコードでは、`index` は負の値になれる `int`、`hitTimes.size()` は符号なしの `std::size_t` です。

```cpp
int index = -1;

if (index < hitTimes.size()) {
    // 意図しない比較になる可能性がある
}
```

比較時に `index` が符号なし型へ変換され、非常に大きな値として扱われる可能性があります。

添字なら、空でないことを確認したうえで `std::size_t` に統一します。

```cpp
const std::size_t index = thresholdPhotonNumber - 1;

if (index < hitTimes.size()) {
    const double time = hitTimes[index];
}
```

負の値を含めて判定したい場合は、符号付き型を使い、型変換の位置と安全性を明確にします。C++20以降なら、コンテナの要素数を符号付き整数として得る `std::ssize()` も使用できます。

```cpp
#include <iterator>

for (auto i = std::ssize(hitTimes) - 1; i >= 0; --i) {
    // 逆順の処理
}
```

ただし、空のvectorを含む逆順ループでは、開始値や終了条件を慎重に設計する必要があります。

## PMT threshold計算の全体例

イベントループ内の中心部分だけを取り出すと、次のようになります。

```cpp
if (!hitTimes || hitTimes->empty()) {
    continue;
}

// vector::size()の戻り値なのでstd::size_tで受け取る
const std::size_t numberOfPhotons = hitTimes->size();

// 保存順が時刻順とは限らないので、コピーを時刻順に並べる
std::vector<double> sortedHitTimes = *hitTimes;
std::sort(sortedHitTimes.begin(), sortedHitTimes.end());

// std::ceilの計算前に浮動小数点数として計算する
const std::size_t thresholdPhotonNumber =
    static_cast<std::size_t>(
        std::ceil(
            static_cast<double>(numberOfPhotons) *
            thresholdPercent / 100.0
        )
    );

// photonNumberは1始まり、vectorの添字は0始まり
const std::size_t index = thresholdPhotonNumber - 1;

const double thresholdTime = sortedHitTimes[index];
histogram->Fill(thresholdTime);
```

変数の役割を整理すると次のようになります。

| 変数 | 型 | 意味 |
|---|---|---|
| `numberOfPhotons` | `std::size_t` | そのイベントの総到達光子数 |
| `thresholdPercent` | `double` | thresholdの割合 |
| `thresholdPhotonNumber` | `std::size_t` | thresholdに対応する光子番号。1始まり |
| `index` | `std::size_t` | vectorへアクセスする添字。0始まり |
| `thresholdTime` | `double` | thresholdに到達した時刻 |

## 型を選ぶときの目安

次の基準で考えると整理しやすくなります。

- vectorの要素数や添字には `std::size_t` を使う。
- 負の値を取り得る一般的な整数には `int` などの符号付き整数型を使う。
- 割合や時刻など、小数部分が必要な値には `double` を使う。
- 異なる型を変換するときは、変換の理由が分かる場所で `static_cast` を使う。
- `std::size_t` から1を引く前に、元の値が0でないことを確認する。
- `std::ceil` を使う前の計算が整数除算になっていないか確認する。

今回のPMT解析では、`std::ceil` は「指定割合に初めて到達する光子番号」を決めるため、`std::size_t` は「vectorの要素数と添字」を安全に扱うために使用しています。
