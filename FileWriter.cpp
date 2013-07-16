#include <assert.h>
#include "FileWriter.h"

// ----------------------------------------------------------------------------
#if LITTLE_ENDIAN
	#define FIRST_BYTE_IDX(x, y)	(x)
	#define NEXT_BYTE_IDX(x)		++(x)
#else
	#define FIRST_BYTE_IDX(x, y)	(y)
	#define NEXT_BYTE_IDX(x)		--(x)
#endif

// ----------------------------------------------------------------------------
FileWriter::FileWriter() : m_pos(0)
{
	initWriteBits();
}

// ----------------------------------------------------------------------------
FileWriter::~FileWriter()
{
}

// ----------------------------------------------------------------------------
void FileWriter::open(const std::wstring& filename)
{
	m_filename = filename;
}

// ----------------------------------------------------------------------------
void FileWriter::close()
{
	writeBuffer();

	m_buffer.clear();
	m_pos = 0;
	m_filename.clear();

	initWriteBits();
}

// ----------------------------------------------------------------------------
unsigned long FileWriter::getPosition()
{
	return m_pos;
}

// ----------------------------------------------------------------------------
void FileWriter::setPosition(unsigned long pos)
{
	m_pos = pos;
}

// ----------------------------------------------------------------------------
unsigned long FileWriter::getFileSize()
{
	return m_buffer.size();
}

// ----------------------------------------------------------------------------
unsigned long FileWriter::resizeFile(unsigned long size)
{
	m_buffer.resize(size);
	return m_buffer.size();
}

// ----------------------------------------------------------------------------
unsigned char* FileWriter::getBufferAtPos(unsigned long pos)
{
	return (&m_buffer[0]+pos);
}

// ----------------------------------------------------------------------------
void FileWriter::shiftContent(unsigned long startPos, unsigned long size, int offset)
{
	// @todo...
	bool resize = false;
	if (startPos+size == m_buffer.size())
		resize = true;
	memmove(&m_buffer[startPos]+offset, &m_buffer[startPos], size);
	m_pos = startPos + size + offset;
	m_buffer.resize(m_pos);
}

// ----------------------------------------------------------------------------
void FileWriter::writeBuffer()
{
	FILE *fp = _wfopen(m_filename.c_str(), L"wb");
	if (fp)
	{
		fwrite(&m_buffer[0], 1, m_buffer.size(), fp);
		fclose(fp);
	}
}

// ----------------------------------------------------------------------------
unsigned long FileWriter::ensureBufferSize(unsigned int size)
{
	unsigned long sizeNeeded = m_pos + size;
	unsigned long bufferSize = m_buffer.size();
	if (sizeNeeded > bufferSize)
	{
		m_buffer.resize(sizeNeeded);
		bufferSize = m_buffer.size();
		assert(sizeNeeded == bufferSize);
	}

	return bufferSize;
}

// ----------------------------------------------------------------------------
void FileWriter::writeByte(unsigned char value)
{
	if (m_pos == m_buffer.size())
	{
		m_buffer.push_back(value);
	}
	else
	{
		ensureBufferSize(1);
		m_buffer[m_pos] = value;
	}

	m_pos += 1;
}

// ----------------------------------------------------------------------------
void FileWriter::writeWord(unsigned short value)
{
	// unsigned int idx = FIRST_BYTE_IDX(0, 1);
	unsigned char* pValue = reinterpret_cast<unsigned char*>(&value);
	if (m_pos == m_buffer.size())
	{
		m_buffer.push_back(pValue[0]);
		m_buffer.push_back(pValue[1]);
	}
	else
	{
		ensureBufferSize(2);
		m_buffer[m_pos]   = pValue[0];
		m_buffer[m_pos+1] = pValue[1];
	}

	m_pos += 2;
}

// ----------------------------------------------------------------------------
void FileWriter::writeLong(unsigned long value)
{
	unsigned char* pValue = reinterpret_cast<unsigned char*>(&value);
	if (m_pos == m_buffer.size())
	{
		m_buffer.push_back(pValue[0]);
		m_buffer.push_back(pValue[1]);
		m_buffer.push_back(pValue[2]);
		m_buffer.push_back(pValue[3]);
	}
	else
	{
		ensureBufferSize(4);
		m_buffer[m_pos]	  = pValue[0];
		m_buffer[m_pos+1] = pValue[1];
		m_buffer[m_pos+2] = pValue[2];
		m_buffer[m_pos+3] = pValue[3];
	}

	m_pos += 4;
}

// ----------------------------------------------------------------------------
void FileWriter::writeData(const Buffer& value)
{
	unsigned long bufferSize = value.size();
	unsigned long sizeNeeded = m_pos + bufferSize;

	if (m_buffer.size() < sizeNeeded)
	{
		m_buffer.resize(sizeNeeded);
		assert(m_buffer.size() >= sizeNeeded);
	}
	std::copy(value.begin(), value.end(), m_buffer.begin()+m_pos);

	m_pos += bufferSize;
}

// ----------------------------------------------------------------------------
void FileWriter::writeString(const std::wstring& value)
{
	unsigned int len = value.length() + 1;

	// @todo: We really should write a routing to convert the wstring to UTF-8...
	if (m_pos == m_buffer.size())
	{
		for (std::wstring::const_iterator i = value.begin(); i != value.end(); ++i)
		{
			m_buffer.push_back(static_cast<unsigned char>(*i));
		}
		m_buffer.push_back(0);
	}
	else
	{
		unsigned long idx = m_pos;
		ensureBufferSize(len);
		for (std::wstring::const_iterator i = value.begin(); i != value.end(); ++i)
		{
			m_buffer[idx++] = static_cast<unsigned char>(*i);
		}
		m_buffer[idx] = 0;
	}

	m_pos += len;
}

// ----------------------------------------------------------------------------
void FileWriter::initWriteBits()
{
	m_writeBitPos = 8;
	m_writeBitBuf = 0;
}

// ----------------------------------------------------------------------------
void FileWriter::flushWriteBits()
{
	//dump remaining bits
	if (m_writeBitPos < 8)
	{
		writeByte(static_cast<unsigned char>(m_writeBitBuf));
		initWriteBits();
	}
}

// ----------------------------------------------------------------------------
void FileWriter::writeBits(int value, unsigned int numBits)
{
	for (;;)
	{
		// Mask off any extra bits.
		value &= (unsigned int) 0xffffffff >> (32 - numBits);

		// The number of bits more than the buffer will hold
		int	s = numBits - m_writeBitPos;
		if (s <= 0)
		{
			// This fits in the buffer, add it and finish.
			m_writeBitBuf |= value << -s;
			m_writeBitPos -= numBits;	// We used x bits in the buffer.
			return;
		}
		else
		{
			// This fills the buffer, fill the remaining space and try again.
			m_writeBitBuf |= value >> s;
			numBits -= m_writeBitPos;	// We placed x bits in the buffer.
			writeByte(static_cast<unsigned char>(m_writeBitBuf));
			m_writeBitBuf = 0;
			m_writeBitPos = 8;
		}
	}
}