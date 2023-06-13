#pragma once

#include <al/Library/LiveActor/LiveActor.h>

namespace al {
class SimpleCircleShadowXZ : public al::LiveActor {
private:
    al::LiveActor *mRootActor;
    sead::Vector3f mOffsetInterpole;
    bool field_11C;
    bool field_11D;
    sead::Vector3f mActorScale;
    sead::Vector3f mScale;
    sead::Vector3f field_138;
    sead::Vector3f mOffset;
    sead::Vector3f field_150;
    sead::Vector3f field_15C;
    sead::Vector3f mRotate;
    s32 mInterpoleStep;
    s32 mInterpoleFrame;
public:
    SimpleCircleShadowXZ(const char*);
    void initSimpleCircleShadow(al::LiveActor *,const al::ActorInitInfo&,const char*,const char*);
    void makeActorAlive() override;
    void updatePose();
    void control() override;
    void syncHostVisible();
    void setOffsetWithInterpole(const sead::Vector3f&);
    void setScaleWithInterpole(const sead::Vector3f&);
    void setRotateWithInterpole(const sead::Vector3f&);
    void setInterpoleFrame(int);
};
static_assert(sizeof(SimpleCircleShadowXZ) == 0x180, "DepthShadowModel Size");
}