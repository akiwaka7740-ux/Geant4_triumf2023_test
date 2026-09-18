#include "Material/GS20Mat.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4PhysicalConstants.hh"

GS20Mat::GS20Mat() {
    G4NistManager* nist = G4NistManager::Instance();

    // 1. Elementの定義
    G4Element* elO  = nist->FindOrBuildElement("O");
    G4Element* elSi = nist->FindOrBuildElement("Si");
    G4Element* elMg = nist->FindOrBuildElement("Mg");
    G4Element* elAl = nist->FindOrBuildElement("Al");
    G4Element* elCe = nist->FindOrBuildElement("Ce");       

    // 2. 濃縮リチウム (6Li 95% enrichment) の定義
    G4Isotope* iso_Li6 = new G4Isotope("Li6", 3, 6, 6.015*g/mole);
    G4Isotope* iso_Li7 = new G4Isotope("Li7", 3, 7, 7.016*g/mole);

    G4Element* elLi_enriched = new G4Element("Enriched_Lithium", "Li", 2);
    elLi_enriched->AddIsotope(iso_Li6, 95.0*perCent);
    elLi_enriched->AddIsotope(iso_Li7,  5.0*perCent);

    // 3. 各構成化合物 (酸化物) の定義
    G4Material* SiO2 = new G4Material("SiO2", 2.20*g/cm3, 2);
    SiO2->AddElement(elSi, 1);
    SiO2->AddElement(elO,  2);

    G4Material* MgO = new G4Material("MgO", 3.58*g/cm3, 2);
    MgO->AddElement(elMg, 1);
    MgO->AddElement(elO,  1);

    G4Material* Al2O3 = new G4Material("Al2O3", 3.95*g/cm3, 2);
    Al2O3->AddElement(elAl, 2);
    Al2O3->AddElement(elO,  3);

    G4Material* Ce2O3 = new G4Material("Ce2O3", 6.20*g/cm3, 2);
    Ce2O3->AddElement(elCe, 2);
    Ce2O3->AddElement(elO,  3);

    G4Material* Li2O = new G4Material("Li2O", 2.01*g/cm3, 2);
    Li2O->AddElement(elLi_enriched, 2);
    Li2O->AddElement(elO,  1);

    // 4. Liガラスシンチレータ (GS20) の完成
    G4double density_GS20 = 2.5 * g/cm3;
    // ここでローカル変数ではなく、クラスのメンバ変数 fMaterial に代入します
    fMaterial = new G4Material("GS20", density_GS20, 5);

    fMaterial->AddMaterial(SiO2,  58.5 * perCent);
    fMaterial->AddMaterial(MgO,    4.0 * perCent);
    fMaterial->AddMaterial(Al2O3, 18.0 * perCent);
    fMaterial->AddMaterial(Ce2O3,  4.0 * perCent);
    fMaterial->AddMaterial(Li2O,  15.5 * perCent);

    G4MaterialPropertiesTable* mpt = new G4MaterialPropertiesTable();

    //吸収長と屈折率は理論式から算出する
    std::vector<G4double> photonEnergies;
    std::vector<G4double> refractiveIndices;
    std::vector<G4double> absorptionLengths;

    // ------------------------------------------------------------
    // GS20 Sellmeier parameters
    // ------------------------------------------------------------
    constexpr G4double B1 = 6.6053e-1;
    constexpr G4double C1 = 1.5052e-2;  // um^2

    constexpr G4double B2 = 6.6044e-1;
    constexpr G4double C2 = 3.9310e-3;  // um^2

    constexpr G4double B3 = 7.6303e-1;
    constexpr G4double C3 = 8.8505e1;   // um^2

    // ------------------------------------------------------------
    // GS20 absorption-coefficient parameters
    // ------------------------------------------------------------

    constexpr G4double A0 = -4.513e-1;  // cm^-1
    constexpr G4double lambda0 = 3.46e-1;  // um

    constexpr G4double A1 = 1.550e1;    // cm^-1
    constexpr G4double l1 = 7.446e-3;   // um

    constexpr G4double A2 = 5.0e-1;     // cm^-1
    constexpr G4double l2 = 3.0;        // um

    // ------------------------------------------------------------
    // Generate optical-property tables
    //
    // Geant4 requires photon energies in ascending order.
    // Because E = hc/lambda, wavelengths must be processed from
    // long wavelength to short wavelength.
    // ------------------------------------------------------------

    for (G4int wavelengthNm = 550;
        wavelengthNm >= 320;
        --wavelengthNm)
    {
        const G4double lambdaUm =
            static_cast<G4double>(wavelengthNm) / 1000.0;

        const G4double lambdaSquared = lambdaUm * lambdaUm;

        // Sellmeier equation
        const G4double refractiveIndexSquared =
            1.0
            + B1 * lambdaSquared / (lambdaSquared - C1)
            + B2 * lambdaSquared / (lambdaSquared - C2)
            + B3 * lambdaSquared / (lambdaSquared - C3);

        const G4double refractiveIndex =
            std::sqrt(refractiveIndexSquared);

        // Absorption coefficient in cm^-1
        const G4double alphaCmInverse =
            A0
            + A1 * std::exp(-(lambdaUm - lambda0) / l1)
            + A2 * std::exp(-(lambdaUm - lambda0) / l2);

        if (alphaCmInverse <= 0.0) {
            G4ExceptionDescription message;
            message
                << "Non-positive GS20 absorption coefficient at "
                << wavelengthNm << " nm: "
                << alphaCmInverse << " cm^-1";

            G4Exception(
                "GS20Mat::GS20Mat",
                "GS20OpticalProperty001",
                FatalException,
                message
            );
        }

        // alpha [cm^-1] -> absorption length
        const G4double absorptionLength =
            (1.0 / alphaCmInverse) * cm;

        const G4double wavelength =
            static_cast<G4double>(wavelengthNm) * nm;

        const G4double photonEnergy =
                (h_Planck * c_light) / wavelength;

        photonEnergies.push_back(photonEnergy);
        refractiveIndices.push_back(refractiveIndex);
        absorptionLengths.push_back(absorptionLength);
    }

    mpt->AddProperty(
        "RINDEX",
        photonEnergies,
        refractiveIndices,
        false,
        false
    );

    mpt->AddProperty(
        "ABSLENGTH",
        photonEnergies,
        absorptionLengths,
        false,
        false
    );

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
        mpt->AddProperty(key, energies, values, false, false);//4=新規か、5 true=spline補完 false=線形補完
    };

    // 波長[nm], normalized intensity
    AddPropertyFromNm("SCINTILLATIONCOMPONENT1", {
        { 320.0, 0.015 },
        { 330.0, 0.055 },
        { 340.0, 0.135 },
        { 350.0, 0.260 },
        { 360.0, 0.440 },
        { 370.0, 0.650 },
        { 380.0, 0.840 },
        { 390.0, 0.965 },
        { 395.0, 1.000 },
        { 400.0, 0.985 },
        { 410.0, 0.900 },
        { 420.0, 0.770 },
        { 430.0, 0.620 },
        { 440.0, 0.470 },
        { 450.0, 0.340 },
        { 460.0, 0.245 },
        { 470.0, 0.170 },
        { 480.0, 0.115 },
        { 490.0, 0.075 },
        { 500.0, 0.047 },
        { 510.0, 0.029 },
        { 520.0, 0.017 },
        { 530.0, 0.009 },
        { 540.0, 0.004 },
        { 550.0, 0.001 }
    });



    //　光子の発生数
    mpt->AddConstProperty("SCINTILLATIONYIELD", 5500.0 /MeV);   //カタログ上はplasticの20~30%(諸説あり)なので、Codexとの相談の上ひとまず5500/MeVとした
    //　光子発生のばらつき（ポアソン分布）  
    mpt->AddConstProperty("RESOLUTIONSCALE", 1.0); 
    //単一の成分のみと仮定
    mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 70*ns);    
    // 瞬間的な立ち上がりと仮定
    mpt->AddConstProperty("SCINTILLATIONRISETIME1", 0.0 * ns); 
    
    //クチンチング（消光効果）に関するパラメータ
    fMaterial->GetIonisation()->SetBirksConstant( (0.021 * mm/MeV) );

    fMaterial->SetMaterialPropertiesTable(mpt);

}

GS20Mat::~GS20Mat() {
    // 補足: G4MaterialはGeant4のカーネルが内部で一括管理して破棄してくれるため、
    // ここでわざわざ delete fMaterial; を書く必要はありません。
}