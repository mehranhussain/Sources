/*---------------------------------------------------------------------------
/                         Integral LLL Algorithm                            |
/                                                                           |
/                           Implemented by                                  |
/          Alberto Vigneron-Tenorio and Alfredo S�nchez-Navarro             |
/            alberto.vigneron@uca.es, alfredo.sanchez@uca.es                |
/                 E.U.E.Empresariales de Jerez.                             |
/                   Universidad de C�diz                                    |
/                   Porvera 54.                                             |
/                 Jerez de la Frontera (C�diz, Spain)                       |
/                                                                           |
/                                                                           |
/ This application computes the LLL-reduced basis of a Z-lattice using      |
/ the algorithm 2.6.7 appeared in:                                          |
/ Cohen, H. A course in computational algebraic number theory.              |
/ GTM 138, Springer 1996.                                                   |
/                                                                           |
/ The Input is a file with the structure:                                   |
/ Number of vectors, Number of coordinates                                  |
/ b11 b12 b13 ... b1C                                                       |
/ b21 b22 b23 ... b2C                                                       |
/       ...                                                                 |
/ bF1 bF2 bF3 ... bFC                                                       |
/                                                                           |
/ All these inputs are integer numbers.                                     |
/                                                                           |
/ The Output is a LLL-reduced basis of the Z-lattice generated by           |
/ (b11,b12,...,b1C),...,(bF1,...,bFC)                                       |
/                                                                           |
/ All these outputs are integer numbers.                                    |
---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
| EXAMPLE:                                                                  |
|                                                                           |
| Let prf be the file:                                                      |
| 2,3                                                                       |
| 1 -2 4                                                                    |
| 2 1 -7                                                                    |
| then:                                                                     |
|                                                                           |
| ./LLL_lattice <prf                                                        |
| Output:                                                                   |
|  1  -2  4                                                                 |
|  3  -1  -3                                                                |
|                                                                           |
| The output is a LLL-reduced basis of the lattice of ZxZxZ generated by    |
| (1,-2,4),(2,1,-7)                                                         |
|                                                                           |
---------------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <new>
#include <cstring>
#include <gmp.h>

#define LLL_MAX(a,b) (a > b ? a : b)

/* Global variables */
mpz_t * lll_d;
mpz_t ** lll_lambda;
mpz_t ** lll_b;

mpz_t **proc_entrada (int *F, int *C);
void lll_swapi (int k, int kmax, int C);
void lll_redi (int k, int l, int C);
void lll_test (int *k, int kma, int C);
void lll_product (mpz_t * a, mpz_t * f, int n, mpz_t * prod);

int main (void)
{
  int i, j, k, kmax;
  int F, C;
  mpz_t prod;
  mpz_t u;
  mpz_t auxi1;
  mpz_t auxi2;
  mpz_init (prod);
  mpz_init (u);
  mpz_init (auxi1);
  mpz_init (auxi2);

  k = 2;    /* Initialize */
  kmax = 1; /* Initialize */

  lll_b = proc_entrada (&F, &C);

  lll_d = new mpz_t[F + 1];
  lll_lambda = new (mpz_t *)[F + 1];
  mpz_init (lll_d[0]);
  for (i = 1; i <= F; i++) {
    mpz_init (lll_d[i]);
    lll_lambda[i] = new mpz_t[F + 1];
    for (j = 1; j <= F; j++)
      mpz_init (lll_lambda[i][j]);
  }
  mpz_set_ui (lll_d[0], 1); /* Initialize */
  lll_product (lll_b[1], lll_b[1], C, &prod);
  mpz_set (lll_d[1], prod); /* Initialize */


/* Incremental Gram-Schmidt */
  while (k <= F)
  {
    if (k > kmax)
    {
      kmax = k;
      for (j = 1; j < k + 1; j++)
      {
        lll_product (lll_b[k], lll_b[j], C, &prod);
        mpz_set (u, prod);
        for (i = 1; i < j - 1 + 1; i++)
        {
          mpz_mul (auxi1, lll_d[i], u);
          mpz_mul (auxi2, lll_lambda[k][i], lll_lambda[j][i]);
          mpz_sub (u, auxi1, auxi2);
          mpz_tdiv_q (u, u, lll_d[i - 1]);
        }
        if (j < k)
        {
          mpz_set (lll_lambda[k][j], u);
        }
        else
        {
          mpz_set (lll_d[k], u);
          if (mpz_cmp_ui (lll_d[k], 0) == 0)
          { /* The input is not a basis */
            fprintf (stderr, "\n INPUT NOT BASIS !!!! \n");
            exit (1);
          }
        }
      }
    }
    lll_test (&k, kmax, C);
    for (i = k - 2; i > 0; i--)
      lll_redi (k, i, C);
    k = k + 1;
  }
  fprintf (stdout, "list res=");
  for (i = 1; i < F + 1; i++)
  {
    printf ("\nlist(");
    for (j = 1; j < C + 1; j++)
    {
      fprintf (stdout, " ");
      mpz_out_str (stdout, 0, lll_b[i][j]);
      if (j!=C) fprintf (stdout, ",");
    }
    if (i!=F) printf("),\n");
    else      printf(");\n");
  }
  mpz_clear (auxi1);
  mpz_clear (auxi2);
  mpz_clear (u);
  mpz_clear (prod);
  mpz_clear (lll_d[0]);
  for (i = 1; i <= F; i++)
  {
    mpz_clear (lll_d[i]);
    for (j = 1; j <= F; j++)
      mpz_clear (lll_lambda[i][j]);
    delete[]lll_lambda[i];
  }
  delete[]lll_lambda;
  delete[]lll_d;
  for (i = 1; i < F + 1; i++)
  {
    for (j = 1; j < C + 1; j++)
       mpz_clear (lll_b[i][j]);
    delete[]lll_b[i];
  }
  delete[]lll_b;
  return 0;
}

