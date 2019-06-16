// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
#ifndef __UPC_EAN_READER_H__
#define __UPC_EAN_READER_H__

/*
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

#include <zxing/oned/OneDReader.h>
#include <zxing/common/BitArray.h>
#include <zxing/Result.h>

namespace zxing {
namespace oned {

class UPCEANReader : public OneDReader {
private:
	std::string decodeRowStringBuffer;
	// UPCEANExtensionSupport extensionReader;
	// EANManufacturerOrgSupport eanManSupport;

	static const float MAX_AVG_VARIANCE;
	static const float MAX_INDIVIDUAL_VARIANCE;

	static int findStartGuardPattern(Ref<BitArray> row, Range &result);

	virtual int decodeEnd(Ref<BitArray> row, int endStart, Range &result);

	static bool checkStandardUPCEANChecksum(Ref<String> const &s);
	static int  getStandardUPCEANChecksum(const std::string &s);

	static int findGuardPattern(Ref<BitArray> row,
		int rowOffset,
		bool whiteFirst,
		std::vector<int> const &pattern,
		std::vector<int> &counters,
		Range &result);


protected:
	static const std::vector<int> START_END_PATTERN;
	static const std::vector<int> MIDDLE_PATTERN;

	static const std::vector<int const *> L_PATTERNS;
	static const std::vector<int const *> L_AND_G_PATTERNS;

	static int findGuardPattern(Ref<BitArray> row,
		int rowOffset,
		bool whiteFirst,
		std::vector<int> const &pattern,
		Range &result);

public:
	UPCEANReader();

	virtual int decodeMiddle(Ref<BitArray> row,
		Range const &startRange,
		std::string &resultString) = 0;

	virtual int decodeRow(int rowNumber, Ref<BitArray> row, Ref<Result> &result);
	virtual int decodeRow(int rowNumber, Ref<BitArray> row, Range const &range, Ref<Result> &result);

	static int decodeDigit(Ref<BitArray> row,
		std::vector<int> &counters,
		int rowOffset,
		std::vector<int const *> const &patterns);

	virtual bool checkChecksum(Ref<String> const &s);

	virtual BarcodeFormat getBarcodeFormat() = 0;
	virtual ~UPCEANReader();

	friend class MultiFormatUPCEANReader;
};

}
}

#endif
