#include "FileWriter.h"

// ----------------------------------------------------------------------------
class SwfWriter : public FileWriter
{
public:
	// ------------------------------------------------------------------------
	enum FlashTagCode
	{
		SwfTag_End					= 0,
		SwfTag_ShowFrame			= 1,
		SwfTag_DefineShape			= 2,
		SwfTag_DefineBits			= 6,
		SwfTag_SetBackgroundColor	= 9,
		SwfTag_DoAction				= 12,
		SwfTag_SoundStreamHead		= 18,
		SwfTag_SoundStreamBlock		= 19,
		SwfTag_DefineBitsLossless	= 20,
		SwfTag_DefineBitsJPEG2		= 21,
		SwfTag_PlaceObject2			= 26,
		SwfTag_RemoveObject2		= 28,
		SwfTag_DefineBitsJPEG3		= 35,
		SwfTag_DefineBitsLossless2	= 36,
		SwfTag_DefineSprite			= 39,
		SwfTag_DefineExportAssets	= 56
	};

	// ------------------------------------------------------------------------
	enum FlashActionCode
	{
		SwfAction_Stop				= 0x07
	};

	// ------------------------------------------------------------------------
	enum SamplingRate
	{
		SwfSampleRate_5_5KHz		= 0,
		SwfSampleRate_11KHz			= 1,
		SwfSampleRate_22KHz			= 2,
		SwfSampleRate_44KHz			= 3
	};

	// ------------------------------------------------------------------------
	enum SoundType
	{
		SwfSndMono					= 0,
		SwfSndStereo				= 1
	};

	// ------------------------------------------------------------------------
	enum SoundCompression
	{
		SwfADPCM					= 1,
		SwfMP3						= 2
	};

private:
	// ------------------------------------------------------------------------
	struct TagInfo
	{
		bool requireFixup;
		FlashTagCode tag;
		unsigned long begin;

		TagInfo() : requireFixup(false), tag(SwfTag_End), begin(0) {}
		TagInfo(bool _requireFixup, FlashTagCode _tag, unsigned long _begin) :
			requireFixup(_requireFixup),
			tag(_tag),
			begin(_begin)
		{
		}
		void clear()
		{ 
			requireFixup = false;
			begin = 0;
			tag = SwfTag_End;
		}
	};
	typedef std::vector<TagInfo> TagInfoList;

public:
	// ------------------------------------------------------------------------
	typedef unsigned short CharacterID;

	// ------------------------------------------------------------------------
	struct Color
	{
		unsigned char red;
		unsigned char green;
		unsigned char blue;

		Color() : red(0), green(0), blue(0) {}
		Color(unsigned char r, unsigned char g, unsigned char b) :
			red(r), green(g), blue(b) {}
	};

	// ------------------------------------------------------------------------
	struct Rect
	{
		int xmin;
		int xmax;
		int ymin;
		int ymax;

		Rect() : xmin(0), xmax(0), ymin(0), ymax(0) {}
		Rect(int _xmin, int _xmax, int _ymin, int _ymax) :
			xmin(_xmin),
			xmax(_xmax),
			ymin(_ymin),
			ymax(_ymax)
		{
		}
		unsigned int findGreatestAbsValue() const
		{
			using namespace std;

			int value = abs(xmin);
			value = max(value, abs(xmax));
			value = max(value, abs(ymin));
			value = max(value, abs(ymax));
			return static_cast<unsigned int>(value);
		}
	};

private:
	// ------------------------------------------------------------------------
	TagInfoList m_tagInfoList;
	bool m_compressSwf;
	CharacterID m_nextCharacterID;
	unsigned short m_frameRate;
	unsigned short m_frameCount;
	Rect m_frameRect;
	unsigned long m_sndStreamFixupPos;

protected:
	// ------------------------------------------------------------------------
	void writeRecordHeaderStart(FlashTagCode tag);
	void writeRecordHeaderStart(FlashTagCode tag, unsigned long size);
	void writeRecordHeaderEnd();
	void writeNextCharacterID();
	void writeColor(const Color& color);
	void writeRect(const Rect& rect);
	void writeVertHorzEdge(bool isVertical, int delta);
	void fixupHeader();

protected:
	// ------------------------------------------------------------------------
	virtual void writeBuffer();

public:
	// ------------------------------------------------------------------------
	SwfWriter();
	virtual ~SwfWriter();

	virtual void close();

	void setCompression(bool compress);
	void setFrameRate(unsigned int fps);
	void setFrameRect(int xmin, int xmax, int ymin, int ymax);
	inline const Rect& getFrameRect() const { return m_frameRect; }

	void outputHeader();
	void outputSetBackground(const Color& color);
	CharacterID outputDefineBitsJPEG2(const std::wstring& jpegfile);
	CharacterID outputDefineBitmapShape(CharacterID bitmapID, const Rect& bounds);
	void outputExportAssets(CharacterID id, const std::wstring& name);
	void outputShowFrame(bool onMainTimeline);
	void outputEnd();

	CharacterID outputDefineSpriteBegin(unsigned int frameCount);
	void outputDefineSpriteEnd();
	void outputPlaceObject2(CharacterID id, unsigned int depth);
	void outputPlaceObject2(CharacterID id, unsigned int depth, const std::wstring& name);
	void outputRemoveObject2(unsigned int depth);

	void outputSoundStreamBegin(SamplingRate playbackRate, 
								SoundType playbackType, 
								SoundCompression compressionType, 
								SamplingRate streamRate, 
								SoundType streamType);
	void ouputMP3StreamEnd(unsigned short sampleCount, short latencySeek);
	void outputMP3StreamBlock(unsigned short sampleCount, unsigned short seekSamples, const Buffer& data);

	void outputDoActionStop();
};