/****************************************
*  Computer Algebra System SINGULAR     *
****************************************/
/*
* ABSTRACT: resolutions
*/

#include <kernel/GBEngine/syz.h>
#include <omalloc/omalloc.h>
#include <coeffs/numbers.h>
#include <kernel/polys.h>
#include <kernel/ideals.h>

#include <vector>
#include <map>

typedef struct {
    poly lt;
    unsigned long sev;
    unsigned int label;
} CLeadingTerm_struct;

typedef std::vector<const CLeadingTerm_struct*> TReducers_test;
typedef std::map<long, TReducers_test> CReducersHash_test;

static poly TraverseTail_test(poly multiplier, const int tail,
   const ideal previous_module, const std::vector<bool> &variables,
   const CReducersHash_test *m_div, const CReducersHash_test *m_checker);
static poly ComputeImage_test(poly multiplier, const int tail,
    const ideal previous_module, const std::vector<bool> &variables,
    const CReducersHash_test *m_div, const CReducersHash_test *m_checker);
static inline poly ReduceTerm_test(poly multiplier, poly term4reduction,
    poly syztermCheck, const ideal previous_module,
    const std::vector<bool> &variables, const CReducersHash_test *m_div,
    const CReducersHash_test *m_checker);
static poly leadmonom_test(const poly p, const ring r, const bool bSetZeroComp = true);

static inline void update_variables(std::vector<bool> &variables,
    const ideal L)
{
    const ring R = currRing;
    const int l = IDELEMS(L)-1;
    int k;
    for (int j = R->N; j > 0; j--) {
        if (variables[j-1]) {
            for (k = l; k >= 0; k--) {
                if (p_GetExp(L->m[k], j, R) > 0) {
                    break;
                }
            }
            if (k < 0) {   // no break
                variables[j-1] = false;
            }
        }
    }
}

static inline bool check_variables(const std::vector<bool> &variables,
    const poly m)
{
    const ring R = currRing;
    for (int j = R->N; j > 0; j--) {
        if (variables[j-1] && p_GetExp(m, j, R) > 0) {
            return true;
        }
    }
    return false;
}

static poly TraverseNF_test(const poly a, const ideal previous_module,
    const std::vector<bool> &variables, const CReducersHash_test *m_div,
    const CReducersHash_test *m_checker)
{
  const ring R = currRing;
  const int r = p_GetComp(a, R) - 1;
  poly aa = leadmonom_test(a, R);
  poly t = TraverseTail_test(aa, r, previous_module, variables, m_div,
      m_checker);
  if (check_variables(variables, aa)) {
    t = p_Add_q(t, ReduceTerm_test(aa, previous_module->m[r], a,
        previous_module, variables, m_div, m_checker), R);
  }
  p_Delete(&aa, R);
  return t;
}

#define CACHE 1

#if CACHE
bool my_p_LmCmp_test (poly a, poly b, const ring r)
{
  return p_LmCmp(a, b, r) == -1;
}

struct CCacheCompare_test
{
  const ring& m_ring_test;
  CCacheCompare_test(): m_ring_test(currRing) {}
  CCacheCompare_test(const ring& r): m_ring_test(r) {}
  CCacheCompare_test(const CCacheCompare_test& lhs):
    m_ring_test(lhs.m_ring_test) {}
  CCacheCompare_test& operator=(const CCacheCompare_test& lhs)
  {
    return (const_cast<CCacheCompare_test&>(lhs));
  }
  inline bool operator() (const poly& l, const poly& r)
    const
  {
    return my_p_LmCmp_test(l, r, m_ring_test);
  }
};

typedef std::map<poly, poly, CCacheCompare_test>
  TP2PCache_test;
typedef std::map<int, TP2PCache_test> TCache_test;

static TCache_test m_cache_test;

