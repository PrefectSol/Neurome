#ifndef __REPLAY_PARSER_H
#define __REPLAY_PARSER_H

#include <iostream>
#include <fstream>
#include <bitset>
#include <vector>
#include <string>

#include <windows.h>
#include <LzmaDec.h>


class ReplayParser {
private:
	
	struct PlayerActivity {
		long LastAction = NULL;
		float X = NULL; //0-512
		float Y = NULL; //0-384
		std::bitset<32> PressedButtons;
	};

	struct HealthBar {
		uint32_t Time = NULL;
		float Health = NULL;
	};

	static void* myalloc(ISzAllocPtr p, size_t size) { return malloc(size); }
	static void myfree(ISzAllocPtr p, void* address) { free(address); }
	ISzAlloc SzAllocDefault = { myalloc, myfree };
	
	std::string filePath;

	BYTE* DataPtr;

	void Read(const std::vector<BYTE>& Replay, char& Storage, uint32_t HowMany);
	
	void WhatModes();

	void GetReplayInfo(const std::vector<BYTE>& Replay);

	void GetReplayTime(const std::vector<BYTE>& Replay);

	bool FileToVector(std::vector<BYTE>& Replay);

	std::string ReadSTR(const std::vector<BYTE>& Replay);

	std::vector<BYTE> ReadCodedPart(const std::vector<BYTE>& Replay);

	std::vector<HealthBar> ParseHealthBar(std::string HP);

	std::vector<PlayerActivity> ParseDecodedData(std::vector<BYTE> Data);

public:

	ReplayParser(std::string Path);

	std::bitset<32> ModesUsed;
	std::string MapHash;
	std::string ReplayHash;
	std::string UserName;
	std::string Mode;
	SYSTEMTIME sDate;
	int32_t OSU_Version;
	int32_t TotalScore;
	int16_t HitsCount300;
	int16_t HitsCount100;
	int16_t HitsCount50;
	int16_t HitsCountGekis;
	int16_t HitsCountKatus;
	int16_t MissCount;
	int16_t HighestCombo;
	long OnlineID;
	bool IdealPass;
	
	std::vector<HealthBar> HP;

	~ReplayParser();

	void PrintReplayInfo();

	std::vector<PlayerActivity> Activity;

	void ParseOSR();
};

#endif // !__REPLAY_PARSER_H