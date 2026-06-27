#include "Material/BC408Mat.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"

BC408Mat::BC408Mat() {
    G4NistManager* nist = G4NistManager::Instance();
    
    // NISTデータベースからポリビニルトルエン（BC408の主成分）を取得
    G4Material* baseMat = nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
    fMaterial = new G4Material("BC408",baseMat->GetDensity(),baseMat);

    G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();

    //本来はエネルギーでパラメータを登録する必要がある
    //ヘルパー関数：[波長(nm), 値] の表からGeant4用の物性配列を自動登録する
    auto AddPropertyFromNm = [&](const char* key, const std::vector<std::pair<double, double>>& data) {
        std::vector<G4double> energies;
        std::vector<G4double> values;

        // Geant4は「エネルギー昇順」必須。データシートの「波長昇順」を後ろから読むとちょうどエネルギー昇順になる
        for (auto it = data.rbegin(); it != data.rend(); ++it) //rbegin は reverse beginを指す（最後尾の要素からスタートする）
        {
            double nm = it->first;
            energies.push_back((1239.84193 / nm) * eV); // λ(nm) -> E(eV) 変換
            values.push_back(it->second);
        }
        mpt->AddProperty(key, energies, values, false, true);//4=新規か、5=spline補完を適用するか
    };


    //屈折率
    AddPropertyFromNm("RINDEX", {
        { 360.0, 1.58 }, { 370.0, 1.58 }, { 380.0, 1.58 }, { 390.0, 1.58 },
        { 400.0, 1.58 }, { 410.0, 1.58 }, { 420.0, 1.58 }, { 430.0, 1.58 },
        { 440.0, 1.58 }, { 450.0, 1.58 }, { 460.0, 1.58 }, { 470.0, 1.58 },
        { 480.0, 1.58 }, { 490.0, 1.58 }, { 500.0, 1.58 }, { 510.0, 1.58 },
        { 520.0, 1.58 }
    });

    //吸収長 (pilsooさんのプログラムでは2100mm)
    AddPropertyFromNm("ABSLENGTH", {
        { 360.0, 3800.*mm }, { 370.0, 3800.*mm }, { 380.0, 3800.*mm }, { 390.0, 3800.*mm },
        { 400.0, 3800.*mm }, { 410.0, 3800.*mm }, { 420.0, 3800.*mm }, { 430.0, 3800.*mm },
        { 440.0, 3800.*mm }, { 450.0, 3800.*mm }, { 460.0, 3800.*mm }, { 470.0, 3800.*mm },
        { 480.0, 3800.*mm }, { 490.0, 3800.*mm }, { 500.0, 3800.*mm }, { 510.0, 3800.*mm },
        { 520.0, 3800.*mm }
    });


    //発光スペクトル
    AddPropertyFromNm("SCINTILLATIONCOMPONENT1", {
        { 360.0, 0.02 }, { 370.0, 0.03 }, { 380.0, 0.07 }, { 390.0, 0.11 },
        { 400.0, 0.21 }, { 410.0, 0.5  }, { 420.0, 0.82 }, { 430.0, 1.0  },
        { 440.0, 0.82 }, { 450.0, 0.54 }, { 460.0, 0.41 }, { 470.0, 0.26 },
        { 480.0, 0.18 }, { 490.0, 0.12 }, { 500.0, 0.07 }, { 510.0, 0.04 },
        { 520.0, 0.02 }
    });

    //　光子の発生数
    mpt->AddConstProperty("SCINTILLATIONYIELD", 10000. /MeV);    
    //　光子発生のばらつき（ポアソン分布）  
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0); 
    //単一の成分のみと仮定
    mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 2.1*ns);    
    mpt->AddConstProperty("SCINTILLATIONRISETIME1", 0.9*ns);

    fMaterial->SetMaterialPropertiesTable(mpt);
    
    //クチンチング（消光効果）に関するパラメータ
    fMaterial->GetIonisation()->SetBirksConstant( (1.31e-2*(g/cm2/MeV)) / (1.032*(g/cm3)) );






}

BC408Mat::~BC408Mat() {
}