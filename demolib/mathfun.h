// Linear algebra and special math functions so I can do fun 3D graphics.
// Forgive the messiness, I haven't formally learned much linear algebra yet.
//
// Angles are in radians, use Deg2Rad. This is because the standard trig
// functions already expect radians.
//
// I'd do SIMD, but that seems excessive, not to mention all the ugly ifdefs
// I'd need to support ARM. I'm also not sure the rules allow it, seeing as
// it's a "system library" in a sense. Maybe if I have time I'll put it behind
// EXTRA_CONFORMANT.

#pragma once

#include "misc.h"

static constexpr float PI = 3.14159365359f;

static inline constexpr float Deg2Rad(float deg)
{
	return deg * PI / 180;
}

static inline constexpr float Rad2Deg(float rad)
{
	return rad * 180 / PI;
}

static inline constexpr float AtLeast(float value, float min = FLT_EPSILON)
{
	return abs(value) > min ? value : 0.0f;
}

static FORCEINLINE bool FloatEqual(float a, float b, float epsilon = FLT_EPSILON)
{
	return abs(a - b) < epsilon;
}

// predeclare these, so they can reference each other
struct Vec2;
struct Vec3;
struct Vec3i;
struct Vec4;
struct Quat;
struct Mat4;
struct Transform;

struct Vec2
{
	// I do realize this construct is non-standard, but every compiler I've encountered respects it
	union {
		struct
		{
			float x;
			float y;
		};
		struct
		{
			float u;
			float v;
		};
		float values[2];
	};

	constexpr Vec2(float x, float y) : x(x), y(y)
	{
	}

	constexpr Vec2(float value = 0.0f) : Vec2(value, value)
	{
	}

	constexpr Vec2(const Vec2& other) : Vec2(other.x, other.y)
	{
	}

	Vec2 operator+(const Vec2& other) const
	{
		return Vec2(x + other.x, y + other.y);
	}

	Vec2 operator+=(const Vec2& other)
	{
		return *this = *this + other;
	}

	Vec2 operator-(const Vec2& other) const
	{
		return Vec2(x - other.x, y - other.y);
	}

	Vec2 operator-=(const Vec2& other)
	{
		return *this = *this - other;
	}

	Vec2 operator-() const
	{
		return *this * -1.0f;
	}

	Vec2 operator*(const Vec2& other) const
	{
		return Vec2(x * other.x, y * other.y);
	}

	Vec2 operator*=(const Vec2& other)
	{
		return *this = *this * other;
	}

	Vec2 operator*(float other) const
	{
		return Vec2(x * other, y * other);
	}

	Vec2 operator*=(float other)
	{
		return *this = *this * other;
	}

	Vec2 operator/(float other) const
	{
		other = 1 / other;
		return Vec2(x * other, y * other);
	}

	Vec2 operator/=(float other)
	{
		return *this = *this / other;
	}

	float operator[](size_t idx) const
	{
		ASSERT((idx < ArraySize(values), "Vector index out of bounds"));
		return values[idx];
	}

	float& operator[](size_t idx)
	{
		ASSERT((idx < ArraySize(values), "Vector index out of bounds"));
		return values[idx];
	}

	float Dot(const Vec2& other) const
	{
		return x * other.x + y * other.y;
	}

	float Cross(const Vec2& other) const
	{
		return x * other.y - other.x * y;
	}

	float Length() const
	{
		return sqrtf(x * x + y * y);
	}

	Vec2 Normalize() const
	{
		float len = Length();
		if (len == 0)
		{
			return Vec2(0);
		}
		len = 1 / len;
		return Vec2(x * len, y * len);
	}

	static const Vec2 UP;
	static const Vec2 DOWN;
	static const Vec2 RIGHT;
	static const Vec2 LEFT;
};

inline constexpr const Vec2 Vec2::UP = Vec2(0.0f, 1.0f);
inline constexpr const Vec2 Vec2::DOWN = Vec2(0.0f, -1.0f);
inline constexpr const Vec2 Vec2::RIGHT = Vec2(1.0f, 0.0f);
inline constexpr const Vec2 Vec2::LEFT = Vec2(-1.0f, 0.0f);

