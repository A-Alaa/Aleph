#include <tests/Base.hh>

#include <aleph/topology/Simplex.hh>
#include <aleph/topology/SimplicialComplex.hh>

#include <aleph/topology/io/GML.hh>
#include <aleph/topology/io/SimplicialComplexReader.hh>

#include <set>

template <class D, class V> void test( const std::string& filename )
{
  ALEPH_TEST_BEGIN( "GML file parsing" );

  using Simplex           = aleph::topology::Simplex<D, V>;
  using SimplicialComplex = aleph::topology::SimplicialComplex<Simplex>;

  SimplicialComplex K;

  aleph::topology::io::GMLReader reader;
  reader( filename, K );

  ALEPH_ASSERT_EQUAL( K.size(), 5 );
  ALEPH_ASSERT_EQUAL( std::count_if( K.begin(), K.end(), [] (const Simplex& s) { return s.dimension() == 0; } ), 3 );
  ALEPH_ASSERT_EQUAL( std::count_if( K.begin(), K.end(), [] (const Simplex& s) { return s.dimension() == 1; } ), 2 );

  std::set<V> vertices;
  K.vertices( std::inserter( vertices, vertices.begin() ) );

  ALEPH_ASSERT_EQUAL( *vertices.begin(),  0 );
  ALEPH_ASSERT_EQUAL( *vertices.rbegin(), 2 );

  {
    SimplicialComplex L;

    aleph::topology::io::SimplicialComplexReader reader;
    reader( filename, L );

    ALEPH_ASSERT_THROW( K == L );
  }

  ALEPH_TEST_END();
}

int main()
{
  std::vector<std::string> inputs = {
    CMAKE_SOURCE_DIR + std::string( "/tests/input/Simple.gml" ),
    CMAKE_SOURCE_DIR + std::string( "/tests/input/Simple_inline.gml" ),
    CMAKE_SOURCE_DIR + std::string( "/tests/input/Simple_with_labels.gml" )
  };

  for( auto&& input : inputs )
  {
    test<double,unsigned>      ( input );
    test<double,unsigned short>( input );
    test<float, unsigned>      ( input );
    test<float, unsigned short>( input );
  }
}
