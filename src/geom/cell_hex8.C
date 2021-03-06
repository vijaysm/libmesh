// The libMesh Finite Element Library.
// Copyright (C) 2002-2014 Benjamin S. Kirk, John W. Peterson, Roy H. Stogner

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


// C++ includes

// Local includes
#include "libmesh/side.h"
#include "libmesh/cell_hex8.h"
#include "libmesh/edge_edge2.h"
#include "libmesh/face_quad4.h"

namespace libMesh
{




// ------------------------------------------------------------
// Hex8 class static member initializations
const unsigned int Hex8::side_nodes_map[6][4] =
  {
    {0, 3, 2, 1}, // Side 0
    {0, 1, 5, 4}, // Side 1
    {1, 2, 6, 5}, // Side 2
    {2, 3, 7, 6}, // Side 3
    {3, 0, 4, 7}, // Side 4
    {4, 5, 6, 7}  // Side 5
  };

const unsigned int Hex8::edge_nodes_map[12][2] =
  {
    {0, 1}, // Side 0
    {1, 2}, // Side 1
    {2, 3}, // Side 2
    {0, 3}, // Side 3
    {0, 4}, // Side 4
    {1, 5}, // Side 5
    {2, 6}, // Side 6
    {3, 7}, // Side 7
    {4, 5}, // Side 8
    {5, 6}, // Side 9
    {6, 7}, // Side 10
    {4, 7}  // Side 11
  };


// ------------------------------------------------------------
// Hex8 class member functions

bool Hex8::is_vertex(const unsigned int) const
{
  return true;
}

bool Hex8::is_edge(const unsigned int) const
{
  return false;
}

bool Hex8::is_face(const unsigned int) const
{
  return false;
}

bool Hex8::is_node_on_side(const unsigned int n,
                           const unsigned int s) const
{
  libmesh_assert_less (s, n_sides());
  for (unsigned int i = 0; i != 4; ++i)
    if (side_nodes_map[s][i] == n)
      return true;
  return false;
}

bool Hex8::is_node_on_edge(const unsigned int n,
                           const unsigned int e) const
{
  libmesh_assert_less (e, n_edges());
  for (unsigned int i = 0; i != 2; ++i)
    if (edge_nodes_map[e][i] == n)
      return true;
  return false;
}



bool Hex8::has_affine_map() const
{
  // Make sure x-edge endpoints are affine
  Point v = this->point(1) - this->point(0);
  if (!v.relative_fuzzy_equals(this->point(2) - this->point(3)) ||
      !v.relative_fuzzy_equals(this->point(5) - this->point(4)) ||
      !v.relative_fuzzy_equals(this->point(6) - this->point(7)))
    return false;
  // Make sure xz-faces are identical parallelograms
  v = this->point(4) - this->point(0);
  if (!v.relative_fuzzy_equals(this->point(7) - this->point(3)))
    return false;
  // If all the above checks out, the map is affine
  return true;
}



AutoPtr<Elem> Hex8::build_side (const unsigned int i,
                                bool proxy) const
{
  libmesh_assert_less (i, this->n_sides());

  if (proxy)
    {
      AutoPtr<Elem> ap(new Side<Quad4,Hex8>(this,i));
      return ap;
    }

  else
    {
      AutoPtr<Elem> face(new Quad4);
      face->subdomain_id() = this->subdomain_id();

      // Think of a unit cube: (-1,1) x (-1,1)x (-1,1)
      switch (i)
        {
        case 0:  // the face at z = -1
          {
            face->set_node(0) = this->get_node(0);
            face->set_node(1) = this->get_node(3);
            face->set_node(2) = this->get_node(2);
            face->set_node(3) = this->get_node(1);

            return face;
          }
        case 1:  // the face at y = -1
          {
            face->set_node(0) = this->get_node(0);
            face->set_node(1) = this->get_node(1);
            face->set_node(2) = this->get_node(5);
            face->set_node(3) = this->get_node(4);

            return face;
          }
        case 2:  // the face at x = 1
          {
            face->set_node(0) = this->get_node(1);
            face->set_node(1) = this->get_node(2);
            face->set_node(2) = this->get_node(6);
            face->set_node(3) = this->get_node(5);

            return face;
          }
        case 3: // the face at y = 1
          {
            face->set_node(0) = this->get_node(2);
            face->set_node(1) = this->get_node(3);
            face->set_node(2) = this->get_node(7);
            face->set_node(3) = this->get_node(6);

            return face;
          }
        case 4: // the face at x = -1
          {
            face->set_node(0) = this->get_node(3);
            face->set_node(1) = this->get_node(0);
            face->set_node(2) = this->get_node(4);
            face->set_node(3) = this->get_node(7);

            return face;
          }
        case 5: // the face at z = 1
          {
            face->set_node(0) = this->get_node(4);
            face->set_node(1) = this->get_node(5);
            face->set_node(2) = this->get_node(6);
            face->set_node(3) = this->get_node(7);

            return face;
          }
        default:
          libmesh_error_msg("Invalid side i = " << i);
        }
    }

  libmesh_error_msg("We'll never get here!");
  AutoPtr<Elem> ap(NULL);
  return ap;
}



AutoPtr<Elem> Hex8::build_edge (const unsigned int i) const
{
  libmesh_assert_less (i, this->n_edges());

  AutoPtr<Elem> ap(new SideEdge<Edge2,Hex8>(this,i));
  return ap;
}



void Hex8::connectivity(const unsigned int libmesh_dbg_var(sc),
                        const IOPackage iop,
                        std::vector<dof_id_type>& conn) const
{
  libmesh_assert(_nodes);
  libmesh_assert_less (sc, this->n_sub_elem());
  libmesh_assert_not_equal_to (iop, INVALID_IO_PACKAGE);

  conn.resize(8);

  switch (iop)
    {
    case TECPLOT:
      {
        conn[0] = this->node(0)+1;
        conn[1] = this->node(1)+1;
        conn[2] = this->node(2)+1;
        conn[3] = this->node(3)+1;
        conn[4] = this->node(4)+1;
        conn[5] = this->node(5)+1;
        conn[6] = this->node(6)+1;
        conn[7] = this->node(7)+1;
        return;
      }

    case VTK:
      {
        conn[0] = this->node(0);
        conn[1] = this->node(1);
        conn[2] = this->node(2);
        conn[3] = this->node(3);
        conn[4] = this->node(4);
        conn[5] = this->node(5);
        conn[6] = this->node(6);
        conn[7] = this->node(7);
        return;
      }

    default:
      libmesh_error_msg("Unsupported IO package " << iop);
    }
}



#ifdef LIBMESH_ENABLE_AMR

const float Hex8::_embedding_matrix[8][8][8] =
  {
    // The 8 children of the Hex-type elements can be thought of as being
    // associated with the 8 vertices of the Hex.  Some of the children are
    // numbered the same as their corresponding vertex, while some are
    // not.  The children which are numbered differently have been marked
    // with ** in the comments below.

    // embedding matrix for child 0 (child 0 is associated with vertex 0)
    {
      //  0     1     2     3     4     5     6     7
      { 1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0}, // 0
      { 0.5,  0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0}, // 1
      { .25,  .25,  .25,  .25,  0.0,  0.0,  0.0,  0.0}, // 2
      { 0.5,  0.0,  0.0,  0.5,  0.0,  0.0,  0.0,  0.0}, // 3
      { 0.5,  0.0,  0.0,  0.0,  0.5,  0.0,  0.0,  0.0}, // 4
      { .25,  .25,  0.0,  0.0,  .25,  .25,  0.0,  0.0}, // 5
      {.125, .125, .125, .125, .125, .125, .125, .125}, // 6
      { .25,  0.0,  0.0,  .25,  .25,  0.0,  0.0,  .25}  // 7
    },

    // embedding matrix for child 1 (child 1 is associated with vertex 1)
    {
      //  0     1     2     3     4     5     6     7
      { 0.5,  0.5,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0}, // 0
      { 0.0,  1.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0}, // 1
      { 0.0,  0.5,  0.5,  0.0,  0.0,  0.0,  0.0,  0.0}, // 2
      { .25,  .25,  .25,  .25,  0.0,  0.0,  0.0,  0.0}, // 3
      { .25,  .25,  0.0,  0.0,  .25,  .25,  0.0,  0.0}, // 4
      { 0.0,  0.5,  0.0,  0.0,  0.0,  0.5,  0.0,  0.0}, // 5
      { 0.0,  .25,  .25,  0.0,  0.0,  .25,  .25,  0.0}, // 6
      {.125, .125, .125, .125, .125, .125, .125, .125}  // 7
    },

    // embedding matrix for child 2 (child 2 is associated with vertex 3**)
    {
      //  0      1    2     3     4     5     6     7
      { 0.5,  0.0,  0.0,  0.5,  0.0,  0.0,  0.0,  0.0}, // 0
      { .25,  .25,  .25,  .25,  0.0,  0.0,  0.0,  0.0}, // 1
      { 0.0,  0.0,  0.5,  0.5,  0.0,  0.0,  0.0,  0.0}, // 2
      { 0.0,  0.0,  0.0,  1.0,  0.0,  0.0,  0.0,  0.0}, // 3
      { .25,  0.0,  0.0,  .25,  .25,  0.0,  0.0,  .25}, // 4
      {.125, .125, .125, .125, .125, .125, .125, .125}, // 5
      { 0.0,  0.0,  .25,  .25,  0.0,  0.0,  .25,  .25}, // 6
      { 0.0,  0.0,  0.0,  0.5,  0.0,  0.0,  0.0,  0.5}  // 7
    },

    // embedding matrix for child 3 (child 3 is associated with vertex 2**)
    {
      //  0      1    2     3     4     5     6     7
      { .25,  .25,  .25,  .25,  0.0,  0.0,  0.0,  0.0}, // 0
      { 0.0,  0.5,  0.5,  0.0,  0.0,  0.0,  0.0,  0.0}, // 1
      { 0.0,  0.0,  1.0,  0.0,  0.0,  0.0,  0.0,  0.0}, // 2
      { 0.0,  0.0,  0.5,  0.5,  0.0,  0.0,  0.0,  0.0}, // 3
      {.125, .125, .125, .125, .125, .125, .125, .125}, // 4
      { 0.0,  .25,  .25,  0.0,  0.0,  .25,  .25,  0.0}, // 5
      { 0.0,  0.0,  0.5,  0.0,  0.0,  0.0,  0.5,  0.0}, // 6
      { 0.0,  0.0,  .25,  .25,  0.0,  0.0,  .25,  .25}  // 7
    },

    // embedding matrix for child 4 (child 4 is associated with vertex 4)
    {
      //  0      1    2     3     4     5     6     7
      { 0.5,  0.0,  0.0,  0.0,  0.5,  0.0,  0.0,  0.0}, // 0
      { .25,  .25,  0.0,  0.0,  .25,  .25,  0.0,  0.0}, // 1
      {.125, .125, .125, .125, .125, .125, .125, .125}, // 2
      { .25,  0.0,  0.0,  .25,  .25,  0.0,  0.0,  .25}, // 3
      { 0.0,  0.0,  0.0,  0.0,  1.0,  0.0,  0.0,  0.0}, // 4
      { 0.0,  0.0,  0.0,  0.0,  0.5,  0.5,  0.0,  0.0}, // 5
      { 0.0,  0.0,  0.0,  0.0,  .25,  .25,  .25,  .25}, // 6
      { 0.0,  0.0,  0.0,  0.0,  0.5,  0.0,  0.0,  0.5}  // 7
    },

    // embedding matrix for child 5 (child 5 is associated with vertex 5)
    {
      //  0      1    2     3     4     5     6     7
      { .25,  .25,  0.0,  0.0,  .25,  .25,  0.0,  0.0}, // 0
      { 0.0,  0.5,  0.0,  0.0,  0.0,  0.5,  0.0,  0.0}, // 1
      { 0.0,  .25,  .25,  0.0,  0.0,  .25,  .25,  0.0}, // 2
      {.125, .125, .125, .125, .125, .125, .125, .125}, // 3
      { 0.0,  0.0,  0.0,  0.0,  0.5,  0.5,  0.0,  0.0}, // 4
      { 0.0,  0.0,  0.0,  0.0,  0.0,  1.0,  0.0,  0.0}, // 5
      { 0.0,  0.0,  0.0,  0.0,  0.0,  0.5,  0.5,  0.0}, // 6
      { 0.0,  0.0,  0.0,  0.0,  .25,  .25,  .25,  .25}  // 7
    },

    // embedding matrix for child 6 (child 6 is associated with vertex 7**)
    {
      //  0      1    2     3     4     5     6     7
      { .25,  0.0,  0.0,  .25,  .25,  0.0,  0.0,  .25}, // 0
      {.125, .125, .125, .125, .125, .125, .125, .125}, // 1
      { 0.0,  0.0,  .25,  .25,  0.0,  0.0,  .25,  .25}, // 2
      { 0.0,  0.0,  0.0,  0.5,  0.0,  0.0,  0.0,  0.5}, // 3
      { 0.0,  0.0,  0.0,  0.0,  0.5,  0.0,  0.0,  0.5}, // 4
      { 0.0,  0.0,  0.0,  0.0,  .25,  .25,  .25,  .25}, // 5
      { 0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.5,  0.5}, // 6
      { 0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  1.0}  // 7
    },

    // embedding matrix for child 7 (child 7 is associated with vertex 6**)
    {
      //  0      1    2     3     4     5     6     7
      {.125, .125, .125, .125, .125, .125, .125, .125}, // 0
      { 0.0,  .25,  .25,  0.0,  0.0,  .25,  .25,  0.0}, // 1
      { 0.0,  0.0,  0.5,  0.0,  0.0,  0.0,  0.5,  0.0}, // 2
      { 0.0,  0.0,  .25,  .25,  0.0,  0.0,  .25,  .25}, // 3
      { 0.0,  0.0,  0.0,  0.0,  .25,  .25,  .25,  .25}, // 4
      { 0.0,  0.0,  0.0,  0.0,  0.0,  0.5,  0.5,  0.0}, // 5
      { 0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  1.0,  0.0}, // 6
      { 0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.5,  0.5}  // 7
    }
  };




#endif



Real Hex8::volume () const
{
  // Compute the volume of the tri-linear hex by splitting it
  // into 6 sub-pyramids and applying the formula in:
  // "Calculation of the Volume of a General Hexahedron
  // for Flow Predictions", AIAA Journal v.23, no.6, 1984, p.954-

  static const unsigned char sub_pyr[6][4] =
    {
      {0, 3, 2, 1},
      {6, 7, 4, 5},
      {0, 1, 5, 4},
      {3, 7, 6, 2},
      {0, 4, 7, 3},
      {1, 2, 6, 5}
    };

  // The centroid is a convenient point to use
  // for the apex of all the pyramids.
  const Point R = this->centroid();
  Node* pyr_base[4];

  Real vol=0.;

  // Compute the volume using 6 sub-pyramids
  for (unsigned int n=0; n<6; ++n)
    {
      // Set the nodes of the pyramid base
      for (unsigned int i=0; i<4; ++i)
        pyr_base[i] = this->_nodes[sub_pyr[n][i]];

      // Compute diff vectors
      Point a ( *pyr_base[0] - R );
      Point b ( *pyr_base[1] - *pyr_base[3] );
      Point c ( *pyr_base[2] - *pyr_base[0] );
      Point d ( *pyr_base[3] - *pyr_base[0] );
      Point e ( *pyr_base[1] - *pyr_base[0] );

      // Compute pyramid volume
      Real sub_vol = (1./6.)*(a*(b.cross(c))) + (1./12.)*(c*(d.cross(e)));

      libmesh_assert (sub_vol>0.);

      vol += sub_vol;
    }

  return vol;
}

} // namespace libMesh
