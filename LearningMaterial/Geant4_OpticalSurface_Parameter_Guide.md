# Geant4 光学パラメータ設定の考え方

## 目的

このメモは、Geant4 でシンチレータ・ライトガイド・反射材・光電面を扱うときに、`RINDEX`、`REFLECTIVITY`、`EFFICIENCY`、`SPECULARLOBECONSTANT` などの光学パラメータをどのような基準で設定するべきかを整理したものです。

特に、このプロジェクトで出てきた次の疑問を解くための実務メモです。

- `BC620Surface` の `RINDEX` は 1.0 か 2.1 か。
- `BC620Surface` は `ground` か `groundbackpainted` か。
- `DiffuseSurface` は Tyvek 反射材の近似として妥当か。
- `TybekSurface` の LUT モデルと、単純な `unified` モデルは何が違うのか。
- `REFLECTIVITY`、`TRANSMITTANCE`、`EFFICIENCY` は、それぞれ何を意味するのか。

## まず押さえるべき大原則

Geant4 の optical photon は、通常の gamma とは別の粒子として扱われます。光子が物質内を進むとき、境界面で反射・屈折・吸収・検出が起こるかどうかは、主に次の情報から決まります。

1. 光子がいる material の optical properties
2. 次に入ろうとしている material の optical properties
3. 境界に `G4LogicalBorderSurface` または `G4LogicalSkinSurface` が設定されているか
4. その `G4OpticalSurface` の `type`、`model`、`finish`
5. surface の material properties table

ここで重要なのは、**同じ `RINDEX` や `REFLECTIVITY` でも、material に置く場合と surface に置く場合で意味が変わる**ことです。

例えば、シンチレータ本体の `RINDEX` は、光子がシンチレータ内を進む速度、境界での Fresnel 反射・屈折、全反射条件などに使われます。一方、`groundbackpainted` の surface に設定する `RINDEX` は、反射塗料や裏面層を表す仮想的な屈折率として使われます。

## material に設定する値と surface に設定する値

### material 側に置くべきもの

material 側の `G4MaterialPropertiesTable` には、その体積の中を光子が進むときに必要な値を置きます。

代表例:

- `RINDEX`
- `ABSLENGTH`
- `RAYLEIGH`
- `SCINTILLATIONCOMPONENT1`
- `SCINTILLATIONYIELD`
- `SCINTILLATIONTIMECONSTANT1`
- `SCINTILLATIONRISETIME1`

このプロジェクトでは、BC408 の `RINDEX`、`ABSLENGTH`、発光スペクトルが `src/Material/BC408Mat.cc` に入っています。BC408 の発光は 360-520 nm の範囲に定義され、ピークは 430 nm 付近です。したがって、反射材や光電面の光学テーブルも、この波長域を最低限カバーする必要があります。

Geant4 optical physics の注意点をまとめた Dietz-Laursonn の論文では、光学物性は photon energy の関数として与え、エネルギー範囲を揃えるべきだと説明されています。波長データを使う場合は、Geant4 に渡す前に photon energy に変換し、energy 昇順で並べる必要があります。

### surface 側に置くべきもの

surface 側の `G4MaterialPropertiesTable` には、境界面そのもののふるまいを置きます。

代表例:

- `REFLECTIVITY`
- `TRANSMITTANCE`
- `EFFICIENCY`
- `RINDEX`
- `REALRINDEX`
- `IMAGINARYRINDEX`
- `SPECULARSPIKECONSTANT`
- `SPECULARLOBECONSTANT`
- `BACKSCATTERCONSTANT`

ただし、surface 側の `RINDEX` はいつも使われるわけではありません。`finish` が `ground` や `polished` のときは、Geant4 は基本的に post volume 側 material の `RINDEX` を見に行きます。`groundbackpainted` や `polishedbackpainted` のときは、surface の `RINDEX` が意味を持ちます。

この違いは非常に重要です。`BC620Surface` に `RINDEX = 2.1` と書いても、`SetFinish(ground)` のままだと「BC-620 塗料の屈折率」として効きにくい可能性があります。

## `G4OpticalSurface` の3つの軸

Geant4 の surface は、主に次の3つで性格が決まります。

```cpp
fSurface->SetType(...);
fSurface->SetModel(...);
fSurface->SetFinish(...);
```

### `type`

`type` は、境界がどの種類の物理境界かを表します。

よく使うもの:

