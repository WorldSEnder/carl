/**
 * @file FormulaPool.h
 *
 * @author Florian Corzilius<corzilius@cs.rwth-aachen.de>
 * @version 2014-05-08
 */

#pragma once

#include "../util/Singleton.h"
#include "../core/VariablePool.h"
#include "Formula.h"
#include "ConstraintPool.h"
#include <mutex>
#include <limits>
#include <boost/variant.hpp>
#include "bitvector/BVConstraintPool.h"
#include "bitvector/BVConstraint.h"

#define SIMPLIFY_FORMULA

namespace carl
{
    
    template<typename Pol>
    class FormulaPool : public Singleton<FormulaPool<Pol>>
    {
        friend Singleton<FormulaPool>;
        friend Formula<Pol>;
        
        private:
            
            // Members:
            /// id allocator
            unsigned mIdAllocator;
            /// The unique formula representing true.
            FormulaContent<Pol>* mpTrue;
            /// The unique formula representing false.
            FormulaContent<Pol>* mpFalse;
            /// The formula pool.
            FastPointerSet<FormulaContent<Pol>> mPool;
            /// Mutex to avoid multiple access to the pool
            mutable std::recursive_mutex mMutexPool;
            ///
            FastPointerMap<FormulaContent<Pol>,const FormulaContent<Pol>*> mTseitinVars;
            ///
            FastPointerMap<FormulaContent<Pol>,typename FastPointerMap<FormulaContent<Pol>,const FormulaContent<Pol>*>::iterator> mTseitinVarToFormula;
            
            #ifdef THREAD_SAFE
            #define FORMULA_POOL_LOCK_GUARD std::lock_guard<std::recursive_mutex> lock( mMutexPool );
            #define FORMULA_POOL_LOCK mMutexPool.lock();
            #define FORMULA_POOL_UNLOCK mMutexPool.unlock();
            #else
            #define FORMULA_POOL_LOCK_GUARD
            #define FORMULA_POOL_LOCK
            #define FORMULA_POOL_UNLOCK
            #endif
            
        protected:
            
            /**
             * Constructor of the formula pool.
             * @param _capacity Expected necessary capacity of the pool.
             */
            FormulaPool( unsigned _capacity = 10000 );
            
            ~FormulaPool();
    
            const FormulaContent<Pol>* trueFormula() const
            {
                return mpTrue;
            }

            const FormulaContent<Pol>* falseFormula() const
            {
                return mpFalse;
            }
            
        public:
            std::size_t size() const {
                return mPool.size();
            }
            
            void print() const
            {
                std::cout << "Formula pool contains:" << std::endl;
                for (const auto& ele: mPool) {
                    std::cout << ele->mId << " @ " << static_cast<const void*>(ele) << " [usages=" << ele->mUsages << "]: " << *ele << ", negation " << static_cast<const void*>(ele->mNegation) << std::endl;
                }
                std::cout << "Tseitin variables:" << std::endl;
                for( const auto& tvVar : mTseitinVars )
                {
                    if( tvVar.second != nullptr )
                    {
                        std::cout << "id " << tvVar.first->mId << "  ->  " << tvVar.second->mId << " [remapping: ";
                        auto iter = mTseitinVarToFormula.find( tvVar.second );
                        if( iter != mTseitinVarToFormula.end() )
                        {
                            assert( iter->second != mTseitinVars.end() );
                            std::cout << iter->first->mId << " -> " << iter->second->first->mId << "]" << std::endl;
                        }
                        else
                            std::cout << "not yet remapped!" << std::endl;
                    }
                    else
                        std::cout << "id " << tvVar.first->mId << "  ->  nullptr" << std::endl;
                }
                std::cout << std::endl;
            }
            
            Formula<Pol> getTseitinVar( const Formula<Pol>& _formula )
            {
                auto iter = mTseitinVars.find( _formula.mpContent );
                if( iter != mTseitinVars.end() )
                {
                    return Formula<Pol>( iter->second );
                }
                return trueFormula();
            }
            
