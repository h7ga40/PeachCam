/*
 *  Copyright 2011 ZXing authors
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

#include <zxing/multi/qrcode/detector/MultiDetector.h>
#include <zxing/multi/qrcode/detector/MultiFinderPatternFinder.h>
#include <zxing/ReaderException.h>

namespace zxing {
namespace multi {
using namespace zxing::qrcode;

MultiDetector::MultiDetector(Ref<BitMatrix> image) : Detector(image) {}

MultiDetector::~MultiDetector() {}

int MultiDetector::detectMulti(DecodeHints hints, std::vector<Ref<DetectorResult>> &results)
{
	Ref<BitMatrix> image = getImage();
	MultiFinderPatternFinder finder = MultiFinderPatternFinder(image, hints.getResultPointCallback());
	int ret;
	std::vector<Ref<FinderPatternInfo> > info;
	if ((ret = finder.findMulti(hints, info)) < 0)
		return ret;
	for (unsigned int i = 0; i < info.size(); i++) {
		int ret;
		Ref<DetectorResult> result;
		if ((ret = processFinderPatternInfo(info[i], result)) == 0) {
			results.push_back(result);
		}
	}

	return 0;
}

} // End zxing::multi namespace
} // End zxing namespace
