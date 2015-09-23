

////////////////////////////////////////////////////////////////////////


using namespace std;

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <cmath>

#include "mm_engine.h"


////////////////////////////////////////////////////////////////////////


   //
   //  Code for class MM_Engine
   //


////////////////////////////////////////////////////////////////////////


MM_Engine::MM_Engine()

{

init_from_scratch();

}


////////////////////////////////////////////////////////////////////////


MM_Engine::~MM_Engine()

{

clear();

}


////////////////////////////////////////////////////////////////////////


MM_Engine::MM_Engine(const MM_Engine & e)

{

init_from_scratch();

assign(e);

}


////////////////////////////////////////////////////////////////////////


MM_Engine & MM_Engine::operator=(const MM_Engine & e)

{

if ( this == &e )  return ( * this );

assign(e);

return ( * this );

}


////////////////////////////////////////////////////////////////////////


void MM_Engine::init_from_scratch()

{

clear();

return;

}


////////////////////////////////////////////////////////////////////////


void MM_Engine::clear()

{

N_Composites = 0;

if ( comp_to_eq )  { delete [] comp_to_eq;  comp_to_eq = 0; }

calc.clear();

part.clear();

graph.clear();


return;

}


////////////////////////////////////////////////////////////////////////


void MM_Engine::assign(const MM_Engine & e)

{

clear();

N_Composites = e.N_Composites;

if ( e.comp_to_eq )  {

   comp_to_eq = new int [N_Composites];

   memcpy(comp_to_eq, e.comp_to_eq, N_Composites*sizeof(int));

}


calc = e.calc;

part = e.part;

graph = e.graph;

return;

}


////////////////////////////////////////////////////////////////////////


void MM_Engine::set_size(const int _n_fcst, const int _n_obs)

{

graph.set_size(_n_fcst, _n_obs);

   //
   //  set up the initial partition
   //

int j;

for (j=0; j<(graph.n_total()); ++j)  {

   part.add_no_repeat(j);

}

   //
   //  done
   //

return;

}


////////////////////////////////////////////////////////////////////////


void MM_Engine::do_match_merge()

{

int j, k, m, n;
int f_i, o_i;


   //
   //  fcst, obs
   //

for (j=0; j<(graph.n_fcst()); ++j)  {

   f_i = graph.f_index(j);

   for (k=0; k<(graph.n_obs()); ++k)  {

      o_i = graph.o_index(k);

      if ( ! graph.has_fo_edge(j, k) )  continue;

      // cout << "\n  Merging fcst " << j << ", obs " << k << '\n' << flush;

      part.merge_values(f_i, o_i);

      // specialzed_dump(graph.n_fcst(), graph.n_obs(), p);

   }   //  for k

}   //  for j


   //
   //  fcst, fcst
   //

   //
   //  obs, obs
   //

   //
   //  get number of fcst and obs composites
   //


bool has_fcst = false;
bool has_obs  = false;
const EquivalenceClass * eq = 0;


N_Composites = 0;

for (j=0; j<(part.n_elements()); ++j)  {

   has_fcst = has_obs = false;

   eq = part(j);

   if ( eq->n_elements() <= 1 )  continue;

   ++N_Composites;

}   //  for j

if ( N_Composites > 0 )  comp_to_eq = new int [N_Composites];

n = 0;

for (j=0; j<(part.n_elements()); ++j)  {

   eq = part(j);
   
   if ( eq->n_elements() <= 1 )  continue;

   comp_to_eq[n++] = j;

}

cout << "Composites ...\n";

for (j=0; j<N_Composites; ++j)  {

   cout << ' ' << comp_to_eq[j];

}

cout << '\n';

   //
   //  done
   //

cout.flush();

return;

}


////////////////////////////////////////////////////////////////////////


void MM_Engine::partition_dump(ostream & out) const

{

part.specialized_dump(out, graph.n_fcst(), graph.n_obs());

return;

}


////////////////////////////////////////////////////////////////////////


int MM_Engine::composite_with_fcst (const int k) const

{

int j, m, fcst_num, index;
const EquivalenceClass * eq = 0;

fcst_num = k;

for (j=0; j<N_Composites; ++j)  {

   m = comp_to_eq[j];

   eq = part(m);

   if ( eq->has(fcst_num) )  return ( j );

}

return ( -1 );

}


////////////////////////////////////////////////////////////////////////


int MM_Engine::composite_with_obs (const int k) const

{

int j, m, obs_num, index;
const EquivalenceClass * eq = 0;

obs_num = k + graph.n_fcst();

for (j=0; j<N_Composites; ++j)  {

   m = comp_to_eq[j];

   eq = part(m);

   if ( eq->has(obs_num) )  return ( j );

}

return ( -1 );

}


////////////////////////////////////////////////////////////////////////


IntArray MM_Engine::fcst_composite(const int _composite_number) const

{

int j, k;
IntArray a;
const EquivalenceClass * eq = part(comp_to_eq[_composite_number]);   //  this does range checking

for (j=0; j<(eq->n_elements()); ++j)  {

   k = eq->element(j);

   if ( k < graph.n_fcst() )  a.add(k);

}

return ( a );

}


////////////////////////////////////////////////////////////////////////


IntArray MM_Engine::obs_composite(const int _composite_number) const

{

int j, k;
IntArray a;
const EquivalenceClass * eq = part(comp_to_eq[_composite_number]);   //  this does range checking

for (j=0; j<(eq->n_elements()); ++j)  {

   k = eq->element(j);

   if ( k >= graph.n_fcst() )  a.add(k - graph.n_fcst());

}

return ( a );

}


////////////////////////////////////////////////////////////////////////


int MM_Engine::map_fcst_id_to_composite(const int id) const   //  zero-based

{

int j, k;


k = id;

j = part.which_class(k);


return ( j );

}


////////////////////////////////////////////////////////////////////////


int MM_Engine::map_obs_id_to_composite(const int id) const   //  zero-based

{

int j, k;


k = id + graph.n_fcst();

j = part.which_class(k);


return ( j );

}


////////////////////////////////////////////////////////////////////////


