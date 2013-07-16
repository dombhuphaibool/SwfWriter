#include <stdio.h>
#include "Compress.h"
#include "SwfWriter.h"

// ----------------------------------------------------------------------------
typedef std::vector<unsigned char> Buffer;

// ----------------------------------------------------------------------------
// Tag codes that the player requires to be large tag headtypes
inline bool IS_FLASH_LARGE_TAG_CODE(SwfWriter::FlashTagCode c) 
{
	return	(c == SwfWriter::SwfTag_DefineBits)				||
			(c == SwfWriter::SwfTag_DefineBitsJPEG2)		||
			(c == SwfWriter::SwfTag_DefineBitsJPEG3)		||
			(c == SwfWriter::SwfTag_DefineBitsLossless)		||
			(c == SwfWriter::SwfTag_DefineBitsLossless2)	||
			(c == SwfWriter::SwfTag_SoundStreamBlock);
}

// ----------------------------------------------------------------------------
// Count bits needed to represent an unsigned value.
// In practice we're detecting the position of the leftmost bit set to 1.
static unsigned int countRequiredBits(unsigned int value)
{
	unsigned int numBits = 0;
	while (value & ~0xF)
	{
		value >>= 4;
		numBits += 4;
	}
	while (value)
	{
		value >>= 1;
		++numBits;
	}
	return numBits;
}

// ----------------------------------------------------------------------------
SwfWriter::SwfWriter() : 
	m_compressSwf(true),
	m_nextCharacterID(0),
	m_frameRate(30),
	m_frameCount(0),
	m_sndStreamFixupPos(0)
{
}

// ----------------------------------------------------------------------------
SwfWriter::~SwfWriter()
{
}

// ----------------------------------------------------------------------------
void SwfWriter::close()
{
	fixupHeader();
	FileWriter::close();
}

// ----------------------------------------------------------------------------
void SwfWriter::setCompression(bool compress)
{
	m_compressSwf = compress;
}

// ----------------------------------------------------------------------------
void SwfWriter::writeBuffer()
{
	if (m_compressSwf)
	{
		FileWriter::Buffer compressedBuffer;

		unsigned int dataBufferSize = getFileSize() - 8;
		ZLIBCompressor compressor;
		compressor.compress(getBufferAtPos(8), dataBufferSize, compressedBuffer);

		unsigned int compressedBufferSize = compressedBuffer.size();
		if (compressedBufferSize < dataBufferSize)
		{
			resizeFile(compressedBufferSize + 8);
			setPosition(8);
			writeData(compressedBuffer);
			/*
			m_buffer.resize(compressedBufferSize + 8);
			std::copy(compressedBuffer.begin(), compressedBuffer.end(), m_buffer.begin()+8);
			*/
		}
		setPosition(0);
		writeByte('C');
		setPosition(getFileSize());
	}

	FileWriter::writeBuffer();
}

// ----------------------------------------------------------------------------
void SwfWriter::setFrameRect(int xmin, int xmax, int ymin, int ymax)
{
	m_frameRect.xmin = xmin;
	m_frameRect.xmax = xmax;
	m_frameRect.ymin = ymin;
	m_frameRect.ymax = ymax;
}

// ----------------------------------------------------------------------------
void SwfWriter::setFrameRate(unsigned int fps)
{
	m_frameRate = fps;
}

// ----------------------------------------------------------------------------
void SwfWriter::writeColor(const Color& color)
{
	writeByte(color.red);
	writeByte(color.green);
	writeByte(color.blue);
}

// ----------------------------------------------------------------------------
void SwfWriter::writeRect(const Rect& rect)
{
	unsigned int numBits = countRequiredBits(rect.findGreatestAbsValue()) + 1; // include the sign bit
	writeBits(numBits, 5);
	writeBits(rect.xmin, numBits);
	writeBits(rect.xmax, numBits);
	writeBits(rect.ymin, numBits);
	writeBits(rect.ymax, numBits);
	flushWriteBits();
}

