#include "ae/ExprRewriter.hpp"

using namespace ufo;
using namespace std;

int main (int argc, char ** argv)
{
  ExprFactory m_efac;
  EZ3 z3(m_efac);
  Expr fla = z3_from_smtlib_file (z3, argv[1]);
  outs () << "Input formula: " << *fla << "\n";

  if (isOpX<AND>(fla))
  {
    ExprVector vec;
    for (unsigned i = 0; i < fla->arity(); i++){
      vec.push_back(fla->arg(i));
    }
    getImplicant(m_efac, vec);
  }
  return 0;
}