- `dielectric_dielectric`
- `dielectric_metal`
- `dielectric_LUT`
- `dielectric_LUTDAVIS`

`dielectric_dielectric` は、シンチレータ、空気、真空、アクリル、ガラスなどの間の光学境界に使います。Fresnel 反射・屈折を考えるときの基本です。

`dielectric_metal` は、金属反射面や、透過を考えない反射材近似に使われます。Ros et al. のプラスチックシンチレータ設計論文では、Tyvek やアルミに近い反射材の近似として、反射しなかった光子を吸収させる目的で `dielectric_metal` を選んでいます。

`dielectric_LUT` と `dielectric_LUTDAVIS` は、実測 angular distribution の lookup table を使うモデルです。Tyvek や Teflon の実測反射分布を使いたい場合に便利ですが、外部データファイルに依存し、単純な `REFLECTIVITY` 設定とは挙動がかなり違います。

### `model`

`model` は、反射のモデルです。

よく使うもの:

- `glisur`
- `unified`
- `LUT`
- `DAVIS`

現在のプロジェクトでは、多くの surface で `unified` が使われています。`unified` モデルでは、反射を次の成分に分けて考えます。

- specular spike
- specular lobe
- backscatter
- Lambertian diffuse

このうち、`SPECULARSPIKECONSTANT`、`SPECULARLOBECONSTANT`、`BACKSCATTERCONSTANT` を明示的に指定できます。これらをすべて 0 にすると、反射が選ばれた場合、残りは Lambertian diffuse として扱われます。

### `finish`

`finish` は、表面状態や塗装状態を表します。

このプロジェクトで特に重要なのは次です。

- `polished`
- `ground`
- `polishedbackpainted`
- `groundbackpainted`
- `groundfrontpainted`
- `groundtyvekair`

`polished` は滑らかな境界です。`ground` は粗い境界です。どちらも基本的には、二つの material の `RINDEX` に基づく Fresnel 境界として扱われます。

`groundbackpainted` は、粗い境界の背後に反射塗料があるようなモデルです。白色反射塗料、拡散反射材、反射コーティングを surface として近似する場合には、`ground` よりも `groundbackpainted` の方が意図に近いことが多いです。

`groundfrontpainted` は、前面に塗料層がある扱いです。Ros et al. の論文では、`groundfrontpainted` は透過をゼロにするような扱いとして言及されています。

## `REFLECTIVITY` の意味

`REFLECTIVITY` は、「反射率」と訳せますが、Geant4 の surface では文脈に注意が必要です。

surface に `REFLECTIVITY = 0.95` を与えた場合、単純には「95%の確率で反射、残り5%は吸収または透過」と考えます。ただし実際には `TRANSMITTANCE`、`EFFICIENCY`、`finish`、`type` によって分岐が変わります。

典型的には、

- `REFLECTIVITY`: 反射する確率
- `TRANSMITTANCE`: 透過する確率
- `EFFICIENCY`: 吸収された光子を検出として扱う確率

です。

例えば反射材 surface で、

```cpp
REFLECTIVITY = 0.95
TRANSMITTANCE = 未設定
EFFICIENCY = 未設定
```

なら、残りの 5% は基本的に吸収になります。光電面では `EFFICIENCY` が重要で、吸収された光子のうち検出に数える確率として使われます。

## `RINDEX` の意味

`RINDEX` は屈折率です。ただし Geant4 では次の2種類を区別して考える必要があります。

1. material の `RINDEX`
2. surface の `RINDEX`

material の `RINDEX` は、その物質中の光速や Fresnel 反射・屈折、全反射条件に直接関係します。シンチレータ、アクリル、ガラス、空気、真空などには必ず入れるべきです。

surface の `RINDEX` は、特に back-painted surface で意味を持ちます。Geant4 11.2.2 の `G4OpBoundaryProcess.cc` では、`polishedbackpainted` または `groundbackpainted` のとき、surface MPT の `RINDEX` を取得して境界処理に使う実装になっています。一方、`polished` または `ground` では post material 側の `RINDEX` を見に行きます。

したがって、

```cpp
fSurface->SetFinish(ground);
RINDEX = 2.1;
```

とした場合、その `RINDEX = 2.1` は「反射塗料 BC-620 の屈折率」としては十分に使われない可能性があります。

BC-620 反射塗料として `n = 2.1` を使いたいなら、

```cpp
fSurface->SetFinish(groundbackpainted);
RINDEX = 2.1;
```

