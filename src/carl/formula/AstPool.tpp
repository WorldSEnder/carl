/**
 * @file AstPool.tpp
 *
 * @author Florian Corzilius<corzilius@cs.rwth-aachen.de>
 * @author Sebastian Junges
 * @author Ulrich Loup
 * @version 2013-10-21
 */

#include "AstPool.h"

using namespace std;

namespace carl
{
    template<typename Pol>
    AstPool<Pol>::AstPool( unsigned _capacity ):
        Singleton<AstPool<Pol>>(),
        mIdAllocator( 3 ),
        mpTrue( new Ast<Pol>( true, 1 ) ),
        mpFalse( new Ast<Pol>( false, 2 ) ),
        mAsts()
    {
        mAsts.reserve( _capacity );
        mAsts.insert( mpTrue );
        mAsts.insert( mpFalse );
        mpTrue->initProperties();
        mpFalse->initProperties();
    }

    template<typename Pol>
    AstPool<Pol>::~AstPool()
    {
        mAsts.erase( mpTrue );
        delete mpTrue;
        mAsts.erase( mpFalse );
        delete mpFalse;
        while( !mAsts.empty() )
        {
            const Ast<Pol>* pAst = (*mAsts.begin());
            mAsts.erase( mAsts.begin() );
            delete pAst;
        }
    }
    
    template<typename Pol>
    const Ast<Pol>* AstPool<Pol>::addAstToPool( Ast<Pol>* _ast )
    {
        AST_POOL_LOCK_GUARD
        auto iterBoolPair = mAsts.insert( _ast );
        if( !iterBoolPair.second ) // Ast has already been generated.
        {
            delete _ast;
        }
        else
        {
            _ast->mId = mIdAllocator; // id should be set here to avoid conflicts when multi-threading
            _ast->initProperties();
            ++mIdAllocator;
            // Add also the negation of the ast to the pool in order to ensure that it
            // has the next id and hence would occur next to the ast in a set of sub-ast,
            // which is sorted by the ids.
            Ast<Pol>* astNegated = new Ast<Pol>( _ast );
            assert( mAsts.find( astNegated ) == mAsts.end() );
            mAsts.insert( astNegated );
            astNegated->mId = mIdAllocator; 
            astNegated->initProperties();
            ++mIdAllocator;
        }
        return *iterBoolPair.first;   
    }
    
    template<typename Pol>
    const Ast<Pol>* AstPool<Pol>::createAst( typename Ast<Pol>::Type _type, PointerSet<Ast<Pol>>&& _subasts )
    {
        assert( _type == Ast<Pol>::AND || _type == Ast<Pol>::OR || _type == Ast<Pol>::XOR || _type == Ast<Pol>::IFF );
//        cout << "create new ast with type " << Ast<Pol>::AstTypeToString( _type ) << endl;
//        for( auto f : _subasts )
//            cout << *f << endl;
        for( auto iter = _subasts.begin(); iter != _subasts.end(); )
        {
            if( (*iter)->getType() == _type && (_type == Ast<Pol>::AND || _type == Ast<Pol>::OR) )
            {
                // We have (op .. (op a1 .. an) b ..), so create (op .. a1 .. an b ..) instead.
                // Note, that a1 to an are definitely created before b, as they were sub-asts
                // of it, hence, the ids of a1 to an are smaller than the one of b and therefore a1<b .. an<b.
                // That means, that a1 .. an are inserted into the given set of sub asts before the position of
                // b (=iter).
                // Note also that the operator of a1 to an cannot be oper, as they where also created with this pool.
                _subasts.insert( (*iter)->subasts().begin(), (*iter)->subasts().end() );
                iter = _subasts.erase( iter );
            }
            else
            {
                auto iterB = iter; 
                ++iter;
                // Check if the sub-ast at iter is the negation of the sub-ast at iterB
                // Note, that the negation of a ast would by construction always be right after the ast
                // in a set of asts whose comparison operator is based on the one of asts This is due to
                // them comparing just the ids and we construct the negation of a ast right after the ast
                // itself and assign the next id to it.
                if( iter != _subasts.end() )
                {
                    if( (*iterB == mpTrue && *iter == mpFalse) || ((*iter)->getType() == Ast<Pol>::NOT && (*iter)->subast() == (**iterB)) )
                    {
                        switch( _type )
                        {
                            case Ast<Pol>::AND:
                            {
                                return mpFalse;
                            }
                            case Ast<Pol>::OR:
                            {
                                return mpTrue;
                            }
                            case Ast<Pol>::IFF:
                            {
                                return mpFalse;
                            }
                            case Ast<Pol>::XOR:
                            {
                                _subasts.erase( iterB );
                                iter = _subasts.erase( iter );
                                _subasts.insert( mpTrue );
                                break;
                            }
                            default:
                            {
                                assert( false );
                                break;
                            }
                        }
                    }
                }
            }
        }
        if( _subasts.empty() )
            return mpFalse;
        else
        {
            #ifdef SIMPLIFY_AST
            if( _type == Ast<Pol>::AND || _type == Ast<Pol>::OR || _type == Ast<Pol>::IFF )
            {
                typename PointerSet<Ast<Pol>>::iterator iterToTrue = _subasts.begin();
                typename PointerSet<Ast<Pol>>::iterator iterToFalse = _subasts.begin();
                if( *iterToTrue == mpTrue )
                {
                    ++iterToFalse;
                    if( iterToFalse != _subasts.end() && *iterToFalse != mpFalse )
                        iterToFalse = _subasts.end();
                }
                else
                {
                    iterToTrue = _subasts.end();
                    if( *iterToFalse != mpFalse )
                        iterToFalse = _subasts.end();
                }
                if( _type == Ast<Pol>::AND )
                {
                    if( iterToTrue != _subasts.end() ) _subasts.erase( iterToTrue );
                    if( iterToFalse != _subasts.end() ) return mpFalse;
                    else if( _subasts.empty() ) return mpTrue;
                }
                else if( _type == Ast<Pol>::OR )
                {
                    if( iterToFalse != _subasts.end() ) _subasts.erase( iterToFalse );
                    if( iterToTrue != _subasts.end() ) return mpTrue;
                    else if( _subasts.empty() ) return mpFalse;
                }
                else // _type == Ast<Pol>::IFF
                {
                    if( iterToFalse != _subasts.end() && iterToTrue != _subasts.end() )
                    {
                        return mpFalse;
                    }
                }
            }
            #endif
            if( _subasts.size() == 1 )
                return newAstWithOneSubast( _type, *(_subasts.begin()) );
        }
        return addAstToPool( new Ast<Pol>( _type, std::move( _subasts ) ) );
    }
}    // namespace carl