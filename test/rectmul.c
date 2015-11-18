/*
 * Program to multiply two rectangualar matrizes A(n,m) * B(m,n), where
 * (n < m) and (n mod 16 = 0) and (m mod n = 0). (Otherwise fill with 0s
 * to fit the shape.)
 *
 * written by Harald Prokop (prokop@mit.edu) Fall 97.
 */
/*
 * Copyright (c) 2003 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <cilk-lib.cilkh>
#include <stdlib.h>
#include <stdio.h>
#include <getoptions.h>

#define BLOCK_EDGE 16
#define BLOCK_SIZE (BLOCK_EDGE*BLOCK_EDGE)
typedef double block[BLOCK_SIZE];
typedef block *pblock;

double min;
double max;
int count;
double sum;

/* compute R = R+AB, where R,A,B are BLOCK_EDGE x BLOCK_EDGE matricies
*/
long long mult_add_block(block *A, block *B, block *R)
{
  int i, j;
  long long flops = 0LL;

  for (j = 0; j < 16; j += 2) {	/* 2 columns at a time */
    double *bp = &((double *) B)[j];
    for (i = 0; i < 16; i += 2) {		/* 2 rows at a time */
      double *ap = &((double *) A)[i * 16];
      double *rp = &((double *) R)[j + i * 16];
      register double s0_0, s0_1;
      register double s1_0, s1_1;
      s0_0 = rp[0];
      s0_1 = rp[1];
      s1_0 = rp[16];
      s1_1 = rp[17];
      s0_0 += ap[0] * bp[0];
      s0_1 += ap[0] * bp[1];
      s1_0 += ap[16] * bp[0];
      s1_1 += ap[16] * bp[1];
      s0_0 += ap[1] * bp[16];
      s0_1 += ap[1] * bp[17];
      s1_0 += ap[17] * bp[16];
      s1_1 += ap[17] * bp[17];
      s0_0 += ap[2] * bp[32];
      s0_1 += ap[2] * bp[33];
      s1_0 += ap[18] * bp[32];
      s1_1 += ap[18] * bp[33];
      s0_0 += ap[3] * bp[48];
      s0_1 += ap[3] * bp[49];
      s1_0 += ap[19] * bp[48];
      s1_1 += ap[19] * bp[49];
      s0_0 += ap[4] * bp[64];
      s0_1 += ap[4] * bp[65];
      s1_0 += ap[20] * bp[64];
      s1_1 += ap[20] * bp[65];
      s0_0 += ap[5] * bp[80];
      s0_1 += ap[5] * bp[81];
      s1_0 += ap[21] * bp[80];
      s1_1 += ap[21] * bp[81];
      s0_0 += ap[6] * bp[96];
      s0_1 += ap[6] * bp[97];
      s1_0 += ap[22] * bp[96];
      s1_1 += ap[22] * bp[97];
      s0_0 += ap[7] * bp[112];
      s0_1 += ap[7] * bp[113];
      s1_0 += ap[23] * bp[112];
      s1_1 += ap[23] * bp[113];
      s0_0 += ap[8] * bp[128];
      s0_1 += ap[8] * bp[129];
      s1_0 += ap[24] * bp[128];
      s1_1 += ap[24] * bp[129];
      s0_0 += ap[9] * bp[144];
      s0_1 += ap[9] * bp[145];
      s1_0 += ap[25] * bp[144];
      s1_1 += ap[25] * bp[145];
      s0_0 += ap[10] * bp[160];
      s0_1 += ap[10] * bp[161];
      s1_0 += ap[26] * bp[160];
      s1_1 += ap[26] * bp[161];
      s0_0 += ap[11] * bp[176];
      s0_1 += ap[11] * bp[177];
      s1_0 += ap[27] * bp[176];
      s1_1 += ap[27] * bp[177];
      s0_0 += ap[12] * bp[192];
      s0_1 += ap[12] * bp[193];
      s1_0 += ap[28] * bp[192];
      s1_1 += ap[28] * bp[193];
      s0_0 += ap[13] * bp[208];
      s0_1 += ap[13] * bp[209];
      s1_0 += ap[29] * bp[208];
      s1_1 += ap[29] * bp[209];
      s0_0 += ap[14] * bp[224];
      s0_1 += ap[14] * bp[225];
      s1_0 += ap[30] * bp[224];
      s1_1 += ap[30] * bp[225];
      s0_0 += ap[15] * bp[240];
      s0_1 += ap[15] * bp[241];
      s1_0 += ap[31] * bp[240];
      s1_1 += ap[31] * bp[241];
      rp[0] = s0_0;
      rp[1] = s0_1;
      rp[16] = s1_0;
      rp[17] = s1_1;
      flops += 128;
    }
  }

  return flops;
}