の方が Geant4 の処理と意図が合います。

## `SPECULARSPIKE`、`SPECULARLOBE`、`BACKSCATTER`、Lambertian

`unified` モデルでは、反射成分を確率的に分けます。

### specular spike

完全に鏡面反射に近い成分です。入射角と反射角がきれいに対応する、理想鏡面に近い反射です。

### specular lobe

鏡面反射を中心に、表面粗さでぼやけた反射です。粗いけれども、まだ幾何学的な鏡面方向の記憶が強い反射です。

### backscatter

入射方向へ戻るような反射成分です。

### Lambertian diffuse

法線方向に対する余弦則に従う拡散反射です。白色拡散反射材の単純近似としてよく使われます。

`unified` モデルでは、

```cpp
SPECULARSPIKECONSTANT = 0
SPECULARLOBECONSTANT = 0
BACKSCATTERCONSTANT = 0
```

にすると、反射が起きたときの残りは Lambertian 反射になります。

したがって、Teflon や BC-620 のように「100% diffuse reflection」として近似したい場合、この設定は自然です。

一方、Tyvek は完全な Lambertian reflector ではない可能性があります。Ros et al. の論文では、Tyvek は Teflon よりも Aluminum に近く、`15% diffuse`、`85% specular-lobe`、反射率 90% として扱われています。つまり、Tyvek を厳密に寄せたいなら、DiffuseSurface のような完全 Lambertian 近似よりも、specular-lobe 成分を入れた方が文献設定に近くなります。

## `sigma_alpha` の意味

`SetSigmaAlpha()` は、`unified` モデルにおける micro-facet normal のばらつきを表します。角度単位で与えます。

直感的には、

- 小さい `sigma_alpha`: 表面が滑らかに近い
- 大きい `sigma_alpha`: 表面が粗い

です。

添付資料の表では、BC-620 について `sigma_alpha = 5 deg` と読めます。この値を使うなら、

```cpp
fSurface->SetSigmaAlpha(5.0 * deg);
```

は妥当です。

ただし、完全 Lambertian 近似にして `SPECULARLOBECONSTANT = 0` とする場合、`sigma_alpha` の影響は限定的になります。`sigma_alpha` は specular-lobe 的な反射を扱うときに特に重要です。

## `ground` と `groundbackpainted` の違い

この違いは、今回の BC-620 設定で特に重要です。

### `ground`

`ground` は、粗い誘電体境界です。光子はまず境界の相手側 material の `RINDEX` を見ます。そのうえで Fresnel 反射・屈折、粗さによる micro-facet 処理が入ります。

つまり、これは「シンチレータと何か別の透明物質の粗い境界」に近いです。

### `groundbackpainted`

`groundbackpainted` は、粗い境界の背後に塗装層がある扱いです。反射塗料、白色反射材、裏面反射膜の近似として使いやすい設定です。

Geant4 11.2.2 の `G4OpBoundaryProcess.cc` では、`groundbackpainted` のとき surface MPT の `RINDEX` を使い、`REFLECTIVITY` に従って反射・吸収・透過を決めます。さらに、`groundbackpainted` では Lambertian reflection が明示的に選ばれる処理があります。

したがって、BC-620 のような白色反射塗料を、薄い coating volume として詳細に作らず surface として近似するなら、`groundbackpainted` が自然です。

## BC-620 の設定基準

添付された資料の表では、BC-620 について次の値が示されています。

| 項目 | BC-620 |
|---|---:|
| chemical composition | C5H8Ti2O7 |
| density | 2.8 g/cm3 |
| refractive index | 2.1 |
| specular-lobe reflection | 0% |
| diffuse reflection | 100% |
| reflectivity | 図 D.3 参照 |
| sigma_alpha | 5 deg |

この表に従うなら、BC-620 の surface 設定方針は次のようになります。

```cpp
fSurface->SetType(dielectric_dielectric);
fSurface->SetModel(unified);
fSurface->SetFinish(groundbackpainted);
fSurface->SetSigmaAlpha(5.0 * deg);

RINDEX = 2.1;
SPECULARSPIKECONSTANT = 0.0;
SPECULARLOBECONSTANT = 0.0;
BACKSCATTERCONSTANT = 0.0;
REFLECTIVITY = 図D.3から読み取った波長依存値;
```

