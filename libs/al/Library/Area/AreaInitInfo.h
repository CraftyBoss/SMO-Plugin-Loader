#pragma once

#include <al/Library/Placement/PlacementInfo.h>
#include <al/Library/Scene/SceneObjHolder.h>

namespace al {
class PlacementInfo;
class StageSwitchDirector;

class AreaInitInfo {
private:
    al::PlacementInfo mPlacementInfo;
    al::StageSwitchDirector* mStageSwitchDirector;
    al::SceneObjHolder* mSceneObjHolder;

public:
    AreaInitInfo();
    AreaInitInfo(const al::PlacementInfo& placementInfo,
                 al::StageSwitchDirector* stageSwitchDirector, al::SceneObjHolder* sceneObjHolder);
    AreaInitInfo(const al::PlacementInfo& placementInfo, const al::AreaInitInfo& initInfo);

    void set(const al::PlacementInfo& placementInfo, al::StageSwitchDirector* stageSwitchDirector,
             al::SceneObjHolder* sceneObjHolder);

    const al::PlacementInfo& getPlacementInfo() const { return mPlacementInfo; }
    al::StageSwitchDirector* getStageSwitchDirector() const { return mStageSwitchDirector; }
    al::SceneObjHolder* getSceneObjHolder() const { return mSceneObjHolder; }
};
}  // namespace al