/*---------------------------------------------------------------------------
| proc_entrada {}:                                                          |
| Read the input file.                                                      |
|___________________________________________________________________________|*/

mpz_t **proc_entrada (int *F, int *C) /* Inputs */
{
  int numF, numC;
  int i, j, c;
  mpz_t **base;
  mpz_t valor;
  char *aux, caux;
  int tamano=100;

  aux=new char[tamano];
  fscanf (stdin, "%i, %i\n", &numF, &numC);
  base = new (mpz_t *)[numF+1];
  for (i = 1; i <= numF; i++) {
    base[i] = new mpz_t[numC+1];
    for (j = 1; j <= numC; j++)
    {
      do { caux=getc (stdin); }
      while (!feof(stdin) && !(caux=='-' || isdigit(caux)));
      mpz_init (base[i][j]);
      c=0;
      if (!feof(stdin))
      {
        do
        {
          aux[c++]=caux;
          if (c==tamano)
          {
            char *nuevo=new char[3*tamano/2];
            memcpy (nuevo, aux, tamano);
            delete[] aux;
            aux=nuevo;
            tamano=3*tamano/2;
          }
          caux=getc(stdin);
        }
        while (!feof(stdin) && caux!=' ' && caux!='\n' && caux!='\t');
        aux[c]='\0';
        mpz_set_str (base[i][j], aux, 0);
      }
      else
      {
        fprintf(stderr,"Empty file.\n");
        mpz_set_ui (base[i][j],0);
      }
    }
  }
  *F = numF;
  *C = numC;
  delete[] aux;
  return (base);
}

/*---------------------------------------------------------------------------
| lll_product {}:                                                           |
| Multiply the integer vectors a and f with n coordinates.                  |
|___________________________________________________________________________|*/

void  lll_product (mpz_t * a, mpz_t * f, int n, mpz_t * prod)
{
  int i;
  mpz_t auxi;

  mpz_init (auxi);
  mpz_set_ui (*prod, 0);
  for (i = 1; i < n + 1; i++)
  {
    mpz_mul (auxi, a[i], f[i]);
    mpz_add (*prod, *prod, auxi);
  }
  mpz_clear (auxi);
}

void lll_test (int *k, int kma, int C) /* Test LLL condition */
{
  int i;
  mpz_t auxi1;
  mpz_t auxi2;
  mpz_t auxi3;
  mpz_t auxi4;

  mpz_init (auxi1);
  mpz_init (auxi2);
  mpz_init (auxi3);

  lll_redi (*k, *k - 1, C);
  mpz_mul (auxi1, lll_d[*k], lll_d[*k - 2]);
  mpz_mul_ui (auxi1, auxi1, 4);
  mpz_mul (auxi2, lll_d[*k - 1], lll_d[*k - 1]);
  mpz_mul_ui (auxi2, auxi2, 3);
  mpz_mul (auxi3, lll_lambda[*k][*k - 1], lll_lambda[*k][*k - 1]);
  mpz_mul_ui (auxi3, auxi3, 4);
  mpz_sub (auxi3, auxi2, auxi3);
  if (mpz_cmp (auxi1, auxi3) < 0)
  {
    lll_swapi (*k, kma, C);
    *k = LLL_MAX (2, *k - 1);
    lll_test (&*k, kma, C);
  }

  mpz_clear (auxi1);
  mpz_clear (auxi2);
  mpz_clear (auxi3);
}