現在のように `REFLECTIVITY = 0.95` を一定値で置くのは、図 D.3 が未入力の場合の暫定値としては悪くありません。ただし、資料に厳密に寄せるなら、図 D.3 から BC408 の発光域、特に 400-460 nm 付近の反射率を読み取り、波長依存テーブルにするのが望ましいです。

BC408 の発光域はこのプロジェクトでは 360-520 nm に設定されています。したがって、BC-620 の reflectivity table も少なくとも 360-520 nm をカバーするべきです。Geant4 に渡すときは、波長 nm ではなく photon energy eV の昇順に変換して渡します。

## Tyvek の設定基準

Tyvek は高反射の白色反射材ですが、完全 Lambertian とみなすかどうかは用途によります。

LHAASO の水チェレンコフ検出器に関する Li et al. の論文では、Tyvek liner の反射率を測定する手法が述べられています。この論文では、Tyvek の反射率は媒質や波長に依存する量として扱われています。405 nm 付近での空気中 Tyvek の反射率はおおむね 94% 程度、水中ではさらに高い値が報告されています。

一方、Ros et al. のプラスチックシンチレータ設計論文では、Tyvek は Teflon のような完全拡散反射材ではなく、Aluminum に近い性格を持つとして、反射率 90%、diffuse 15%、specular-lobe 85% として扱われています。

したがって、Tyvek の近似は目的で分けるべきです。

### 安定な簡易近似

```cpp
type = dielectric_dielectric
model = unified
finish = groundbackpainted
REFLECTIVITY = 0.94 - 0.96
SPECULARSPIKE = 0
SPECULARLOBE = 0
BACKSCATTER = 0
```

これは「高反射な白色拡散材」として安定に動かすための近似です。厳密な Tyvek の角度分布は再現しません。

### 文献設定に寄せる近似

```cpp
REFLECTIVITY = 0.90
SPECULARLOBECONSTANT = 0.85
SPECULARSPIKECONSTANT = 0.0
BACKSCATTERCONSTANT = 0.0
```

この場合、残りが diffuse 成分として扱われます。Tyvek を「鏡面方向の記憶がかなり残る反射材」として見る設定です。

### LUT モデル

`dielectric_LUT` と `groundtyvekair` を使えば、Geant4 の LBNL LUT データによる Tyvek 反射を使えます。

ただし、LUT モデルは外部データ `G4REALSURFACEDATA` に依存します。また、薄い真空層や複雑な境界で使うと、通常の `unified` モデルより挙動が不安定になることがあります。過去に `ScintiToBumper` を Tyvek LUT にするとエラーが出て、Diffuse に戻すと止まったという現象は、この LUT モデルと極薄境界の組み合わせが一因だった可能性があります。

## Teflon の設定基準

添付資料の表では、Teflon は次のように読めます。

| 項目 | Teflon |
|---|---:|
| refractive index | 1.365 |
| specular-lobe reflection | 0% |
| diffuse reflection | 100% |
| reflectivity | 99% |

したがって、Teflon を単純化して surface として置くなら、

```cpp
type = dielectric_dielectric
model = unified
finish = groundbackpainted
RINDEX = 1.365
REFLECTIVITY = 0.99
SPECULARSPIKE = 0
SPECULARLOBE = 0
BACKSCATTER = 0
```

という設定が自然です。

## Aluminum の設定基準

Aluminum は金属なので、単純な `RINDEX` だけではなく、複素屈折率、つまり `REALRINDEX` と `IMAGINARYRINDEX` を使う方が物理的です。

ただし、検出器シミュレーションでは、反射材としての効きだけを近似するために、

```cpp
type = dielectric_metal
REFLECTIVITY = 実測または文献値
```

として扱うこともあります。

添付資料では Aluminum の specular-lobe reflection が 95%、diffuse reflection が 5% とされています。これは、完全鏡面ではなく、粗さで少し広がった金属反射材として扱う設定です。

## 光電面 cathode の設定基準

光電面では `REFLECTIVITY` と `EFFICIENCY` の意味を分ける必要があります。

- `REFLECTIVITY`: 反射される確率
- `EFFICIENCY`: 吸収された光子が検出として数えられる確率

Geant4 の optical boundary process では、吸収が起きた光子について `EFFICIENCY` により detection status が立つ処理があります。したがって、PMT や SiPM の量子効率を入れたい場合は、単に `REFLECTIVITY` を下げるのではなく、`EFFICIENCY` を波長依存で設定するのが自然です。

## BorderSurface と SkinSurface

`G4LogicalSkinSurface` は、ある logical volume の全表面に同じ surface を貼ります。

