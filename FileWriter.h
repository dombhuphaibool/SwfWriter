#include <vector>
#include <string>

class FileWriter
{
public:
	typedef std::vector<unsigned char> Buffer;

private:
	std::wstring m_filename;
	Buffer m_buffer;
	unsigned long m_pos;

	unsigned int m_writeBitPos;
	unsigned int m_writeBitBuf;

private:
	unsigned long ensureBufferSize(unsigned int size);

protected:
	unsigned long getPosition();
	void setPosition(unsigned long pos);
	unsigned long getFileSize();
	unsigned long resizeFile(unsigned long size);
	unsigned char *getBufferAtPos(unsigned long pos);

protected:
	virtual void writeBuffer();

public:
	FileWriter();
	virtual ~FileWriter();

	virtual void open(const std::wstring& filename);
	virtual void close();

	void initWriteBits();
	void flushWriteBits();
	void writeBits(int value, unsigned int numBits);

	void writeByte(unsigned char value);
	void writeWord(unsigned short value);
	void writeLong(unsigned long value);
	void writeData(const Buffer& value);
	void writeString(const std::wstring& value);

	void shiftContent(unsigned long startPos, unsigned long size, int offset);
};
