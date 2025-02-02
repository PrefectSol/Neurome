#ifndef __MEMORY_READER_H
#define __MEMORY_READER_H

#include <cstdint>
#include <thread>
#include <atomic>
#include <array>
#include <algorithm>

#include <osu_memory/osu_memory.h>

class MemoryReader
{
public:
	explicit MemoryReader();

	~MemoryReader(); 

	bool isAccessible();

	bool isPlay() const;

	std::array<int32_t, 6> getHits();

	int32_t getHitMiss();

	int32_t getHit50();

	int32_t getHit100();

	int32_t getHit200();

	int32_t getHit300();

	int32_t getHitPerfect();

private:
	osu_memory::reader m_reader;

	std::thread m_stream;

	std::atomic<bool> m_isStream;

	std::atomic<bool> m_isPlay;

	void processStream();

	bool detectGameStart(const std::array<int32_t, 6> &hits) const;

	bool detectGameEnd(const std::array<int32_t, 6> &hits, 
					   const std::array<int32_t, 6> &lastHits) const;
};

#endif // !__MEMORY_READER_H