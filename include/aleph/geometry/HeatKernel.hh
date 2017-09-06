#ifndef ALEPH_GEOMETRY_HEAT_KERNEL_HH__
#define ALEPH_GEOMETRY_HEAT_KERNEL_HH__

#include <aleph/config/Eigen.hh>

#ifdef ALEPH_WITH_EIGEN
  #include <Eigen/Core>
  #include <Eigen/Eigenvalues>
#endif

#include <aleph/math/KahanSummation.hh>

#include <unordered_map>
#include <vector>

#include <cmath>

namespace aleph
{

namespace geometry
{

#ifdef ALEPH_WITH_EIGEN

/**
  Extracts a weighted adjacency matrix from a simplicial complex. At
  present, this function only supports adjacencies between edges, so
  the resulting matrix is a graph adjacency matrix.

  @param K Simplicial complex

  @returns Weighted adjacency matrix. The indices of rows and columns
           follow the order of the vertices in the complex.
*/

template <class SimplicialComplex> auto weightedAdjacencyMatrix( const SimplicialComplex& K ) -> Eigen::Matrix<typename SimplicialComplex::ValueType::DataType, Eigen::Dynamic, Eigen::Dynamic>
{
  using Simplex    = typename SimplicialComplex::ValueType;
  using VertexType = typename Simplex::VertexType;
  using DataType   = typename Simplex::DataType;
  using Matrix     = Eigen::Matrix<DataType, Eigen::Dynamic, Eigen::Dynamic>;

#if EIGEN_VERSION_AT_LEAST(3,3,0)
  using IndexType  = Eigen::Index;
#else
  using IndexType  = typename Matrix::Index;
#endif

  // Prepare map from vertex to index ----------------------------------

  std::unordered_map<VertexType, IndexType> vertex_to_index;
  IndexType n = IndexType();

  {
    std::vector<VertexType> vertices;
    K.vertices( std::back_inserter( vertices ) );

    IndexType index = IndexType();

    for( auto&& vertex : vertices )
      vertex_to_index[vertex] = index++;

    n = static_cast<IndexType>( vertices.size() );
  }

  // Prepare matrix ----------------------------------------------------

  Matrix W( n, n );

  for(auto&& s : K )
  {
    if( s.dimension() != 1 )
      continue;

    auto&& u = s[0];
    auto&& v = s[1];
    auto&& i = vertex_to_index.at( u );
    auto&& j = vertex_to_index.at( v );

    W(i,j)   = s.data();
    W(j,i)   = W(i,j);
  }

  return W;
}

/**
  Calculates the weighhted Laplacian matrix of a given simplicial
  complex and returns it.

  @param K Simplicial complex

  @returns Weighted Laplacian matrix. The indices of rows and columns
           follow the order of the vertices in the complex.
*/

template <class SimplicialComplex> auto weightedLaplacianMatrix( const SimplicialComplex& K ) -> Eigen::Matrix<typename SimplicialComplex::ValueType::DataType, Eigen::Dynamic, Eigen::Dynamic>
{
  auto W          = weightedAdjacencyMatrix( K );
  using Matrix    = decltype(W);
  using IndexType = typename Matrix::Index;

  Matrix L( W.rows(), W.cols() );

  auto V = W.rowwise().sum();

  for( IndexType i = 0; i < V.size(); i++ )
    L(i,i) = V(i);

  return L - W;
}

#endif

/**
  @class HeatKernel
  @brief Caclulates the heat kernel for simplicial complexes

  This class acts as a query functor for the heat kernel values of
  vertices in a weighted simplicial complex. It will pre-calculate
  the heat matrix and permit queries about the progression of heat
  values for *all* vertices for some time \f$t\f$.
*/

class HeatKernel
{
public:

#ifdef ALEPH_WITH_EIGEN
  using T      = double;
  using Matrix = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
  using Vector = Eigen::Matrix<T, 1, Eigen::Dynamic>;

#if EIGEN_VERSION_AT_LEAST(3,3,0)
  using IndexType  = Eigen::Index;
#else
  using IndexType  = typename Matrix::Index;
#endif

#endif

  /**
    Constructs a heat kernel from a given simplicial complex. Afterwards,
    the functor will be ready for queries.

    @param K Simplicial complex
  */

  template <class SimplicialComplex> HeatKernel( const SimplicialComplex& K )
  {
    auto L = weightedLaplacianMatrix( K );

#ifdef ALEPH_WITH_EIGEN

    Eigen::SelfAdjointEigenSolver< decltype(L) > solver;
    solver.compute( L );

    auto&& eigenvalues  = solver.eigenvalues(). template cast<T>();
    auto&& eigenvectors = solver.eigenvectors().template cast<T>();

    _eigenvalues.reserve( eigenvalues.size() );
    _eigenvectors.reserve( eigenvectors.size() );

    using IndexType_ = typename decltype(L)::Index;

    // Skip the first eigenvector and the first eigenvalue because they
    // do not contribute anything to the operations later on.

    for( IndexType_ i = 1; i < eigenvalues.size(); i++ )
      _eigenvalues.push_back( eigenvalues(i) );

    for( IndexType_ i = 1; i < eigenvectors.cols(); i++ )
      _eigenvectors.push_back( eigenvectors.col(i) );

#else

    // FIXME: throw error?
    (void) L;

#endif

  }

  /**
    Evaluates the heat kernel for two vertices \f$i\f$ and \f$j\f$ at
    a given time \f$t\f$ and returns the result.
  */

  T operator()( IndexType i, IndexType j, double t )
  {
#ifdef ALEPH_WITH_EIGEN

    aleph::math::KahanSummation<T> result = T();

    for( std::size_t k = 0; k < _eigenvalues.size(); k++ )
    {
      auto&& lk  = std::exp( -t * _eigenvalues[k] );
      auto&& uik = _eigenvectors[k](i);
      auto&& ujk = _eigenvectors[k](j);

      result += lk * uik * ujk;
    }

    return result;

#else
  // FIXME: throw error?
#endif
  }

private:

  /**
    Stores the eigenvalues of the heat matrix, or, more precisely, the
    eigenvalues of the Laplacian. They will be used for the evaluation
    of the heat kernel.
  */

  std::vector<T> _eigenvalues;

#ifdef ALEPH_WITH_EIGEN

  /** Stores the eigenvectors of the heat matrix */
  std::vector<Vector> _eigenvectors;

  /**
    Heat matrix; will be created automatically upon constructing this
    functor class.
  */

  Matrix _H;
#endif

};

} // namespace geometry

} // namespace aleph

#endif