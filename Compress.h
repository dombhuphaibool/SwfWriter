#include <vector>

class ZLIBCompressor
{
private:
	enum CompressionLevel
	{
		ZLIB_BEST_COMPRESSION		= 9,	///< Best but slower compression level.
		ZLIB_MEDIUM_COMPRESSION		= 6,	///< Balanced compression level.
		ZLIB_FAST_COMPRESSION		= 3,	///< Fast and somewhat acceptable compression level.
		ZLIB_BEST_SPEED   			= 1,	///< Fast but poor compression level.
		ZLIB_NO_COMPRESSION			= 0,	///< No content compression is performed.

		ZLIB_DEFAULT_COMPRESSION	= ZLIB_BEST_COMPRESSION
	};

private:
	CompressionLevel m_quality;

	inline unsigned int getMaxCompressionSize(unsigned int inSize) const
	{
		return ((inSize) + ((inSize) / 100) + 12 + 1); // from a zlib formula
	}

public:
	ZLIBCompressor();

	unsigned int compress(const unsigned char* dBuffer, unsigned int dSize, std::vector<unsigned char>& outBuf);
};