            Formula<Pol> createTseitinVar( const Formula<Pol>& _formula )
            {
                auto iter = mTseitinVars.insert( std::make_pair( _formula.mpContent, nullptr ) );
                if( iter.second )
                {
                    const FormulaContent<Pol>* hi = create( carl::freshBooleanVariable() );
                    hi->mDifficulty = _formula.difficulty();
                    iter.first->second = hi;
                    mTseitinVarToFormula[hi] = iter.first;
                }
                return Formula<Pol>( iter.first->second );
            }
            
        private:
            bool isBaseFormula(const Constraint<Pol>& c) const {
                return c < c.negation();
            }
            bool isBaseFormula(const FormulaContent<Pol>* f) const {
                assert(f->mType == FormulaType::CONSTRAINT);
#ifdef __VS
                const auto& a = *f->mpConstraintVS;
                const auto& b = *f->mNegation->mpConstraintVS;
#else
                const auto& a = f->mConstraint;
                const auto& b = f->mNegation->mConstraint;
#endif
                return a < b;
            }

            const FormulaContent<Pol>* getBaseFormula(const FormulaContent<Pol>* f) const {
                assert(f != nullptr);
                if (f->mType == FormulaType::NOT) {
                    CARL_LOG_TRACE("carl.formula", "Base formula of " << *f << " is " << *f->mNegation);
                    return f->mNegation;
                }
                if (f->mType == FormulaType::CONSTRAINT) {
                    if (isBaseFormula(f)) {
                        CARL_LOG_TRACE("carl.formula", "Base formula of " << *f << " is " << *f);
                        return f;
                    } else {
                        CARL_LOG_TRACE("carl.formula", "Base formula of " << *f << " is " << *f->mNegation);
                        return f->mNegation;
                    }
                }
                CARL_LOG_TRACE("carl.formula", "Base formula of " << *f << " is " << *f);
                return f;
            }

            FormulaContent<Pol>* createNegatedContent(const FormulaContent<Pol>* f) const {
                if (f->mType == FormulaType::CONSTRAINT) {
#ifdef __VS
                    return new FormulaContent<Pol>(f->mpConstraintVS.negation());
#else
                    return new FormulaContent<Pol>(f->mConstraint.negation());
#endif
                } else {
                    return new FormulaContent<Pol>(NOT, std::move(Formula<Pol>(f)));
                }
            }

            // ##### Core Theory

            /**
             * Create formula representing a boolean value.
             * @param _type Formula type, may be either TRUE or FALSE.
             * @return A formula representing the given bool.
             */
            const FormulaContent<Pol>* create(FormulaType _type) {
                assert(_type == TRUE || _type == FALSE);
                return (_type == TRUE) ? trueFormula() : falseFormula();
            }

            /**
             * Create formula representing a boolean variable.
             * @param _booleanVar The Boolean variable wrapped by this formula.
             * @return A formula with wrapping the given Boolean variable.
             */
            const FormulaContent<Pol>* create(Variable::Arg _variable) {
                return add(new FormulaContent<Pol>(_variable));
            }
            
            /**
             * @param _constraint The constraint wrapped by this formula.
             * @return A formula with wrapping the given constraint.
             */
            const FormulaContent<Pol>* create(Constraint<Pol>&& _constraint) {
                #ifdef SIMPLIFY_FORMULA
                switch (_constraint.isConsistent()) {
                    case 0: return falseFormula();
                    case 1: return trueFormula();
                    default: ;
                }
                #endif
                if (isBaseFormula(_constraint)) {
                    return add(new FormulaContent<Pol>(std::move(_constraint)));
                } else {
                    return add(new FormulaContent<Pol>(_constraint.negation()))->mNegation;
                }
            }
            const FormulaContent<Pol>* create(const Constraint<Pol>& _constraint) {
                return create(std::move(Constraint<Pol>(_constraint)));
            }
			const FormulaContent<Pol>* create(VariableComparison<Pol>&& _variableComparison) {
                return add(new FormulaContent<Pol>(std::move(_variableComparison)));
            }
            const FormulaContent<Pol>* create(const VariableComparison<Pol>& _variableComparison) {
				auto val = _variableComparison.asConstraint();
				if (val) return create(*val);
                return create(std::move(VariableComparison<Pol>(_variableComparison)));
            }
			const FormulaContent<Pol>* create(VariableAssignment<Pol>&& _variableAssignment) {
                return add(new FormulaContent<Pol>(std::move(_variableAssignment)));
            }
            const FormulaContent<Pol>* create(const VariableAssignment<Pol>& _variableAssignment) {
				return create(std::move(VariableAssignment<Pol>(_variableAssignment)));
            }
            
