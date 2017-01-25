#include "geometry/RipsExpander.hh"

#include "tests/Base.hh"

#include "topology/Simplex.hh"
#include "topology/SimplicialComplex.hh"

#include <vector>

#include <cmath>

using namespace aleph::geometry;
using namespace aleph::topology;
using namespace aleph;

template <class Data, class Vertex> void triangle()
{
  ALEPH_TEST_BEGIN( "Triangle" );

  using Simplex           = Simplex<Data, Vertex>;
  using SimplicialComplex = SimplicialComplex<Simplex>;

  std::vector<Simplex> simplices
    = { {0}, {1}, {2}, {0,1}, {0,2}, {1,2} };

  SimplicialComplex K( simplices.begin(), simplices.end() );
  RipsExpander<SimplicialComplex> ripsExpander;

  auto vr1 = ripsExpander( K, 2 );
  auto vr2 = ripsExpander( K, 3 );

  ALEPH_ASSERT_THROW( vr1.empty() == false );
  ALEPH_ASSERT_THROW( vr2.empty() == false );
  ALEPH_ASSERT_THROW( vr1.size()  == vr2.size() );
  ALEPH_ASSERT_THROW( vr1.size()  == 7 );

  ALEPH_TEST_END();
}

template <class Data, class Vertex> void quad()
{
  ALEPH_TEST_BEGIN( "Quad" );

  using Simplex           = Simplex<Data, Vertex>;
  using SimplicialComplex = SimplicialComplex<Simplex>;

  std::vector<Simplex> simplices
    = { {0},
        {1},
        {2},
        {3},
        Simplex( {0,1}, Data(1) ),
        Simplex( {1,2}, Data(1) ),
        Simplex( {2,3}, Data(1) ),
        Simplex( {0,3}, Data(1) ),
        Simplex( {0,2}, Data( std::sqrt(2.0) ) ),
        Simplex( {1,3}, Data( std::sqrt(2.0) ) ) };

  SimplicialComplex K( simplices.begin(), simplices.end() );
  RipsExpander<SimplicialComplex> ripsExpander;

  auto vr1 = ripsExpander( K, 1 );
  auto vr2 = ripsExpander( K, 2 );
  auto vr3 = ripsExpander( K, 3 );

  vr1 = ripsExpander.assignMaximumWeight( vr1 );
  vr2 = ripsExpander.assignMaximumWeight( vr2 );
  vr3 = ripsExpander.assignMaximumWeight( vr3 );

  ALEPH_ASSERT_THROW( vr1.empty() == false );
  ALEPH_ASSERT_THROW( vr2.empty() == false );
  ALEPH_ASSERT_THROW( vr3.empty() == false );

  ALEPH_ASSERT_THROW( vr1.size() == simplices.size()     );
  ALEPH_ASSERT_THROW( vr2.size() == vr1.size()       + 4 ); // +4 triangles
  ALEPH_ASSERT_THROW( vr3.size() == vr2.size()       + 1 ); // +1 tetrahedron

  ALEPH_TEST_END();
}

int main()
{
  triangle<double, unsigned>();
  quad<double, unsigned>();
}