// ----------------------------------------------------------------------------
void SwfWriter::writeRecordHeaderStart(FlashTagCode tag)
{
	TagInfo tagInfo(true, tag, getPosition());

	// Use long record header
	writeWord((tag << 6) | 0x3f);
	writeLong(0);	// Place holder

	m_tagInfoList.push_back(tagInfo);
}

// ----------------------------------------------------------------------------
void SwfWriter::writeRecordHeaderStart(FlashTagCode tag, unsigned long size)
{
	TagInfo tagInfo(false, tag, getPosition());

	if (size < 0x3f && !IS_FLASH_LARGE_TAG_CODE(tag))
	{
		// Use short record header
		unsigned short recordHeader = (tag << 6) | static_cast<unsigned short>(size);
		writeWord(recordHeader);
	}
	else
	{
		// Use long record header
		writeWord((tag << 6) | 0x3f);
		writeLong(size);
	}

	m_tagInfoList.push_back(tagInfo);
}

// ----------------------------------------------------------------------------
void SwfWriter::writeRecordHeaderEnd()
{
	TagInfo tagInfo = m_tagInfoList.back();
	m_tagInfoList.pop_back();

	if (tagInfo.requireFixup)
	{
		unsigned long dataBegin = tagInfo.begin + 6;
		unsigned long end = getPosition();
		unsigned long size = end - dataBegin;

		if (size < 0x3f && !IS_FLASH_LARGE_TAG_CODE(tagInfo.tag))
		{
			// Use short record header
			unsigned short recordHeader = (tagInfo.tag << 6) | static_cast<unsigned short>(size);
			setPosition(tagInfo.begin);
			writeWord(recordHeader);
			shiftContent(dataBegin, size, -4);
			setPosition(end - 4);
		}
		else
		{
			// Use long record header
			setPosition(tagInfo.begin + 2);
			writeLong(size);
			setPosition(end);
		}
	}

	// tagInfo.clear();
}

// ----------------------------------------------------------------------------
void SwfWriter::writeNextCharacterID()
{
	writeWord(++m_nextCharacterID);
}

// ----------------------------------------------------------------------------
void SwfWriter::outputHeader()
{
	writeByte('F');
	writeByte('W');
	writeByte('S');
	writeByte(6);	// SWF version 6+
	writeLong(0);	// Place holder
	writeRect(m_frameRect);
	writeWord(m_frameRate * 256);	// or shift left 8 (<<8) for 8.8 notation
	writeWord(m_frameCount);
}

// ----------------------------------------------------------------------------
void SwfWriter::fixupHeader()
{
	// Skip FWS and version
	setPosition(4);
	writeLong(getFileSize());
	writeRect(m_frameRect);
	writeWord(m_frameRate * 256);	// or shift left 8 (<<8) for 8.8 notation
	writeWord(m_frameCount);
}

// ----------------------------------------------------------------------------
void SwfWriter::outputEnd()
{
	writeRecordHeaderStart(SwfTag_End, 0);
	writeRecordHeaderEnd();
}

// ----------------------------------------------------------------------------
void SwfWriter::outputSetBackground(const Color& color)
{
	writeRecordHeaderStart(SwfTag_SetBackgroundColor, 3);
	writeColor(color);
	writeRecordHeaderEnd();
}

// ----------------------------------------------------------------------------
void SwfWriter::outputShowFrame(bool onMainTimeline)
{
	writeRecordHeaderStart(SwfTag_ShowFrame, 0);
	writeRecordHeaderEnd();
	if (onMainTimeline)
	{
		++m_frameCount;
	}
}