            const FormulaContent<Pol>* create(BVConstraint&& _constraint) {
                #ifdef SIMPLIFY_FORMULA
                if (_constraint.isAlwaysConsistent()) return trueFormula();
                if (_constraint.isAlwaysInconsistent()) return falseFormula();
                #endif
                return add(new FormulaContent<Pol>(std::move(_constraint)));
            }
            const FormulaContent<Pol>* create(const BVConstraint& _constraint) {
                return create(std::move(BVConstraint(_constraint)));
            }
			const FormulaContent<Pol>* create(const PBConstraint<Pol>& _constraint) {
                return create(std::move(PBConstraint<Pol>(_constraint)));
            }
            
            
            /**
             * Create formula representing a unary function.
             * @param _type Formula type specifying the function.
             * @param _subFormula Formula representing the function argument.
             * @return A formula representing the given function call.
             */
            const FormulaContent<Pol>* create(FormulaType _type, Formula<Pol>&& _subFormula) {
                switch (_type) {
                    case ITE:
                    case EXISTS:
                    case FORALL:
                        assert(false); break;

                    // Core Theory
                    case TRUE:
                    case FALSE:
                    case BOOL:
                        assert(false); break;
                    case NOT:
                        return _subFormula.mpContent->mNegation;
                    case IMPLIES:
                        assert(false); break;
                    case AND:
                    case OR:
                    case XOR:
                        return _subFormula.mpContent;
                    case IFF:
                        return create(TRUE);

                    // Arithmetic Theory
                    case CONSTRAINT:
                        assert(false); break;
					case VARCOMPARE:
					case VARASSIGN:
                        assert(false); break;
                    case BITVECTOR:
                    case UEQ:
					case PBCONSTRAINT:
                        assert(false); break;
                }
                return nullptr;
            }
            
            /**
             * Create formula representing a nary function.
             * @param _type Formula type specifying the function.
             * @param _subformulas Formula representing the function arguments.
             * @return A formula representing the given function call.
             */
            const FormulaContent<Pol>* create(FormulaType _type, const Formulas<Pol>& _subformulas) {
                return create(_type, std::move(Formulas<Pol>(_subformulas)));
            }
            const FormulaContent<Pol>* create(FormulaType _type, const std::initializer_list<Formula<Pol>>& _subformulas) {
                return create(_type, std::move(Formulas<Pol>(_subformulas.begin(), _subformulas.end())));
            }
            const FormulaContent<Pol>* create(FormulaType _type, Formulas<Pol>&& _subformulas) {
                switch (_type) {
                    case ITE:
                        return createITE(std::move(_subformulas));
                    case EXISTS:
                    case FORALL:
                    // Core Theory
                    case TRUE:
                    case FALSE:
                    case BOOL:
                    case NOT:
                        assert(false); break;
                    case IMPLIES:
                        return createImplication(std::move(_subformulas));
                    case AND:
                    case OR:
                    case XOR:
                    case IFF:
                        return createNAry(_type, std::move(_subformulas));
                    // Arithmetic Theory
                    case CONSTRAINT:
					// VarCompare
					case VARCOMPARE:
					case VARASSIGN:
                    // Bitvector Theory
                    case BITVECTOR:
                    // Uninterpreted Theory
                    case UEQ:
					// Pseudoboolean
					case PBCONSTRAINT:
                        assert(false); break;
                }
                return nullptr;
            }
    
