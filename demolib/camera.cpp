#include "pch.h"
#include "camera.h"
#include "demolib.h"

CCamera::CCamera(const Transform& transform, float fov /*= Deg2Rad(78.0)*/, float nearZ /*= 0.1*/, float farZ /*= 1500.0*/)
	: m_transform(transform), m_fov(fov), m_nearZ(nearZ), m_farZ(farZ)
{
	Rotate(0.0f, 0.0f);
	UpdateMatrices();
}

void CCamera::UpdateMatrices()
{
	Mat4 proj = Mat4::Perspective(m_fov, g_aspect, m_nearZ, m_farZ);
	Mat4 view = Mat4::Look(m_transform);
	m_viewProj = proj * view;
}

Vec3 CCamera::GetMovement(float speed /*= 1.0f*/, float x /*= 0.0f*/, float y /*= 0.0f*/, float z /*= 0.0f*/)
{
	Quat qYaw = Quat(Vec3::UP, m_yaw);
	Vec3 forward = qYaw.ActiveRotate(Vec3::FORWARD);
	Vec3 right = qYaw.ActiveRotate(Vec3::RIGHT);
	Vec3 up = Vec3::UP;
	Vec3 movement = (right * x + up * y + forward * z).Normalize() * speed;
	return movement;
}

void CCamera::SetPosition(const Vec3& pos)
{
	m_transform.position = pos;
}

void CCamera::Rotate(float yaw /*= 0.0f*/, float pitch /*= 0.0f*/)
{
	m_pitch = std::clamp(m_pitch + pitch, -m_fov, m_fov); // clamp pitch to 180 degree range
	m_yaw = fmodf(m_yaw + yaw, 2 * PI); // prevent yaw from getting too big to keep float error small
	m_transform.rotation = (Quat(Vec3::RIGHT, -m_pitch) * Quat(Vec3::UP, m_yaw)).Normalize();
	m_forward = (m_transform.rotation * Vec3::FORWARD).Normalize();
	m_up = (m_transform.rotation * Vec3::UP).Normalize();
}
