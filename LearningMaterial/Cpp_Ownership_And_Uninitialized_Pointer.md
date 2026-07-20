# C++ の所有権と未初期化ポインタ

## 今回起きた問題

`AnalysisOutput` クラスを新しく導入したとき、`RunAction` に以下のようなメンバを追加した。

```cpp
AnalysisOutput* fAnalysisOutput;
```

これは `AnalysisOutput` へのポインタを持つだけであり、`AnalysisOutput` の実体はまだ作られていない。

しかし `RunAction` のコンストラクタで、以下のように使っていた。

```cpp
fAnalysisOutput->Book(fEventAction);
```

この時点で `fAnalysisOutput` は有効な `AnalysisOutput` オブジェクトを指していない。そのため、存在しない場所に対して `Book()` を呼ぼうとして、Segmentation fault の原因になった。

## ポインタ宣言だけではオブジェクトは作られない

以下は、オブジェクトを作っているのではなく、アドレスを入れるための変数を用意しているだけ。

```cpp
AnalysisOutput* fAnalysisOutput;
```

イメージとしては、

```text
fAnalysisOutput という住所メモだけがある
でも、その住所にはまだ何も書かれていない
```

という状態。

この状態で、

```cpp
fAnalysisOutput->Book(...);
```

とすると、どこにあるかわからない `AnalysisOutput` にアクセスすることになる。

## ポインタとして使うなら new が必要

ポインタのまま所有するなら、実体を作る必要がある。

```cpp
RunAction::RunAction(EventAction* eventAction)
 : fEventAction(eventAction),
   fAnalysisOutput(new AnalysisOutput())
{
    fAnalysisOutput->Book(fEventAction);
    fEventAction->SetAnalysisOutput(fAnalysisOutput);
}
```

この場合、`RunAction` が `AnalysisOutput` を所有しているので、デストラクタで解放する必要がある。

```cpp
RunAction::~RunAction()
{
    delete fAnalysisOutput;
}
```

また、安全のためヘッダ側では初期化しておくとよい。

```cpp
AnalysisOutput* fAnalysisOutput = nullptr;
```

ただし、`new/delete` を手で管理する方法はミスしやすい。

## 今回は値メンバで持つ方が簡単

今回の `AnalysisOutput` は、`RunAction` と同じ寿命で存在すればよい。

そのため、ポインタではなく値メンバとして持つ方が自然。

```cpp
class RunAction : public G4UserRunAction {
private:
    EventAction* fEventAction;
    AnalysisOutput fAnalysisOutput;
};

この場合、`RunAction` が作られると `fAnalysisOutput` も自動的に作られる。

使うときは `->` ではなく `.` を使う。

```cpp
RunAction::RunAction(EventAction* eventAction)
 : fEventAction(eventAction)
{
    fAnalysisOutput.Book(fEventAction);
    fEventAction->SetAnalysisOutput(&fAnalysisOutput);
}
```

ここで `SetAnalysisOutput(&fAnalysisOutput)` としているのは、`EventAction` 側には `AnalysisOutput` の場所だけ教えればよいため。

## 所有する側と参照する側

今回の設計で大事なのは、どのクラスが `AnalysisOutput` を所有するか。

```text
RunAction
  AnalysisOutput を所有する

EventAction
  RunAction が所有する AnalysisOutput を参照する

ScintiSD / CathodeSD
  EventAction 経由で AnalysisOutput を参照する
```

`RunAction` は所有者なので、値メンバとして持てる。

```cpp
AnalysisOutput fAnalysisOutput;
```

一方、`EventAction`、`ScintiSD`、`CathodeSD` は所有者ではない。すでに存在している `AnalysisOutput` を使いたいだけなので、ポインタで参照する。

```cpp
AnalysisOutput* fAnalysisOutput = nullptr;
```

## なぜ CathodeSD で値メンバにしないのか

もし `CathodeSD` が値メンバとして `AnalysisOutput` を持つと、

```cpp
class CathodeSD {
private:
    AnalysisOutput fAnalysisOutput;
};
```

`RunAction` が持っているものとは別の `AnalysisOutput` が作られてしまう。

すると、

```text
RunAction の AnalysisOutput
CathodeSD の AnalysisOutput
ScintiSD の AnalysisOutput
EventAction の AnalysisOutput
```

のように、別々の出力管理オブジェクトが存在することになる。

これは column ID や ntuple の状態が分裂する原因になる。

そのため、`CathodeSD` や `ScintiSD` は `AnalysisOutput` を作らず、すでにあるものを参照するだけにする。

## . と -> の違い

値オブジェクトに対してメンバ関数を呼ぶときは `.` を使う。

```cpp
AnalysisOutput output;
output.Book(eventAction);
```

ポインタに対してメンバ関数を呼ぶときは `->` を使う。

```cpp
AnalysisOutput* output = new AnalysisOutput();
output->Book(eventAction);
```

ただし、`->` を使う前に、そのポインタが有効なオブジェクトを指している必要がある。

## 今回の教訓

ポインタをメンバに追加しただけでは、オブジェクトは作られない。

```cpp
AnalysisOutput* fAnalysisOutput;
```

これは「実体」ではなく「実体の場所を入れる変数」。

今回のように、あるクラスが明確に所有するオブジェクトなら、まずは値メンバを検討するとよい。

```cpp
AnalysisOutput fAnalysisOutput;
```

ポインタを使うのは、主に以下の場合。

- 他のクラスが所有するオブジェクトを参照したい。
- オブジェクトの寿命を明示的に制御したい。
- 実行時に別の実装へ差し替えたい。
- `nullptr` によって「存在しない」状態を表したい。

今回の `RunAction` では値メンバが自然で、`EventAction` や `CathodeSD` では参照用ポインタが自然。

この違いは、文法上の都合ではなく、所有権の違いから来ている。