/* compute R = AB, where R,A,B are BLOCK_EDGE x BLOCK_EDGE matricies
*/
long long multiply_block(block *A, block *B, block *R)
{
  int i, j;
  long long flops = 0LL;

  for (j = 0; j < 16; j += 2) {	/* 2 columns at a time */
    double *bp = &((double *) B)[j];
    for (i = 0; i < 16; i += 2) {		/* 2 rows at a time */
      double *ap = &((double *) A)[i * 16];
      double *rp = &((double *) R)[j + i * 16];
      register double s0_0, s0_1;
      register double s1_0, s1_1;
      s0_0 = ap[0] * bp[0];
      s0_1 = ap[0] * bp[1];
      s1_0 = ap[16] * bp[0];
      s1_1 = ap[16] * bp[1];
      s0_0 += ap[1] * bp[16];
      s0_1 += ap[1] * bp[17];
      s1_0 += ap[17] * bp[16];
      s1_1 += ap[17] * bp[17];
      s0_0 += ap[2] * bp[32];
      s0_1 += ap[2] * bp[33];
      s1_0 += ap[18] * bp[32];
      s1_1 += ap[18] * bp[33];
      s0_0 += ap[3] * bp[48];
      s0_1 += ap[3] * bp[49];
      s1_0 += ap[19] * bp[48];
      s1_1 += ap[19] * bp[49];
      s0_0 += ap[4] * bp[64];
      s0_1 += ap[4] * bp[65];
      s1_0 += ap[20] * bp[64];
      s1_1 += ap[20] * bp[65];
      s0_0 += ap[5] * bp[80];
      s0_1 += ap[5] * bp[81];
      s1_0 += ap[21] * bp[80];
      s1_1 += ap[21] * bp[81];
      s0_0 += ap[6] * bp[96];
      s0_1 += ap[6] * bp[97];
      s1_0 += ap[22] * bp[96];
      s1_1 += ap[22] * bp[97];
      s0_0 += ap[7] * bp[112];
      s0_1 += ap[7] * bp[113];
      s1_0 += ap[23] * bp[112];
      s1_1 += ap[23] * bp[113];
      s0_0 += ap[8] * bp[128];
      s0_1 += ap[8] * bp[129];
      s1_0 += ap[24] * bp[128];
      s1_1 += ap[24] * bp[129];
      s0_0 += ap[9] * bp[144];
      s0_1 += ap[9] * bp[145];
      s1_0 += ap[25] * bp[144];
      s1_1 += ap[25] * bp[145];
      s0_0 += ap[10] * bp[160];
      s0_1 += ap[10] * bp[161];
      s1_0 += ap[26] * bp[160];
      s1_1 += ap[26] * bp[161];
      s0_0 += ap[11] * bp[176];
      s0_1 += ap[11] * bp[177];
      s1_0 += ap[27] * bp[176];
      s1_1 += ap[27] * bp[177];
      s0_0 += ap[12] * bp[192];
      s0_1 += ap[12] * bp[193];
      s1_0 += ap[28] * bp[192];
      s1_1 += ap[28] * bp[193];
      s0_0 += ap[13] * bp[208];
      s0_1 += ap[13] * bp[209];
      s1_0 += ap[29] * bp[208];
      s1_1 += ap[29] * bp[209];
      s0_0 += ap[14] * bp[224];
      s0_1 += ap[14] * bp[225];
      s1_0 += ap[30] * bp[224];
      s1_1 += ap[30] * bp[225];
      s0_0 += ap[15] * bp[240];
      s0_1 += ap[15] * bp[241];
      s1_0 += ap[31] * bp[240];
      s1_1 += ap[31] * bp[241];
      rp[0] = s0_0;
      rp[1] = s0_1;
      rp[16] = s1_0;
      rp[17] = s1_1;
      flops += 124;
    }
  }

  return flops;
}


