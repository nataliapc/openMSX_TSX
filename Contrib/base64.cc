// compile with:
//   g++ -std=c++20 -Wall -Os base64.cc -I ../src/utils/ ../src/utils/Base64.cc -lz -o encode-gz-base64
//   g++ -std=c++20 -Wall -Os base64.cc -I ../src/utils/ ../src/utils/Base64.cc -lz -o decode-gz-base64

#include "Base64.hh"

#include <zlib.h>

#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include <sys/stat.h>
#include <vector>

std::string encode(const void* data, unsigned len)
{
	uLongf dstLen = len + len / 1000 + 12 + 1; // worst-case
	std::vector<unsigned char> buf(dstLen);
	if (compress2(buf.data(), &dstLen,
	              std::bit_cast<const Bytef*>(data), len, 9)
	    != Z_OK) {
		std::cerr << "Error while compressing blob.\n";
		exit(1);
	}
	return Base64::encode(std::span(buf.data(), dstLen));
}

std::string decode(const char* data, unsigned len)
{
	static const unsigned MAX_SIZE = 1024 * 1024; // 1MB
	const auto& [decBuf, decBufSize] = Base64::decode(std::string_view(data, len));
	std::vector<char> buf(MAX_SIZE);
	uLongf dstLen = MAX_SIZE;
	if (uncompress(std::bit_cast<Bytef*>(buf.data()), &dstLen,
	               std::bit_cast<const Bytef*>(decBuf.data()), uLong(decBufSize))
	    != Z_OK) {
		std::cerr << "Error while decompressing blob.\n";
		exit(1);
	}
	return {buf.data(), dstLen};
}

int main(int argc, const char** argv)
{
	std::span<const char*> arg(argv, argc);
	if (arg.size() != 3) {
		std::cerr << "Usage: " << arg[0] << " <input> <output>\n";
		exit(1);
	}
	FILE* inf = fopen(arg[1], "rb");
	if (!inf) {
		std::cerr << "Error while opening " << arg[1] << '\n';
		exit(1);
	}
	struct stat st;
	fstat(fileno(inf), &st);
	size_t size = st.st_size;
	std::vector<char> inBuf(size);
	if (fread(inBuf.data(), size, 1, inf) != 1) {
		std::cerr << "Error while reading " << arg[1] << '\n';
		exit(1);
	}
	fclose(inf);

	std::string result;
	if        (strstr(arg[0], "encode-gz-base64")) {
		result = encode(inBuf.data(), inBuf.size());
	} else if (strstr(arg[0], "decode-gz-base64")) {
		result = decode(inBuf.data(), inBuf.size());
	} else {
		std::cerr << "This executable should be named 'encode-gz-base64' or "
		             "'decode-gz-base64'.\n";
		exit(1);
	}

	FILE* outf = fopen(arg[2], "wb+");
	if (!outf) {
		std::cerr << "Error while opening " << arg[2] << '\n';
		exit(1);
	}

	if (fwrite(result.data(), result.size(), 1, outf) != 1) {
		std::cerr << "Error while writing " << arg[2] << '\n';
		exit(1);
	}
	fclose(outf);
}
