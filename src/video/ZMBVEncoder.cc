// $Id$

// Code based on DOSBox-0.65

#include "ZMBVEncoder.hh"
#include "FrameSource.hh"
#include "unreachable.hh"
#include "openmsx.hh"
#include "build-info.hh"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cmath>

namespace openmsx {

static const unsigned char DBZV_VERSION_HIGH = 0;
static const unsigned char DBZV_VERSION_LOW = 1;
static const unsigned char COMPRESSION_ZLIB = 1;
static const unsigned MAX_VECTOR = 16;
static const unsigned BLOCK_WIDTH  = MAX_VECTOR;
static const unsigned BLOCK_HEIGHT = MAX_VECTOR;
static const unsigned FLAG_KEYFRAME = 0x01;

struct CodecVector {
	double cost() const {
		double c = sqrt(double(x * x + y * y));
		if ((x == 0) || (y == 0)) {
			// no penalty for purely horizontal/vertical offset
			c *= 1.0;
		} else if (abs(x) == abs(y)) {
			// small penalty for pure diagonal
			c *= 2.0;
		} else {
			// bigger penalty for 'random' direction
			c *= 4.0;
		}
		return c;
	}
	int x;
	int y;
};
bool operator<(const CodecVector& l, const CodecVector& r)
{
	return l.cost() < r.cost();
}

static const unsigned VECTOR_TAB_SIZE =
	1 +                                       // center
	8 * MAX_VECTOR +                          // horizontal, vertial, diagonal
	MAX_VECTOR * MAX_VECTOR - 2 * MAX_VECTOR; // rest (only MAX_VECTOR/2)
CodecVector vectorTable[VECTOR_TAB_SIZE];

struct KeyframeHeader {
	unsigned char high_version;
	unsigned char low_version;
	unsigned char compression;
	unsigned char format;
	unsigned char blockwidth;
	unsigned char blockheight;
};

const char* ZMBVEncoder::CODEC_4CC = "ZMBV";


static inline unsigned short pixelBEtoLE(unsigned short pixel)
{
	return (pixel >> 8) | (pixel << 8);
}

static inline unsigned int pixelBEtoLE(unsigned int pixel)
{
	return ((pixel << 24)             ) | ((pixel <<  8) & 0x00FF0000)
	     | ((pixel >>  8) & 0x0000FF00) | ((pixel >> 24)             );
}

static void createVectorTable()
{
	unsigned p = 0;
	// center
	vectorTable[p].x = 0;
	vectorTable[p].y = 0;
	p += 1;
	// horizontal, vertial, diagonal
	for (int i = 1; i <= int(MAX_VECTOR); ++i) {
		vectorTable[p + 0].x =  i;
		vectorTable[p + 0].y =  0;
		vectorTable[p + 1].x = -i;
		vectorTable[p + 1].y =  0;
		vectorTable[p + 2].x =  0;
		vectorTable[p + 2].y =  i;
		vectorTable[p + 3].x =  0;
		vectorTable[p + 3].y = -i;
		vectorTable[p + 4].x =  i;
		vectorTable[p + 4].y =  i;
		vectorTable[p + 5].x = -i;
		vectorTable[p + 5].y =  i;
		vectorTable[p + 6].x =  i;
		vectorTable[p + 6].y = -i;
		vectorTable[p + 7].x = -i;
		vectorTable[p + 7].y = -i;
		p += 8;
	}
	// rest
	for (int y = 1; y <= int(MAX_VECTOR / 2); ++y) {
		for (int x = 1; x <= int(MAX_VECTOR / 2); ++x) {
			if (x == y) continue; // already have diagonal
			vectorTable[p + 0].x =  x;
			vectorTable[p + 0].y =  y;
			vectorTable[p + 1].x = -x;
			vectorTable[p + 1].y =  y;
			vectorTable[p + 2].x =  x;
			vectorTable[p + 2].y = -y;
			vectorTable[p + 3].x = -x;
			vectorTable[p + 3].y = -y;
			p += 4;
		}
	}
	assert(p == VECTOR_TAB_SIZE);

	std::sort(&vectorTable[0], &vectorTable[VECTOR_TAB_SIZE]);
}

ZMBVEncoder::ZMBVEncoder(unsigned width_, unsigned height_, unsigned bpp)
	: width(width_)
	, height(height_)
{
	setupBuffers(bpp);
	createVectorTable();
	memset(&zstream, 0, sizeof(zstream));
	deflateInit(&zstream, 6); // compression level

	// I did a small test: compression level vs compression speed
	//  (recorded Space Manbow intro, video only)
	//
	// level |  time  | size
	// ------+--------+----------
	//   0   | 1m12.6 | 139442594
	//   1   | 1m12.1 |   5217288
	//   2   | 1m10.8 |   4887258
	//   3   | 1m11.8 |   4610668
	//   4   | 1m13.1 |   3791932  <-- old default
	//   5   | 1m14.2 |   3602078
	//   6   | 1m14.5 |   3363766  <-- current default
	//   7   | 1m15.8 |   3333938
	//   8   | 1m25.0 |   3301168
	//   9   | 2m04.1 |   3253706
	//
	// Level 6 seems a good compromise between size/speed for THIS test.
}

ZMBVEncoder::~ZMBVEncoder()
{
	delete[] oldframe;
	delete[] newframe;
	delete[] work;
	delete[] output;
}

void ZMBVEncoder::setupBuffers(unsigned bpp)
{
	switch (bpp) {
	case 15:
		format = ZMBV_FORMAT_15BPP;
		pixelSize = 2;
		break;
	case 16:
		format = ZMBV_FORMAT_16BPP;
		pixelSize = 2;
		break;
	case 32:
		format = ZMBV_FORMAT_32BPP;
		pixelSize = 4;
		break;
	default:
		UNREACHABLE;
	}

	pitch = width + 2 * MAX_VECTOR;
	unsigned bufsize = (height + 2 * MAX_VECTOR) * pitch * pixelSize + 2048;

	oldframe = new unsigned char[bufsize];
	newframe = new unsigned char[bufsize];
	memset(oldframe, 0, bufsize);
	memset(newframe, 0, bufsize);
	work = new unsigned char[bufsize];
	outputSize = neededSize();
	output = new unsigned char[outputSize];

	assert((width  % BLOCK_WIDTH ) == 0);
	assert((height % BLOCK_HEIGHT) == 0);
	unsigned xblocks = width / BLOCK_WIDTH;
	unsigned yblocks = height / BLOCK_HEIGHT;
	for (unsigned y = 0; y < yblocks; ++y) {
		for (unsigned x = 0; x < xblocks; ++x) {
			blockOffsets.push_back(
				((y * BLOCK_HEIGHT) + MAX_VECTOR) * pitch +
				(x * BLOCK_WIDTH) + MAX_VECTOR);
		}
	}
}

unsigned ZMBVEncoder::neededSize()
{
	unsigned f = pixelSize;
	f = f * width * height + 2 * (1 + (width / 8)) * (1 + (height / 8)) + 1024;
	return f + f / 1000;
}

template<class P>
unsigned ZMBVEncoder::possibleBlock(int vx, int vy, unsigned offset)
{
	int ret = 0;
	P* pold = &(reinterpret_cast<P*>(oldframe))[offset + (vy * pitch) + vx];
	P* pnew = &(reinterpret_cast<P*>(newframe))[offset];
	for (unsigned y = 0; y < BLOCK_HEIGHT; y += 4) {
		for (unsigned x = 0; x < BLOCK_WIDTH; x += 4) {
			if (pold[x] != pnew[x]) ++ret;
		}
		pold += pitch * 4;
		pnew += pitch * 4;
	}
	return ret;
}

template<class P>
unsigned ZMBVEncoder::compareBlock(int vx, int vy, unsigned offset)
{
	int ret = 0;
	P* pold = &(reinterpret_cast<P*>(oldframe))[offset + (vy * pitch) + vx];
	P* pnew = &(reinterpret_cast<P*>(newframe))[offset];
	for (unsigned y = 0; y < BLOCK_HEIGHT; ++y) {
		for (unsigned x = 0; x < BLOCK_WIDTH; ++x) {
			if (pold[x] != pnew[x]) ++ret;
		}
		pold += pitch;
		pnew += pitch;
	}
	return ret;
}

template<class P>
void ZMBVEncoder::addXorBlock(int vx, int vy, unsigned offset)
{
	P* pold = &(reinterpret_cast<P*>(oldframe))[offset + (vy * pitch) + vx];
	P* pnew = &(reinterpret_cast<P*>(newframe))[offset];
	for (unsigned y = 0; y < BLOCK_HEIGHT; ++y) {
		for (unsigned x = 0; x < BLOCK_WIDTH; ++x) {
			P pxor = pnew[x] ^ pold[x];
			if (OPENMSX_BIGENDIAN) {
				pxor = pixelBEtoLE(pxor);
			}
			*reinterpret_cast<P*>(&work[workUsed]) = pxor;
			workUsed += sizeof(P);
		}
		pold += pitch;
		pnew += pitch;
	}
}

template<class P>
void ZMBVEncoder::addXorFrame()
{
	signed char* vectors = reinterpret_cast<signed char*>(&work[workUsed]);

	// Align the following xor data on 4 byte boundary
	unsigned blockcount = unsigned(blockOffsets.size());
	workUsed = (workUsed + blockcount * 2 + 3) & ~3;

	int bestvx = 0;
	int bestvy = 0;
	for (unsigned b = 0; b < blockcount; ++b) {
		unsigned offset = blockOffsets[b];
		// first try best vector of previous block
		unsigned bestchange = compareBlock<P>(bestvx, bestvy, offset);
		if (bestchange >= 4) {
			int possibles = 64;
			for (unsigned v = 0; v < VECTOR_TAB_SIZE; ++v) {
				int vx = vectorTable[v].x;
				int vy = vectorTable[v].y;
				if (possibleBlock<P>(vx, vy, offset) < 4) {
					unsigned testchange = compareBlock<P>(vx, vy, offset);
					if (testchange < bestchange) {
						bestchange = testchange;
						bestvx = vx;
						bestvy = vy;
						if (bestchange < 4) break;
					}
					--possibles;
					if (possibles == 0) break;
				}
			}
		}
		vectors[b * 2 + 0] = (bestvx << 1);
		vectors[b * 2 + 1] = (bestvy << 1);
		if (bestchange) {
			vectors[b * 2 + 0] |= 1;
			addXorBlock<P>(bestvx, bestvy, offset);
		}
	}
}

template<class P>
void ZMBVEncoder::addFullFrame()
{
	unsigned char* readFrame =
		newframe + pixelSize * (MAX_VECTOR + MAX_VECTOR * pitch);
	for (unsigned i = 0; i < height; ++i) {
		if (OPENMSX_BIGENDIAN) {
			lineBEtoLE<P>(readFrame, width);
		} else {
			memcpy(&work[workUsed], readFrame, width * sizeof(P));
		}
		readFrame += pitch * sizeof(P);
		workUsed += width * sizeof(P);
	}
}

template<class P>
void ZMBVEncoder::lineBEtoLE(unsigned char* input, unsigned width)
{
	P* pixelsIn = reinterpret_cast<P*>(input);
	P* pixelsOut = reinterpret_cast<P*>(&work[workUsed]);
	for (unsigned i = 0; i < width; i++) {
		pixelsOut[i] = pixelBEtoLE(pixelsIn[i]);
	}
}

const void* ZMBVEncoder::getScaledLine(FrameSource* frame, unsigned y)
{
#if HAVE_32BPP
	if (pixelSize == 4) { // 32bpp
		switch (height) {
		case 240:
			return frame->getLinePtr320_240<unsigned int>(y);
		case 480:
			return frame->getLinePtr640_480<unsigned int>(y);
		default:
			UNREACHABLE;
		}
	}
#endif
#if HAVE_16BPP
	if (pixelSize == 2) { // 15bpp or 16bpp
		switch (height) {
		case 240:
			return frame->getLinePtr320_240<unsigned short>(y);
		case 480:
			return frame->getLinePtr640_480<unsigned short>(y);
		default:
			UNREACHABLE;
		}
	}
#endif
	UNREACHABLE;
	return 0; // avoid warning
}

void ZMBVEncoder::compressFrame(bool keyFrame, FrameSource* frame,
                                void*& buffer, unsigned& written)
{
	std::swap(newframe, oldframe); // replace oldframe with newframe

	// Reset the work buffer
	workUsed = 0;
	unsigned writeDone = 1;
	unsigned char* writeBuf = output;

	output[0] = 0; // first byte contains info about this frame
	if (keyFrame) {
		output[0] |= FLAG_KEYFRAME;
		KeyframeHeader* header =
			reinterpret_cast<KeyframeHeader*>(writeBuf + writeDone);
		header->high_version = DBZV_VERSION_HIGH;
		header->low_version = DBZV_VERSION_LOW;
		header->compression = COMPRESSION_ZLIB;
		header->format = format;
		header->blockwidth = BLOCK_WIDTH;
		header->blockheight = BLOCK_HEIGHT;
		writeDone += sizeof(KeyframeHeader);
		deflateReset(&zstream); // restart deflate
	}

	// copy lines (to add black border)
	unsigned linePitch = pitch * pixelSize;
	unsigned lineWidth = width * pixelSize;
	unsigned char* dest = newframe +
	                      pixelSize * (MAX_VECTOR + MAX_VECTOR * pitch);
	for (unsigned i = 0; i < height; ++i) {
		memcpy(dest, getScaledLine(frame, i), lineWidth);
		dest += linePitch;
	}

	// Add the frame data.
	if (keyFrame) {
		// Key frame: full frame data.
		switch (pixelSize) {
		case 2:
			addFullFrame<unsigned short>();
			break;
		case 4:
			addFullFrame<unsigned int>();
			break;
		default:
			UNREACHABLE;
		}
	} else {
		// Non-key frame: delta frame data.
		switch (pixelSize) {
		case 2:
			addXorFrame<unsigned short>();
			break;
		case 4:
			addXorFrame<unsigned int>();
			break;
		default:
			UNREACHABLE;
		}
	}
	// Compress the frame data with zlib.
	zstream.next_in = static_cast<Bytef*>(work);
	zstream.avail_in = workUsed;
	zstream.total_in = 0;

	zstream.next_out = static_cast<Bytef*>(writeBuf + writeDone);
	zstream.avail_out = outputSize - writeDone;
	zstream.total_out = 0;
	deflate(&zstream, Z_SYNC_FLUSH);

	buffer = output;
	written = writeDone + zstream.total_out;
}

} // namespace openmsx
