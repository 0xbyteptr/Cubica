#pragma once
#include <array>
#include <cmath>

namespace Math {

using Mat4 = std::array<float, 16>;

static inline Mat4 identity() {
    Mat4 m{};
    m[0]=1; m[5]=1; m[10]=1; m[15]=1;
    return m;
}

static inline Mat4 perspective(float fovDeg, float aspect, float znear, float zfar) {
    float f = 1.0f / std::tan(fovDeg * 0.5f * 3.14159265f / 180.0f);
    Mat4 m{};
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (zfar + znear) / (znear - zfar);
    m[11] = -1.0f;
    m[14] = (2.0f * zfar * znear) / (znear - zfar);
    return m;
}

struct Vec3 { float x, y, z; };

static inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    auto sub = [](const Vec3&a,const Vec3&b){return Vec3{a.x-b.x,a.y-b.y,a.z-b.z};};
    auto norm = [](Vec3 v){float l = std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); return Vec3{v.x/l,v.y/l,v.z/l};};
    auto cross = [](const Vec3&a,const Vec3&b){return Vec3{a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};};
    auto dot = [](const Vec3&a,const Vec3&b){return a.x*b.x+a.y*b.y+a.z*b.z;};

    Vec3 f = norm(sub(center, eye));
    Vec3 s = norm(cross(f, norm(up)));
    Vec3 u = cross(s, f);

    Mat4 m = identity();
    m[0] = s.x; m[4] = s.y; m[8]  = s.z;
    m[1] = u.x; m[5] = u.y; m[9]  = u.z;
    m[2] = -f.x; m[6] = -f.y; m[10] = -f.z;
    m[12] = -dot(s, eye); m[13] = -dot(u, eye); m[14] = dot(f, eye);
    return m;
}

} // namespace Math
