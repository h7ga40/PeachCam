// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
/*
 *  DecodedBitStreamParser.cpp
 *  zxing
 *
 *  Created by Christian Brunschen on 20/05/2008.
 *  Copyright 2008 ZXing authors All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zxing/qrcode/decoder/DecodedBitStreamParser.h>
#include <zxing/common/CharacterSetECI.h>
#include <zxing/FormatException.h>
#include <zxing/common/StringUtils.h>
#include <iostream>
#ifndef NO_ICONV
#include <iconv.h>
#endif

// Required for compatibility. TODO: test on Symbian
#ifdef ZXING_ICONV_CONST
#undef ICONV_CONST
#define ICONV_CONST const
#endif

#ifndef ICONV_CONST
#define ICONV_CONST /**/
#endif

using namespace std;
using namespace zxing;
using namespace zxing::qrcode;
using namespace zxing::common;

const char DecodedBitStreamParser::ALPHANUMERIC_CHARS[] =
{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
  'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', ' ', '$', '%', '*', '+', '-', '.', '/', ':'
};

namespace { int GB2312_SUBSET = 1; }

int DecodedBitStreamParser::append(std::string &result, string const &in, const char *src)
{
	return append(result, (char const *)in.c_str(), in.length(), src);
}

int DecodedBitStreamParser::append(std::string &result,
	const char *bufIn,
	size_t nIn,
	const char *src)
{
#ifndef NO_ICONV
	if (nIn == 0) {
		return 0;
	}

	iconv_t cd = iconv_open(StringUtils::UTF8, src);
	if (cd == (iconv_t)-1) {
		result.append((const char *)bufIn, nIn);
		return 0;
	}

	const int maxOut = 4 * nIn + 1;
	char *bufOut = new char[maxOut];

	ICONV_CONST char *fromPtr = (ICONV_CONST char *)bufIn;
	size_t nFrom = nIn;
	char *toPtr = (char *)bufOut;
	size_t nTo = maxOut;

	while (nFrom > 0) {
		size_t oneway = iconv(cd, &fromPtr, &nFrom, &toPtr, &nTo);
		if (oneway == (size_t)(-1)) {
			iconv_close(cd);
			delete[] bufOut;
			return -1;
		}
	}
	iconv_close(cd);

	int nResult = maxOut - nTo;
	bufOut[nResult] = '\0';
	result.append((const char *)bufOut);
	delete[] bufOut;
#else
	result.append((const char *)bufIn, nIn);
#endif
	return 0;
}

int DecodedBitStreamParser::decodeHanziSegment(Ref<BitSource> bits_,
	string &result,
	int count)
{
	BitSource &bits(*bits_);
	// Don't crash trying to read more bits than we have available.
	if (count * 13 > bits.available()) {
		return -1;
	}

	// Each character will require 2 bytes. Read the characters as 2-byte pairs
	// and decode as GB2312 afterwards
	size_t nBytes = 2 * count;
	char *buffer = new char[nBytes];
	int offset = 0;
	while (count > 0) {
		// Each 13 bits encodes a 2-byte character
		int twoBytes = bits.readBits(13);
		int assembledTwoBytes = ((twoBytes / 0x060) << 8) | (twoBytes % 0x060);
		if (assembledTwoBytes < 0x003BF) {
			// In the 0xA1A1 to 0xAAFE range
			assembledTwoBytes += 0x0A1A1;
		}
		else {
			// In the 0xB0A1 to 0xFAFE range
			assembledTwoBytes += 0x0A6A1;
		}
		buffer[offset] = (char)((assembledTwoBytes >> 8) & 0xFF);
		buffer[offset + 1] = (char)(assembledTwoBytes & 0xFF);
		offset += 2;
		count--;
	}

	int ret;
	if ((ret = append(result, buffer, nBytes, StringUtils::GB2312)) < 0) {
		delete[] buffer;
		return -1;
	}

	delete[] buffer;
	return 0;
}

int DecodedBitStreamParser::decodeKanjiSegment(Ref<BitSource> bits, std::string &result, int count)
{
// Each character will require 2 bytes. Read the characters as 2-byte pairs
// and decode as Shift_JIS afterwards
	size_t nBytes = 2 * count;
	char *buffer = new char[nBytes];
	int offset = 0;
	while (count > 0) {
		// Each 13 bits encodes a 2-byte character
		int twoBytes = bits->readBits(13);
		int assembledTwoBytes = ((twoBytes / 0x0C0) << 8) | (twoBytes % 0x0C0);
		if (assembledTwoBytes < 0x01F00) {
			// In the 0x8140 to 0x9FFC range
			assembledTwoBytes += 0x08140;
		}
		else {
			// In the 0xE040 to 0xEBBF range
			assembledTwoBytes += 0x0C140;
		}
		buffer[offset] = (char)(assembledTwoBytes >> 8);
		buffer[offset + 1] = (char)assembledTwoBytes;
		offset += 2;
		count--;
	}
	int ret;
	if ((ret = append(result, buffer, nBytes, StringUtils::SHIFT_JIS)) < 0) {
		delete[] buffer;
		return -1;
	}
	delete[] buffer;
	return 0;
}

