#pragma once

#include <al/Library/LiveActor/LiveActor.h>

namespace al {
class PartsModel : public al::LiveActor {
private:
    al::LiveActor *mParentModel = nullptr;
    const sead::Matrix34f* mJointMtx = nullptr;
    bool mIsUseLocalPos = false;
    sead::Vector3f mLocalTrans = sead::Vector3f(0.0f,0.0f,0.0f);
    sead::Vector3f mLocalRotate = sead::Vector3f(0.0f,0.0f,0.0f);
    sead::Vector3f mLocalScale = sead::Vector3f(1.0f,1.0f,1.0f);
    bool mIsUseFollowMtxScale = false;
    bool mIsUseLocalScale = false;
    bool field_142 = true;
public:
    PartsModel(char const*);
    void endClipped() override;
    void calcAnim() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(al::SensorMsg const*, al::HitSensor*, al::HitSensor*) override;

    void initPartsDirect(al::LiveActor*, al::ActorInitInfo const&, char const*, sead::Matrix34f const*, sead::Vector3f const&, sead::Vector3f const&, sead::Vector3f const&, bool);
    void initPartsSuffix(al::LiveActor*, al::ActorInitInfo const&, char const*, char const*, sead::Matrix34f const*, bool);
    void initPartsMtx(al::LiveActor*, al::ActorInitInfo const&, char const*, sead::Matrix34f const*, bool);
    void initPartsFixFile(al::LiveActor*, al::ActorInitInfo const&, char const*, char const*, char const*);
    void initPartsFixFileNoRegister(al::LiveActor*, al::ActorInitInfo const&, char const*,
                                    char const*, char const*);

    void updatePose();
    void offSyncAppearAndHide();
    void onSyncAppearAndHide();
};
}
