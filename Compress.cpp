#include "zlib.h"
#include "Compress.h"

// ----------------------------------------------------------------------------
ZLIBCompressor::ZLIBCompressor() : 
	m_quality(ZLIB_DEFAULT_COMPRESSION)
{

}

// ----------------------------------------------------------------------------
unsigned int ZLIBCompressor::compress(const unsigned char* dBuffer, unsigned int dSize, std::vector<unsigned char>& outBuf)
{
	unsigned long cSize = getMaxCompressionSize(dSize);
	
	outBuf.clear();
	outBuf.resize(cSize);	// throws on error
	if (compress2(&outBuf[0], &cSize, dBuffer, dSize, m_quality) != Z_OK)
	{
		// @todo throw some error here...
	}
	outBuf.resize(cSize);

	return cSize;
}