int DecodedBitStreamParser::decodeByteSegment(Ref<BitSource> bits_,
	string &result,
	int count,
	CharacterSetECI *currentCharacterSetECI,
	ArrayRef< ArrayRef<char> > &byteSegments,
	Hashtable const &hints)
{
	int nBytes = count;
	BitSource &bits(*bits_);
	// Don't crash trying to read more bits than we have available.
	if (count << 3 > bits.available()) {
		return -1;
	}

	ArrayRef<char> bytes_(count);
	char *readBytes = &(*bytes_)[0];
	for (int i = 0; i < count; i++) {
		readBytes[i] = (char)bits.readBits(8);
	}
	string encoding;
	if (currentCharacterSetECI == 0) {
	  // The spec isn't clear on this mode; see
	  // section 6.4.5: t does not say which encoding to assuming
	  // upon decoding. I have seen ISO-8859-1 used as well as
	  // Shift_JIS -- without anything like an ECI designator to
	  // give a hint.
		encoding = StringUtils::guessEncoding(readBytes, count, hints);
	}
	else {
		encoding = currentCharacterSetECI->name();
	}
	int ret;
	if ((ret = append(result, readBytes, nBytes, encoding.c_str())) < 0) {
		return -1;
	}
	byteSegments->values().push_back(bytes_);
	return 0;
}

int DecodedBitStreamParser::decodeNumericSegment(Ref<BitSource> bits, std::string &result, int count)
{
	int nBytes = count;
	char *bytes = new char[nBytes];
	int i = 0;
	// Read three digits at a time
	while (count >= 3) {
	  // Each 10 bits encodes three digits
		if (bits->available() < 10) {
			delete[] bytes;
			return -1;
		}
		int threeDigitsBits = bits->readBits(10);
		if (threeDigitsBits >= 1000) {
			ostringstream s;
			s << "Illegal value for 3-digit unit: " << threeDigitsBits;
			delete[] bytes;
			return -1;
		}
		bytes[i++] = ALPHANUMERIC_CHARS[threeDigitsBits / 100];
		bytes[i++] = ALPHANUMERIC_CHARS[(threeDigitsBits / 10) % 10];
		bytes[i++] = ALPHANUMERIC_CHARS[threeDigitsBits % 10];
		count -= 3;
	}
	if (count == 2) {
		if (bits->available() < 7) {
			delete[] bytes;
			return -1;
		}
		// Two digits left over to read, encoded in 7 bits
		int twoDigitsBits = bits->readBits(7);
		if (twoDigitsBits >= 100) {
			ostringstream s;
			s << "Illegal value for 2-digit unit: " << twoDigitsBits;
			delete[] bytes;
			return -1;
		}
		bytes[i++] = ALPHANUMERIC_CHARS[twoDigitsBits / 10];
		bytes[i++] = ALPHANUMERIC_CHARS[twoDigitsBits % 10];
	}
	else if (count == 1) {
		if (bits->available() < 4) {
			delete[] bytes;
			return -1;
		}
		// One digit left over to read
		int digitBits = bits->readBits(4);
		if (digitBits >= 10) {
			ostringstream s;
			s << "Illegal value for digit unit: " << digitBits;
			delete[] bytes;
			return -1;
		}
		bytes[i++] = ALPHANUMERIC_CHARS[digitBits];
	}
	int ret;
	if ((ret = append(result, bytes, nBytes, StringUtils::ASCII)) < 0) {
		delete[] bytes;
		return ret;
	}
	delete[] bytes;
	return 0;
}

char DecodedBitStreamParser::toAlphaNumericChar(size_t value)
{
	if (value >= sizeof(DecodedBitStreamParser::ALPHANUMERIC_CHARS)) {
		return -1;
	}
	return ALPHANUMERIC_CHARS[value];
}

int DecodedBitStreamParser::decodeAlphanumericSegment(Ref<BitSource> bits_,
	string &result,
	int count,
	bool fc1InEffect)
{
	BitSource &bits(*bits_);
	ostringstream bytes;
	// Read two characters at a time
	while (count > 1) {
		if (bits.available() < 11) {
			return -1;
		}
		int nextTwoCharsBits = bits.readBits(11);
		bytes << toAlphaNumericChar(nextTwoCharsBits / 45);
		bytes << toAlphaNumericChar(nextTwoCharsBits % 45);
		count -= 2;
	}
	if (count == 1) {
		// special case: one character left
		if (bits.available() < 6) {
			return -1;
		}
		bytes << toAlphaNumericChar(bits.readBits(6));
	}
	// See section 6.4.8.1, 6.4.8.2
	string s = bytes.str();
	if (fc1InEffect) {
		// We need to massage the result a bit if in an FNC1 mode:
		ostringstream r;
		for (size_t i = 0; i < s.length(); i++) {
			if (s[i] != '%') {
				r << s[i];
			}
			else {
				if (i < s.length() - 1 && s[i + 1] == '%') {
					// %% is rendered as %
					r << s[i++];
				}
				else {
					// In alpha mode, % should be converted to FNC1 separator 0x1D
					r << (char)0x1D;
				}
			}
		}
		s = r.str();
	}
	int ret;
	if ((ret = append(result, s, StringUtils::ASCII)) < 0)
		return ret;
	return 0;
}

