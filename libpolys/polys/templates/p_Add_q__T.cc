/****************************************
*  Computer Algebra System SINGULAR     *
****************************************/
/***************************************************************
 *  File:    p_Add_q__Template.cc
 *  Purpose: template for p_Add_q
 *  Author:  obachman (Olaf Bachmann)
 *  Created: 8/00
 *******************************************************************/

/***************************************************************
 *
 * Returns:  p + q,
 *           Shorter, where Shorter == Length(p) + Length(q) - Length(p+q);
 * Destroys: p, q
 *
 ***************************************************************/
LINKAGE poly p_Add_q__T(poly p, poly q, int &Shorter, const ring r)
{
  p_Test(p, r);
  p_Test(q, r);
#if PDEBUG > 0
  int l = pLength(p) + pLength(q);
#endif
  assume(p!=NULL && q!=NULL);

  Shorter = 0;

  number t, n1, n2;
  int shorter = 0;
  spolyrec rp;
  poly a = &rp;
  DECLARE_LENGTH(const unsigned long length = r->CmpL_Size);
  DECLARE_ORDSGN(const long* ordsgn = r->ordsgn);

  Top:     // compare p and q w.r.t. monomial ordering
  p_MemCmp__T(p->exp, q->exp, length, ordsgn, goto Equal, goto Greater , goto Smaller);

  Equal:
  n1 = pGetCoeff(p);
  n2 = pGetCoeff(q);
  n_InpAdd__T(n1,n2,r->cf);
  t = n1;
  n_Delete__T(&n2, r->cf);
  q = p_LmFreeAndNext(q, r);

  if (n_IsZero__T(t, r->cf))
  {
    shorter += 2;
    n_Delete__T(&t, r->cf);
    p = p_LmFreeAndNext(p, r);
  }
  else
  {
    shorter++;
    pSetCoeff0(p,t);
    a = pNext(a) = p;
    pIter(p);
  }
  if (p==NULL) { pNext(a) = q; goto Finish;}
  if (q==NULL) { pNext(a) = p; goto Finish;}
  goto Top;

  Greater:
  a = pNext(a) = p;
  pIter(p);
  if (p==NULL) { pNext(a) = q; goto Finish;}
  goto Top;

  Smaller:
  a = pNext(a) = q;
  pIter(q);
  if (q==NULL) { pNext(a) = p; goto Finish;}
  goto Top;


  Finish:
  Shorter = shorter;

  p_Test(pNext(&rp), r);
#if PDEBUG > 0
  pAssume1(l - pLength(pNext(&rp)) == Shorter);
#endif
  return pNext(&rp);
}

