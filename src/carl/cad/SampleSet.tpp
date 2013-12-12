/* 
 * File:   SampleSet.tpp
 * Author: Gereon Kremer <gereon.kremer@cs.rwth-aachen.de>
 */

#pragma once

#include "SampleSet.h"

#include <cassert>

#include "../core/RealAlgebraicNumber.h"

namespace carl {
namespace CAD {

template<typename Number>
std::pair<typename SampleSet<Number>::iterator, bool> SampleSet<Number>::insert(const RealAlgebraicNumber<Number>* r) {
	if (r->isNumeric()) {
		RealAlgebraicNumberNR<Number>* rNR = new RealAlgebraicNumberNR<Number>(r->value(), r->isRoot());
		iterator position = this->samples.begin();
		if (! this->samples.empty()) {
			position = std::lower_bound(position, this->samples.end(), rNR, carl::less);
			if (position != this->samples.end() && carl::Equal<Number>()(*position, rNR)) { // already contained in the list
				return std::pair<iterator, bool>(position, false);    // return iterator to the already contained element
			}
			// else: append r to the end of the list
		}
		this->NRqueue.push_back(rNR);
		if (rNR->isRoot()) {
			this->rootQueue.push_back(rNR);
		} else {
			this->nonRootQueue.push_back(rNR);
		}
		this->queue.push_back(rNR);
		return std::pair<iterator, bool>(this->samples.insert(position,rNR), true);    // insert safely and return iterator to the new element
	}
}

template<typename Number>
typename SampleSet<Number>::iterator SampleSet<Number>::remove(typename SampleSet<Number>::iterator position) {
	assert(position != this->samples.end());
	this->removeFromQueue(*position);
	this->removeFromNRIR(*position);
	this->removeFromNonrootRoot(*position);
	return this->samples.erase(position);
}

template<typename Number>
inline const RealAlgebraicNumber<Number>* SampleSet<Number>::next() {
	if (this->samples.empty()) assert(false);
	return this->queue.front();
}

template<typename Number>
inline const RealAlgebraicNumber<Number>* SampleSet<Number>::nextNR() {
	if (this->samples.empty()) assert(false);
	if (this->NRqueue.empty()) return this->IRqueue.front();
	return this->NRqueue.front();
}

template<typename Number>
inline const RealAlgebraicNumber<Number>* SampleSet<Number>::nextNonRoot() {
	if (this->samples.empty()) assert(false);
	if (this->nonRootQueue.empty()) return this->rootQueue.front();
	return this->nonRootQueue.front();
}

template<typename Number>
inline const RealAlgebraicNumber<Number>* SampleSet<Number>::nextRoot() {
	if (this->samples.empty()) assert(false);
	if (this->rootQueue.empty()) return this->nonRootQueue.front();
	return this->rootQueue.front();
}

template<typename Number>
void SampleSet<Number>::pop() {
	if (this->samples.empty()) return;
	
	const RealAlgebraicNumber<Number>* r = this->next();
	//iterator position = std::lower_bound(this->samples.begin(), this->samples.end(), r, (bool(const RealAlgebraicNumber<Number>*,const RealAlgebraicNumber<Number>*))(carl::template less<Number>));
	iterator position = std::lower_bound(this->samples.begin(), this->samples.end(), r, carl::Less<Number>());
	//iterator position = this->samples.begin();
	
	assert(position != this->samples.end()); // r should be in this list
	this->samples.erase(position); // remove next()

	this->queue.pop_front();
	this->removeFromNRIR(r);
	this->removeFromNonrootRoot(r);
}

template<typename Number>
void SampleSet<Number>::popNR() {
	if (this->samples.empty()) return;
	
	RealAlgebraicNumber<Number>* r = this->nextNR();
	iterator position = std::lower_bound(this->samples.begin(), this->samples.end(), r, carl::less);
	
	assert(position != this->samples.end());
	this->samples.erase(position); // remove nextNR()
	
	// remove next also from its bucket
	if (this->NRqueue.empty()) {
		this->IRqueue.pop_front();
	} else {
		this->NRqueue.pop_front();
	}
	
	this->removeFromQueue(r);
	this->removeFromNonrootRoot(r);
}

template<typename Number>
void SampleSet<Number>::popNonroot() {
	if (this->samples.empty()) return;
	
	RealAlgebraicNumber<Number>* r = this->nextNonRoot();
	iterator position = std::lower_bound(this->samples.begin(), this->samples.end(), r, carl::less);
	
	assert(position != this->samples.end());
	this->samples.erase(position); // remove nextNonRoot()
	
	// remove next from its bucket
	if (this->nonRootQueue.empty()) {
		this->rootQueue.pop_front();
	} else {
		this->nonRootQueue.pop_front();
	}
	
	this->removeFromNRIR(r);
	this->removeFromQueue(r);
}

template<typename Number>
void SampleSet<Number>::popRoot() {
	if (this->samples.empty()) return;
	
	RealAlgebraicNumber<Number>* r = this->nextRoot();
	iterator position = std::lower_bound(this->samples.begin(), this->samples.end(), r, carl::less);

	assert(position != this->samples.end());
	this->samples.erase(position); // remove nextRoot()
	
	// remove next from its bucket
	if (this->rootQueue.empty()) {
		this->nonRootQueue.pop_front();
	} else {
		this->rootQueue.pop_front();
	}
	
	this->removeFromNRIR(r);
	this->removeFromQueue(r);
}

template<typename Number>
bool SampleSet<Number>::simplify(const RealAlgebraicNumberIR<Number>* from, const RealAlgebraicNumberNR<Number>* to) {
	iteratorIR position = std::find(this->IRqueue.begin(), this->IRqueue.end(), from);
	if (position != this->IRqueue.end()) {
		return this->simplify(from, to, position);
	}
	return false;
}

template<typename Number>
bool SampleSet<Number>::simplify(const RealAlgebraicNumberIR<Number>* from, const RealAlgebraicNumberNR<Number>* to, SampleSet<Number>::iteratorIR& fromIt ) {
	assert(from->isRoot() == to->isRoot());

	// replace in basic list
	iterator position = std::lower_bound(this->samples.begin(), this->samples.end(), from, carl::less);
	if (position == this->samples.end()) return false;
	*position = to; // replace ir by nr
	
	// add to NRs
	this->NRqueue.push_back(to);
	// erase in IRs
	fromIt = this->IRqueue.erase(fromIt);
	// replace in root/non-root lists
	if (from->isRoot()) {
		iterator position = std::find(this->rootQueue.begin(), this->rootQueue.end(), from);
		// there must be an occurrence in the sample list or there was an error inserting the number
		assert(position != this->rootQueue.end());
		*position = to;
	} else {
		iterator position = std::find(this->nonRootQueue.begin(), this->nonRootQueue.end(), from);
		assert(position != this->nonRootQueue.end());    // there must be an occurrence in the sample list or there was an error inserting the number
		*position = to;
	}
	// replace in queue
	iterator queuePosition = std::find(this->queue.begin(), this->queue.end(), from);
	assert(queuePosition != this->queue.end());    // there must be an occurrence in the sample list or there was an error inserting the number
	*queuePosition = to;
	return true;
}

template<typename Number>
std::pair<typename SampleSet<Number>::SampleSimplification, bool> SampleSet<Number>::simplify() {
	std::pair<SampleSimplification, bool> simplification;
	simplification.second = false;
	for (iteratorIR irIter = this->IRqueue.begin(); irIter != this->IRqueue.end(); irIter++) {
		if (!(*irIter)->isNumeric() && (*irIter)->refinementCount() == 0) {// try at least one refinement
			(*irIter)->refine();
		}
		if ((*irIter)->isNumeric()) {
			RealAlgebraicNumberNR<Number>* nr = new RealAlgebraicNumberNR<Number>((*irIter)->value(), (*irIter)->isRoot());
			if (this->simplify(*irIter, nr, irIter)) { // store simplification result
				simplification.first[*irIter] = nr;
				simplification.second = true;
			} else {
				assert( false );
			}
		} else { // try to maximally coarsen the interval
			irIter++;
		}
	}
	return simplification;
}

template<typename Number>
bool SampleSet<Number>::contains(const RealAlgebraicNumber<Number>* r) const {
	auto pos = std::lower_bound(this->samples.begin(), this->samples.end(), r, carl::less);
	return pos != this->samples.end();
}

template<typename Number>
std::ostream& operator<<(std::ostream& os, const SampleSet<Number>& s) {
	for (auto sample: s.samples) {
		os << *sample << "  ";
	}
	return os;
}

template<typename Number>
void SampleSet<Number>::removeFromNonrootRoot(const RealAlgebraicNumber<Number>* r) {
	if (r->isRoot()) {
		iterator pos = std::find(this->rootQueue.begin(), this->rootQueue.end(), r); // find in roots non-root list
		assert(pos != this->rootQueue.end());
		this->rootQueue.erase(pos);
	} else {
		iterator pos = std::find(this->nonRootQueue.begin(), this->nonRootQueue.end(), r); // find in roots non-root list
		assert(pos != this->nonRootQueue.end());
		this->nonRootQueue.erase(pos);
	}
}

template<typename Number>
void SampleSet<Number>::removeFromQueue(const RealAlgebraicNumber<Number>* r) {
	iterator pos = std::find(this->queue.begin(), this->queue.end(), r); // find in roots non-root list
	assert(pos != this->queue.end());
	this->queue.erase(pos);
}

template<typename Number>
void SampleSet<Number>::removeFromNRIR(const RealAlgebraicNumber<Number>* r) {
	if (r->isNumeric()) {
		const RealAlgebraicNumberNR<Number>* rNR = static_cast<const RealAlgebraicNumberNR<Number>*>(r); // needs to be a dynamic cast here in order to determine the correct type always
		iteratorNR pos = std::find(this->NRqueue.begin(), this->NRqueue.end(), rNR);
		assert(pos != this->NRqueue.end()); // r should be in this list, otherwise it was maybe simplified and moved to the other list
		this->NRqueue.erase(pos);
	} else {
		const RealAlgebraicNumberIR<Number>* rIR = static_cast<const RealAlgebraicNumberIR<Number>*>(r); // needs to be a dynamic cast here in order to determine the correct type always
		iteratorIR pos = std::find(this->IRqueue.begin(), this->IRqueue.end(), rIR);
		assert(pos != this->IRqueue.end()); // r should be in this list
		this->IRqueue.erase(pos);
	}
}

}
}