namespace {
int parseECIValue(BitSource &bits)
{
	int firstByte = bits.readBits(8);
	if ((firstByte & 0x80) == 0) {
		// just one byte
		return firstByte & 0x7F;
	}
	if ((firstByte & 0xC0) == 0x80) {
		// two bytes
		int secondByte = bits.readBits(8);
		return ((firstByte & 0x3F) << 8) | secondByte;
	}
	if ((firstByte & 0xE0) == 0xC0) {
		// three bytes
		int secondThirdBytes = bits.readBits(16);
		return ((firstByte & 0x1F) << 16) | secondThirdBytes;
	}
	return -1;
}
}

int
DecodedBitStreamParser::decode(ArrayRef<char> bytes,
	Version *version,
	ErrorCorrectionLevel const &ecLevel,
	Hashtable const &hints,
	Ref<DecoderResult> &rresult)
{
	Ref<BitSource> bits_(new BitSource(bytes));
	BitSource &bits(*bits_);
	string result;
	int ret;
	result.reserve(50);
	ArrayRef< ArrayRef<char> > byteSegments(0);
	try {
		CharacterSetECI *currentCharacterSetECI = 0;
		bool fc1InEffect = false;
		Mode *mode = 0;
		do {
			// While still another segment to read...
			if (bits.available() < 4) {
				// OK, assume we're done. Really, a TERMINATOR mode should have been recorded here
				mode = &Mode::TERMINATOR;
			}
			else {
				mode = &Mode::forBits(bits.readBits(4)); // mode is encoded by 4 bits
				if (mode == NULL) {
					return -1;
				}
			}
			if (mode != &Mode::TERMINATOR) {
				if ((mode == &Mode::FNC1_FIRST_POSITION) || (mode == &Mode::FNC1_SECOND_POSITION)) {
					// We do little with FNC1 except alter the parsed result a bit according to the spec
					fc1InEffect = true;
				}
				else if (mode == &Mode::STRUCTURED_APPEND) {
					if (bits.available() < 16) {
						return -1;
					}
					// not really supported; all we do is ignore it
					// Read next 8 bits (symbol sequence #) and 8 bits (parity data), then continue
					bits.readBits(16);
				}
				else if (mode == &Mode::ECI) {
					// Count doesn't apply to ECI
					int value = parseECIValue(bits);
					currentCharacterSetECI = CharacterSetECI::getCharacterSetECIByValue(value);
					if (currentCharacterSetECI == 0) {
						return -1;
					}
				}
				else {
					// First handle Hanzi mode which does not start with character count
					if (mode == &Mode::HANZI) {
						//chinese mode contains a sub set indicator right after mode indicator
						int subset = bits.readBits(4);
						int countHanzi = bits.readBits(mode->getCharacterCountBits(version));
						if (subset == GB2312_SUBSET) {
							if ((ret = decodeHanziSegment(bits_, result, countHanzi)) < 0)
								return ret;
						}
					}
					else {
						// "Normal" QR code modes:
						// How many characters will follow, encoded in this mode?
						int count = bits.readBits(mode->getCharacterCountBits(version));
						if (mode == &Mode::NUMERIC) {
							if ((ret = decodeNumericSegment(bits_, result, count)) < 0)
								return ret;
						}
						else if (mode == &Mode::ALPHANUMERIC) {
							if ((ret = decodeAlphanumericSegment(bits_, result, count, fc1InEffect)) < 0)
								return ret;
						}
						else if (mode == &Mode::BYTE) {
							if ((ret = decodeByteSegment(bits_, result, count, currentCharacterSetECI, byteSegments, hints)) < 0)
								return ret;
						}
						else if (mode == &Mode::KANJI) {
							if ((ret = decodeKanjiSegment(bits_, result, count)) < 0)
								return ret;
						}
						else {
							return -1;
						}
					}
				}
			}
		} while (mode != &Mode::TERMINATOR);
	}
	catch (IllegalArgumentException const &iae) {
		(void)iae;
		// from readBits() calls
		return -1;
	}

	rresult = new DecoderResult(bytes, Ref<String>(new String(result)), byteSegments, (string)ecLevel);
	return 0;
}