static FORCE_INLINE poly myp_Head_test(const poly p, const bool bIgnoreCoeff,
  const ring r)
{
  p_LmCheckPolyRing1(p, r);
  poly np;
  omTypeAllocBin(poly, np, r->PolyBin);
  p_SetRingOfLm(np, r);
  memcpy(np->exp, p->exp, r->ExpL_Size*sizeof(long));
  pNext(np) = NULL;
  pSetCoeff0(np, (bIgnoreCoeff)? NULL : n_Copy(pGetCoeff(p), r->cf));
  p_LmCheckPolyRing1(np, r);
  return np;
}
#endif   // CACHE

static poly TraverseTail_test(poly multiplier, const int tail,
    const ideal previous_module, const std::vector<bool> &variables,
    const CReducersHash_test *m_div, const CReducersHash_test *m_checker)
{
  const ring& r = currRing;
#if CACHE
  TCache_test::iterator top_itr = m_cache_test.find(tail);
  if ( top_itr != m_cache_test.end() )
  {
     TP2PCache_test& T = top_itr->second;
     TP2PCache_test::iterator itr = T.find(multiplier);
     if( itr != T.end() )
     {
       if( itr->second == NULL )
         return (NULL);
       poly p = p_Copy(itr->second, r);
       if( !n_Equal( pGetCoeff(multiplier), pGetCoeff(itr->first), r) )
       {
         number n = n_Div( pGetCoeff(multiplier), pGetCoeff(itr->first), r);
         p = p_Mult_nn(p, n, r);
         n_Delete(&n, r);
       }
       return p;
     }
     const poly p = ComputeImage_test(multiplier, tail, previous_module,
         variables, m_div, m_checker);
     itr = T.find(multiplier);
     if( itr == T.end() )
     {
       T.insert(TP2PCache_test::value_type(myp_Head_test(multiplier, (p==NULL),
         r), p) );
       return p_Copy(p, r);
     }
     return p;
  }
#endif   // CACHE
  const poly p = ComputeImage_test(multiplier, tail, previous_module,
      variables, m_div, m_checker);
#if CACHE
  top_itr = m_cache_test.find(tail);
  if ( top_itr != m_cache_test.end() )
  {
    TP2PCache_test& T = top_itr->second;
    TP2PCache_test::iterator itr = T.find(multiplier);
    if( itr == T.end() )
    {
      T.insert(TP2PCache_test::value_type(myp_Head_test(multiplier, (p==NULL),
          r), p));
      return p_Copy(p, r);
    }
    return p;
  }
  CCacheCompare_test o(r);
  TP2PCache_test T(o);
  T.insert(TP2PCache_test::value_type(myp_Head_test(multiplier, (p==NULL), r),
    p));
  m_cache_test.insert( TCache_test::value_type(tail, T) );
  return p_Copy(p, r);
#else
  return p;
#endif   // CACHE
}

static poly ComputeImage_test(poly multiplier, const int t,
    const ideal previous_module, const std::vector<bool> &variables,
    const CReducersHash_test *m_div, const CReducersHash_test *m_checker)
{
  const poly tail = previous_module->m[t]->next;
  if(tail != NULL)
  {
    if (!check_variables(variables, multiplier))
    {
      return NULL;
    }
    sBucket_pt sum = sBucketCreate(currRing);
    for(poly p = tail; p != NULL; p = pNext(p))   // iterate over the tail
    {
      const poly rt = ReduceTerm_test(multiplier, p, NULL, previous_module,
          variables, m_div, m_checker);
      sBucket_Add_p(sum, rt, pLength(rt));
    }
    poly s;
    int l;
    sBucketClearAdd(sum, &s, &l);
    sBucketDestroy(&sum);
    return s;
  }
  return NULL;
}

static poly leadmonom_test(const poly p, const ring r, const bool bSetZeroComp)
{
  poly m = p_LmInit(p, r);
  p_SetCoeff0(m, n_Copy(p_GetCoeff(p, r), r), r);
  if( bSetZeroComp )
    p_SetComp(m, 0, r);
  p_Setm(m, r);
  return m;
}