struct Vec3
{
	union {
		struct
		{
			float x;
			float y;
			float z;
		};
		struct
		{
			float u;
			float v;
			float w;
		};
		float values[3];
	};

	constexpr Vec3(float x, float y, float z) : x(x), y(y), z(z)
	{
	}

	constexpr Vec3(float value = 0.0f) : Vec3(value, value, value)
	{
	}

	constexpr Vec3(const Vec3& other) : Vec3(other.x, other.y, other.z)
	{
	}

	constexpr Vec3(const Vec2& other, float z = 0.0f) : Vec3(other.x, other.y, z)
	{
	}

	Vec3 operator+(const Vec3& other) const
	{
		return Vec3(x + other.x, y + other.y, z + other.z);
	}

	Vec3 operator+=(const Vec3& other)
	{
		return *this = *this + other;
	}

	Vec3 operator-(const Vec3& other) const
	{
		return Vec3(x - other.x, y - other.y, z - other.z);
	}

	Vec3 operator-=(const Vec3& other)
	{
		return *this = *this - other;
	}

	Vec3 operator-() const
	{
		return *this * -1.0f;
	}

	Vec3 operator*(const Vec3& other) const
	{
		return Vec3(x * other.x, y * other.y, z * other.z);
	}

	Vec3 operator*=(const Vec3& other)
	{
		return *this = *this * other;
	}

	Vec3 operator*(float other) const
	{
		return Vec3(x * other, y * other, z * other);
	}

	Vec3 operator*=(float other)
	{
		return *this = *this * other;
	}

	Vec3 operator/(float other) const
	{
		other = 1 / other;
		return Vec3(x * other, y * other, z * other);
	}

	Vec3 operator/=(float other)
	{
		return *this = *this / other;
	}

	float operator[](size_t idx) const
	{
		ASSERT((idx < ArraySize(values), "Vector index out of bounds"));
		return values[idx];
	}

	float& operator[](size_t idx)
	{
		ASSERT((idx < ArraySize(values), "Vector index out of bounds"));
		return values[idx];
	}

	float Dot(const Vec3& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

	Vec3 Cross(const Vec3& other) const
	{
		return Vec3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
	}

	float Length() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	float Sign() const
	{
		return (x + y + z) / Length();
	}

	Vec3 Normalize() const
	{
		float len = Length();
		if (len < FLT_EPSILON)
		{
			return Vec3(0);
		}
		len = 1 / len;
		return Vec3(x * len, y * len, z * len);
	}

	Vec3 NormalToColor() const
	{
		return *this * 0.5 + 0.5;
	}

	Vec3 ColorToNormal() const
	{
		return *this * 2.0f - 1.0f;
	}

	Vec3 Lerp(const Vec3& p1, float t) const
	{
		const Vec3& p0 = *this;
		return p0 * (1.0f - t) + p1 * t;
	}

	float Major() const
	{
		return std::max(x, std::max(y, z));
	}

	float Minor() const
	{
		return std::min(x, std::min(y, z));
	}

	static const Vec3 UP;
	static const Vec3 DOWN;
	static const Vec3 RIGHT;
	static const Vec3 LEFT;
	static const Vec3 FORWARD;
	static const Vec3 BACKWARD;
};

inline constexpr const Vec3 Vec3::UP = Vec3(0.0f, 1.0f, 0.0f);
inline constexpr const Vec3 Vec3::DOWN = Vec3(0.0f, -1.0f, 0.0f);
inline constexpr const Vec3 Vec3::RIGHT = Vec3(1.0f, 0.0f, 0.0f);
inline constexpr const Vec3 Vec3::LEFT = Vec3(-1.0f, 0.0f, 0.0f);
inline constexpr const Vec3 Vec3::FORWARD = Vec3(0.0f, 0.0f, -1.0f);
inline constexpr const Vec3 Vec3::BACKWARD = Vec3(0.0f, 0.0f, 1.0f);

struct Vec3i
{
	union {
		struct
		{
			int x;
			int y;
			int z;
		};
		struct
		{
			int u; // roll
			int v; // pitch
			int w; // yaw
		};
		int values[3];
	};

	constexpr Vec3i(int x, int y, int z) : x(x), y(y), z(z)
	{
	}

	constexpr Vec3i(int value = 0.0f) : Vec3i(value, value, value)
	{
	}