            /**
             * Create formula representing an implication.
             * @param _subformulas
             * @return 
             */
            const FormulaContent<Pol>* createImplication(Formulas<Pol>&& _subformulas);
            
            const FormulaContent<Pol>* createNAry(FormulaType _type, Formulas<Pol>&& _subformulas);

            const FormulaContent<Pol>* createITE(Formulas<Pol>&& _subformulas);
            
			/**
			 *
			 * @param _type
			 * @param _vars
			 * @param _term
			 * @return
			 */
			const FormulaContent<Pol>* create(FormulaType _type, std::vector<Variable>&& _vars, const Formula<Pol>& _term) {
				assert(_type == FormulaType::EXISTS || _type == FormulaType::FORALL);
				if (_vars.size() > 0) {
					return add( new FormulaContent<Pol>(_type, std::move(_vars), _term ) );
				} else {
					return _term.mpContent;
				}
			}
            
            /**
             * @param _subformulas The sub-formulas of the formula to create.
             * @return A formula with the given operator and sub-formulas.
             */
            const FormulaContent<Pol>* create( const FormulasMulti<Pol>& _subformulas )
            {
                if( _subformulas.empty() ) return falseFormula();
                if( _subformulas.size() == 1 )
                {
                    return _subformulas.begin()->mpContent;
                }
                Formulas<Pol> subFormulas;
                auto lastSubFormula = _subformulas.begin();
                auto subFormula = lastSubFormula;
                ++subFormula;
                int counter = 1;
                while( subFormula != _subformulas.end() )
                {
                    if( *lastSubFormula == *subFormula )
                    {
                        ++counter;
                    }
                    else
                    {
                        if( counter % 2 == 1 )
                        {
                            subFormulas.insert( subFormulas.end(), *lastSubFormula ); // set has same order as the multiset
                        }
                        lastSubFormula = subFormula;
                        counter = 1;
                    }
                    ++subFormula;
                }
                if( counter % 2 == 1 )
                {
                    subFormulas.insert( subFormulas.end(), *lastSubFormula );
                }
                return create( FormulaType::XOR, std::move( subFormulas ) );
            }
            
			const FormulaContent<Pol>* create( const UEquality::Arg& _lhs, const UEquality::Arg& _rhs, bool _negated )
			{
                #ifdef SIMPLIFY_FORMULA
                if( boost::apply_visitor(UEquality::IsUVariable(), _lhs) && boost::apply_visitor(UEquality::IsUVariable(), _rhs) )
                {
                    if( boost::get<UVariable>(_lhs) < boost::get<UVariable>(_rhs) )
                        return add( new FormulaContent<Pol>( UEquality( boost::get<UVariable>(_lhs), boost::get<UVariable>(_rhs), _negated, true ) ) );
                    if( boost::get<UVariable>(_rhs) < boost::get<UVariable>(_lhs) )
                        return add( new FormulaContent<Pol>( UEquality( boost::get<UVariable>(_rhs), boost::get<UVariable>(_lhs), _negated, true ) ) );
                    else if( _negated )
                        return falseFormula();
                    else
                        return trueFormula();
                }
				else if( boost::apply_visitor(UEquality::IsUVariable(), _lhs) && boost::apply_visitor(UEquality::IsUFInstance(), _rhs) )
                {
                    return add( new FormulaContent<Pol>( UEquality( boost::get<UVariable>(_lhs), boost::get<UFInstance>(_rhs), _negated ) ) );
                }
                else if( boost::apply_visitor(UEquality::IsUFInstance(), _lhs) && boost::apply_visitor(UEquality::IsUVariable(), _rhs) )
                {
                    return add( new FormulaContent<Pol>( UEquality( boost::get<UVariable>(_rhs), boost::get<UFInstance>(_lhs), _negated ) ) );
                }
                else
                {
                    assert( boost::apply_visitor(UEquality::IsUFInstance(), _lhs) && boost::apply_visitor(UEquality::IsUFInstance(), _rhs) );
                    if( boost::get<UFInstance>(_lhs) < boost::get<UFInstance>(_rhs) )
                        return add( new FormulaContent<Pol>( UEquality( boost::get<UFInstance>(_lhs), boost::get<UFInstance>(_rhs), _negated, true ) ) );
                    if( boost::get<UFInstance>(_rhs) < boost::get<UFInstance>(_lhs) )
                        return add( new FormulaContent<Pol>( UEquality( boost::get<UFInstance>(_rhs), boost::get<UFInstance>(_lhs), _negated, true ) ) );
                    else if( _negated )
                        return falseFormula();
                    else
                        return trueFormula();
                }
                #else
                return add( new FormulaContent<Pol>( UEquality( _lhs, _rhs, _negated ) ) );
                #endif
			}

