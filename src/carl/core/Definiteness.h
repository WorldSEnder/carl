/** 
 * @file   Definiteness.h
 * @author Sebastian Junges
 * @author Florian Corzilius
 */

#pragma once

namespace carl
{
	/**
	 * Regarding a polynomial \f$p\f$ as a function \f$p: X \rightarrow Y\f$, its definiteness gives information about the codomain \f$Y\f$.
	 */
    enum class Definiteness {
		/// Indicates that \f$y < 0 \forall y \in Y\f$.
		NEGATIVE = 0,
		/// Indicates that \f$y \leq 0 \forall y \in Y\f$.
		NEGATIVE_SEMI = 1,
		/// Indicates that values may be positive and negative.
		NON = 2,
		/// Indicates that \f$y \geq 0 \forall y \in Y\f$.
		POSITIVE_SEMI = 3,
		/// Indicates that \f$y > 0 \forall y \in Y\f$.
		POSITIVE = 4
	};
}