	constexpr Vec3i(const Vec3i& other) : Vec3i(other.x, other.y, other.z)
	{
	}

	Vec3i operator+(const Vec3i& other) const
	{
		return Vec3i(x + other.x, y + other.y, z + other.z);
	}

	Vec3i operator+=(const Vec3i& other)
	{
		return *this = *this + other;
	}

	Vec3i operator-(const Vec3i& other) const
	{
		return Vec3i(x - other.x, y - other.y, z - other.z);
	}

	Vec3i operator-=(const Vec3i& other)
	{
		return *this = *this - other;
	}

	Vec3i operator-() const
	{
		return *this * -1;
	}

	Vec3i operator*(int other) const
	{
		return Vec3i(x * other, y * other, z * other);
	}

	Vec3i operator*=(int other)
	{
		return *this = *this * other;
	}

	Vec3i operator/(int other) const
	{
		float inv = 1.0f / other;
		return Vec3i((int)(x * inv), (int)(y * inv), (int)(z * inv));
	}

	Vec3i operator/=(int other)
	{
		return *this = *this / other;
	}

	int operator[](size_t idx) const
	{
		ASSERT((idx < ArraySize(values), "Vector index out of bounds"));
		return values[idx];
	}

	int& operator[](size_t idx)
	{
		ASSERT((idx < ArraySize(values), "Vector index out of bounds"));
		return values[idx];
	}

	int Dot(const Vec3i& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}

	Vec3i Cross(const Vec3i& other) const
	{
		return Vec3i(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
	}

	float Length() const
	{
		return sqrtf((float)(x * x + y * y + z * z));
	}

	static const Vec3i UP;
	static const Vec3i DOWN;
	static const Vec3i RIGHT;
	static const Vec3i LEFT;
	static const Vec3i FORWARD;
	static const Vec3i BACKWARD;
};

inline constexpr const Vec3i Vec3i::UP = Vec3i(0, 1, 0);
inline constexpr const Vec3i Vec3i::DOWN = Vec3i(0, -1, 0);
inline constexpr const Vec3i Vec3i::RIGHT = Vec3i(1, 0, 0);
inline constexpr const Vec3i Vec3i::LEFT = Vec3i(-1, 0, 0);
inline constexpr const Vec3i Vec3i::FORWARD = Vec3i(0, 0, -1);
inline constexpr const Vec3i Vec3i::BACKWARD = Vec3i(0, 0, 1);

struct Vec4
{
	union {
		struct
		{
			float x;
			float y;
			float z;
			float w;
		};
		struct
		{
			float r;
			float g;
			float b;
			float a;
		};
		struct
		{
			float h; // radians
			float s;
			float v;
			float a;
		};
		float values[4];
	};

	constexpr Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w)
	{
	}

	constexpr Vec4(float value = 0.0f) : Vec4(value, value, value, value)
	{
	}

	constexpr Vec4(const Vec4& other) : Vec4(other.x, other.y, other.z, other.w)
	{
	}

	constexpr Vec4(const Vec3& other, float w = 1.0f) : Vec4(other.x, other.y, other.z, w)
	{
	}

	constexpr Vec4(const Vec2& other, float z = 0.0f, float w = 1.0f) : Vec4(other.x, other.y, z, w)
	{
	}

	Vec4 operator+(const Vec4& other) const
	{
		return Vec4(x + other.x, y + other.y, z + other.z, w + other.w);
	}

	Vec4 operator+=(const Vec4& other)
	{
		return *this = *this + other;
	}

	Vec4 operator-(const Vec4& other) const
	{
		return Vec4(x - other.x, y - other.y, z - other.z, w - other.w);
	}

	Vec4 operator-=(const Vec4& other)
	{
		return *this = *this - other;
	}

	Vec4 operator-() const
	{
		return *this * -1.0f;
	}

	Vec4 operator*(const Vec4& other) const
	{
		return Vec4(x * other.x, y * other.y, z * other.z, w * other.w);
	}

	Vec4 operator*=(const Vec4& other)
	{
		return *this = *this * other;
	}

	constexpr Vec4 operator*(float other) const
	{
		return Vec4(x * other, y * other, z * other, w * other);
	}

	Vec4 operator*=(float other)
	{
		return *this = *this * other;
	}