			const FormulaContent<Pol>* create( UEquality&& eq )
			{
				return add( new FormulaContent<Pol>( std::move( eq ) ) );
			}
			
			const FormulaContent<Pol>* create( PBConstraint<Pol>&& pbc )
			{
				return add( new FormulaContent<Pol>( std::move( pbc ) ) );
			}
            
            void free( const FormulaContent<Pol>* _elem )
            {
                FORMULA_POOL_LOCK_GUARD
                const FormulaContent<Pol>* tmp = getBaseFormula(_elem);
                //const FormulaContent<Pol>* tmp = _elem->mType == FormulaType::NOT ? _elem->mNegation : _elem;
                CARL_LOG_DEBUG("carl.formula", "Freeing " << static_cast<const void*>(tmp) << ", current usage: " << tmp->mUsages);
                assert( tmp->mUsages > 0 );
                --tmp->mUsages;
                if( tmp->mUsages == 1 )
                {
                    bool stillStoredAsTseitinVariable = false;
                    if( freeTseitinVariable( tmp ) )
                        stillStoredAsTseitinVariable = true;
                    if( freeTseitinVariable( tmp->mNegation ) )
                        stillStoredAsTseitinVariable = true;
                    if( !stillStoredAsTseitinVariable )
                    {
                        mPool.erase( tmp );
                        delete tmp->mNegation;
                        delete tmp;
                    }
                }
            }
            
            bool freeTseitinVariable( const FormulaContent<Pol>* _toDelete )
            {
                bool stillStoredAsTseitinVariable = false;
                auto tvIter = mTseitinVars.find( _toDelete );
                if( tvIter != mTseitinVars.end() )
                {
                    // if this formula HAS a tseitin variable
                    if( tvIter->second->mUsages == 1 )
                    {
                        // the tseitin variable is not used -> delete it
                        const FormulaContent<Pol>* tmp = tvIter->second;
                        mTseitinVars.erase( tvIter );
                        assert( mTseitinVarToFormula.find( tmp ) != mTseitinVarToFormula.end() );
                        mTseitinVarToFormula.erase( tmp );
                        mPool.erase( tmp );
                        delete tmp->mNegation;
                        delete tmp;
                    }
                    else // the tseitin variable is used, so we cannot delete the formula
                        stillStoredAsTseitinVariable = true;
                }
                else
                {
                    auto tmpTVIter = mTseitinVarToFormula.find( _toDelete );
                    if( tmpTVIter != mTseitinVarToFormula.end() )
                    {
                        const FormulaContent<Pol>* fcont = tmpTVIter->second->first;
                        // if this formula IS a tseitin variable
                        if( fcont->mUsages == 1 )
                        {
                            // the formula variable is not used -> delete it
                            const FormulaContent<Pol>* tmp = getBaseFormula(fcont);
                            //const FormulaContent<Pol>* tmp = fcont->mType == FormulaType::NOT ? fcont->mNegation : fcont;
                            mTseitinVars.erase( tmpTVIter->second );
                            mTseitinVarToFormula.erase( tmpTVIter );
                            mPool.erase( tmp );
                            delete tmp->mNegation;
                            delete tmp;
                        }
                        else // the formula is used, so we cannot delete the tseitin variable
                            stillStoredAsTseitinVariable = true;
                    }
                }
                return stillStoredAsTseitinVariable;
            }
            