```cpp
new G4LogicalSkinSurface("ScintiSkin", LogVol_Scinti, surfMirror);
```

これは便利ですが、すべての面に効くので、特定の接合面だけ別設定にしたい場合は `G4LogicalBorderSurface` で上書きする必要があります。

`G4LogicalBorderSurface` は、二つの physical volume の組み合わせに対して surface を設定します。

```cpp
new G4LogicalBorderSurface("ScintiToGuide", physScinti, physGuide, surfDiel);
```

注意点は、border surface は方向を持つことです。

```cpp
new G4LogicalBorderSurface("AtoB", physA, physB, surface);
```

は、A から B へ進む光子に対する surface です。逆方向にも同じ境界を適用したいなら、

```cpp
new G4LogicalBorderSurface("BtoA", physB, physA, surface);
```

も必要です。

この点は Dietz-Laursonn の論文でも注意点として説明されています。

## このプロジェクトでの推奨整理

### DiffuseSurface

現在の `DiffuseSurface` は、

```cpp
type = dielectric_dielectric
model = unified
finish = groundbackpainted
RINDEX = 1.0
REFLECTIVITY = 0.96
SPECULARSPIKE = 0
SPECULARLOBE = 0
BACKSCATTER = 0
```

です。

これは「高反射な白色 Lambertian 反射材」としては妥当です。Tyvek の厳密再現ではありませんが、安定な近似としては使いやすい設定です。

### TybekSurface

現在の `TybekSurface` は、

```cpp
type = dielectric_LUT
model = LUT
finish = groundtyvekair
```

です。

これは Geant4 の LUT データを使うため、Tyvek の実測 angular distribution に近い可能性があります。ただし、`G4REALSURFACEDATA` に依存し、通常の `unified` モデルより制御しづらいです。薄い bumper volume との境界には慎重に使うべきです。

### BC620Surface

添付資料に従うなら、BC-620 は次の設定が自然です。

```cpp
type = dielectric_dielectric
model = unified
finish = groundbackpainted
RINDEX = 2.1
REFLECTIVITY = 図D.3から読み取る。暫定なら0.95程度
SPECULARSPIKE = 0
SPECULARLOBE = 0
BACKSCATTER = 0
sigma_alpha = 5 deg
```

現在の `ground` は、BC-620 反射塗料のモデルとしては少し意図がずれます。`groundbackpainted` の方が、白色塗料・裏面反射材の surface 近似として自然です。

### DielectricSurface

グリスや光学接着でシンチレータとライトガイドを密着させるなら、本来は「反射率を高くする」のではなく、「屈折率差を小さくして Fresnel 反射を減らし、透過を増やす」方向で考えるべきです。

したがって、グリスを volume として明示しない場合でも、surface 設定では、

- `polished`
- 反射材ではない
- `REFLECTIVITY` を人為的に高くしすぎない
- 必要ならグリス相当の `RINDEX` をどう扱うか検討する

という考え方が必要です。

現在の `DielectricSurface` は `REFLECTIVITY = 0.999` になっていますが、これは「グリスで透過させる」というより「ほぼ反射する面」に近い読み方をされる可能性があります。グリス接合を表現したいなら、今後見直し候補です。

## パラメータを決める手順

光学 surface を追加するときは、次の順番で決めると迷いにくいです。

1. その境界は透明接合か、反射材か、検出面かを決める。
2. 透明接合なら、両側 material の `RINDEX` を重視する。
3. 反射材なら、反射しなかった光子を吸収するのか、透過させるのかを決める。
4. 白色塗料や反射膜を surface で近似するなら `groundbackpainted` を第一候補にする。
5. 金属反射なら `dielectric_metal`、または複素屈折率を検討する。
6. 反射方向を、鏡面・lobe・Lambertian のどれで近似するか決める。
7. `REFLECTIVITY` は文献値・メーカー値・実測値から、波長依存で入れる。
8. 波長表を Geant4 用の photon energy 昇順に変換する。
9. BC408 の発光域 360-520 nm をカバーしているか確認する。
10. skin surface と border surface の優先関係、方向性を確認する。

## よくある落とし穴

### 波長順のまま入れてしまう

Geant4 の `AddProperty` は photon energy の昇順で与えるのが基本です。波長 nm の昇順は photon energy の降順なので、そのまま変換すると順序が逆になります。

### property のエネルギー範囲が揃っていない