	Vec4 operator/(float other) const
	{
		other = 1 / other;
		return Vec4(x * other, y * other, z * other, w * other);
	}

	Vec4 operator/=(float other)
	{
		return *this = *this / other;
	}

	float operator[](size_t idx) const
	{
		ASSERT((idx < ArraySize(values), "Vector index out of bounds"));
		return values[idx];
	}

	float& operator[](size_t idx)
	{
		ASSERT((idx < ArraySize(values), "Vector index out of bounds"));
		return values[idx];
	}

	float Dot(const Vec4& other) const
	{
		return x * other.x + y * other.y + z * other.z + w * other.w;
	}

	float Length() const
	{
		return sqrtf(x * x + y * y + z * z + w * w);
	}

	float Sign() const
	{
		return (x + y + z + w) / Length();
	}

	Vec4 Normalize() const
	{
		float len = Length();
		if (len == 0)
		{
			return Vec4(0);
		}
		len = 1 / len;
		return Vec4(x * len, y * len, z * len, w * len);
	}

	static const Vec4 UP;
	static const Vec4 DOWN;
	static const Vec4 RIGHT;
	static const Vec4 LEFT;
	static const Vec4 FORWARD;
	static const Vec4 BACKWARD;
	static const Vec4 WHITE;
	static const Vec4 BLACK;
	static const Vec4 RED;
	static const Vec4 GREEN;
	static const Vec4 BLUE;
};

inline constexpr const Vec4 Vec4::UP = Vec4(Vec3::UP, 0.0f);
inline constexpr const Vec4 Vec4::DOWN = Vec4(Vec3::DOWN, 0.0f);
inline constexpr const Vec4 Vec4::RIGHT = Vec4(Vec3::RIGHT, 0.0f);
inline constexpr const Vec4 Vec4::LEFT = Vec4(Vec3::LEFT, 0.0f);
inline constexpr const Vec4 Vec4::FORWARD = Vec4(Vec3::FORWARD, 0.0f);
inline constexpr const Vec4 Vec4::BACKWARD = Vec4(Vec3::BACKWARD, 0.0f);
inline constexpr const Vec4 Vec4::WHITE = Vec4(1.0f);
inline constexpr const Vec4 Vec4::BLACK = Vec4(0.0f);
inline constexpr const Vec4 Vec4::RED = Vec4(1.0f, 0.0f, 0.0f, 1.0f);
inline constexpr const Vec4 Vec4::GREEN = Vec4(0.0f, 1.0f, 0.0f, 1.0f);
inline constexpr const Vec4 Vec4::BLUE = Vec4(0.0f, 0.0f, 1.0f, 1.0f);

// https://danceswithcode.net/engineeringnotes/quaternions/quaternions.html
// Euler stuff is ZYX ordered
// Technically a versor cause its always a *unit* quaternion but Q is cooler than V
struct Quat
{
	// w is real, xyz are imaginary
	union {
		struct
		{
			float x; // q1
			float y; // q2
			float z; // q3
			float w; // q0
		};
		float values[4];
	};

	// make sure to normalize the quaternion after this if necessary
	constexpr Quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w)
	{
	}

	Quat(const Vec3& axis = Vec3::UP, float angle = 0.0f)
	{
		Vec3 normalAxis = axis.Normalize();
		float factor = sinf(angle * 0.5f);
		w = cosf(angle * 0.5f);
		x = normalAxis.x * factor;
		y = normalAxis.y * factor;
		z = normalAxis.z * factor;
		*this = Normalize();
	}

	// https://gamedev.stackexchange.com/a/181279
	static Quat FromAngularVelocity(const Vec3& v)
	{
		float len = v.Length();
		if (len < FLT_EPSILON)
		{
			return Quat();
		}

		float half = len * 0.5f;
		float s = sinf(half);
		float c = cosf(half);
		return Quat(v.x * s, v.y * s, v.z * s, len * c).Normalize();
	}

	// Quat(const Vec3& euler)
	//{
	//	Vec3 h = euler * 0.5;
	//	float cu = cos(h.u);
	//	float cv = cos(h.v);
	//	float cw = cos(h.w);
	//	float su = sin(h.u);
	//	float sv = sin(h.v);
	//	float sw = sin(h.w);
	//	w = cu * cv * cw + su * sv * sw;
	//	x = su * cv * cw - cu * sv * sw;
	//	y = cu * sv * cw + su * cv * sw;
	//	z = cu * cv * sw - su * sv * cw;
	// }