// _p_LmDivisibleByNoComp for a | b*c
static inline BOOLEAN _p_LmDivisibleByNoComp(const poly a, const poly b, const poly c, const ring r)
{
  int i=r->VarL_Size - 1;
  unsigned long divmask = r->divmask;
  unsigned long la, lb;

  if (r->VarL_LowIndex >= 0)
  {
    i += r->VarL_LowIndex;
    do
    {
      la = a->exp[i];
      lb = b->exp[i] + c->exp[i];
      if ((la > lb) ||
          (((la & divmask) ^ (lb & divmask)) != ((lb - la) & divmask)))
      {
        return FALSE;
      }
      i--;
    }
    while (i>=r->VarL_LowIndex);
  }
  else
  {
    do
    {
      la = a->exp[r->VarL_Offset[i]];
      lb = b->exp[r->VarL_Offset[i]] + c->exp[r->VarL_Offset[i]];
      if ((la > lb) ||
          (((la & divmask) ^ (lb & divmask)) != ((lb - la) & divmask)))
      {
        return FALSE;
      }
      i--;
    }
    while (i>=0);
  }
  return TRUE;
}

static void deleteCRH(CReducersHash_test *C)
{
    for (CReducersHash_test::iterator it = C->begin(); it != C->end(); it++) {
        TReducers_test& v = it->second;
        for (TReducers_test::const_iterator vit = v.begin(); vit != v.end();
            vit++) {
            omfree(const_cast<CLeadingTerm_struct*>(*vit));
        }
        v.erase(v.begin(), v.end());
    }
    C->erase(C->begin(), C->end());
}

bool IsDivisible(const CReducersHash_test *C, const poly product)
{
    CReducersHash_test::const_iterator m_itr
        = C->find(p_GetComp(product, currRing));
    if (m_itr == C->end()) {
        return false;
    }
    TReducers_test::const_iterator m_current = (m_itr->second).begin();
    TReducers_test::const_iterator m_finish  = (m_itr->second).end();
    const unsigned long m_not_sev = ~p_GetShortExpVector(product, currRing);
    for ( ; m_current != m_finish; ++m_current) {
        if (p_LmShortDivisibleByNoComp((*m_current)->lt, (*m_current)->sev,
            product, m_not_sev, currRing)) {
            return true;
        }
    }
    return false;
}


poly FindReducer(const poly multiplier, const poly t, const poly syzterm,
    const CReducersHash_test *syz_checker, const CReducersHash_test *m_div)
{
  const ring r = currRing;
  CReducersHash_test::const_iterator m_itr
      = m_div->find(p_GetComp(t, currRing));
  if (m_itr == m_div->end()) {
    return NULL;
  }
  TReducers_test::const_iterator m_current = (m_itr->second).begin();
  TReducers_test::const_iterator m_finish  = (m_itr->second).end();
  if (m_current == m_finish) {
    return NULL;
  }
  const poly q = p_New(r);
  pNext(q) = NULL;
  const unsigned long m_not_sev = ~p_GetShortExpVector(multiplier, t, r);
  for( ; m_current != m_finish; ++m_current) {
    if ( ((*m_current)->sev & m_not_sev)
        || !(_p_LmDivisibleByNoComp((*m_current)->lt, multiplier, t, r))) {
      continue;
    }
    const poly p = (*m_current)->lt;
    const int k  = (*m_current)->label;
    p_ExpVectorSum(q, multiplier, t, r); // q == product == multiplier * t
    p_ExpVectorDiff(q, q, p, r); // (LM(product) / LM(L[k]))
    p_SetComp(q, k + 1, r);
    p_Setm(q, r);
    // cannot allow something like: a*gen(i) - a*gen(i)
    if (syzterm != NULL && (k == p_GetComp(syzterm, r) - 1)
        && p_ExpVectorEqual(syzterm, q, r)) {
      continue;
    }
    if (IsDivisible(syz_checker, q)) {
      continue;
    }
    number n = n_Mult(p_GetCoeff(multiplier, r), p_GetCoeff(t, r), r);
    p_SetCoeff0(q, n_InpNeg(n, r), r);
    return q;
  }
  p_LmFree(q, r);
  return NULL;
}