発光スペクトルは 360-520 nm なのに、反射率や `RINDEX` が 400-500 nm しかない、という状態は避けるべきです。範囲外光子で `NoRINDEX` や不自然な補間が起こり得ます。

### `ground` と `groundbackpainted` を混同する

反射塗料を surface で表したいなら、`ground` ではなく `groundbackpainted` の方が自然です。

### グリス接合を高反射面として設定してしまう

光学グリスの目的は、屈折率整合により Fresnel 反射を減らして透過を増やすことです。`REFLECTIVITY` を高くするのは、反射材の設定に近くなります。

### Tyvek LUT を安易に使う

LUT モデルは強力ですが、外部データと境界条件に依存します。まずは `unified` モデルで安定な近似を作り、必要になったら LUT を検証する方が安全です。

## 参考にした資料

### Geant4 ローカルソース・example

- `/home/hashizume/Geant4/geant4-11.2.2/source/processes/optical/src/G4OpBoundaryProcess.cc`
  - boundary process の実装確認に使用。
  - `REFLECTIVITY`、`TRANSMITTANCE`、`EFFICIENCY`、`RINDEX`、`groundbackpainted` の扱いを確認。
- `/home/hashizume/Geant4/geant4-11.2.2/source/processes/optical/include/G4OpBoundaryProcess.hh`
  - `LambertianReflection`、`DoReflection()` などの実装確認に使用。
- `/home/hashizume/Geant4/geant4-11.2.2/source/materials/include/G4OpticalSurface.hh`
  - `G4OpticalSurfaceFinish`、`G4OpticalSurfaceModel`、LUT 関連 enum の確認に使用。
- `/home/hashizume/Geant4/geant4-11.2.2/examples/extended/optical/OpNovice2`
  - optical surface の設定例、macro での `RINDEX`、`REFLECTIVITY`、`groundbackpainted`、LUT surface の使い方を確認。

### 論文・外部資料

- Erik Dietz-Laursonn, "Peculiarities in the Simulation of Optical Physics with Geant4", arXiv:1612.05162.  
  https://arxiv.org/abs/1612.05162  
  Geant4 optical physics の一般的な注意点、property を photon energy 昇順で与えること、surface の優先関係、`G4OpticalSurface` の落とし穴の整理に使用。

- G. Ros et al., "On the design of experiments based on plastic scintillators using Geant4 simulations", arXiv:1804.08975.  
  https://arxiv.org/abs/1804.08975  
  プラスチックシンチレータ、反射コーティング、Tyvek/Aluminum/Teflon 的な反射材設定、`dielectric_metal` や `groundfrontpainted` の使い分け、Tyvek の diffuse/specular-lobe 比の参考に使用。

- Xiurong Li et al., "Novel methods for measuring the optical parameters of the water Cherenkov detector", arXiv:1801.03797.  
  https://arxiv.org/abs/1801.03797  
  Tyvek liner の反射率測定、Tyvek 反射率が媒質・波長に依存することの参考に使用。

- ユーザ添付資料の表。  
  Aluminum、Teflon、Tyvek、BC-620 の chemical composition、density、refractive index、specular-lobe/diffuse reflection、reflectivity、attenuation、sigma_alpha の設定値の根拠として使用。

## まとめ

BC-620 を添付資料に沿って扱うなら、`RINDEX = 2.1` は妥当です。ただし、`SetFinish(ground)` のままでは、その値が BC-620 反射塗料の屈折率として意図通り使われにくい可能性があります。

BC-620 反射塗料を surface として近似するなら、基本方針は次です。

```cpp
SetType(dielectric_dielectric);
SetModel(unified);
SetFinish(groundbackpainted);
SetSigmaAlpha(5.0 * deg);

RINDEX = 2.1;
REFLECTIVITY = 図D.3または暫定0.95;
SPECULARSPIKECONSTANT = 0.0;
SPECULARLOBECONSTANT = 0.0;
BACKSCATTERCONSTANT = 0.0;
```

Tyvek は、安定な高反射拡散材として扱うなら `DiffuseSurface` 的な近似でよいですが、文献設定に寄せるなら specular-lobe 成分を入れるべきです。

光学パラメータは「数字が合っているか」だけでなく、**その数字が Geant4 のどの分岐で使われるか** が重要です。`type`、`model`、`finish` の組み合わせを先に決め、その上で `RINDEX` や `REFLECTIVITY` を置く場所を決めるのが、最も事故が少ない設定手順です。
