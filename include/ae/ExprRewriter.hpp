#ifndef EXPRREWRITER__HPP__
#define EXPRREWRITER__HPP__

#include "ae/ExprSimpl.hpp"
#include "ae/SMTUtils.hpp"

using namespace std;
using namespace boost;
namespace ufo
{
  inline Expr getImplicant(ExprFactory& m_efac, ExprVector& vec)
  {
    SMTUtils u(m_efac);
    Expr sol = mk<TRUE>(m_efac);

    // TODO: synthesize sol

    ExprSet t;
    t.insert(sol);
    t.insert(vec[0]);

    for (unsigned i = 1; i < vec.size(); i++)
    {
      if (u.implies(conjoin(t, m_efac), vec[i])) outs () << "SUCCESS\n";
      else outs () << "FAIL\n";
      t.insert(vec[i]);
    }
    return sol;
  }
}

#endif
