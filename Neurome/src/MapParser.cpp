#include "MapParser.h"


MapParser::MapParser(std::string Path) {
	
	filePath = Path;
}

MapParser::~MapParser() {}

bool MapParser::Read(const std::vector<BYTE>& Map, char& Storage, uint32_t HowMany) {

	std::memcpy(&Storage, DataPtr, HowMany);
	DataPtr += HowMany;
	return true;
}

bool MapParser::FileToVector(std::vector<BYTE>& Map) {

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

		Map.resize(size);
		file.read(reinterpret_cast<char*>(Map.data()), size);
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

void MapParser::ToHeader(const std::vector<BYTE>& Map, std::string HeaderName) {

	BYTE byte = 0x00;
	std::string Header;
	DataPtr -= 2;

	while(HeaderName != Header)
	{
		Header.clear();

		while (byte != 0x5b)
		{
			Read(Map, (char&)byte , 1);
		}
		Read(Map, (char&)byte, 1);

		while (byte != 0x5d)
		{
			Header += byte;
			Read(Map, (char&)byte, 1);
		}
	}
}

std::string MapParser::ReadWord(const std::vector<BYTE> Map, BYTE& byte) {

	std::string Buffer;

	while (byte < 0x0f || byte == 0x2c)
	{
		Read(Map, (char&)byte, 1);
	}

	while (byte != 0x2c && byte > 0x0f)
	{
		Buffer += byte;
		Read(Map, (char&)byte, 1);
	}

	return Buffer;
}

std::string MapParser::GetContent(const std::vector<BYTE>& Map) {
	
	std::string Result;
	BYTE byte = 0x00;

	do
	{
		Read(Map, (char&)byte, 1);
	} while (byte != 0x3a);

	while (byte > 0x0f)
	{
		Read(Map, (char&)byte, 1);
		Result += byte;
	}

	return Result;
}

std::vector<MapParser::Colour> MapParser::GetColours(const std::vector<BYTE>& Map) {
	
	ToHeader(Map, "Colours");

	std::vector<Colour> Result;
	BYTE byte = 0x00;
	Read(Map, (char&)byte, 1);


	while (byte != 0x5b)
	{
		Colour Colour;

		while (byte != 0x3a)
		{
			Read(Map, (char&)byte, 1);
		}
		Read(Map, (char&)byte, 1);
		Read(Map, (char&)byte, 1);

		Colour.R = stoi(ReadWord(Map, byte));
		Colour.G = stoi(ReadWord(Map, byte));
		Colour.B = stoi(ReadWord(Map, byte));

		while (byte < 0x0f)
		{
			Read(Map, (char&)byte, 1);
		}

		Result.push_back(Colour);
	}

	return Result;
}

std::vector<MapParser::TimingPoint> MapParser::GetTimingPoints(const std::vector<BYTE>& Map) {
	
	ToHeader(Map, "TimingPoints");
	
	std::vector<TimingPoint> Result;
	BYTE byte = 0x00;
	Read(Map, (char&)byte, 1);


	while (byte != 0x5b)
	{
		TimingPoint Point;

		Point.StartTiming = stoi(ReadWord(Map, byte));
		Point.Changer = stof(ReadWord(Map, byte));
		Point.Meter = stoi(ReadWord(Map, byte));
		Point.TimingType = stoi(ReadWord(Map, byte));
		Point.SampleSet = stoi(ReadWord(Map, byte));
		Point.Volume = stoi(ReadWord(Map, byte));
		Point.Kiai = stoi(ReadWord(Map, byte));
		Point.Uninherited = stoi(ReadWord(Map, byte));

		while (byte < 0x0f)
		{
			Read(Map, (char&)byte, 1);
		}

		Result.push_back(Point);
	}

	return Result;
}

uint16_t MapParser::GetMapVersion(const std::vector<BYTE>& Map) {

	BYTE byte = 0x00;
	std::string Result;

	while (byte != 0x76)
	{
		Read(Map, (char&)byte, 1);
	}

	Read(Map, (char&)byte, 1);

	while (byte != 0x0d)
	{
		Result += byte;
		Read(Map, (char&)byte, 1);
	}

	return stoi(Result);
}

MapParser::Note MapParser::GetNote(const std::vector<BYTE>& Map) {

	Note Result;
	BYTE byte = 0x00;

	Result.X = stoi(ReadWord(Map, byte));

	Result.Y = stoi(ReadWord(Map, byte));

	Result.AppearanceTime = stoi(ReadWord(Map, byte));

	Result.NoteType = stoi(ReadWord(Map, byte));

	Result.NoteHitSound = ReadWord(Map, byte);

	if (Result.NoteType == 0x05 || Result.NoteType == 0x01)
	{
		while (byte > 0x0f)
		{
			Result.NoteHitSound += ReadWord(Map, byte);
		}

		return Result;
	}
	
	if (Result.NoteType == 0x0c)
	{
		Result.EndOfSpinner = stoi(ReadWord(Map, byte));

		while (byte > 0x0f)
		{
			Result.NoteHitSound += ReadWord(Map, byte);
		}

		return Result;
	}

	Read(Map, (char&)byte, 1);
	Result.TypeOfCurve = byte;
	std::string Buffer;

	while (byte != 0x2c)
	{
		ControlPoint Point;

			while(byte != 0x3a)
			{
				if (byte > 0x2f && byte < 0x3a)
				{
					Buffer += byte;
				}

				Read(Map, (char&)byte, 1);
			}

			Point.X = stoi(Buffer);
			Buffer.clear();
			Read(Map, (char&)byte, 1);
		
			while (byte != 0x7c && byte != 0x2c)
			{
				if (byte > 0x2f && byte < 0x3a)
				{
					Buffer += byte;
				}

				Read(Map, (char&)byte, 1);
			}

			Point.Y = stoi(Buffer);
			Buffer.clear();

		Result.SliderCoords.push_back(Point);
	}

	Result.SliderRepeats = stoi(ReadWord(Map, byte));
	Result.SliderLen = stoi(ReadWord(Map, byte));
	
	while (byte > 0x0f)
	{ 
		Result.SliderHitsouds += ReadWord(Map, byte);
	}

	return Result;
}

MapParser::Metadata MapParser::GetMetadata(const std::vector<BYTE>& Map) {
	
	ToHeader(Map, "Metadata");
	
	Metadata Meta;
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

	if (Version < 10)
	{
	Meta.Title = GetContent(Map);
	Meta.Artist = GetContent(Map);
	Meta.Creator = GetContent(Map);
	Meta.Version = GetContent(Map);
	Meta.Source = GetContent(Map);
	Meta.Tags = converter.from_bytes(GetContent(Map));
	
	return Meta;
	}

	Meta.Title = GetContent(Map);	
	Meta.TitleUnicode = converter.from_bytes(GetContent(Map));
	Meta.Artist = GetContent(Map);
	Meta.ArtistUnicode = converter.from_bytes(GetContent(Map));
	Meta.Creator = GetContent(Map);
	Meta.Version = GetContent(Map);
	Meta.Source = GetContent(Map);
	Meta.Tags = converter.from_bytes(GetContent(Map));
	Meta.BeatmapID = stoi(GetContent(Map));
	Meta.BeatmapSetID = stoi(GetContent(Map));

	return Meta;
}

MapParser::EditorInfo MapParser::GetEditorInfo(const std::vector<BYTE>& Map) {

	ToHeader(Map, "Editor");

	EditorInfo Editor;
	BYTE byte = 0x00;
	
	if (Version < 10)
	{
	Editor.DistanceSpacing = stoi(GetContent(Map));
	Editor.BeatDivisor = stoi(GetContent(Map));
	Editor.GridSize = stoi(GetContent(Map));

	return Editor;
	}

	while (byte > 0x0f)
	{
		Editor.Bookmarks.push_back(stoi(ReadWord(Map, byte)));
		Read(Map, (char&)byte, 1);
	}

	Editor.DistanceSpacing = stoi(GetContent(Map));
	Editor.BeatDivisor = stoi(GetContent(Map));
	Editor.GridSize = stoi(GetContent(Map));
	Editor.TimelineZoom = stoi(GetContent(Map));

	return Editor;
}

MapParser::Difficulty MapParser::GetDifficultyInfo(const std::vector<BYTE>& Map) {

	ToHeader(Map, "Difficulty");

	Difficulty Dif;

	Dif.HPDrainRate = stof(GetContent(Map));
	Dif.CircleSize = stof(GetContent(Map));
	Dif.OverallDifficulty = stof(GetContent(Map));

	if (Version > 10)
	{
		MapDif.ApproachRate = stof(GetContent(Map));
	}

	Dif.SliderMultiplier = stof(GetContent(Map));
	Dif.SliderTickRate = stof(GetContent(Map));

	return Dif;
}

MapParser::GeneralInfo MapParser::GetGeneralInfo(const std::vector<BYTE>& Map) {

	ToHeader(Map, "General");
	
	GeneralInfo General;

	General.AudioFilename = GetContent(Map);
	General.AudioLeadIn = stoi(GetContent(Map));
	General.PreviewTime = stoi(GetContent(Map));
	General.Countdown = stoi(GetContent(Map));
	General.SampleSet = GetContent(Map);
	General.StackLeniency = stof(GetContent(Map));
	General.Mode = stoi(GetContent(Map));
	General.LetterboxInBreaks = stoi(GetContent(Map));
	
	if (Version < 10)
	{
		General.SkinPreference = GetContent(Map);
		
		return General;
	}

	if (Version < 14)
	{
		General.EpilepsyWarning = stoi(GetContent(Map));
	}

	General.WidescreenStoryboard = stoi(GetContent(Map));

	return General;
}

void MapParser::ParseMap() {

	std::vector<BYTE> Map;
	BYTE byte;
	
	if(!FileToVector(Map))
	{
		exit(727);
	}

	DataPtr = Map.data();
	EndPtr = &Map.back();

	Version = GetMapVersion(Map);

	MapGeneral = GetGeneralInfo(Map);
	MapEditor = GetEditorInfo(Map);
	MapMetadata = GetMetadata(Map);
	MapDif = GetDifficultyInfo(Map);
	MapTimingPoints = GetTimingPoints(Map);

	if (Version < 14)
	{
		MapColours = GetColours(Map);
	}

	ToHeader(Map, "HitObjects");

	while(DataPtr + 2 < EndPtr)
	{
		Notes.push_back(GetNote(Map));
	}
}