/*---------------------------------------------------------------------------
| lll_redi {}:                                                              |
| Make some changes in global variables.                                    |
|___________________________________________________________________________|*/


void lll_redi (int k, int l, int C)
{
  int i;
  mpz_t q;
  mpz_t auxi1;

  mpz_init (q);
  mpz_init (auxi1);

  mpz_mul_ui (auxi1, lll_lambda[k][l], 2);
  mpz_abs (auxi1, auxi1);

    if (mpz_cmp (auxi1, lll_d[l]) > 0)
    {
      mpz_mul_ui (auxi1, lll_lambda[k][l], 2);
      mpz_add (auxi1, auxi1, lll_d[l]);
      mpz_mul_ui (q, lll_d[l], 2);
      mpz_fdiv_q (q, auxi1, q);
      for (i = 1; i < C + 1; i++)
      {
        mpz_mul (auxi1, q, lll_b[l][i]);
        mpz_sub (lll_b[k][i], lll_b[k][i], auxi1);
      }
      mpz_mul (auxi1, q, lll_d[l]);
      mpz_sub (lll_lambda[k][l], lll_lambda[k][l], auxi1);
      for (i = 1; i <= l - 1; i++)
      {
        mpz_mul (auxi1, q, lll_lambda[l][i]);
        mpz_sub (lll_lambda[k][i], lll_lambda[k][i], auxi1);
      }
    }
    mpz_clear (q);
    mpz_clear (auxi1);
}

/*---------------------------------------------------------------------------
| lll_swapi {}:                                                             |
| Exchange some global variables                                            |
|___________________________________________________________________________|*/

void lll_swapi (int k, int kmax, int C)
{
  int j;
  mpz_t lamb;
  mpz_t B;
  mpz_t auxi1;
  mpz_t auxi2;
  mpz_t t;
  mpz_t swap;

  mpz_init (lamb);
  mpz_init (B);
  mpz_init (auxi1);
  mpz_init (auxi2);
  mpz_init (t);
  mpz_init (swap);

  for (j = 1; j < C + 1; j++)
  {
    mpz_set (swap, lll_b[k][j]);
    mpz_set (lll_b[k][j], lll_b[k - 1][j]);
    mpz_set (lll_b[k - 1][j], swap);
  }
  if (k > 2)
  {
    for (j = 1; j <= k - 2; j++)
    {
      mpz_set (swap, lll_lambda[k][j]);
      mpz_set (lll_lambda[k][j], lll_lambda[k - 1][j]);
      mpz_set (lll_lambda[k - 1][j], swap);
    }
  }

  mpz_set (lamb, lll_lambda[k][k - 1]);
  mpz_mul (auxi1, lll_d[k - 2], lll_d[k]);
  mpz_mul (auxi2, lamb, lamb);
  mpz_add (B, auxi1, auxi2);
  mpz_tdiv_q (B, B, lll_d[k - 1]);
  for (j = k + 1; j <= kmax; j++)
  {
    mpz_set (t, lll_lambda[j][k]);
    mpz_mul (auxi1, lll_d[k], lll_lambda[j][k - 1]);
    mpz_mul (auxi2, lamb, t);
    mpz_sub (auxi1, auxi1, auxi2);
    mpz_tdiv_q (lll_lambda[j][k], auxi1, lll_d[k - 1]);
    mpz_mul (auxi2, lamb, lll_lambda[j][k]);
    mpz_mul (auxi1, B, t);
    mpz_add (auxi1, auxi1, auxi2);
    mpz_tdiv_q (lll_lambda[j][k - 1], auxi1, lll_d[k]);
  }
  mpz_set (lll_d[k - 1], B);

  mpz_clear (auxi1);
  mpz_clear (auxi2);
  mpz_clear (lamb);
  mpz_clear (B);
  mpz_clear (t);
  mpz_clear (swap);
}