	Quat operator+(const Quat& other) const
	{
		return Quat(x + other.x, y + other.y, z + other.z, w + other.w);
	}

	Quat operator+=(const Quat& other)
	{
		return *this = *this + other;
	}

	Quat operator-(const Quat& other) const
	{
		return Quat(x - other.x, y - other.y, z - other.z, w - other.w);
	}

	Quat operator-=(const Quat& other)
	{
		return *this = *this - other;
	}

	Quat operator*(const Quat& other) const
	{
		return Quat(
			w * other.x + x * other.w - y * other.z + z * other.y,
			w * other.y - x * other.z + y * other.w - z * other.x,
			w * other.z - x * other.y + y * other.x + z * other.w,
			w * other.w - x * other.x - y * other.y - z * other.z);
	}

	Quat operator*=(const Quat& other)
	{
		return *this = (*this * other).Normalize();
	}

	Quat operator*(float other) const
	{
		return Quat(x * other, y * other, z * other, w * other);
	}

	Quat operator*=(float other)
	{
		return *this = *this * other;
	}

	Quat operator/(float other) const
	{
		other = 1 / other;
		return Quat(x * other, y * other, z * other, w * other);
	}

	Quat operator/=(float other)
	{
		return *this = *this / other;
	}

	// yummy quaternion math from https://en.wikipedia.org/wiki/Quaternion#Exponential,_logarithm,_and_power_functions
	// not sure if ^ is a weird choice or not, feels natural enough as exponentiation for something that isn't an int
	Quat operator^(float other)
	{
		float norm = Norm();
		float phi = acosf(w / norm);
		Quat ns = GetAxis().Normalize() * sinf(other * phi);
		return (Quat(ns.x, ns.y, ns.z, cosf(other * phi)) * powf(norm, other)).Normalize();
	}

	Vec3 ActiveRotate(const Vec3& other) const
	{
		Quat p(other.x, other.y, other.z, 0);
		Quat result = Inverse() * p * *this;
		return Vec3(result.x, result.y, result.z);
	}

	Vec3 PassiveRotate(const Vec3& other) const
	{
		Quat p(other.x, other.y, other.z, 0);
		Quat result = *this * p * Inverse();
		return Vec3(result.x, result.y, result.z);
	}

	Vec3 operator*(const Vec3& other) const
	{
		return PassiveRotate(other);
	}

	Quat Inverse() const
	{
		return Quat(-x, -y, -z, w);
	}

	float Norm() const
	{
		return sqrtf(x * x + y * y + z * z + w * w);
	}

	Quat Normalize() const
	{
		float len = Norm();
		if (len == 0)
		{
			return Quat(0.0f, 0.0f);
		}
		len = 1 / len;
		return Quat(x * len, y * len, z * len, w * len);
	}

	Vec3 GetAxis() const
	{
		return Vec3(x, y, z);
	}

	float GetAngle() const
	{
		return 2 * acosf(w);
	}

	Vec3 ToEuler() const
	{
		return Vec3(
			atan2f(2 * (w * x + y * z), w * w - x * x - y * y + z * z),
			asinf(2 * (w * y - x * z)),
			atan2f(2 * (w * z + x * y), w * w + x * x - y * y - z * z));
	}

	Quat Slerp(const Quat& q1, float t) const
	{
		const Quat& q0 = *this;
		return (q0 * (q0.Inverse() * q1) ^ t).Normalize();
	}
};

// Column major 4x4 matrix, to keep things simple with OpenGL
struct Mat4
{
	Vec4 columns[4];

	constexpr Mat4() : Mat4(1.0f)
	{
	}

	constexpr Mat4(float value)
		: Mat4(
			  Vec4(value, 0.0f, 0.0f, 0.0f),
			  Vec4(0.0f, value, 0.0f, 0.0f),
			  Vec4(0.0f, 0.0f, value, 0.0f),
			  Vec4(0.0f, 0.0f, 0.0f, value))
	{
	}

