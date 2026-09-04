#ifndef DETECTORCHANNELKEY_HH
#define DETECTORCHANNELKEY_HH

#include "globals.hh"

struct DetectorKey {
    G4int detectorCopyNo = -1;

    DetectorKey() = default;

    explicit DetectorKey(G4int copyNo):detectorCopyNo(copyNo){}

    bool operator==(const DetectorKey& other) const {return detectorCopyNo==other.detectorCopyNo;}
    bool operator<(const DetectorKey& other) const {return detectorCopyNo<other.detectorCopyNo;}

};

struct PmtChannelKey {
    DetectorKey detector;
    G4int pmtCopyNo = -1;

    PmtChannelKey(const DetectorKey& detectorKey,G4int copyNo)
    : detector(detectorKey),pmtCopyNo(copyNo)
    {
    }   

    bool operator==(const PmtChannelKey& other) 
    const {return detector == other.detector&& pmtCopyNo == other.pmtCopyNo; }

    bool operator<(const PmtChannelKey& other)
    const {

        //そもそも同一detector内で比較することが想定されている
        if (detector < other.detector){
            return true;
        }

        if (other.detector < detector){
            return false;
        }

        return pmtCopyNo < other.pmtCopyNo;
    }

};

#endif
