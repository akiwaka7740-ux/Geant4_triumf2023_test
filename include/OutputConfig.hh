#ifndef OUTPUTCONFIG_HH
#define OUTPUTCONFIG_HH

#include "globals.hh"

enum class OutputNamingMode {
    Timestamp,
    Source
};

struct OutputConfig {
    // 実行中は固定する識別子
    G4String executionId;

    // results/<実行ID> に対応するディレクトリ
    G4String executionDirectory;

    // ROOT ファイル名の命名方式
    OutputNamingMode namingMode = OutputNamingMode::Timestamp;
};

#endif