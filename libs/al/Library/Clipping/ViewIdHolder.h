#pragma once

#include <container/seadBuffer.h>
#include <al/Library/Placement/PlacementId.h>

namespace al {
class PlacementInfo;

class ViewIdHolder {
private:
    s32 mNumPlacements = 0;
    al::PlacementId* mPlacementIds = nullptr;

public:
    ViewIdHolder();
    void init(const al::PlacementInfo& placementInfo);
    al::PlacementId &getViewId(int idx) const;

    static ViewIdHolder* tryCreate(const al::PlacementInfo& placementInfo);
};
}  // namespace al
