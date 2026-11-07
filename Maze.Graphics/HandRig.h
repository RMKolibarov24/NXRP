#pragma once
#include "Maze.CreatureMesh.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <istream>
#include <optional>
#include <vector>

namespace HandMotion
{
struct V
{
    float x=0,y=0,z=0;
    V operator+(V b)const{return {x+b.x,y+b.y,z+b.z};}
    V operator-(V b)const{return {x-b.x,y-b.y,z-b.z};}
    V operator*(float k)const{return {x*k,y*k,z*k};}
};
inline float dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float length(V a){return std::sqrt(dot(a,a));}
inline V unit(V a,V fallback={0,1,0}){const float n=length(a);return n>1e-6f?a*(1.f/n):fallback;}
struct R
{
    std::array<float,9> m{1,0,0,0,1,0,0,0,1};
    V operator*(V a)const{return {m[0]*a.x+m[1]*a.y+m[2]*a.z,m[3]*a.x+m[4]*a.y+m[5]*a.z,m[6]*a.x+m[7]*a.y+m[8]*a.z};}
    R operator*(const R& b)const
    {
        R out;out.m.fill(0.f);
        for(int i=0;i<3;++i)for(int j=0;j<3;++j)for(int k=0;k<3;++k)out.m[i*3+j]+=m[i*3+k]*b.m[k*3+j];
        return out;
    }
    R transpose()const{return {{m[0],m[3],m[6],m[1],m[4],m[7],m[2],m[5],m[8]}};}
};
inline R basis(V x,V y,V z){return {{x.x,y.x,z.x,x.y,y.y,z.y,x.z,y.z,z.z}};}
inline R axisAngle(V axis,float radians)
{
    axis=unit(axis);const float c=std::cos(radians),s=std::sin(radians),t=1-c;
    const float x=axis.x,y=axis.y,z=axis.z;
    return {{t*x*x+c,t*x*y-s*z,t*x*z+s*y,t*x*y+s*z,t*y*y+c,t*y*z-s*x,t*x*z-s*y,t*y*z+s*x,t*z*z+c}};
}
inline R between(V from,V to)
{
    from=unit(from);to=unit(to);
    const float c=std::clamp(dot(from,to),-1.f,1.f);
    const auto axis=cross(from,to);
    if(length(axis)<1e-5f)return c>0.f?R{}:axisAngle(cross(from,std::abs(from.x)<.8f?V{1,0,0}:V{0,1,0}),3.14159265f);
    return axisAngle(axis,std::acos(c));
}
struct ArmSolution{V elbow,wrist;};
inline ArmSolution solveArm(V shoulder,V target,V pole,float upper,float lower)
{
    upper=std::max(.001f,upper);lower=std::max(.001f,lower);
    const V delta=target-shoulder,forward=unit(delta,{0,0,-1});
    const float distance=std::clamp(length(delta),std::abs(upper-lower)+.0001f,upper+lower-.0001f);
    const float along=(upper*upper-lower*lower+distance*distance)/(2.f*distance);
    const V bend=unit(pole-forward*dot(pole,forward),unit(cross(forward,{1,0,0}),{0,0,1}));
    return {shoulder+forward*along+bend*std::sqrt(std::max(0.f,upper*upper-along*along)),shoulder+forward*distance};
}
struct Bone{int parent=-1;unsigned role=0;float bend=0;V at;R rotation;};
struct Vertex{V at,normal;std::array<float,2> uv;std::array<float,4> weights;std::array<std::uint32_t,4> joints;};
struct Rig
{
    std::vector<Bone> bones;
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    R palmBasis;
    float palmLength=0;
    int upper=-1,lower=-1,hand=-1;
};
inline std::optional<Rig> readRig(std::istream& in)
{
    static_assert(sizeof(float)==4 && sizeof(V)==12);
    const auto read=[&](auto& value){return static_cast<bool>(in.read(reinterpret_cast<char*>(&value),sizeof(value)));};
    char magic[4];std::uint32_t vertices=0,indices=0,bones=0;
    if(!in.read(magic,4) || !std::equal(magic,magic+4,"NXR1") || !read(vertices) || !read(indices) || !read(bones)
        || vertices<3 || vertices>100000 || indices<3 || indices>600000 || indices%3 || bones<3 || bones>64)return {};
    Rig rig;
    if(!read(rig.palmLength) || !read(rig.palmBasis.m) || !std::isfinite(rig.palmLength) || rig.palmLength<.01f || rig.palmLength>.3f)return {};
    const auto finiteV=[](V v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z) && length(v)<10.f;};
    const auto validR=[](R r)
    {
        for(float f:r.m)if(!std::isfinite(f))return false;
        const R check=r.transpose()*r;
        for(int i=0;i<9;++i)if(std::abs(check.m[i]-(i%4==0?1.f:0.f))>.005f)return false;
        return dot(cross(r*V{1,0,0},r*V{0,1,0}),r*V{0,0,1})>.99f;
    };
    if(!validR(rig.palmBasis))return {};
    rig.bones.resize(bones);
    for(unsigned i=0;i<bones;++i)
    {
        auto& b=rig.bones[i];
        if(!read(b.parent)||!read(b.role)||!read(b.bend)||!read(b.at)||!read(b.rotation.m)
            || b.parent>=static_cast<int>(i) || b.parent< -1 || (i>0 && b.parent<0) || b.role>5
            || !std::isfinite(b.bend) || b.bend<0.f || b.bend>2.f || !finiteV(b.at)||!validR(b.rotation))return {};
        int* role=b.role==1?&rig.upper:b.role==2?&rig.lower:b.role==3?&rig.hand:nullptr;
        if(role){if(*role>=0)return {};*role=static_cast<int>(i);}
    }
    if(rig.upper!=0 || rig.lower<0 || rig.hand<0 || rig.bones[rig.lower].parent!=rig.upper || rig.bones[rig.hand].parent!=rig.lower)return {};
    for(int child:{rig.lower,rig.hand})
    {const float n=length(rig.bones[child].at-rig.bones[rig.bones[child].parent].at);if(n<.05f || n>.6f)return {};}
    rig.vertices.resize(vertices);rig.indices.resize(indices);
    for(auto& v:rig.vertices)
    {
        if(!read(v.at)||!read(v.normal)||!read(v.uv)||!read(v.weights)||!read(v.joints)||!finiteV(v.at)||!finiteV(v.normal))return {};
        for(float u:v.uv)if(!std::isfinite(u)||u<0.f||u>1.f)return {};
        float total=0;
        for(int j=0;j<4;++j){if(!std::isfinite(v.weights[j])||v.weights[j]<0.f||v.weights[j]>1.f||v.joints[j]>=bones)return {};total+=v.weights[j];}
        if(std::abs(total-1.f)>.002f)return {};
    }
    if(!in.read(reinterpret_cast<char*>(rig.indices.data()),indices*sizeof(std::uint32_t)))return {};
    for(auto i:rig.indices)if(i>=vertices)return {};
    return rig;
}
struct Pose
{
    std::vector<CreatureVertex> vertices;
    std::vector<V> joints;
    std::vector<R> rotations;
    std::vector<R> skinRotations;
    std::vector<V> skinOffsets;
    ArmSolution arm;
};
inline void skin(const Rig& rig,Pose& pose,V shoulder,V palm,V fingerDirection,V palmAcross,float grip,float wristRoll=0.f)
{
    const V forward=unit(fingerDirection,{0,0,-1});
    const V across=unit(palmAcross-forward*dot(palmAcross,forward),{1,0,0});
    const R desired=axisAngle(forward,wristRoll)*basis(across,forward,cross(across,forward))*rig.palmBasis.transpose();
    const V wrist=palm-forward*rig.palmLength;
    const auto& u=rig.bones[rig.upper];const auto& l=rig.bones[rig.lower];const auto& h=rig.bones[rig.hand];
    pose.arm=solveArm(shoulder,wrist,{shoulder.x<0.f?-.6f:.6f,-1.f,.15f},length(l.at-u.at),length(h.at-l.at));
    pose.joints.resize(rig.bones.size());pose.rotations.resize(rig.bones.size());
    for(std::size_t i=0;i<rig.bones.size();++i)
    {
        const auto& b=rig.bones[i];
        if(static_cast<int>(i)==rig.upper)
        {pose.joints[i]=shoulder;pose.rotations[i]=between(l.at-u.at,pose.arm.elbow-shoulder)*b.rotation;}
        else if(static_cast<int>(i)==rig.lower)
        {pose.joints[i]=pose.arm.elbow;pose.rotations[i]=between(h.at-l.at,pose.arm.wrist-pose.arm.elbow)*b.rotation;}
        else if(static_cast<int>(i)==rig.hand)
        {pose.joints[i]=pose.arm.wrist;pose.rotations[i]=desired*b.rotation;}
        else
        {
            const auto& parent=rig.bones[b.parent];
            pose.joints[i]=pose.joints[b.parent]+pose.rotations[b.parent]*(parent.rotation.transpose()*(b.at-parent.at));
            const R local=parent.rotation.transpose()*b.rotation;
            // Original source finger bones and skin weights, evaluated at runtime.
            const float curl=(b.role==4 || b.role==5)?b.bend*std::clamp(grip,0.f,1.f):0.f;
            pose.rotations[i]=pose.rotations[b.parent]*local*axisAngle({1,0,0},curl);
        }
    }
    pose.skinRotations.resize(rig.bones.size());pose.skinOffsets.resize(rig.bones.size());
    for(std::size_t i=0;i<rig.bones.size();++i)
    {
        pose.skinRotations[i]=pose.rotations[i]*rig.bones[i].rotation.transpose();
        pose.skinOffsets[i]=pose.joints[i]-pose.skinRotations[i]*rig.bones[i].at;
    }
    pose.vertices.resize(rig.vertices.size());
    for(std::size_t i=0;i<rig.vertices.size();++i)
    {
        const auto& source=rig.vertices[i];V at{},normal{};
        for(int j=0;j<4;++j)
        {
            const auto bone=source.joints[j];const float w=source.weights[j];
            if(w<=0.f)continue;
            const R rotate=pose.skinRotations[bone];
            at=at+(pose.skinOffsets[bone]+rotate*source.at)*w;
            normal=normal+(rotate*source.normal)*w;
        }
        normal=unit(normal);auto& out=pose.vertices[i];
        out={{at.x,at.y,at.z},{normal.x,normal.y,normal.z},{source.uv[0],source.uv[1]}};
    }
}
}
