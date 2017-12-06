#ifndef ALEPH_TOPOLOGY_SPINE_HH__
#define ALEPH_TOPOLOGY_SPINE_HH__

#include <aleph/topology/Intersections.hh>

#include <set>
#include <vector>

namespace aleph
{

namespace topology
{

/**
  Checks whether a simplex in a simplicial complex is principal, i.e.
  whether it is not a proper face of any other simplex in K.
*/

template <class SimplicialComplex, class Simplex> bool isPrincipal( const Simplex& s, const SimplicialComplex& K )
{
  // Individual vertices cannot be considered to be principal because
  // they do not have a free face.
  if( s.dimension() == 0 )
    return false;

  bool principal = true;
  auto itPair    = K.range( s.dimension() + 1 );

  for( auto it = itPair.first; it != itPair.second; ++it )
  {
    auto&& t = *it;

    // This check assumes that the simplicial complex is valid, so it
    // suffices to search faces in one dimension _below_ s. Note that
    // the check only has to evaluate the *size* of the intersection,
    // as this is sufficient to determine whether a simplex is a face
    // of another simplex.
    if( sizeOfIntersection(s,t) == s.size() )
      principal = false;
  }

  return principal;
}

/**
  Checks whether a simplex in a simplicial complex is admissible, i.e.
  the simplex is *principal* and has at least one free face.
*/

template <class SimplicialComplex, class Simplex> bool isAdmissible( const Simplex& s, const SimplicialComplex& K )
{
  if( !isPrincipal(s,K) )
    return false;

  // Check whether a free face exists ----------------------------------

  std::vector<Simplex> faces( s.begin_boundary(), s.end_boundary() );
  std::vector<bool> admissible( faces.size(), true );

  std::size_t i = 0;
  auto itPair   = K.range( s.dimension() ); // valid range for searches, viz. *all*
                                            // faces in "one dimension up"

  for( auto&& face : faces )
  {
    for( auto it = itPair.first; it != itPair.second; ++it )
    {
      auto&& t = *it;

      // We do not have to check for intersections with the original
      // simplex from which we started---we already know that we are
      // a face.
      if( t != s )
      {
        if( sizeOfIntersection(face,t) == face.size() )
        {
          admissible[i] = false;
          break;
        }
      }
    }

    ++i;
  }

  // TODO: return free face along with admissibility condition
  return std::find( admissible.begin(), admissible.end(), true ) != admissible.end();
}

/**
  Checks whether a pair of a simplex and its face are admissible, i.e.
  the simplex is *principal* and the face is free.
*/

template <class SimplicialComplex, class Simplex> bool isAdmissible( const Simplex& sigma, const Simplex& delta, const SimplicialComplex& K )
{
  if( !isPrincipal(sigma,K) )
    return false;

  // Check whether the face is free ------------------------------------

  bool admissible = true;
  auto itPair     = K.range( delta.dimension() + 1 );

  for( auto it = itPair.first; it != itPair.second; ++it )
  {
    auto&& s = *it;

    // The simplex we are looking for should be a free face of sigma, so
    // we must skip it when checking for other co-faces.
    if( s != sigma )
    {
      if( sizeOfIntersection(delta, s) == delta.size() )
      {
        admissible = false;
        break;
      }
    }
  }

  return admissible;
}

/**
  Calculates all principal faces of a given simplicial complex and
  returns them.
*/

template <class SimplicialComplex> std::unordered_set<typename SimplicialComplex::value_type> principalFaces( const SimplicialComplex& K )
{
  using Simplex = typename SimplicialComplex::value_type;
  auto L        = K;

  std::unordered_set<Simplex> admissible;

  // Step 1: determine free faces --------------------------------------
  //
  // This first checks which simplices have at least one free face,
  // meaning that they may be potentially admissible.

  for( auto it = L.begin(); it != L.end(); ++it )
  {
    if( it->dimension() == 0 )
      continue;

    // The range of the complex M is sufficient because we have
    // already encountered all lower-dimensional simplices that
    // precede the current one given by `it`.
    //
    // This complex will be used for testing free faces.
    SimplicialComplex M( L.begin(), it );

    // FIXME:
    //
    // In case of equal data values, the assignment from above does
    // *not* work and will result in incorrect candidates.
    M = L;

    bool hasFreeFace = false;
    for( auto itFace = it->begin_boundary(); itFace != it->end_boundary(); ++itFace )
    {
      bool isFace = false;
      for( auto&& simplex : M )
      {
        if( itFace->dimension() + 1 == simplex.dimension() && simplex != *it )
        {
          // The current face must *not* be a face of another simplex in
          // the simplicial complex.
          if( intersect( *itFace, simplex ) == *itFace )
          {
            isFace = true;
            break;
          }
        }
      }

      hasFreeFace = !isFace;
      if( hasFreeFace )
        break;
    }

    if( hasFreeFace )
      admissible.insert( *it );
  }

  // Step 2: determine principality ------------------------------------
  //
  // All simplices that are faces of higher-dimensional simplices are
  // now removed from the map of admissible simplices.

  for( auto&& s : L )
  {
    for( auto itFace = s.begin_boundary(); itFace != s.end_boundary(); ++itFace )
      admissible.erase( *itFace );
  }

  return admissible;
}

/**
  Performs an iterated elementary simplicial collapse until *all* of the
  admissible simplices have been collapsed. This leads to the *spine* of
  the simplicial complex.

  @see S. Matveev, "Algorithmic Topology and Classification of 3-Manifolds"
*/

template <class SimplicialComplex> SimplicialComplex spine( const SimplicialComplex& K )
{
  using Simplex = typename SimplicialComplex::value_type;
  auto L        = K;

  // Step 1: obtain initial set of principal faces to start the process
  // of collapsing the complex.
  auto admissible = principalFaces( L );

  // Step 2: collapse until no admissible simplices are left -----------

  while( !admissible.empty() )
  {
    auto s           = *admissible.begin();
    bool hasFreeFace = false;

    // TODO: this check could be simplified by *storing* the free face
    // along with the given simplex
    for( auto itFace = s.begin_boundary(); itFace != s.end_boundary(); ++itFace )
    {
      auto t = *itFace;

      if( isAdmissible( s, t, L ) )
      {
        L.remove_without_validation( s );
        L.remove_without_validation( t );

        admissible.erase( s );

        // New simplices -----------------------------------------------
        //
        // Add new admissible simplices that may potentially have been
        // spawned by the removal of s.

        // 1. Add all faces of the principal simplex, as they may
        //    potentially become admissible again.
        std::vector<Simplex> faces( s.begin_boundary(), s.end_boundary() );

        std::for_each( faces.begin(), faces.end(),
          [&t, &L, &admissible] ( const Simplex& s )
          {
            if( t != s && isAdmissible( s, L ) )
              admissible.insert( s );
          }
        );

        // 2. Add all faces othe free face, as they may now themselves
        //    become admissible.
        faces.assign( t.begin_boundary(), t.end_boundary() );

        std::for_each( faces.begin(), faces.end(),
          [&L, &admissible] ( const Simplex& s )
          {
            if( isAdmissible( s, L ) )
              admissible.insert( s );
          }
        );

        hasFreeFace = true;
        break;
      }
    }

    // The admissible simplex does not have a free face, so it must not
    // be used.
    if( !hasFreeFace )
      admissible.erase( s );

    // The heuristic above is incapable of detecting *all* principal
    // faces of the complex because this may involve searching *all*
    // co-faces. Instead, it is easier to fill up the admissible set
    // here.
    if( admissible.empty() )
      admissible = principalFaces( L );
  }

  return L;
}

} // namespace topology

} // namespace aleph

#endif
