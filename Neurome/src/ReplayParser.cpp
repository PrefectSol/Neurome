#include "ReplayParser.h"


ReplayParser::ReplayParser(std::string Path){

	filePath = Path;
}

ReplayParser::~ReplayParser(){}

void ReplayParser::Read(const std::vector<BYTE>& Replay,char& Storage, uint32_t HowMany) {

	std::memcpy(&Storage, DataPtr, HowMany);
	DataPtr += HowMany;
}

bool ReplayParser::FileToVector(std::vector<BYTE>& Replay) {

	std::ifstream file(filePath, std::ios::binary | std::ios::ate);

	if (!file)
	{
		std::cerr << "WARN: can't open file." << std::endl;
		return false;
	}

	try
	{
		uint64_t size = file.tellg();
		file.seekg(0, std::ios::beg);

		Replay.resize(size);
		file.read(reinterpret_cast<char*>(Replay.data()), size);
	}
	catch (const std::bad_alloc& FileToVectorErr) 
	{
		std::cerr << "WARN: can't alloc memory for file. " << FileToVectorErr.what() << std::endl;
		file.close();
		return false;
	}

	file.close();
	return true;
}

void ReplayParser::WhatModes(){

	int32_t NoMode = ModesUsed.to_ulong();

	if (!NoMode)
	{
		std::cout << "NoMode" << std::endl;
		return;
	}

	std::string Modes[] { "NoFail", "Easy", "TouchDevice", "Hidden", "HardRock", "SuddenDeath",
		"DoubleTime", "Relax", "HalfTime", "Nightcore", "Flashlight", "Autoplay", "SpinOut", "Relax2",
		"Perfect", "Key4", "Key5", "Key6", "Key7", "Key8", "Fadein", "Random", "LastMod",
		"TargetPractice", "Key9", "Coop", "Key1", "Key3", "Key2", "ScoreV2", "Mirror" };

	for (uint8_t i = 0; i < 32; ++i) {
		if (ModesUsed[i])
			std::cout << Modes[i] << " ";
	}
	std::cout << std::endl;
}

void ReplayParser::GetReplayInfo(const std::vector<BYTE>& Replay) {

	BYTE byte = 0x00;
	Read(Replay, (char&)byte, 1);
	std::bitset<8> bits(byte);

	std::string Modes[] { "osu!" , "osu!taiko" , "osu!catch" , "osu!mania" };
	Mode = Modes[byte];

	Read(Replay, (char&)OSU_Version, 4);
	
	MapHash = ReadSTR(Replay);

	UserName = ReadSTR(Replay);

	ReplayHash = ReadSTR(Replay);

	Read(Replay, (char&)HitsCount300, 2);
	Read(Replay, (char&)HitsCount100, 2);
	Read(Replay, (char&)HitsCount50, 2);
	Read(Replay, (char&)HitsCountGekis, 2);
	Read(Replay, (char&)HitsCountKatus, 2);
	Read(Replay, (char&)MissCount, 2);
	Read(Replay, (char&)TotalScore, 4);
	Read(Replay, (char&)HighestCombo, 2);
	Read(Replay, (char&)IdealPass, 1);
	Read(Replay, (char&)ModesUsed, 4);
}

void ReplayParser::GetReplayTime(const std::vector<BYTE>& Replay) {

	ULARGE_INTEGER Time;
	FILETIME fDate;

	Read(Replay, (char&)Time.QuadPart, 8);

	fDate.dwLowDateTime = Time.LowPart;
	fDate.dwHighDateTime = Time.HighPart;
	FileTimeToSystemTime(&fDate, &sDate);
}

void ReplayParser::PrintReplayInfo() {
	
	std::cout << "Time: " << sDate.wHour << ":" << sDate.wMinute << ":" << sDate.wSecond << std::endl;
	std::cout << "Date: " << sDate.wYear - 1600 << "." << sDate.wMonth << "." << sDate.wDay << std::endl;
	std::cout << "Map hash: " << MapHash << std::endl;
	std::cout << "Replay hash: " << ReplayHash << std::endl;
	std::cout << "Username: " << UserName << std::endl;
	std::cout << "Version: " << OSU_Version << std::endl;
	std::cout << "Mode: " << Mode << std::endl;
	std::cout << "Modes used: ";
	WhatModes();
	std::cout << "Total score: " << TotalScore << std::endl;
	std::cout << "300: " << HitsCount300 << std::endl;
	std::cout << "100: " << HitsCount100 << std::endl;
	std::cout << "50: " << HitsCount50 << std::endl;
	std::cout << "Gekis: " << HitsCountGekis << std::endl;
	std::cout << "Katus: " << HitsCountKatus << std::endl;
	std::cout << "Misses: " << MissCount << std::endl;
	std::cout << "Highest combo: " << HighestCombo << std::endl;
	std::cout << "Ideal pass: " << IdealPass << std::endl;
	std::cout << "OnlineID: " << OnlineID << std::endl;
}

