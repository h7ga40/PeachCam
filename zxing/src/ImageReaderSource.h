// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
#ifndef __IMAGE_READER_SOURCE_H_
#define __IMAGE_READER_SOURCE_H_
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

#include "mbed.h"
#include <iostream>
#include <fstream>
#include <string>
#include <zxing/LuminanceSource.h>
#include <zxing/common/Counted.h>
#include <zxing/Binarizer.h>
#include <zxing/MultiFormatReader.h>
#include <zxing/Result.h>
#include <zxing/ReaderException.h>
#include <zxing/common/GlobalHistogramBinarizer.h>
#include <zxing/common/HybridBinarizer.h>
#include <exception>
#include <zxing/Exception.h>
#include <zxing/common/IllegalArgumentException.h>
#include <zxing/BinaryBitmap.h>
#include <zxing/DecodeHints.h>

#include <zxing/qrcode/QRCodeReader.h>
#include <zxing/multi/qrcode/QRCodeMultiReader.h>
#include <zxing/multi/ByQuadrantReader.h>
#include <zxing/multi/MultipleBarcodeReader.h>
#include <zxing/multi/GenericMultipleBarcodeReader.h>

using std::string;
using std::ostringstream;
using zxing::Ref;
using zxing::ArrayRef;
using zxing::LuminanceSource;

using namespace std;
using namespace zxing;
using namespace zxing::multi;
using namespace zxing::qrcode;


class ImageReaderSource : public zxing::LuminanceSource {
private:
	typedef LuminanceSource Super;

	const zxing::ArrayRef<char> image;
	const int comps;

	char convertPixel(const char *pixel) const;

public:
	static int create(char *buf, int buf_size, int width, int height, zxing::Ref<LuminanceSource> &result);

	ImageReaderSource(zxing::ArrayRef<char> image, int width, int height, int comps);

	int getRow(int y, zxing::ArrayRef<char> row, zxing::ArrayRef<char> &result) const;
	zxing::ArrayRef<char> getMatrix() const;
};

extern int ex_decode(uint8_t *buf, int buf_size, int width, int height, vector<Ref<Result> > *results, DecodeHints &hints);


#endif /* __IMAGE_READER_SOURCE_H_ */
