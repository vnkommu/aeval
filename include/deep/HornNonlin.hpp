#ifndef HORNNONLIN__HPP__
#define HORNNONLIN__HPP__

#include "ae/AeValSolver.hpp"

using namespace std;
using namespace boost;

namespace ufo
{
  // all adapted from Horn.hpp; experimental; to merge with Horn.hpp at some point
  inline bool rewriteHelperConsts(Expr& body, Expr v1, Expr v2)
  {
    if (isOpX<MPZ>(v1))
    {
      body = mk<AND>(body, mk<EQ>(v1, v2));
      return true;
    }
    else if (isOpX<TRUE>(v1))
    {
      body = mk<AND>(body, v2);
      return true;
    }
    else if (isOpX<FALSE>(v1))
    {
      body = mk<AND>(body, mk<NEG>(v2));
      return true;
    }
    return false;
  }

  struct HornRuleExt
  {
    vector<ExprVector> srcVars;
    ExprVector dstVars;
    ExprVector locVars;

    Expr body;
    Expr head;

    ExprVector srcRelations;
    Expr dstRelation;

    bool isFact;
    bool isQuery;
    bool isInductive;

    void assignVarsAndRewrite (vector<ExprVector>& _srcVars, vector<ExprVector>& invVarsSrc,
                               ExprVector& _dstVars, ExprVector& invVarsDst)
    {
      for (int i = 0; i < _srcVars.size(); i++)
      {
        ExprVector tmp;
        for (int j = 0; j < _srcVars[i].size(); j++)
        {
          tmp.push_back(invVarsSrc[i][j]);
          body = mk<AND>(body, mk<EQ>(_srcVars[i][j], tmp[j]));
        }
        srcVars.push_back(tmp);
      }

      for (int i = 0; i < _dstVars.size(); i++)
      {
        // primed copy of var:
        Expr new_name = mkTerm<string> (lexical_cast<string>(invVarsDst[i]) + "'", body->getFactory());
        Expr var = cloneVar(invVarsDst[i], new_name);
        dstVars.push_back(var);
        body = mk<AND>(body, mk<EQ>(_dstVars[i], dstVars[i]));
      }
    }
  };

  class CHCs
  {
    private:
    set<int> indeces;
    string varname = "_FH_";

    public:

    ExprFactory &m_efac;
    EZ3 &m_z3;

    ExprSet decls;
    Expr failDecl;
    vector<HornRuleExt> chcs;
    map<Expr, ExprVector> invVars;
    map<Expr, vector<int>> incms;
    int qCHCNum;  // index of the query in chc
    int total_var_cnt = 0;

    CHCs(ExprFactory &efac, EZ3 &z3) : m_efac(efac), m_z3(z3) {};

    bool isFapp (Expr e)
    {
      if (isOpX<FAPP>(e))
        if (e->arity() > 0)
          if (isOpX<FDECL>(e->arg(0)))
            if (e->arg(0)->arity() >= 2)
              return true;
      return false;
    }

    void preprocess (Expr term, ExprVector& locVars, vector<ExprVector>& srcVars, ExprVector &srcRelations, ExprSet& lin)
    {
      if (isOpX<AND>(term))
      {
        for (auto it = term->args_begin(), end = term->args_end(); it != end; ++it)
        {
          preprocess(*it, locVars, srcVars, srcRelations, lin);
        }
      }
      else
      {
        if (bind::isBoolConst(term))
        {
          lin.insert(term);
        }
        if (isOpX<FAPP>(term) && find(locVars.begin(), locVars.end(), term) == locVars.end())
        {
          if (term->arity() > 0)
          {
            if (isOpX<FDECL>(term->arg(0)))
            {
              Expr rel = term->arg(0);
              if (rel->arity() >= 2)
              {
                addDecl(rel);
                srcRelations.push_back(rel->arg(0));
                ExprVector tmp;
                for (auto it = term->args_begin()+1, end = term->args_end(); it != end; ++it)
                  tmp.push_back(*it);
                srcVars.push_back(tmp);
              }
            }
          }
        }
        else
        {
          lin.insert(term);
        }
      }
    }

