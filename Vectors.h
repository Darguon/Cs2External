#pragma once
#include <cmath>

struct Vector2
{
    float x, y;

    Vector2() : x(0.f), y(0.f) {}
    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator+(const Vector2& other) const { return Vector2(x + other.x, y + other.y); }
    Vector2 operator-(const Vector2& other) const { return Vector2(x - other.x, y - other.y); }
    Vector2 operator*(float scalar) const { return Vector2(x * scalar, y * scalar); }
    Vector2 operator/(float scalar) const { return Vector2(x / scalar, y / scalar); }

    float Length() const { return std::sqrt(x * x + y * y); }
    float DistanceTo(const Vector2& other) const { return (*this - other).Length(); }
};

struct Vector3
{
    float x, y, z;

    Vector3() : x(0.f), y(0.f), z(0.f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& other) const { return Vector3(x + other.x, y + other.y, z + other.z); }
    Vector3 operator-(const Vector3& other) const { return Vector3(x - other.x, y - other.y, z - other.z); }
    Vector3 operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
    Vector3 operator/(float scalar) const { return Vector3(x / scalar, y / scalar, z / scalar); }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float Length2D() const { return std::sqrt(x * x + y * y); }
    float DistanceTo(const Vector3& other) const { return (*this - other).Length(); }

    bool IsZero() const { return (x == 0.f && y == 0.f && z == 0.f); }
};

struct Vector4
{
    float x, y, z, w;

    Vector4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

struct Matrix4x4
{
    float m[4][4];

    Matrix4x4()
    {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                m[i][j] = 0.f;
    }
};

#define	PITCH 0 // Up / Down
#define	YAW	  1 // Left / Right
#define	ROLL  2 // Fall over