	constexpr Mat4(const Vec4& a, const Vec4& b, const Vec4& c, const Vec4& d) : columns {a, b, c, d}
	{
	}

	constexpr Mat4 operator*(float other) const
	{
		return Mat4(Vec4(columns[0] * other), Vec4(columns[1] * other), Vec4(columns[3] * other), Vec4(columns[2] * other));
	}

	constexpr Mat4 operator/(float other) const
	{
		return *this * (1.0f / other);
	}

	Mat4 operator*(const Mat4& other) const
	{
		Mat4 result;
		result[0] = *this * other[0];
		result[1] = *this * other[1];
		result[3] = *this * other[3];
		result[2] = *this * other[2];

		return result;
	}

	Mat4 operator*=(const Mat4& other)
	{
		return *this = *this * other;
	}

	Vec4 operator*(const Vec4& other) const;

	Vec4 operator[](size_t idx) const
	{
		ASSERT((idx < ArraySize(columns), "Matrix index out of bounds"));
		return columns[idx];
	}

	Vec4& operator[](size_t idx)
	{
		ASSERT((idx < ArraySize(columns), "Matrix index out of bounds"));
		return columns[idx];
	}

	static Mat4 Translate(float x, float y, float z)
	{
		// 1 0 0 x
		// 0 1 0 y
		// 0 0 1 z
		// 0 0 0 1
		return Mat4(
			Vec4(1.0f, 0.0f, 0.0f, 0.0f), Vec4(0.0f, 1.0f, 0.0f, 0.0f), Vec4(0.0f, 0.0f, 1.0f, 0.0f), Vec4(x, y, z, 1.0f));
	}

	static Mat4 Translate(const Vec3& v)
	{
		return Translate(v.x, v.y, v.z);
	}

	static Mat4 Rotate(const Quat& q)
	{
		float x2 = q.x * q.x;
		float y2 = q.y * q.y;
		float z2 = q.z * q.z;
		float w2 = q.w * q.w;
		return Mat4(
			Vec4(w2 + x2 - y2 - z2, 2 * q.x * q.y + 2 * q.w * q.z, 2 * q.x * q.z - 2 * q.w * q.y, 0.0f),
			Vec4(2 * q.x * q.y - 2 * q.w * q.z, w2 - x2 + y2 - z2, 2 * q.y * q.z + 2 * q.w * q.x, 0.0f),
			Vec4(2 * q.x * q.z + 2 * q.w * q.y, 2 * q.y * q.z - 2 * q.w * q.x, w2 - x2 - y2 - z2, 0.0f),
			Vec4(0.0f, 0.0f, 0.0f, 1.0f));
	}

	static Mat4 Scale(float x, float y, float z)
	{
		// x 0 0 0
		// 0 y 0 0
		// 0 0 z 0
		// 0 0 0 1
		return Mat4(
			Vec4(x, 0.0f, 0.0f, 0.0f), Vec4(0.0f, y, 0.0f, 0.0f), Vec4(0.0f, 0.0f, z, 0.0f), Vec4(0.0f, 0.0f, 0.0f, 1.0f));
	}

	static Mat4 Scale(const Vec3& v)
	{
		return Scale(v.x, v.y, v.z);
	}

	static Mat4 Scale(float v)
	{
		return Scale(v, v, v);
	}

	static Mat4 LookAt(const Vec3& camera, const Vec3& target, const Vec3& up = Vec3::UP)
	{
		// https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/gluLookAt.xml
		Vec3 f = (camera - target).Normalize();
		Vec3 s = f.Cross(up).Normalize();
		Vec3 u = s.Cross(f);

		return Mat4(
			Vec4(s.x, u.x, f.x, 0.0f),
			Vec4(s.y, u.y, f.y, 0.0f),
			Vec4(s.z, u.z, f.z, 0.0f),
			Vec4(-s.Dot(camera), -u.Dot(camera), -f.Dot(camera), 1.0f));
	}

	static Mat4 Look(const Transform& transform);

	static Mat4 Perspective(float fov, float aspect, float zNear, float zFar)
	{
		float f = 1.0f / tanf(fov * 0.5f);
		float range = 1.0f / (zNear - zFar);

		// https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml
		return Mat4(
			Vec4(f / aspect, 0.0f, 0.0f, 0.0f),
			Vec4(0.0f, f, 0.0f, 0.0f),
			Vec4(0.0f, 0.0f, (zFar + zNear) * range, -1.0f),
			Vec4(0.0f, 0.0f, 2 * zNear * zFar * range, 0.0f));
	}
};

