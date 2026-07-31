#pragma once

#include <algorithm>
#include <cassert>
#include <vector>

template <typename T> class Vector2D
{
  public:
	Vector2D() = default;

	Vector2D(int width, int height)
	{
		Resize(width, height);
	}

	void Resize(int width, int height)
	{
		assert(width >= 0);
		assert(height >= 0);

		m_width = width;
		m_height = height;
		m_data.resize(static_cast<size_t>(width) * height);
	}

	void Clear()
	{
		m_data.clear();
		m_width = 0;
		m_height = 0;
	}

	void Fill(const T& value)
	{
		std::fill(m_data.begin(), m_data.end(), value);
	}

	int Width() const
	{
		return m_width;
	}

	int Height() const
	{
		return m_height;
	}

	size_t Size() const
	{
		return m_data.size();
	}

	bool InBounds(int x, int y) const
	{
		return x >= 0 && x < m_width && y >= 0 && y < m_height;
	}

	T& operator()(int x, int y)
	{
		ASSERT(InBounds(x, y));
		return m_data[static_cast<size_t>(y) * m_width + x];
	}

	const T& operator()(int x, int y) const
	{
		ASSERT(InBounds(x, y));
		return m_data[static_cast<size_t>(y) * m_width + x];
	}

	T& AtClamped(int x, int y)
	{
		x = std::clamp(x, 0, m_width - 1);
		y = std::clamp(y, 0, m_height - 1);
		return m_data[static_cast<size_t>(y) * m_width + x];
	}

	const T& AtClamped(int x, int y) const
	{
		x = std::clamp(x, 0, m_width - 1);
		y = std::clamp(y, 0, m_height - 1);
		return m_data[static_cast<size_t>(y) * m_width + x];
	}

	T* Data()
	{
		return m_data.data();
	}

	const T* Data() const
	{
		return m_data.data();
	}

  private:
	int m_width = 0;
	int m_height = 0;
	std::vector<T> m_data;
};