#ifndef EXPRREWRITER__HPP__
#define EXPRREWRITER__HPP__

#include "ae/ExprSimpl.hpp"
#include "ae/SMTUtils.hpp"

#include <typeinfo>

using namespace std;
using namespace boost;
namespace ufo
{

  inline static void getVars(Expr e, ExprSet& vars) {
    if (e->arity() == 1) {
      vars.insert(e); 
    }
    else {
      for (unsigned i = 0; i < e->arity(); i++) getVars(e->arg(i), vars);
    }
  }

  inline static void parser(Expr e, ExprSet& vars) {
    if (e->arity() == 1) vars.insert(e); 
    else if (isOpX<MULT>(e) && e->arity() == 2) vars.insert(e); 
    else if (isOpX<MINUS>(e)) {
      vars.insert(e); 
      parser(e->left(), vars); 
    }
    else for (unsigned i = 0; i < e->arity(); i++) parser(e->arg(i), vars); 
  }

  inline static Expr createSol(Expr solLHS, Expr input, int rhs, ExprFactory& m_efac) {
    Expr solRHS = mkTerm(mpz_class(rhs), m_efac);
    Expr sol; 
    if (isOpX<LT>(input)) sol = mk<LT>(solLHS, solRHS); 
    else if (isOpX<GT>(input)) sol = mk<GT>(solLHS, solRHS); 
    else if (isOpX<LEQ>(input)) sol = mk<LEQ>(solLHS, solRHS); 
    else if (isOpX<GEQ>(input)) sol = mk<GEQ>(solLHS, solRHS); 
    else sol = mk<TRUE>(m_efac); 
    return sol; 
  }

  inline Expr getImplicant(ExprFactory& m_efac, ExprVector& vec)
  {
    SMTUtils u(m_efac);
    Expr sol = mk<TRUE>(m_efac);
    ExprSet t;
    t.insert(sol);
    t.insert(vec[0]);   

    // synthesizing sol based on given ExprVector
    ExprSet varsSet; 
    for (unsigned i = 0; i < vec.size(); i++) {
      getVars(vec[i], varsSet); 
    }
    ExprVector varsVec; 
    for (auto it = varsSet.begin(); it != varsSet.end(); ++it) {
      varsVec.push_back(*it);
    }
    map<Expr, int> varsMap; 
    for (unsigned i = 0; i < varsVec.size(); i++) {
      varsMap[varsVec[i]] = i; 
    }
    int r = vec.size(); 
    int c = varsVec.size()+1; 
    int eqs[r][c]; 

    // make a linear eq matrix
    for (unsigned i = 0; i < r; i++) {
      eqs[i][c-1] = lexical_cast<int>(*vec[i]->right()); 
      ExprSet vars;
      parser(vec[i], vars);
      for (auto it = vars.begin(); it!=vars.end(); ++it) {
        Expr e = *it; 
        if (e->arity() == 1) {
          int col = varsMap[e]; 
          eqs[i][col] = 1; 
        }
        else if (isOpX<MULT>(e)) {
          int col = varsMap[e->right()]; 
          eqs[i][col] = lexical_cast<int>(*e->left()); 
        }
        else if (isOpX<MINUS>(e)) {
          Expr e_sub = e->right();
          if (e_sub->arity() == 1) {
            int col = varsMap[e_sub]; 
            eqs[i][col] = -1; 
          }
          else if (isOpX<MULT>(e_sub)) {
            int col = varsMap[e_sub->right()];  
            eqs[i][col] = -1*lexical_cast<int>(*e_sub->left()); 
          }
        }
      } // end for loop
    } // end construction of eq matrix 

    // print eq matrix -- testing 
    // for (unsigned i=0;i<r;i++) {
    //   for (unsigned j=0;j<c;j++) {
    //     outs() << eqs[i][j] << " "; 
    //   }
    //   outs() << "\n"; 
    // }

    // synthesizing sol -- brute force 
    for (unsigned i = 0; i < r; i++) {
      for (unsigned j = 0; j < r; j++) {
        int coef[c];
        if (i != j) {
          ExprSet solTerms; 
          for (unsigned col = 0; col < c; col++) {
            coef[col] = eqs[i][col]-eqs[j][col]; 
          }
          for (unsigned col = 0; col < c-1; col++) {
            if (coef[col] != 0) {
              Expr term = mk<MULT>(mkTerm(mpz_class(coef[col]), m_efac), varsVec[col]); 
              solTerms.insert(term); 
            }
          }
          Expr solLHS = mkplus<ExprSet>(solTerms, m_efac); 
          // c is length of coef 
          sol = createSol(solLHS, vec[0], coef[c-1], m_efac); 
          t.insert(sol); 
          bool success = true; 
          // inserted vec[0] previously, review that later
          for (unsigned k = 1; k < vec.size(); k++) {
            if(!(u.implies(conjoin(t, m_efac), vec[k]))) success = false;
            else t.insert(vec[k]);  
          }
          if (success) {
            outs() << "SUCCESS " << *sol << "\n"; 
            return sol; 
          } 
        } 
      }
    }
    sol = mk<TRUE>(m_efac);
    return sol; 
    
    /*
    Expr RHS = mkTerm(mpz_class(1), m_efac); 
    Expr LHS = vec[0]->left()->left(); 
    Expr temp = mk<GT>(LHS, RHS); 
    sol = temp; 
    t.insert(sol); 
    outs() << *sol << "\n";
    outs() << "\n\n"; 
    for (unsigned i = 1; i < vec.size(); i++)
    {
      if (u.implies(conjoin(t, m_efac), vec[i])) outs () << "SUCCESS\n";
      else outs () << "FAIL\n";
      t.insert(vec[i]);
    }
    return sol;
    */


  }
}

#endif