static inline poly ReduceTerm_test(poly multiplier, poly term4reduction,
    poly syztermCheck, const ideal previous_module,
    const std::vector<bool> &variables, const CReducersHash_test *m_div,
    const CReducersHash_test *m_checker)
{
  const ring r = currRing;
  poly s = FindReducer(multiplier, term4reduction, syztermCheck, m_checker,
        m_div);
  if( s == NULL )
  {
    return NULL;
  }
  poly b = leadmonom_test(s, r);
  const int c = p_GetComp(s, r) - 1;
  const poly t
      = TraverseTail_test(b, c, previous_module, variables, m_div, m_checker);
  pDelete(&b);
  if( t != NULL )
    s = p_Add_q(s, t, r);
  return s;
}

static void initialize(CReducersHash_test &C, const ideal L)
{
  if( L != NULL )
  {
    const ring R = currRing;
    for( int k = IDELEMS(L) - 1; k >= 0; k-- )
    {
      const poly a = L->m[k];
      if( a != NULL )
      {
        CLeadingTerm_struct *CLT
            = (CLeadingTerm_struct*)omalloc(sizeof(CLeadingTerm_struct));
        CLT->lt = a;
        CLT->sev = p_GetShortExpVector(a, R);
        CLT->label = k;
        C[p_GetComp(a, R)].push_back( CLT );
      }
    }
  }
}

/*****************************************************************************/

typedef poly syzHeadFunction(ideal, int, int);

static poly syzHeadFrame(const ideal G, const int i, const int j)
{
    const ring r = currRing;
    const poly f_i = G->m[i];
    const poly f_j = G->m[j];
    poly head = p_Init(r);
    pSetCoeff0(head, n_Init(1, r->cf));
    long exp_i, exp_j, lcm;
    for (int k = (int)r->N; k > 0; k--) {
        exp_i = p_GetExp(f_i, k, r);
        exp_j = p_GetExp(f_j, k, r);
        lcm = si_max(exp_i, exp_j);
        p_SetExp(head, k, lcm-exp_i, r);
    }
    p_SetComp(head, i+1, r);
    p_Setm(head, r);
    return head;
}

static poly syzHeadExtFrame(const ideal G, const int i, const int j)
{
    const ring r = currRing;
    const poly f_i = G->m[i];
    const poly f_j = G->m[j];
    poly head = p_Init(r);
    pSetCoeff0(head, n_Init(1, r->cf));
    poly head_ext = p_Init(r);
    pSetCoeff0(head_ext, n_InpNeg(n_Div(pGetCoeff(f_j), pGetCoeff(f_i), r->cf),
        r->cf));
    long exp_i, exp_j, lcm;
    for (int k = (int)r->N; k > 0; k--) {
        exp_i = p_GetExp(f_i, k, r);
        exp_j = p_GetExp(f_j, k, r);
        lcm = si_max(exp_i, exp_j);
        p_SetExp(head, k, lcm-exp_i, r);
        p_SetExp(head_ext, k, lcm-exp_j, r);
    }
    p_SetComp(head, i+1, r);
    p_Setm(head, r);
    p_SetComp(head_ext, j+1, r);
    p_Setm(head_ext, r);
    head->next = head_ext;
    return head;
}

typedef ideal syzM_i_Function(ideal, int, syzHeadFunction);