/* Checks if each A[i,j] of a martix A of size nb x nb blocks has value v
*/

int check_block (block *R, double v)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i++)
    if (((double *) R)[i] != v)
      return 1;

  return 0;
}

cilk int check_matrix(block *R, long x, long y, long o, double v)
{
  int a,b;

  if ((x*y) == 1)
    return check_block(R,v);
  else {
    if (x>y) {
      a = spawn check_matrix(R,x/2,y,o,v);
      b = spawn check_matrix(R+(x/2)*o,(x+1)/2,y,o,v);
      sync;
    }
    else {
      a = spawn check_matrix(R,x,y/2,o,v);
      b = spawn check_matrix(R+(y/2),x,(y+1)/2,o,v);
      sync;
    }
  }
  return (a+b);
}

long long add_block(block *T, block *R)
{
  long i;

  for (i = 0; i < BLOCK_SIZE; i += 4) {
    ((double *) R)[i] += ((double *) T)[i];
    ((double *) R)[i + 1] += ((double *) T)[i + 1];
    ((double *) R)[i + 2] += ((double *) T)[i + 2];
    ((double *) R)[i + 3] += ((double *) T)[i + 3];
  }

  return BLOCK_SIZE;
}

/* Add matrix T into matrix R, where T and R are bl blocks in size
 *
 */

cilk long long add_matrix(block *T, long ot, block *R, long or, long x, long y)
{
  long long flops = 0LL;

  if ((x+y)==2) {
    return add_block(T,R);
  }
  else {
    if (x>y) {
      flops += spawn add_matrix(T,ot,R,or,x/2,y);
      flops += spawn add_matrix(T+(x/2)*ot,ot,R+(x/2)*or,or,(x+1)/2,y);
    }
    else {
      flops += spawn add_matrix(T,ot,R,or,x,y/2);
      flops += spawn add_matrix(T+(y/2),ot,R+(y/2),or,x,(y+1)/2);
    }

    sync;
    return flops;
  }
}


void init_block(block* R, double v)
{
  int i;

  for (i = 0; i < BLOCK_SIZE; i++)
    ((double *) R)[i] = v;
}

cilk void init_matrix(block *R, long x, long y, long o, double v)
{
  if ((x+y)==2)
    init_block(R,v);
  else  {
    if (x>y) {
      spawn init_matrix(R,x/2,y,o,v);
      spawn init_matrix(R+(x/2)*o,(x+1)/2,y,o,v);
    }
    else {
      spawn init_matrix(R,x,y/2,o,v);
      spawn init_matrix(R+(y/2),x,(y+1)/2,o,v);
    }
  }
}

cilk long long multiply_matrix(block *A, long oa, block *B, long ob,
    long x, long y, long z, block *R, long or, int add)
{
  if ((x+y+z) == 3) {
    if (add)
      return mult_add_block(A, B, R);
    else
      return multiply_block(A, B, R);
  }
  else {
    long long flops = 0LL;

    if ((x>=y) && (x>=z)) {
      flops += spawn multiply_matrix(A, oa, B, ob, x/2, y, z, R, or, add);
      flops += spawn multiply_matrix(A + (x / 2) * oa, oa, B, ob,
          (x + 1) / 2, y, z, R + (x / 2) * or, or, add);
    }
    else {
      if ((y>x) && (y>z)){
        flops += spawn multiply_matrix(A + (y / 2), oa, B + (y / 2) * ob,
            ob, x, (y + 1) / 2, z, R, or, add);

        if (SYNCHED) {
          flops += spawn multiply_matrix(A, oa, B, ob, x, y / 2, z, R, or, 1);
        }
        else {
          block *tmp = Cilk_alloca(x*z*sizeof(block));

          flops += spawn multiply_matrix(A, oa, B, ob, x, y / 2, z, tmp, z, 0);
          sync;

          flops += spawn add_matrix(tmp, z, R, or, x, z);
        }
      }
      else {
        flops += spawn multiply_matrix(A, oa, B, ob, x, y, z / 2, R, or, add);
        flops += spawn multiply_matrix(A, oa, B + (z / 2), ob, x, y,
            (z + 1) / 2, R + (z / 2), or, add);
      }
    }

    sync;
    return flops;
  }
}

