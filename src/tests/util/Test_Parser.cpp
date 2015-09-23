#include "gtest/gtest.h"
#include "carl/numbers/numbers.h"
#include "carl/core/Variable.h"
#include "carl/util/parser/Parser.h"

#ifdef USE_CLN_NUMBERS
#include <cln/cln.h>
typedef cln::cl_RA Rational;
typedef cln::cl_I Integer;
#else
#include <gmpxx.h>
typedef mpq_class Rational;
typedef mpz_class Integer;
#endif

using namespace carl;

TEST(Parser, Polynomial)
{
	carl::parser::Parser<MultivariatePolynomial<Rational>> parser;
	carl::Variable x = freshRealVariable("x");
	carl::Variable y = freshRealVariable("y");
	parser.addVariable(x);
	parser.addVariable(y);
	
	EXPECT_EQ(Rational(1), parser.polynomial("1"));
	EXPECT_EQ(x, parser.polynomial("x"));
	EXPECT_EQ(x*y, parser.polynomial("x*y"));
	EXPECT_EQ(x*x, parser.polynomial("x*x"));
	EXPECT_EQ(x*x, parser.polynomial("x^2"));
}

TEST(Parser, RationalFunction)
{
	typedef RationalFunction<MultivariatePolynomial<Rational>> RF;
	carl::parser::Parser<MultivariatePolynomial<Rational>> parser;
	
	RF f = parser.rationalFunction("2*x / x^2");
	std::cout << f << std::endl;
}

TEST(Parser, Formula)
{
    using carl::VariableType;
    typedef Formula<MultivariatePolynomial<Rational>> FT;
    carl::parser::Parser<MultivariatePolynomial<Rational>> parser;
    carl::Variable b1 = freshBooleanVariable("O4853");
    carl::Variable b2 = freshBooleanVariable("O3838");
    carl::Variable b3 = freshBooleanVariable("O4848");
    carl::Variable b4 = freshBooleanVariable("O4851");
    carl::Variable b5 = freshBooleanVariable("O4849");
    carl::Variable b6 = freshBooleanVariable("O4850");
    carl::Variable b7 = freshBooleanVariable("O6262");
    carl::Variable b8 = freshBooleanVariable("O6285");
    carl::Variable b9 = freshBooleanVariable("O6217");
    carl::Variable b10 = freshBooleanVariable("O8504");
    carl::Variable b11 = freshBooleanVariable("O8665");
    parser.addVariable(b1);
    parser.addVariable(b2);
    parser.addVariable(b3);
    parser.addVariable(b4);
    parser.addVariable(b5);
    parser.addVariable(b6);
    parser.addVariable(b7);
    parser.addVariable(b8);
    parser.addVariable(b9);
    parser.addVariable(b10);
    parser.addVariable(b11);

    FT result = parser.formula("(O4853 IMPLIES (O3838 AND ((((((((O4848) OR (O4851)) OR (O4849)) OR (O4850)) OR (O6262)) OR (O6285)) OR (O6217)) OR (O8504)) AND (NOT O8665)))");
    std::cout << result << std::endl;
    EXPECT_EQ(FT(IMPLIES, FT(b1), FT(AND, FT(b2), FT(OR, FT(OR, FT(b3), FT(b4)), FT(OR, FT(b5), FT(b6), FT(b7)), FT(OR, FT(b8), FT(b9), FT(b10))), FT(NOT, FT(b11)))), result);
}