static ideal syzM_i_unsorted(const ideal G, const int i,
    const syzHeadFunction *syzHead)
{
    ideal M_i = NULL;
    int comp = pGetComp(G->m[i]);
    int ncols = 0;
    for (int j = i-1; j >= 0; j--) {
        if (pGetComp(G->m[j]) == comp) ncols++;
    }
    if (ncols > 0) {
        M_i = idInit(ncols, G->ncols);
        int k = ncols-1;
        for (int j = i-1; j >= 0; j--) {
            if (pGetComp(G->m[j]) == comp) {
                M_i->m[k] = syzHead(G, i, j);
                k--;
            }
        }
        id_DelDiv(M_i, currRing);
        idSkipZeroes(M_i);
    }
    return M_i;
}

static ideal syzM_i_sorted(const ideal G, const int i,
    const syzHeadFunction *syzHead)
{
    ideal M_i = NULL;
    int comp = pGetComp(G->m[i]);
    int index = i-1;
    while (pGetComp(G->m[index]) == comp) index--;
    index++;
    int ncols = i-index;
    if (ncols > 0) {
        M_i = idInit(ncols, G->ncols);
        for (int j = ncols-1; j >= 0; j--) {
            M_i->m[j] = syzHead(G, i, j+index);
        }
        id_DelDiv(M_i, currRing);
        idSkipZeroes(M_i);
    }
    return M_i;
}

static ideal idConcat(const ideal* M, const int size, const int rank)
{
    int ncols = 0;
    for (int i = size-1; i >= 0; i--) {
        if (M[i] != NULL) {
            ncols += M[i]->ncols;
        }
    }
    if (ncols == 0) return idInit(1, rank);
    ideal result = idInit(ncols, rank);
    int k = ncols-1;
    for (int i = size-1; i >= 0; i--) {
        if (M[i] != NULL) {
            for (int j = M[i]->ncols-1; j >= 0; j--) {
                result->m[k] = M[i]->m[j];
                k--;
            }
        }
    }
    return result;
}

#define SORT_MI 1

#if SORT_MI
static int compare_Mi(const void* a, const void *b)
{
    const ring r = currRing;
    poly p_a = *((poly *)a);
    poly p_b = *((poly *)b);
    int cmp;
    int deg_a = p_Deg(p_a, r);
    int deg_b = p_Deg(p_b, r);
    cmp = (deg_a > deg_b) - (deg_a < deg_b);
    if (cmp != 0) {
         return cmp;
    }
    int comp_a = p_GetComp(p_a, r);
    int comp_b = p_GetComp(p_b, r);
    cmp = (comp_a > comp_b) - (comp_a < comp_b);
    if (cmp != 0) {
         return cmp;
    }
    int exp_a[r->N+1];
    int exp_b[r->N+1];
    p_GetExpV(p_a, exp_a, r);
    p_GetExpV(p_b, exp_b, r);
    for (int i = r->N; i > 0; i--) {
        cmp = (exp_a[i] > exp_b[i]) - (exp_a[i] < exp_b[i]);
        if (cmp != 0) {
            return cmp;
        }
    }
    return 0;
}
#endif   // SORT_MI

static ideal computeFrame(const ideal G, const syzM_i_Function syzM_i,
    const syzHeadFunction *syzHead)
{
    ideal* M = (ideal *)omalloc((G->ncols-1)*sizeof(ideal));
    for (int i = G->ncols-2; i >= 0; i--) {
        M[i] = syzM_i(G, i+1, syzHead);
    }
    ideal frame = idConcat(M, G->ncols-1, G->ncols);
    for (int i = G->ncols-2; i >= 0; i--) {
        if (M[i] != NULL) {
            omFreeSize(M[i]->m, IDELEMS(M[i])*sizeof(poly));
            omFreeBin(M[i], sip_sideal_bin);
        }
    }
    omFree(M);
#if SORT_MI
    qsort(frame->m, IDELEMS(frame), sizeof(poly), compare_Mi);
#endif
    return frame;
}

