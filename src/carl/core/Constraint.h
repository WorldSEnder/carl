/** 
 * @file core/Constraint.h
 * @author Sebastian Junges
 * @ingroup constraints
 * @since April 4, 2014
 */

#pragma once
#include "CompareRelation.h"
namespace carl
{

template<typename LhsType>
class Constraint
{
	public:
		Constraint(bool v) : mLhs(v ? 0 : 1), mRelation(CompareRelation::EQ) {}
		Constraint(const LhsType& lhs, CompareRelation rel) : mLhs(lhs), mRelation(rel)
		{}
	
		const LhsType& lhs() const {return mLhs;}
		const CompareRelation& rel() const {return mRelation;}
		
		bool isTrivialTrue() {
			if(mLhs.isConstant())
			{
				auto cp = mLhs.constantPart();
				if(cp == 0) {
					return !relationIsStrict(mRelation);
				} else if(cp > 0) {
					return mRelation == CompareRelation::GEQ || mRelation == CompareRelation::GT || mRelation == CompareRelation::NEQ;
				} else {
					assert(cp < 0);
					return mRelation == CompareRelation::LEQ || mRelation == CompareRelation::LT || mRelation == CompareRelation::NEQ;
				}
			}
			return false;
		}
		
		bool isTrivialFalse() {
			if(mLhs.isConstant())
			{
				auto cp = mLhs.constantPart();
				if(cp == 0) {
					return relationIsStrict(mRelation);
				} else if(cp > 0) {
					return mRelation == CompareRelation::LEQ || mRelation == CompareRelation::LT || mRelation == CompareRelation::EQ;
				} else {
					assert(cp < 0);
					return mRelation == CompareRelation::GEQ || mRelation == CompareRelation::GT || mRelation == CompareRelation::EQ;
				}
			}
			return false;
		}
				
	private:
		LhsType mLhs;
		CompareRelation mRelation;
};

template<typename LhsT>
bool operator==(const Constraint<LhsT>& lhs, const Constraint<LhsT>& rhs)
{
	return lhs.rel() == rhs.rel() && lhs.lhs() == rhs.lhs();
}

template<typename LhsT>
bool operator!=(const Constraint<LhsT>& lhs, const Constraint<LhsT>& rhs)
{
	return !(lhs == rhs);
}

template<typename LhsT>
std::ostream& operator<<(std::ostream& os, const Constraint<LhsT>& rhs)
{
	return os << rhs.lhs() << " " << rhs.rel() << " 0";
}


}


namespace std {

template<typename LhsType>
struct hash<carl::Constraint<LhsType>> 
{
	std::size_t operator()(const carl::Constraint<LhsType>& c) const {
		std::hash<LhsType> h1;
		std::hash<carl::CompareRelation> h2;
		return h1(c.lhs()) << 2 | h2(c.rel());
	}
};

}
