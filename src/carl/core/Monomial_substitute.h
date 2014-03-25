/**
 * @file Monomial_substitute.h
 * @ingroup multirp
 * @author Sebastian Junges
 */

#pragma once

#include "Monomial.h"
#include "Term.h"
#include "../numbers/numbers.h"

namespace carl
{
template<typename Coefficient, typename SubstitutionType>
Term<Coefficient>* Monomial::substitute(const std::map<Variable,SubstitutionType>& substitutions, Coefficient factor) const
{
	Monomial* m = new Monomial();
	m->mTotalDegree = mTotalDegree;
	for(const VarExpPair& ve : mExponents) 
	{
		typename std::map<Variable,SubstitutionType>::const_iterator it = substitutions.find(ve.var);
		if(it == substitutions.end())
		{
			m->mExponents.push_back(ve);
		}
		else
		{
			factor *= carl::pow(it->second, ve.exp);
			m->mTotalDegree -= ve.exp;
		}
	}
	if(m->mTotalDegree == 0)
	{
		assert(m->mExponents.size() == 0);
		delete m;
		return new Term<Coefficient>(factor);
	}
	return new Term<Coefficient>(factor, std::shared_ptr<const Monomial>(m));	
}

template<typename Coefficient>
Term<Coefficient>* Monomial::substitute(const std::map<Variable,Term<Coefficient>>& substitutions, const Coefficient&  coeff) const
{
	Monomial m;
	m.mTotalDegree = mTotalDegree;
	Term<Coefficient> factor(1);
	for(const VarExpPair& ve : mExponents) 
	{
		typename std::map<Variable,Term<Coefficient>>::const_iterator it = substitutions.find(ve.var);
		if(it == substitutions.end())
		{
			m.mExponents.push_back(ve);
		}
		else
		{
			Term<Coefficient>* power = it->second.pow(ve.exp);
			factor *= *power;
			delete power;
			m.mTotalDegree -= ve.exp;
		}
	}
	if(m.mTotalDegree == 0)
	{
		assert(m.mExponents.size() == 0);
		return new Term<Coefficient>(coeff * factor.coeff());
	}
	
	if(factor.monomial())
	{
		return new Term<Coefficient>(coeff * factor.coeff(),m * *factor.monomial());	
	}
	else
	{
		return new Term<Coefficient>(coeff * factor.coeff(),m);	
	}
}


}