/*
 *  Copyright 2010-2011 ZXing authors
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

#include "ImageReaderSource.h"
#include <zxing/common/IllegalArgumentException.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <algorithm>


using std::string;
using std::ostringstream;
using zxing::Ref;
using zxing::ArrayRef;
using zxing::LuminanceSource;

#define INPUTIMG_RGB888    (0)    /* 1:RGB888, 0:YCbCr422 */

inline char ImageReaderSource::convertPixel(char const *pixel_) const
{
	unsigned char const *pixel = (unsigned char const *)pixel_;
	if (comps == 1 || comps == 2) {
		// Gray or gray+alpha
		return pixel[0];
	} if (comps == 3 || comps == 4) {
		// Red, Green, Blue, (Alpha)
		// We assume 16 bit values here
		// 0x200 = 1<<9, half an lsb of the result to force rounding
		return (char)((306 * (int)pixel[0] + 601 * (int)pixel[1] +
			117 * (int)pixel[2] + 0x200) >> 10);
	}
	else {
		return -1;
	}
}

ImageReaderSource::ImageReaderSource(ArrayRef<char> image_, int width, int height, int comps_)
	: Super(width, height), image(image_), comps(comps_)
{
}

int ImageReaderSource::create(char *buf, int buf_size, int width, int height, Ref<LuminanceSource> &result)
{
#if INPUTIMG_RGB888    //1:RGB888 0:YCBCR422
	int comps = 4;
	zxing::ArrayRef<char> image;

	image = zxing::ArrayRef<char>(4 * width * height);
	memcpy(&image[0], &buf[0], buf_size);
	if (!image) {
		ostringstream msg;
		msg << "Loading failed.";
		return -1;
	}
#else
	int comps = 1;
	zxing::ArrayRef<char> image;
	char *sourceadr;

	image = zxing::ArrayRef<char>(width * height);
	char *srcimg_adr = &image[0];
	int cnt_source_x, cnt_source_y, cnt_target;
	int cnt_edgh1 = 0;
	int cnt_edgh2 = 0;
	for (cnt_source_y = 0; cnt_source_y < height; cnt_source_y++) {
		for (cnt_source_x = 0, cnt_target = cnt_source_y * width; cnt_source_x < width * 2; cnt_source_x += 8, cnt_target += 4)
		{
			srcimg_adr[cnt_target] = buf[cnt_source_y * width * 2 + cnt_source_x + 4];
			srcimg_adr[cnt_target + 1] = buf[cnt_source_y * width * 2 + cnt_source_x + 6];
			srcimg_adr[cnt_target + 2] = buf[cnt_source_y * width * 2 + cnt_source_x];
			srcimg_adr[cnt_target + 3] = buf[cnt_source_y * width * 2 + cnt_source_x + 2];
		}
	}
#endif

	result = new ImageReaderSource(image, width, height, comps);
	return 0;
}

int ImageReaderSource::getRow(int y, zxing::ArrayRef<char> row, zxing::ArrayRef<char> &result) const
{
#if INPUTIMG_RGB888    //1:RGB888 0:YCBCR422
	const char *pixelRow = &image[0] + y * getWidth() * 4;
	if (!row) {
		row = zxing::ArrayRef<char>(getWidth());
	}
	for (int x = 0; x < getWidth(); x++) {
		row[x] = convertPixel(pixelRow + (x * 4));
	}
#else
	const char *pixelRow = &image[0] + y * getWidth();
	if (!row) {
		row = zxing::ArrayRef<char>(getWidth());
	}
	for (int x = 0; x < getWidth(); x++) {
		row[x] = pixelRow[x];
	}
#endif
	result = row;
	return 0;
}

/** This is a more efficient implementation. */
zxing::ArrayRef<char> ImageReaderSource::getMatrix() const
{
#if INPUTIMG_RGB888  //1:RGB888 0:YCBCR422
	const char *p = &image[0];
	zxing::ArrayRef<char> matrix(getWidth() * getHeight());
	char *m = &matrix[0];
	for (int y = 0; y < getHeight(); y++) {
		for (int x = 0; x < getWidth(); x++) {
			*m = convertPixel(p);
			m++;
			p += 4;
		}
	}
	return matrix;
#else
	return image;
#endif
}

int decode(Ref<BinaryBitmap> image, DecodeHints hints, vector<Ref<Result>> &results)
{
	Ref<Reader> reader(new MultiFormatReader);
	int ret;
	Ref<Result> result;
	if ((ret = reader->decode(image, hints, result)) < 0)
		return ret;
	results.push_back(result);
	return 0;
}

int decode_image(Ref<LuminanceSource> source, bool hybrid, vector<Ref<Result>> *results, DecodeHints &hints)
{
	string cell_result;
	int ret;

	Ref<Binarizer> binarizer;
	if (hybrid) {
		binarizer = new HybridBinarizer(source);
	}
	else {
		binarizer = new GlobalHistogramBinarizer(source);
	}
	Ref<BinaryBitmap> binary(new BinaryBitmap(binarizer));

	if ((ret = decode(binary, hints, *results)) < 0)
		return ret;

	return 0;
}

int ex_decode(uint8_t *buf, int buf_size, int width, int height, vector<Ref<Result>> *results, DecodeHints &hints)
{
	int h_result = 1;
	int result = 0;
	Ref<LuminanceSource> source;

	int ret;
	ret = ImageReaderSource::create((char *)buf, buf_size, width, height, source);
	if (ret < 0) {
		cerr << ret << " (ignoring)" << endl;
	}

	h_result = decode_image(source, false, results, hints);
	if (h_result != 0) {
		result = -1;
	}

	return result;
}

