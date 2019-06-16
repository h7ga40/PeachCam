// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
/*
 *  MonochromeRectangleDetector.cpp
 *  y_wmk
 *
 *  Created by Luiz Silva on 09/02/2010.
 *  Copyright 2010 y_wmk authors All rights reserved.
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

#include <zxing/NotFoundException.h>
#include <zxing/common/detector/MonochromeRectangleDetector.h>
#include <sstream>
#include <algorithm>

using std::vector;
using zxing::Ref;
using zxing::ResultPoint;
using zxing::TwoInts;
using zxing::MonochromeRectangleDetector;

int MonochromeRectangleDetector::detect(vector<Ref<ResultPoint>> &corners)
{
	int height = image_->getHeight();
	int width = image_->getWidth();
	int halfHeight = height / 2;
	int halfWidth = width / 2;
	int deltaY = std::max(1, height / (MAX_MODULES * 8));
	int deltaX = std::max(1, width / (MAX_MODULES * 8));

	int ret;
	int top = 0;
	int bottom = height;
	int left = 0;
	int right = width;
	Ref<ResultPoint> pointA;
	if ((ret = findCornerFromCenter(halfWidth, 0, left, right,
		halfHeight, -deltaY, top, bottom, halfWidth / 2, pointA)) < 0)
		return ret;
	top = (int)pointA->getY() - 1;;
	Ref<ResultPoint> pointB;
	if ((ret = findCornerFromCenter(halfWidth, -deltaX, left, right,
		halfHeight, 0, top, bottom, halfHeight / 2, pointB)) < 0)
		return ret;
	left = (int)pointB->getX() - 1;
	Ref<ResultPoint> pointC;
	if ((ret = findCornerFromCenter(halfWidth, deltaX, left, right,
		halfHeight, 0, top, bottom, halfHeight / 2, pointC)) < 0)
		return ret;
	right = (int)pointC->getX() + 1;
	Ref<ResultPoint> pointD;
	if ((ret = findCornerFromCenter(halfWidth, 0, left, right,
		halfHeight, deltaY, top, bottom, halfWidth / 2, pointD)) < 0)
		return ret;
	bottom = (int)pointD->getY() + 1;

	Ref<ResultPoint> pointAd;
	// Go try to find point A again with better information -- might have been off at first.
	if ((ret = findCornerFromCenter(halfWidth, 0, left, right,
		halfHeight, -deltaY, top, bottom, halfWidth / 4, pointAd)) < 0)
		return ret;
	pointA.reset(pointAd);

	corners = vector<Ref<ResultPoint>>(4);
	corners[0].reset(pointA);
	corners[1].reset(pointB);
	corners[2].reset(pointC);
	corners[3].reset(pointD);
	return 0;
}

int MonochromeRectangleDetector::findCornerFromCenter(int centerX, int deltaX, int left, int right,
	int centerY, int deltaY, int top, int bottom, int maxWhiteRun, Ref<ResultPoint> &result)
{
	Ref<TwoInts> lastRange(NULL);
	for (int y = centerY, x = centerX;
		y < bottom && y >= top && x < right && x >= left;
		y += deltaY, x += deltaX) {
		Ref<TwoInts> range(NULL);
		if (deltaX == 0) {
			// horizontal slices, up and down
			range = blackWhiteRange(y, maxWhiteRun, left, right, true);
		}
		else {
			// vertical slices, left and right
			range = blackWhiteRange(x, maxWhiteRun, top, bottom, false);
		}
		if (range == NULL) {
			if (lastRange == NULL) {
				return -1;
			}
			else {
				// lastRange was found
				if (deltaX == 0) {
					int lastY = y - deltaY;
					if (lastRange->start < centerX) {
						if (lastRange->end > centerX) {
							// straddle, choose one or the other based on direction
							result = new ResultPoint(deltaY > 0 ? lastRange->start : lastRange->end, lastY);
							return 0;
						}
						result = new ResultPoint(lastRange->start, lastY);
						return 0;
					}
					else {
						result = new ResultPoint(lastRange->end, lastY);
						return 0;
					}
				}
				else {
					int lastX = x - deltaX;
					if (lastRange->start < centerY) {
						if (lastRange->end > centerY) {
							result = new ResultPoint(lastX, deltaX < 0 ? lastRange->start : lastRange->end);
							return 0;
						}
						result = new ResultPoint(lastX, lastRange->start);
						return 0;
					}
					else {
						result = new ResultPoint(lastX, lastRange->end);
						return 0;
					}
				}
			}
		}
		lastRange = range;
	}
	return -1;
}

Ref<TwoInts> MonochromeRectangleDetector::blackWhiteRange(int fixedDimension, int maxWhiteRun, int minDim, int maxDim,
	bool horizontal)
{

	int center = (minDim + maxDim) / 2;

	// Scan left/up first
	int start = center;
	while (start >= minDim) {
		if (horizontal ? image_->get(start, fixedDimension) : image_->get(fixedDimension, start)) {
			start--;
		}
		else {
			int whiteRunStart = start;
			do {
				start--;
			} while (start >= minDim && !(horizontal ? image_->get(start, fixedDimension) :
				image_->get(fixedDimension, start)));
			int whiteRunSize = whiteRunStart - start;
			if (start < minDim || whiteRunSize > maxWhiteRun) {
				start = whiteRunStart;
				break;
			}
		}
	}
	start++;

	// Then try right/down
	int end = center;
	while (end < maxDim) {
		if (horizontal ? image_->get(end, fixedDimension) : image_->get(fixedDimension, end)) {
			end++;
		}
		else {
			int whiteRunStart = end;
			do {
				end++;
			} while (end < maxDim && !(horizontal ? image_->get(end, fixedDimension) :
				image_->get(fixedDimension, end)));
			int whiteRunSize = end - whiteRunStart;
			if (end >= maxDim || whiteRunSize > maxWhiteRun) {
				end = whiteRunStart;
				break;
			}
		}
	}
	end--;
	Ref<TwoInts> result(NULL);
	if (end > start) {
		result = new TwoInts;
		result->start = start;
		result->end = end;
	}
	return result;
}
