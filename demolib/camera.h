#pragma once

#include "mathfun.h"

class CCamera
{
  public:
	CCamera(const Transform& transform, float fov = Deg2Rad(78.0), float nearZ = 0.1, float farZ = 1500.0);

	// call this if you move or rotate the camera
	void UpdateMatrices();

	const Mat4& GetMatrix() const
	{
		return m_viewProj;
	}

	const Vec3& GetForward() const
	{
		return m_forward;
	}

	const Transform& GetTransform() const
	{
		return m_transform;
	}

	float GetYaw() const
	{
		return m_yaw;
	}

	float GetPitch() const
	{
		return m_pitch;
	}

	// remember to call UpdateMatrices for these

	// get the correct movement vector for the given xz movement
	Vec3 GetMovement(float speed = 1.0f, float x = 0.0f, float y = 0.0f, float z = 0.0f);

	// set the camera's position
	void SetPosition(const Vec3& pos);

	// rotate the camera by the specified angles
	void Rotate(float yaw = 0.0f, float pitch = 0.0f);

  private:
	Transform m_transform;

	// these have to be tracked explicitly so they can be clamped
	float m_pitch = 0.0f;
	float m_yaw = 0.0f;

	float m_fov;
	float m_nearZ;
	float m_farZ;

	Mat4 m_viewProj;

	Vec3 m_forward;
	Vec3 m_up;
};