    void addDecl (Expr a)
    {
      if (invVars[a->arg(0)].size() == 0)
      {
        decls.insert(a);
        for (int i = 1; i < a->arity()-1; i++)
        {
          Expr new_name = mkTerm<string> (varname + to_string(total_var_cnt), m_efac);
          total_var_cnt++;
          Expr var;
          if (isOpX<INT_TY> (a->arg(i)))
            var = bind::intConst(new_name);
          else if (isOpX<REAL_TY> (a->arg(i)))
            var = bind::realConst(new_name);
          else if (isOpX<BOOL_TY> (a->arg(i)))
            var = bind::boolConst(new_name);
          else if (isOpX<ARRAY_TY> (a->arg(i))) // GF: currently support only arrays over Ints
          {
            var = bind::mkConst(new_name, mk<ARRAY_TY>
                  (mk<INT_TY> (m_efac), mk<INT_TY> (m_efac)));
          }
          invVars[a->arg(0)].push_back(var);
        }
      }
    }

    bool normalize (Expr& r, HornRuleExt& hr)
    {
      r = regularizeQF(r);

      // TODO: support more syntactic replacements
      while (isOpX<FORALL>(r))
      {
        for (int i = 0; i < r->arity() - 1; i++)
        {
          hr.locVars.push_back(bind::fapp(r->arg(i)));
        }
        r = r->last();
      }

      if (isOpX<NEG>(r) && isOpX<EXISTS>(r->first()))
      {
        for (int i = 0; i < r->first()->arity() - 1; i++)
          hr.locVars.push_back(bind::fapp(r->first()->arg(i)));

        r = mk<IMPL>(r->first()->last(), mk<FALSE>(m_efac));
      }

      if (isOpX<NEG>(r))
      {
        r = mk<IMPL>(r->first(), mk<FALSE>(m_efac));
      }
      else if (isOpX<OR>(r) && r->arity() == 2 && isOpX<NEG>(r->left()) && hasUninterp(r->left()))
      {
        r = mk<IMPL>(r->left()->left(), r->right());
      }
      else if (isOpX<OR>(r) && r->arity() == 2 && isOpX<NEG>(r->right()) && hasUninterp(r->right()))
      {
        r = mk<IMPL>(r->right()->left(), r->left());
      }

      if (isOpX<IMPL>(r) && !isFapp(r->right()) && !isOpX<FALSE>(r->right()))
      {
        if (isOpX<TRUE>(r->right()))
        {
          return false;
        }
        r = mk<IMPL>(mk<AND>(r->left(), mk<NEG>(r->right())), mk<FALSE>(m_efac));
      }

      if (!isOpX<IMPL>(r)) r = mk<IMPL>(mk<TRUE>(m_efac), r);

      return true;
    }

