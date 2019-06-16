// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
/*
 *  Created by Christian Brunschen on 05/05/2008.
 *  Copyright 2008 Google UK. All rights reserved.
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

#include <iostream>

#include <memory>
#include <zxing/common/reedsolomon/ReedSolomonDecoder.h>
#include <zxing/common/reedsolomon/ReedSolomonException.h>
#include <zxing/common/IllegalArgumentException.h>
#include <zxing/IllegalStateException.h>

using std::vector;
using zxing::Ref;
using zxing::ArrayRef;
using zxing::ReedSolomonDecoder;
using zxing::GenericGFPoly;
using zxing::IllegalStateException;

// VC++
using zxing::GenericGF;

ReedSolomonDecoder::ReedSolomonDecoder(Ref<GenericGF> field_) : field(field_) {}

ReedSolomonDecoder::~ReedSolomonDecoder()
{
}

int ReedSolomonDecoder::decode(ArrayRef<int> received, int twoS)
{
	Ref<GenericGFPoly> poly(new GenericGFPoly(*field, received));
	if (!poly->IsActive())
		return -1;
	ArrayRef<int> syndromeCoefficients(twoS);
	bool noError = true;
	for (int i = 0; i < twoS; i++) {
		int eval = poly->evaluateAt(field->exp(i + field->getGeneratorBase()));
		syndromeCoefficients[syndromeCoefficients->size() - 1 - i] = eval;
		if (eval != 0) {
			noError = false;
		}
	}
	if (noError) {
		return 0;
	}

	int ret;
	Ref<GenericGFPoly> syndrome(new GenericGFPoly(*field, syndromeCoefficients));
	if (!syndrome->IsActive())
		return -1;
	Ref<GenericGFPoly> pory;
	if ((ret = field->buildMonomial(twoS, 1, pory)) < 0)
		return ret;
	vector<Ref<GenericGFPoly> > sigmaOmega;
	if ((ret = runEuclideanAlgorithm(pory, syndrome, twoS, sigmaOmega)) < 0)
		return ret;
	Ref<GenericGFPoly> sigma = sigmaOmega[0];
	Ref<GenericGFPoly> omega = sigmaOmega[1];
	ArrayRef<int> errorLocations;
	if ((ret = findErrorLocations(sigma, errorLocations)) < 0)
		return ret;
	ArrayRef<int> errorMagitudes = findErrorMagnitudes(omega, errorLocations);
	for (int i = 0; i < errorLocations->size(); i++) {
		int position = received->size() - 1 - field->log(errorLocations[i]);
		if (position < 0) {
			return -1;
		}
		received[position] = GenericGF::addOrSubtract(received[position], errorMagitudes[i]);
	}
	return 0;
}

int ReedSolomonDecoder::runEuclideanAlgorithm(Ref<GenericGFPoly> a,
	Ref<GenericGFPoly> b, int R, vector<Ref<GenericGFPoly>> &result)
{
	// Assume a's degree is >= b's
	if (a->getDegree() < b->getDegree()) {
		Ref<GenericGFPoly> tmp = a;
		a = b;
		b = tmp;
	}

	Ref<GenericGFPoly> rLast(a);
	Ref<GenericGFPoly> r(b);
	Ref<GenericGFPoly> tLast(field->getZero());
	Ref<GenericGFPoly> t(field->getOne());

	// Run Euclidean algorithm until r's degree is less than R/2
	while (r->getDegree() >= R / 2) {
		Ref<GenericGFPoly> rLastLast(rLast);
		Ref<GenericGFPoly> tLastLast(tLast);
		rLast = r;
		tLast = t;

		// Divide rLastLast by rLast, with quotient q and remainder r
		if (rLast->isZero()) {
		  // Oops, Euclidean algorithm already terminated?
			return -1;
		}
		r = rLastLast;
		Ref<GenericGFPoly> q = field->getZero();
		int denominatorLeadingTerm = rLast->getCoefficient(rLast->getDegree());
		int dltInverse = field->inverse(denominatorLeadingTerm);
		while (r->getDegree() >= rLast->getDegree() && !r->isZero()) {
			int degreeDiff = r->getDegree() - rLast->getDegree();
			int scale = field->multiply(r->getCoefficient(r->getDegree()), dltInverse);
			int ret;
			Ref<GenericGFPoly> pory;
			if ((ret = field->buildMonomial(degreeDiff, scale, pory)) < 0)
				return ret;
			q = q->addOrSubtract(pory);
			r = r->addOrSubtract(rLast->multiplyByMonomial(degreeDiff, scale));
		}

		t = q->multiply(tLast)->addOrSubtract(tLastLast);

		if (r->getDegree() >= rLast->getDegree()) {
			return -1;
		}
	}

	int sigmaTildeAtZero = t->getCoefficient(0);
	if (sigmaTildeAtZero == 0) {
		return -1;
	}

	int inverse = field->inverse(sigmaTildeAtZero);
	Ref<GenericGFPoly> sigma(t->multiply(inverse));
	Ref<GenericGFPoly> omega(r->multiply(inverse));
	result = vector<Ref<GenericGFPoly>>(2);
	result[0] = sigma;
	result[1] = omega;
	return 0;
}

int ReedSolomonDecoder::findErrorLocations(Ref<GenericGFPoly> errorLocator, ArrayRef<int> &result)
{
// This is a direct application of Chien's search
	int numErrors = errorLocator->getDegree();
	if (numErrors == 1) { // shortcut
		result = new Array<int>(1);
		result[0] = errorLocator->getCoefficient(1);
		return 0;
	}
	result = new Array<int>(numErrors);
	int e = 0;
	for (int i = 1; i < field->getSize() && e < numErrors; i++) {
		if (errorLocator->evaluateAt(i) == 0) {
			result[e] = field->inverse(i);
			e++;
		}
	}
	if (e != numErrors) {
		return -1;
	}
	return 0;
}

ArrayRef<int> ReedSolomonDecoder::findErrorMagnitudes(Ref<GenericGFPoly> errorEvaluator, ArrayRef<int> errorLocations)
{
	// This is directly applying Forney's Formula
	int s = errorLocations->size();
	ArrayRef<int> result(new Array<int>(s));
	for (int i = 0; i < s; i++) {
		int xiInverse = field->inverse(errorLocations[i]);
		int denominator = 1;
		for (int j = 0; j < s; j++) {
			if (i != j) {
				int term = field->multiply(errorLocations[j], xiInverse);
				int termPlus1 = (term & 0x1) == 0 ? term | 1 : term & ~1;
				denominator = field->multiply(denominator, termPlus1);
			}
		}
		result[i] = field->multiply(errorEvaluator->evaluateAt(xiInverse),
			field->inverse(denominator));
		if (field->getGeneratorBase() != 0) {
			result[i] = field->multiply(result[i], xiInverse);
		}
	}
	return result;
}
