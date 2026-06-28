# test
## 目的
UROKO、HILE、Li-glass それぞれ単体でテストするためのプログラム

## 現段階での進捗
- 放射性崩壊(90Sr,137Cs) or neutron をコマンドから選択できる（mygen)

## 今後の展望
- ~~UROKOでシンチレーション光まで考慮したプログラムを作成する~~
- optical photonの収集率を反映するプログラムを作成する
  

## LogVol
### UROKO
光学パラメータに関する設定はMaterial、Surfaceに分離して記述した

#### Scintilation光の発生
Materialに記述。値に関しては、Pilsooさんのプログラムから引用。（BC408）.  

#### 屈折-反射など
Material、Surfaecにそれぞれ記述。値に関しては、吉岡さん卒論から引用.   
- Scintillator表面　→ 乱反射 （Borderで上書き設定、そのために薄い真空層をScintillator直前に設置）
- Scintillator側面　→ 鏡面反射（Skinで設定）
- LightGuide側面　→ 鏡面反射 (Skinで設定)
- Cathode表面 → 全吸収（Skinで設定、量子効率が100%であると仮定）
- ScintillatorーLightGuide　境界 → 誘電体同士の接合（Dielectric）
- LightGuideーPMT 境界　→ 誘電体同士の接合（Dielectric）