cilk int run(long x, long y, long z, int check)
{
  block *A, *B, *R;
  long long flops;
  double f;
  Cilk_time tm_begin, tm_elapsed;
  Cilk_time wk_begin, wk_elapsed;
  Cilk_time cp_begin, cp_elapsed;

  A = malloc(x*y * sizeof(block));
  B = malloc(y*z * sizeof(block));
  R = malloc(x*z * sizeof(block));

  spawn init_matrix(A, x, y, y, 1.0);
  spawn init_matrix(B, y, z, z, 1.0);
  spawn init_matrix(R, x, z, z, 0.0);
  sync;

  /* Timing. "Start" timers */
  sync;
  cp_begin = Cilk_user_critical_path;
  wk_begin = Cilk_user_work;
  tm_begin = Cilk_get_wall_time();

  flops = spawn multiply_matrix(A,y,B,z,x,y,z,R,z,0);
  sync;

  /* Timing. "Stop" timers */
  tm_elapsed = Cilk_get_wall_time() - tm_begin;
  wk_elapsed = Cilk_user_work - wk_begin;
  cp_elapsed = Cilk_user_critical_path - cp_begin;

  f = (double) flops / 1.0E6 / Cilk_time_to_sec(tm_elapsed);

  if (check) {
    check = spawn check_matrix(R, x, z, z, y*16);
    sync;
  }

  if (min>f) min=f;
  if (max<f) max=f;
  sum+=f;
  count++;

  if (check)
    printf("WRONG RESULT!\n");
  else {
    printf("\nCilk Example: rectmul\n");
    printf("	      running on %d processor%s\n\n", Cilk_active_size,
        Cilk_active_size > 1 ? "s" : "");
    printf("Options: x = %ld\n", BLOCK_EDGE*x);
    printf("         y = %ld\n", BLOCK_EDGE*y);
    printf("         z = %ld\n\n", BLOCK_EDGE*z);
    printf("Mflops        = %4f \n", (double) flops / 1.0E6 /
        Cilk_time_to_sec(tm_elapsed));
    printf("Running time  = %4f s\n", Cilk_wall_time_to_sec(tm_elapsed));
    printf("Work          = %4f s\n", Cilk_time_to_sec(wk_elapsed));
    printf("Critical path = %4f s\n\n", Cilk_time_to_sec(cp_elapsed));
  }

  free(A);
  free(B);
  free(R);

  return 0;
}


int usage(void) {
  fprintf(stderr, "\nUsage: rectmul [<cilk-options>] [<options>]\n\n");
  return 1;
}

char *specifiers[] = { "-x", "-y", "-z", "-c", "-benchmark", "-h", 0};
int opt_types[] = {INTARG, INTARG, INTARG, BOOLARG, BENCHMARK, BOOLARG, 0 };

cilk int cilk_main(int argc, char *argv[])
{
  int x, y, z, benchmark, help, t, check;

  /* standard benchmark options */
  x = 512;
  y = 512;
  z = 512;
  check = 0;

  get_options(argc, argv, specifiers, opt_types, &x, &y, &z, &check,
      &benchmark, &help);

  if (help) return usage();

  if (benchmark) {
    switch (benchmark) {
      case 1:      /* short benchmark options -- a little work*/
        x = 256;
        y = 256;
        z = 256;
        break;
      case 2:      /* standard benchmark options*/
        x = 512;
        y = 512;
        z = 512;
        break;
      case 3:      /* long benchmark options -- a lot of work*/
        x = 2048;
        y = 2048;
        z = 2048;
        break;
    }
  }

  x = x/BLOCK_EDGE;
  y = y/BLOCK_EDGE;
  z = z/BLOCK_EDGE;

  if (x<1) x=1;
  if (y<1) y=1;
  if (z<1) z=1;

  t = spawn run(x,y,z,check);

  return t;
}
