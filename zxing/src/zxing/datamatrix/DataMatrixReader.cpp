// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
/*
 *  DataMatrixReader.cpp
 *  zxing
 *
 *  Created by Luiz Silva on 09/02/2010.
 *  Copyright 2010 ZXing authors All rights reserved.
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

#include <zxing/datamatrix/DataMatrixReader.h>
#include <zxing/datamatrix/detector/Detector.h>
#include <iostream>

namespace zxing {
namespace datamatrix {

using namespace std;

DataMatrixReader::DataMatrixReader() :
	decoder_() {
}

int DataMatrixReader::decode(Ref<BinaryBitmap> image, DecodeHints hints, Ref<Result> &result) {
	(void)hints;
	int ret;
	Ref<BitMatrix> matrix;
	if ((ret = image->getBlackMatrix(matrix)) < 0)
		return ret;
	Detector detector(matrix);
	Ref<DetectorResult> detectorResult;
	if ((ret = detector.detect(detectorResult)) < 0)
		return ret;
	ArrayRef< Ref<ResultPoint> > points(detectorResult->getPoints());


	Ref<DecoderResult> decoderResult;
	if ((ret = decoder_.decode(detectorResult->getBits(), decoderResult)) < 0)
		return ret;

	result = new Result(decoderResult->getText(), decoderResult->getRawBytes(), points, BarcodeFormat::DATA_MATRIX);

	return 0;
}

DataMatrixReader::~DataMatrixReader() {
}

}
}
