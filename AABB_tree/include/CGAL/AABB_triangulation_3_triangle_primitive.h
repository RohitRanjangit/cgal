// Copyright (c) 2009 INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s)     : Stéphane Tayeb, Pierre Alliez
//

#ifndef AABB_TRIANGULATION_3_TRIANGLE_PRIMITIVE_H_
#define AABB_TRIANGULATION_3_TRIANGLE_PRIMITIVE_H_

#include <CGAL/license/AABB_tree.h>


#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>

namespace CGAL {
    // \ingroup PkgAABB_tree
    // Primitive type that wraps a facet handle of a CGAL::Triangulation_3,
    // which is used as id, and allows the construction of the datum on
    // the fly. Since only the facet handle is stored in this primitive,
    // the TriangleMesh from which the AABB tree is built should not be
    // deleted while the AABB tree is in use.
    //
    // \cgalModels `AABBPrimitive`
    // \tparam GeomTraits must provides a \c %Point_3
    // type, used as \c Point, and a \c %Triangle_3 type, used as \c
    // Datum and constructible from three arguments of type \c
    // Point.
    // \tparam  Tr must be a
    // \c CGAL::Triangulation_3 whose points have type \c Point.
    //
    // \sa `AABBPrimitive`
    template<typename GeomTraits, typename Tr>
    class AABB_triangulation_3_triangle_primitive
    {
    public:
        typedef typename GeomTraits::Point_3 Point;
        // \name Types
        // @{

        // Id type.
        typedef typename Tr::Facet Id;
        // Geometric data type.
        typedef typename GeomTraits::Triangle_3 Datum;

        // @}

        // Self
        typedef AABB_triangulation_3_triangle_primitive<GeomTraits, Tr> Self;

        // Constructors
        AABB_triangulation_3_triangle_primitive() {}
        AABB_triangulation_3_triangle_primitive(const AABB_triangulation_3_triangle_primitive& primitive)
        {
            m_facet = primitive.id();
        }
        AABB_triangulation_3_triangle_primitive(const Id& handle)
            : m_facet(handle)  { }
        AABB_triangulation_3_triangle_primitive(const Id* ptr)
            : m_facet(*ptr)  { }
        template <class Iterator>
        AABB_triangulation_3_triangle_primitive( Iterator it,
                                            typename boost::enable_if<
                                                       boost::is_same<Id,typename Iterator::value_type>
                                            >::type* =0
        ) : m_facet(*it)  { }


        // Default destructor, copy constructor and assignment operator are ok

        // Returns by constructing on the fly the geometric datum wrapped by the primitive
        Datum datum() const
        {
          typename GeomTraits::Construct_point_3 wp2p =
              GeomTraits().construct_point_3_object();

          int i = m_facet.second;
          const Point& a = wp2p(m_facet.first->vertex((i+1) &3)->point());
          const Point& b = wp2p(m_facet.first->vertex((i+2) &3)->point());
          const Point& c = wp2p(m_facet.first->vertex((i+3) &3)->point());

          return Datum(a,b,c);
        }

        // Returns a point on the primitive
        Point reference_point() const
        {
          typename GeomTraits::Construct_point_3 wp2p =
              GeomTraits().construct_point_3_object();
          return wp2p(m_facet.first->vertex((m_facet.second +1) &3)->point());
        }

        // Returns the identifier
        const Id& id() const { return m_facet; }
        Id& id() { return m_facet; }

    private:
        // The id, here a Tr::Facet
        Id m_facet;
    };  // end class AABB_triangulation_3_triangle_primitive



}  // end namespace CGAL


#endif // AABB_TRIANGULATION_3_TRIANGLE_PRIMITIVE_H_