// this unfortunately has to be down here, so it has Mat4::operator[] available
inline Vec4 Mat4::operator*(const Vec4& v) const
{
	const auto& m = *this;
	return Vec4(
		m[0].x * v.x + m[1].x * v.y + m[2].x * v.z + m[3].x * v.w,
		m[0].y * v.x + m[1].y * v.y + m[2].y * v.z + m[3].y * v.w,
		m[0].z * v.x + m[1].z * v.y + m[2].z * v.z + m[3].z * v.w,
		m[0].w * v.x + m[1].w * v.y + m[2].w * v.z + m[3].w * v.w);
}

struct Transform
{
	Vec3 position;
	Quat rotation;
	Vec3 scale;

	Transform(const Vec3& position = Vec3(), const Quat& rotation = Quat(), const Vec3& scale = Vec3(1.0f))
		: position(position), rotation(rotation), scale(scale)
	{
	}

	Mat4 GetMatrix() const
	{
		return Mat4::Translate(position) * Mat4::Rotate(rotation) * Mat4::Scale(scale);
	}

	Transform Lerp(const Transform& other, float t) const
	{
		return Transform(position.Lerp(other.position, t), rotation.Slerp(other.rotation, t), other.scale);
	}
};

inline Mat4 Mat4::Look(const Transform& transform)
{
	return Mat4::Rotate(transform.rotation.Inverse()) * Mat4::Translate(-transform.position);
}

static FORCEINLINE Vec4 HsvToRgb(const Vec4& hsv)
{
	float c = hsv.v * hsv.s;
	float x = c * (1 - abs(fmodf(hsv.h / (PI / 3), 2) - 1));
	float m = hsv.v - c;
	float h = fmodf(hsv.h, 2 * PI);
	Vec4 rgb;
	if (hsv.h >= 0.0f && hsv.h < PI / 3)
	{
		rgb = Vec4(c, x, 0, hsv.a);
	}
	else if (hsv.h >= PI / 3 && hsv.h < 2 * PI / 3)
	{
		rgb = Vec4(x, c, 0, hsv.a);
	}
	else if (hsv.h >= 2 * PI / 3 && hsv.h < PI)
	{
		rgb = Vec4(0, c, x, hsv.a);
	}
	else if (hsv.h >= PI && hsv.h < 4 * PI / 3)
	{
		rgb = Vec4(0, x, c, hsv.a);
	}
	else if (hsv.h >= 4 * PI / 3 && hsv.h < 10 * PI / 6)
	{
		rgb = Vec4(x, 0, c, hsv.a);
	}
	else if (hsv.h >= 10 * PI / 6 && hsv.h < 2 * PI)
	{
		rgb = Vec4(c, 0, x, hsv.a);
	}
	rgb += Vec4(Vec3(m), 0.0);
	return rgb;
}

static FORCEINLINE Vec4 RgbToHsv(const Vec4& rgb)
{
	float cMin = std::min(rgb.r, std::min(rgb.g, rgb.b));
	float cMax = std::max(rgb.r, std::max(rgb.g, rgb.b));
	float d = cMax - cMin;

	Vec4 hsv(0.0, 0.0, cMax, rgb.a);
	if (FloatEqual(d, 0.0f))
	{
		hsv.h = 0.0f;
	}
	else if (FloatEqual(cMax, rgb.r))
	{
		hsv.h = (PI / 3) * fmodf((rgb.g - rgb.b) / d, 6.0f);
	}
	else if (FloatEqual(cMax, rgb.g))
	{
		hsv.h = (PI / 3) * ((rgb.b - rgb.r) / d + 2.0f);
	}
	else if (FloatEqual(cMax, rgb.b))
	{
		hsv.h = (PI / 3) * ((rgb.r - rgb.g) / d + 4.0f);
	}

	if (FloatEqual(cMax, 0.0f))
	{
		hsv.s = 0.0f;
	}
	else
	{
		hsv.s = d / cMax;
	}

	return hsv;
}
