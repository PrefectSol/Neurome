#include "MemoryReader.h"

MemoryReader::MemoryReader() 
	: m_reader(), m_stream(),
	m_isStream(true), m_isPlay(false)
{
	m_stream = std::thread(&MemoryReader::processStream, this);
}

MemoryReader::~MemoryReader()
{
	m_isStream.store(false, std::memory_order_relaxed);
	if (m_stream.joinable()) 
	{
		m_stream.join();
	}
}

bool MemoryReader::isAccessible()
{
	return m_reader.get_status().has_value();
}

bool MemoryReader::isPlay() const
{
	return m_isPlay.load(std::memory_order_relaxed);
}

std::array<int32_t, 6> MemoryReader::getHits()
{
	return { getHitPerfect(), getHit300(), getHit200(), getHit100(), getHit50(), getHitMiss()};
}

int32_t MemoryReader::getHitMiss()
{
	const auto hit = m_reader.get_hit_miss();
	if (hit)
	{
		return hit.value();
	}

	return -1;
}

int32_t MemoryReader::getHit50()
{
	const auto hit = m_reader.get_hit_50();
	if (hit)
	{
		return hit.value();
	}

	return -1;
}

int32_t MemoryReader::getHit100()
{
	const auto hit = m_reader.get_hit_100();
	if (hit)
	{
		return hit.value();
	}

	return -1;
}

int32_t MemoryReader::getHit200()
{
	const auto hit = m_reader.get_hit_200();
	if (hit)
	{
		return hit.value();
	}

	return -1;
}

int32_t MemoryReader::getHit300()
{
	const auto hit = m_reader.get_hit_300();
	if (hit)
	{
		return hit.value();
	}

	return -1;
}

int32_t MemoryReader::getHitPerfect()
{
	const auto hit = m_reader.get_hit_perfect();
	if (hit)
	{
		return hit.value();
	}

	return -1;
}

void MemoryReader::processStream()
{
	std::array<int32_t, 6> lastHits;

	while (m_isStream.load())
	{
		const std::array<int32_t, 6> hits = getHits();

		if (detectGameStart(hits))
		{
			lastHits = { 0, 0, 0, 0, 0, 0 };
			m_isPlay.store(true, std::memory_order_relaxed);
		}

		if (detectGameEnd(hits, lastHits))
		{
			m_isPlay.store(false, std::memory_order_relaxed);
		}

		lastHits = hits;
	}
}

bool MemoryReader::detectGameStart(const std::array<int32_t, 6> &hits) const
{
	return (hits[0] | hits[1] | hits[2] | hits[3] | hits[4] | hits[5]) == 0;
}

bool MemoryReader::detectGameEnd(const std::array<int32_t, 6> &hits,
							     const std::array<int32_t, 6> &lastHits) const
{
	return	hits[0] < 0 || hits[1] < 0 || hits[2] < 0 ||
			hits[3] < 0 || hits[4] < 0 || hits[5] < 0 ||
			hits[0] - lastHits[0] > 1 || hits[1] - lastHits[1] > 1 ||
			hits[2] - lastHits[2] > 1 || hits[3] - lastHits[3] > 1 ||
			hits[4] - lastHits[4] > 1 || hits[5] - lastHits[5] > 1;
}