#define __MAP_PARSER_H
#ifdef __MAP_PARSER_H

#include <vector>
#include <string>
#include <fstream>
#include <codecvt>
#include <iostream>

#include <windows.h>

class MapParser {
private:

	struct ControlPoint {
		uint16_t X;
		uint16_t Y;
	};

	struct Colour {
		uint8_t R;
		uint8_t G;
		uint8_t B;
	};

	struct GeneralInfo {
		std::string	AudioFilename;
		int32_t AudioLeadIn;
		int32_t PreviewTime;
		uint8_t Countdown;
		std::string SampleSet;
		float StackLeniency;
		uint8_t Mode;
		bool LetterboxInBreaks;
		std::string SkinPreference = "";
		bool EpilepsyWarning = false;
		bool WidescreenStoryboard = false;
	};

	struct EditorInfo {
		std::vector<uint32_t> Bookmarks;
		uint8_t DistanceSpacing;
		uint8_t BeatDivisor;
		uint8_t GridSize;
		uint8_t TimelineZoom;
	};

	struct Metadata {
		std::string Title;
		std::wstring TitleUnicode = L"";
		std::string Artist;
		std::wstring ArtistUnicode = L"";
		std::string Creator;
		std::string Version;
		std::string Source;
		std::wstring Tags;
		uint32_t BeatmapID = NULL;
		uint32_t BeatmapSetID = NULL;
	};
	
	struct Difficulty {
		float HPDrainRate;
		float CircleSize;
		float OverallDifficulty;
		float ApproachRate = NULL;
		float SliderMultiplier;
		float SliderTickRate;
	};

	struct TimingPoint {
		int32_t StartTiming;
		double Changer;
		uint8_t TimingType;
		uint32_t Meter;
		uint16_t SampleSet;
		uint16_t Volume;
		bool Kiai;
		bool Uninherited;
	};

	struct Note {
		uint16_t X;
		uint16_t Y;
		uint32_t AppearanceTime;
		uint8_t NoteType; // 1 - common, 2 - slider, 3 - spinner
		std::string NoteHitSound;
		uint32_t EndOfSpinner = NULL;
		char TypeOfCurve = NULL; // L - straight, P - parabola, B - Beze, C - circle
		std::vector<ControlPoint> SliderCoords;
		uint32_t SliderRepeats = NULL;
		uint32_t SliderLen = NULL;
		std::string SliderHitsouds;
	};

	uint8_t Version;
	std::string filePath;

	BYTE* DataPtr;
	BYTE* EndPtr;

	bool FileToVector(std::vector<BYTE>& Map);

	bool Read(const std::vector<BYTE>& Map, char& Storage, uint32_t HowMany);

	void ToHeader(const std::vector<BYTE>& Map, std::string HeaderName);

	Note GetNote(const std::vector<BYTE>& Map);

	uint16_t GetMapVersion(const std::vector<BYTE>& Map);

	Metadata GetMetadata(const std::vector<BYTE>& Map);

	EditorInfo GetEditorInfo(const std::vector<BYTE>& Map);

	Difficulty GetDifficultyInfo(const std::vector<BYTE>& Map);

	GeneralInfo GetGeneralInfo(const std::vector<BYTE>& Map);

	std::string ReadWord(const std::vector<BYTE> Map, BYTE& byte);

	std::string GetContent(const std::vector<BYTE>& Map);

	std::vector<Colour> GetColours(const std::vector<BYTE>& Map);

	std::vector<TimingPoint> GetTimingPoints(const std::vector<BYTE>& Map);

public:
	Metadata MapMetadata;
	Difficulty MapDif;
	EditorInfo MapEditor;
	GeneralInfo MapGeneral;
	
	std::vector<Note> Notes;
	std::vector<Colour> MapColours;
	std::vector<TimingPoint> MapTimingPoints;
	
	MapParser(std::string Path);

	~MapParser();

	void ParseMap();
};

#endif // !__MAP_PARSER_H