    void parse(string smt)
    {
      std::unique_ptr<ufo::ZFixedPoint <EZ3> > m_fp;
      m_fp.reset (new ZFixedPoint<EZ3> (m_z3));
      ZFixedPoint<EZ3> &fp = *m_fp;
      fp.loadFPfromFile(smt);

      for (auto &r: fp.m_rules)
      {
        chcs.push_back(HornRuleExt());
        HornRuleExt& hr = chcs.back();

        if (!normalize(r, hr))
        {
          chcs.pop_back();
          continue;
        }

        Expr body = r->arg(0);
        Expr head = r->arg(1);

        vector<ExprVector> origSrcSymbs;
        ExprSet lin;
        preprocess(body, hr.locVars, origSrcSymbs, hr.srcRelations, lin);
        if (hr.srcRelations.size() == 0)
        {
          if (hasUninterp(body))
          {
            errs () << "Unsupported format\n";
            errs () << "   " << *body << "\n";
            exit (0);
          }
        }

        hr.isFact = hr.srcRelations.empty();

        if (isOpX<FAPP>(head))
        {
          if (head->arg(0)->arity() == 2 && !hr.isFact)
          {
            addFailDecl(head->arg(0)->arg(0));
          }
          else
          {
            addDecl(head->arg(0));
          }
          hr.head = head->arg(0);
          hr.dstRelation = hr.head->arg(0);
        }
        else
        {
          if (!isOpX<FALSE>(head)) body = mk<AND>(body, mk<NEG>(head));
          addFailDecl(mk<FALSE>(m_efac));
          hr.head = mk<FALSE>(m_efac);
          hr.dstRelation = mk<FALSE>(m_efac);
        }

        hr.isQuery = (hr.dstRelation == failDecl);
        hr.isInductive = (hr.srcRelations.size() == 1 && hr.srcRelations[0] == hr.dstRelation);
        if (hr.isQuery) qCHCNum = chcs.size() - 1;

        ExprVector allOrigSymbs;
        for (auto & a : origSrcSymbs) for (auto & b : a) allOrigSymbs.push_back(b);
        ExprVector origDstSymbs;
        if (!hr.isQuery)
        {
          for (auto it = head->args_begin()+1, end = head->args_end(); it != end; ++it)
            origDstSymbs.push_back(*it);
        }
        allOrigSymbs.insert(allOrigSymbs.end(), origDstSymbs.begin(), origDstSymbs.end());
        simplBoolReplCnj(allOrigSymbs, lin); // perhaps, not a very important optimization now; consider removing
        hr.body = conjoin(lin, m_efac);

        vector<ExprVector> tmp;
        // we may have several applications of the same predicate symbol in the body:
        for (int i = 0; i < hr.srcRelations.size(); i++)
        {
          auto & a = hr.srcRelations[i];
          ExprVector tmp1;
          for (int j = 0; j < i; j++)
          {
            if (hr.srcRelations[i] == hr.srcRelations[j])
            {
              for (int k = 0; k < invVars[a].size(); k++)
              {
                Expr new_name = mkTerm<string> (varname + to_string(++total_var_cnt), m_efac);
                tmp1.push_back(cloneVar(invVars[a][k], new_name));
              }
              break;
            }
          }
          if (tmp1.empty())
          {
            tmp1 = invVars[a];
          }
          tmp.push_back(tmp1);
        }
        hr.assignVarsAndRewrite (origSrcSymbs, tmp,
                                 origDstSymbs, invVars[hr.dstRelation]);

        hr.body = simpleQE(hr.body, hr.locVars);

        // GF: ideally, hr.locVars should be empty after QE,
        // but the QE procedure is imperfect, so
        ExprVector body_vars;
        expr::filter (hr.body, bind::IsConst(), std::inserter (body_vars, body_vars.begin ()));
        for (auto it = hr.locVars.begin(); it != hr.locVars.end(); )
        {
          if (find(body_vars.begin(), body_vars.end(), *it) == body_vars.end())
            it = hr.locVars.erase(it);
          else ++it;
        }
      }

      for (int i = 0; i < chcs.size(); i++)
        incms[chcs[i].dstRelation].push_back(i);
    }

    void addFailDecl(Expr decl)
    {
      if (failDecl == NULL)
      {
        failDecl = decl;
      }
      else
      {
        if (failDecl != decl)
        {
          errs () << "Multiple queries are not supported\n";
          exit(0);
        }
      }
    }

    Expr getPostcondition (int i, ExprVector& vars)
    {
      HornRuleExt& hr = chcs[i];
      ExprSet cnjs;
      ExprSet newCnjs;
      getConj(hr.body, cnjs);
      ExprVector allVars = hr.locVars;
      for (auto & a : hr.srcVars) allVars.insert(allVars.end(), a.begin(), a.end());
      for (auto & a : cnjs)
      {
        if (emptyIntersect(a, allVars)) newCnjs.insert(a);
      }
      Expr res = conjoin(newCnjs, m_efac);
      return replaceAll(res, hr.dstVars, vars);
    }

    void print()
    {
      outs() << "CHCs:\n";
      for (auto &hr: chcs) print(hr);
    }

    void print(HornRuleExt& hr)
    {
        if (hr.isFact) outs() << "  INIT:\n";
        if (hr.isInductive) outs() << "  TRANSITION RELATION:\n";
        if (hr.isQuery) outs() << "  BAD:\n";

        outs () << "    ";

        for (int i = 0; i < hr.srcRelations.size(); i++)
        {
          outs () << * hr.srcRelations[i];
          outs () << " (";
          for(auto &a: hr.srcVars[i]) outs() << *a << ", ";
            outs () << "\b\b)";
          outs () << " /\\ ";
        }
        outs () << "\b\b\b\b -> " << * hr.dstRelation;

        if (hr.dstVars.size() > 0)
        {
          outs () << " (";
          for(auto &a: hr.dstVars) outs() << *a << ", ";
          outs () << "\b\b)";
        }
        outs() << "\n    body: " << * hr.body << "\n";
    }
  };
}
#endif