// ----------------------------------------------------------------------------
SwfWriter::CharacterID SwfWriter::outputDefineBitsJPEG2(const std::wstring& jpegfile)
{
	CharacterID characterID = 0;
	FILE* fp = _wfopen(jpegfile.c_str(), L"rb");
	if (fp)
	{
		long fileSize = 0;
		FileWriter::Buffer buffer;
		if (fseek(fp, 0, SEEK_END) == 0)
		{
			fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
		}

		if (fileSize > 0)
		{
			buffer.resize(fileSize);
			fread(&buffer[0], 1, fileSize, fp);

			writeRecordHeaderStart(SwfTag_DefineBitsJPEG2, fileSize + 2);
			writeNextCharacterID();
			writeData(buffer);
			writeRecordHeaderEnd();
			characterID = m_nextCharacterID;
		}

		fclose(fp);
	}

	return characterID;
}

// ----------------------------------------------------------------------------
void SwfWriter::writeVertHorzEdge(bool isVertical, int delta)
{
	unsigned int numBits = countRequiredBits(std::abs(delta)) + 1; // include sign bit
	writeBits(1, 1);	// edge record
	writeBits(1, 1);	// straight edge
	writeBits(numBits-2, 4);
	writeBits(0, 1);	// vert/horz line
	writeBits(isVertical ? 1 : 0, 1);	// vertical line
	writeBits(delta, numBits);
}

// ----------------------------------------------------------------------------
SwfWriter::CharacterID SwfWriter::outputDefineBitmapShape(CharacterID bitmapID, const Rect& bounds)
{
	writeRecordHeaderStart(SwfTag_DefineShape);
	writeNextCharacterID();
	writeRect(bounds);

	// Output the ShapeWithStyle record...
	// ===================================

	// Output FillStyle Array
	writeByte(1);		// 1 fill style
	writeByte(0x41);	// clipped bitmap fill
	writeWord(bitmapID);
	
	// Output scale matrix of 20/20
	initWriteBits();
	int scaleValue = 20 << 16;	// convert to fixed 16.16
	unsigned int numBits = countRequiredBits(scaleValue) + 1;	// include the sign bit
	writeBits(1, 1);	// has scale
	writeBits(numBits, 5);
	writeBits(scaleValue, numBits);
	writeBits(scaleValue, numBits);
	writeBits(0, 1);	// no rotate or skew
	writeBits(0, 5);	// no translate
	flushWriteBits();

	// Output LineStyle Array
	writeByte(0);		// no line style
	writeByte(1<<4);	// num of fill bits / line bits

	// Style Change Record
	writeBits(0, 1);	// non-edge record
	writeBits(0, 1);	// new style flag
	writeBits(0, 1);	// line style change
	writeBits(1, 1);	// fill style 1 change
	writeBits(0, 1);	// fill style 0 change
	writeBits(0, 1);	// moveto flag
	writeBits(1, 1);	// change to fill style #1

	// Horizontal Straight Edge Record
	writeVertHorzEdge(false, bounds.xmax-bounds.xmin);

	// Vertical Straight Edge Record
	writeVertHorzEdge(true, bounds.ymax-bounds.ymin);

	// Horizontal Straight Edge Record
	writeVertHorzEdge(false, bounds.xmin-bounds.xmax);

	// Vertical Straight Edge Record
	writeVertHorzEdge(true, bounds.ymin-bounds.ymax);

	// End Shape Record
	writeBits(0, 6);
	flushWriteBits();

	writeRecordHeaderEnd();

	return m_nextCharacterID;
}

// ----------------------------------------------------------------------------
SwfWriter::CharacterID SwfWriter::outputDefineSpriteBegin(unsigned int frameCount)
{
	writeRecordHeaderStart(SwfTag_DefineSprite);
	writeNextCharacterID();
	writeWord(frameCount);

	return m_nextCharacterID;
}

// ----------------------------------------------------------------------------
void SwfWriter::outputDefineSpriteEnd()
{
	outputEnd();
	writeRecordHeaderEnd();
}