            void reg( const FormulaContent<Pol>* _elem ) const
            {
                FORMULA_POOL_LOCK_GUARD
                const FormulaContent<Pol>* tmp = getBaseFormula(_elem);
                //const FormulaContent<Pol>* tmp = _elem->mType == FormulaType::NOT ? _elem->mNegation : _elem;
                assert( tmp != nullptr );
                assert( tmp->mUsages < std::numeric_limits<size_t>::max() );
                CARL_LOG_DEBUG("carl.formula", "Registering " << static_cast<const void*>(tmp) << ", current usage: " << tmp->mUsages);
                ++tmp->mUsages;
                if (tmp->mUsages == 1 && _elem->mType == FormulaType::CONSTRAINT) {
                    CARL_LOG_DEBUG("carl.formula", "Is a constraint, increasing again");
                    ++tmp->mUsages;
                }
            }
            
        public:
            template<typename ArgType>
            void forallDo( void (*_func)( ArgType*, const Formula<Pol>& ), ArgType* _arg ) const
            {
                FORMULA_POOL_LOCK_GUARD
                for( const FormulaContent<Pol>* formula : mPool )
                {
                    (*_func)( _arg, Formula<Pol>( formula ) );
                    if( formula != mpFalse )
                    {
                        (*_func)( _arg, Formula<Pol>( formula->mNegation ) );
                    }
                }
            }
            
            template<typename ReturnType, typename ArgType>
            std::map<const Formula<Pol>,ReturnType> forallDo( ReturnType (*_func)( ArgType*, const Formula<Pol>& ), ArgType* _arg ) const
            {
                FORMULA_POOL_LOCK_GUARD
                std::map<const Formula<Pol>,ReturnType> result;
                for( const FormulaContent<Pol>* elem : mPool )
                {
                    Formula<Pol> form(elem);
                    result[form] = (*_func)( _arg, form );
                    if( elem != mpFalse )
                    {
                        Formula<Pol> form2(elem->mNegation);
                        result[form2] = (*_func)( _arg, form2 );
                    }
                }
                return result;
            }
            
            /**
             */
            bool formulasInverse( const Formula<Pol>& _subformulaA, const Formula<Pol>& _subformulaB );
        
            /**
             * @param _type The type of the n-ary operator (n>1) of the formula to create.
             * @param _subformulas The sub-formulas of the formula to create.
             * @return A formula with the given operator and sub-formulas.
             * Note, that if you use this method to create a formula with the operator XOR
             * and you have collected the sub-formulas in a set, multiple occurrences of a
             * sub-formula are condensed. You should only use it, if you can exlcude this 
             * possibility. Otherwise use the method newExclusiveDisjunction.
             */
            //const FormulaContent<Pol>* create( FormulaType _type, Formulas<Pol>&& _subformulas );
            
    private:
            
            /**
             * Inserts the given formula to the pool, if it does not yet occur in there.
             * @param _formula The formula to add to the pool.
             * @return The position of the given formula in the pool and true, if it did not yet occur in the pool;
             *         The position of the equivalent formula in the pool and false, otherwise.
             */
            std::pair<typename FastPointerSet<FormulaContent<Pol>>::iterator,bool> insert( FormulaContent<Pol>* _formula );
            
            /**
             * Adds the given formula to the pool, if it does not yet occur in there.
             * Note, that this method uses the allocator which is locked before calling.
             * @param _formula The formula to add to the pool.
             * @return The given formula, if it did not yet occur in the pool;
             *         The equivalent formula already occurring in the pool, otherwise.
             */
            const FormulaContent<Pol>* add( FormulaContent<Pol>* _formula );
    };
}    // namespace carl

#include "FormulaPool.tpp"