std::string ReplayParser::ReadSTR(const std::vector<BYTE>& Replay) {

	std::string Result;
	BYTE byte = 0x00;

	Read(Replay, (char&)byte, 1);

	if (byte == 0x00)
	{ 
		return "String is empty.";
	}

	uint64_t UTF8_len = 0;
	int shift = 0;

	do {
		Read(Replay, (char&)byte, 1);
		std::bitset<8> bits(byte);
		UTF8_len |= static_cast<uint64_t>(byte & 0x7F) << shift;
		shift += 7;
	} while (byte & 0x80);

	while (UTF8_len)
	{
		char sign;
		Read(Replay, (char&)sign, 1);
		--UTF8_len;
		Result.push_back(sign);
	}

	return Result;
}

std::vector<BYTE> ReplayParser::ReadCodedPart(const std::vector<BYTE>& Replay) {

	int32_t SizeOfCodedPart;
	Read(Replay, (char&)SizeOfCodedPart, 4);
	SizeT SizeTOfCodedPart = static_cast<SizeT>(SizeOfCodedPart);

	std::vector<BYTE> CodedData;
	std::vector<BYTE> DecodedData;
	Byte lzmaProps[LZMA_PROPS_SIZE];
	ELzmaStatus Status;
	int64_t LZMA_Header = 0;

	Read(Replay, (char&)lzmaProps, LZMA_PROPS_SIZE);
	Read(Replay, (char&)LZMA_Header, 8);

	CodedData.resize(SizeOfCodedPart);
	DecodedData.resize(LZMA_Header);

	ISzAlloc LZMA_Alloc = SzAllocDefault;
	SizeT OutputSize =  static_cast<SizeT>(LZMA_Header);

	std::memcpy(CodedData.data(), DataPtr, SizeOfCodedPart);
	DataPtr += SizeOfCodedPart;

	if (CodedData.data() == 0)
	{
		std::cerr << "WARN: can't read compressed data." << std::endl;
		return CodedData;
	}
	
	SRes res = LzmaDecode (DecodedData.data(),
						   &OutputSize,
						   CodedData.data(),
					  	   &SizeTOfCodedPart,
						   lzmaProps,
						   LZMA_PROPS_SIZE,
						   LZMA_FINISH_ANY,
						   &Status,
						   &LZMA_Alloc);  

	if (res)
	{
		std::cerr << "WARN: can't decode data. Err code: " << res << std::endl;
		return CodedData;
	}

	return DecodedData;
}

std::vector<ReplayParser::HealthBar> ReplayParser::ParseHealthBar(std::string HP)  {
	
	std::vector<HealthBar> Result;

	for (uint32_t i = 0, j = 0; i < HP.size(); ++j)
	{
		HealthBar CurHealthBar;
		std::string Buffer;
		while (HP[i] != 124)
		{
			Buffer += HP[i];
			++i;
		}

		CurHealthBar.Time = stoi(Buffer);
		Buffer.clear();
		++i;

		while (HP[i] != 44)
		{
			Buffer += HP[i];
			++i;
		}

		CurHealthBar.Health = stof(Buffer);
		Result.push_back(CurHealthBar);
		++i;
	}

	return Result;
}

std::vector<ReplayParser::PlayerActivity> ReplayParser::ParseDecodedData(std::vector<BYTE> Data)  {
	
	std::vector<PlayerActivity> Result;

	for (uint64_t i = 0, j = 0; i < Data.size(); ++j)
	{
		PlayerActivity Action;
		std::string Buffer;
		std::bitset<32> Bits;

		while (Data[i] != 124)
		{
			Buffer += Data[i];
			++i;
		}

		Action.LastAction = stol(Buffer);
		Buffer.clear();
		++i;

		while (Data[i] != 124)
		{
			Buffer += Data[i];
			++i;
		}

		Action.X = stof(Buffer);
		Buffer.clear();
		++i;

		while (Data[i] != 124)
		{
			Buffer += Data[i];
			++i;
		}

		Action.Y = stof(Buffer);
		Buffer.clear();
		++i;

		while (Data[i] != 44)
		{
			Buffer += Data[i];
			++i;
		}

		Action.PressedButtons = std::bitset<32>(stoi(Buffer));
		Result.push_back(Action);
		++i;
	}

	return Result;
}

void ReplayParser::ParseOSR() {

	std::vector<BYTE> Replay;

	if (!FileToVector(Replay))
	{
		exit(-727);
	}

	DataPtr = Replay.data();

	GetReplayInfo(Replay);

	HP = ParseHealthBar(ReadSTR(Replay));

	GetReplayTime(Replay);

	std::vector<BYTE> DecodedResult = ReadCodedPart(Replay);
	Activity = ParseDecodedData(DecodedResult);

	Read(Replay, (char&)OnlineID, 8);
}