// ----------------------------------------------------------------------------
void SwfWriter::outputPlaceObject2(CharacterID id, unsigned int depth)
{
	writeRecordHeaderStart(SwfTag_PlaceObject2, 5);
	writeByte(0x02);	// has character ID
	writeWord(depth);
	writeWord(id);
	writeRecordHeaderEnd();
}

// ----------------------------------------------------------------------------
void SwfWriter::outputPlaceObject2(CharacterID id, unsigned int depth, const std::wstring& name)
{
	writeRecordHeaderStart(SwfTag_PlaceObject2, 5 + name.length() + 1);
	writeByte(0x22);	// has character ID & name
	writeWord(depth);
	writeWord(id);
	writeString(name);
	writeRecordHeaderEnd();
}

// ----------------------------------------------------------------------------
void SwfWriter::outputRemoveObject2(unsigned int depth)
{
	writeRecordHeaderStart(SwfTag_RemoveObject2, 2);
	writeWord(depth);
	writeRecordHeaderEnd();
}

// ----------------------------------------------------------------------------
void SwfWriter::outputExportAssets(CharacterID id, const std::wstring& name)
{
	writeRecordHeaderStart(SwfTag_DefineExportAssets, name.length() + 5);
	writeWord(1);
	writeWord(id);
	writeString(name);
	writeRecordHeaderEnd();
}

// ----------------------------------------------------------------------------
void SwfWriter::outputSoundStreamBegin(SamplingRate playbackRate, 
									   SoundType playbackType, 
									   SoundCompression compressionType, 
									   SamplingRate streamRate, 
									   SoundType streamType)
{
	writeRecordHeaderStart(SwfTag_SoundStreamHead, 6);
	
	initWriteBits();
	writeBits(0, 4);				// Reserved - always zeros
	writeBits(playbackRate, 2);		// Playback sampling rate
	writeBits(1, 1);				// Playback sample size - always 1 => 16 bits
	writeBits(playbackType, 1);		// Playback channels - mono or stereo
	writeBits(compressionType, 4);	// Compression - ADPCM or MP3
	writeBits(streamRate, 2);		// Streaming sampling rate
	writeBits(1, 1);				// Streaming sample size - always 1 => 16 bits
	writeBits(streamType, 1);		// Streaming channels - mono or stereo
	flushWriteBits();

	// These fields are initialized to zero, will be fixed up later...
	m_sndStreamFixupPos = getPosition();
	writeWord(0);					// Average # of samples in each SoundStreamBlock
	if (compressionType == SwfMP3)
	{								// Latency seek should match SeekSamples field in
		writeWord(0);				// the first SoundStream block for this stream.
	}								
	writeRecordHeaderEnd();
}

// ----------------------------------------------------------------------------
void SwfWriter::ouputMP3StreamEnd(unsigned short sampleCount, short latencySeek)
{
	if (m_sndStreamFixupPos > 0)
	{
		unsigned long currPos = getPosition();
		setPosition(m_sndStreamFixupPos);

		writeWord(sampleCount);		// Average # of samples in each SoundStreamBlock
		writeWord(latencySeek);		// Latency seek should match SeekSamples field in
									// the first SoundStream block for this stream.
		m_sndStreamFixupPos = 0;
		setPosition(currPos);
	}
}

// ----------------------------------------------------------------------------
void SwfWriter::outputMP3StreamBlock(unsigned short sampleCount, unsigned short seekSamples, const Buffer& data)
{
	unsigned int dataSize = data.size();
	writeRecordHeaderStart(SwfTag_SoundStreamBlock, dataSize + 4);
	writeWord(sampleCount);
	writeWord(seekSamples);
	writeData(data);
	writeRecordHeaderEnd();
}

// ----------------------------------------------------------------------------
void SwfWriter::outputDoActionStop()
{
	writeRecordHeaderStart(SwfTag_DoAction, 2);
	writeByte(SwfAction_Stop);
	writeByte(0);
	writeRecordHeaderEnd();
}