static void setGlobalVariables()
{
#if CACHE
    for (TCache_test::iterator it = m_cache_test.begin();
        it != m_cache_test.end(); it++) {
        TP2PCache_test& T = it->second;
        for (TP2PCache_test::iterator vit = T.begin(); vit != T.end(); vit++) {
            p_Delete((&(vit->second)), currRing);
            p_Delete(const_cast<poly*>(&(vit->first)), currRing);
        }
        T.erase(T.begin(), T.end());
    }
    m_cache_test.erase(m_cache_test.begin(), m_cache_test.end());
#endif   // CACHE
}

static void computeLiftings(const resolvente res, const int index,
    std::vector<bool> &variables)
{
    update_variables(variables, res[index-1]);
    CReducersHash_test m_div;
    initialize(m_div, res[index-1]);
    CReducersHash_test m_checker;
    initialize(m_checker, res[index]);
    setGlobalVariables();
    poly p;
    for (int j = res[index]->ncols-1; j >= 0; j--) {
        p = res[index]->m[j];
        pDelete(&res[index]->m[j]->next);
        p->next = NULL;
        res[index]->m[j]->next = TraverseNF_test(p, res[index-1], variables,
            &m_div, &m_checker);
    }
    deleteCRH(&m_checker);
    deleteCRH(&m_div);
}

static void sortPolysTails(const resolvente res, const int index)
{
    const ring r = currRing;
    for (int j = res[index]->ncols-1; j >= 0; j--) {
        if (res[index]->m[j]->next != NULL) {
            res[index]->m[j]->next->next
                = p_SortAdd(res[index]->m[j]->next->next, r);
        }
    }
}

static int computeResolution(resolvente &res, const int length,
    const syzHeadFunction *syzHead)
{
    int max_index = length-1;
    int index = 0;
    if (!idIs0(res[index]) && index < max_index) {
        index++;
        res[index] = computeFrame(res[index-1], syzM_i_unsorted, syzHead);
        std::vector<bool> variables;
        variables.resize(currRing->N, true);
        while (!idIs0(res[index])) {
#if 1
            computeLiftings(res, index, variables);
            sortPolysTails(res, index);
#endif   // LIFT
            if (index < max_index) {
                index++;
                res[index] = computeFrame(res[index-1], syzM_i_sorted,
                    syzHead);
            }
            else {
                break;
            }
        }
        variables.clear();
    }
    max_index = index+1;
    if (max_index < length) {
        res = (resolvente)omReallocSize(res, (length+1)*sizeof(ideal),
            (max_index+1)*sizeof(ideal));
    }
    return max_index;
}

static void sortPolys(const resolvente res, const int length)
{
    const ring r = currRing;
    for (int i = length-1; i > 0; i--) {
        for (int j = res[i]->ncols-1; j >= 0; j--) {
            res[i]->m[j] = p_SortAdd(res[i]->m[j], r, TRUE);
        }
    }
}

syStrategy syFrank(const ideal arg, int &length, const char *method)
{
    syStrategy result = (syStrategy)omAlloc0(sizeof(ssyStrategy));
    resolvente res = (resolvente)omAlloc0((length+1)*sizeof(ideal));
    res[0] = id_Copy(arg, currRing);
    syzHeadFunction *syzHead;
    if (strcmp(method, "frame") == 0 || strcmp(method, "linear strand") == 0) {
        syzHead = syzHeadFrame;
    }
    else {
        syzHead = syzHeadExtFrame;
    }
    length = computeResolution(res, length, syzHead);
    sortPolys(res, length);
#if CACHE
    for (TCache_test::iterator it = m_cache_test.begin();
        it != m_cache_test.end(); it++) {
        TP2PCache_test& T = it->second;
        for (TP2PCache_test::iterator vit = T.begin(); vit != T.end(); vit++) {
            p_Delete((&(vit->second)), currRing);
            p_Delete(const_cast<poly*>(&(vit->first)), currRing);
        }
        T.erase(T.begin(), T.end());
    }
    m_cache_test.erase(m_cache_test.begin(), m_cache_test.end());
#endif   // CACHE

    result->fullres = res;
    result->length = length;
    return result;
}

