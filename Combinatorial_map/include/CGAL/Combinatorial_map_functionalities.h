// Copyright (c) 2017 CNRS and LIRIS' Establishments (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 3 of the License,
// or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// SPDX-License-Identifier: LGPL-3.0+
//
// Author(s)     : Guillaume Damiand <guillaume.damiand@liris.cnrs.fr>
//
#ifndef CGAL_COMBINATORIAL_MAP_FUNCTIONALITIES_H
#define CGAL_COMBINATORIAL_MAP_FUNCTIONALITIES_H 1

#include <stack>
#include <CGAL/Union_find.h>
#include <boost/unordered_map.hpp>
 
namespace CGAL {

  template<typename Map>
  class Combinatorial_map_tools
  {
  public:
    typedef typename Map::Dart_handle Dart_handle;
    typedef typename Map::Dart_const_handle Dart_const_handle;
    typedef CGAL::Union_find<Dart_handle> UFTree;
    typedef typename UFTree::handle UFTree_handle;
    
    Combinatorial_map_tools(Map& amap) : m_map(amap)
    {
      if (!m_map.is_without_boundary(1))
      {
        std::cerr<<"ERROR: the given amap has 1-boundaries; such a surface is not possible to process here."
                 <<std::endl;
      }
      if (!m_map.is_without_boundary(2))
      {
        std::cerr<<"ERROR: the given amap has 2-boundaries; which are not yet considered (but this will be done later)."
                 <<std::endl;
      }
    }
    
    void initialize_vertices(UFTree& uftrees,
                             boost::unordered_map<Dart_const_handle, UFTree_handle>& vertices)
    {
      uftrees.clear();
      vertices.clear();

      typename Map::size_type treated=m_map.get_new_mark();
      for (typename Map::Dart_range::iterator it=m_map.darts().begin(),
           itend=m_map.darts().end(); it!=itend; ++it)
      {
        if (!m_map.is_marked(it, treated))
        {
          UFTree_handle newuf=uftrees.make_set(it);
          for (typename Map::template Dart_of_cell_basic_range<0>::iterator
               itv=m_map.template darts_of_cell_basic<0>(it, treated).begin(),
               itvend=m_map.template darts_of_cell_basic<0>(it, treated).end();
               itv!=itvend; ++itv)
          {
            vertices[itv]=newuf;
            m_map.mark(itv, treated);
          }
        }
      }
      m_map.free_mark(treated);
    }

    void initialize_faces(UFTree& uftrees,
                          boost::unordered_map<Dart_const_handle, UFTree_handle>& faces)
    {
      uftrees.clear();
      faces.clear();

      typename Map::size_type treated=m_map.get_new_mark();
      for (typename Map::Dart_range::iterator it=m_map.darts().begin(), itend=m_map.darts().end();
           it!=itend; ++it)
      {
        if (!m_map.is_marked(it, treated))
        {
          UFTree_handle newuf=uftrees.make_set(it);
          Dart_handle cur=it;
          do
          {
            faces[cur]=newuf;
            m_map.mark(cur, treated);
            cur=m_map.template beta<1>(cur);
          }
          while (cur!=it);
        }
      }
      m_map.free_mark(treated);
    }

    UFTree_handle get_uftree(const UFTree& uftrees,
                             const boost::unordered_map<Dart_const_handle,
                             UFTree_handle>& mapdhtouf,
                             Dart_const_handle dh)
    {
      assert(dh!=NULL);
      assert(mapdhtouf.find(dh)!=mapdhtouf.end());
      return uftrees.find(mapdhtouf.find(dh)->second);
    }

    void surface_simplification_in_one_face()
    {
      UFTree uftrees; // uftree of faces; one tree for each face, contains one dart of the face
      boost::unordered_map<Dart_const_handle, UFTree_handle> faces;
      initialize_faces(uftrees, faces);

      m_map.set_automatic_attributes_management(false);

      std::stack<Dart_handle> to_treat;
      
      typename Map::size_type treated=m_map.get_new_mark();
      Dart_handle currentdart=NULL;
      bool dangling=false;
      
      typename Map::Dart_range::iterator it=m_map.darts().begin();
      while(!to_treat.empty() || it!=m_map.darts().end())
      {
        if (!to_treat.empty())
        {
          currentdart=to_treat.top();
          to_treat.pop();
          dangling=true;
        }
        else
        {
          currentdart=it++;
          dangling=false;
        }
        
        if (m_map.is_dart_used(currentdart) &&
            (dangling || !m_map.is_marked(currentdart, treated)))
        {
          if (m_map.template is_free<2>(currentdart))
          {
            m_map.mark(currentdart, treated);
          }
          else
          {
            m_map.mark(currentdart, treated);
            m_map.mark(m_map.template beta<2>(currentdart), treated);
            
            // We remove dangling edges and degree two edges.
            // The two first tests allow to keep isolated edges (case of spheres)
            if ((m_map.template beta<0>(currentdart)!=m_map.template beta<2>(currentdart) ||
                 m_map.template beta<1>(currentdart)!=m_map.template beta<2>(currentdart)) &&
                (dangling || get_uftree(uftrees, faces, currentdart)!=
                 get_uftree(uftrees, faces, m_map.template beta<2>(currentdart))))
            {
              // We push in the stack all the neighboors of the current
              // edge that will become dangling after the removal.
              /*              if (!m_map.template is_free<2>(m_map.template beta<0>(currentdart)) &&
                  m_map.template beta<0,2>(currentdart)==m_map.template beta<2,1>(currentdart))
              { to_treat.push(m_map.template beta<0>(currentdart)); }
              if (!m_map.template is_free<2>(m_map.template beta<1>(currentdart)) &&
                  m_map.template beta<1,2>(currentdart)==m_map.template beta<2,0>(currentdart))
              { to_treat.push(m_map.template beta<1>(currentdart)); }
              */
              // TODO LATER (?) OPTIMIZE AND REPLACE THE REMOVE_CELL CALL BY THE MODIFICATION BY HAND
              // Moreover, we make the removal manually instead of calling
              // remove_cell<1>(currentdart) for optimisation reasons.
              // Now we update alpha2
              /*t1 = alpha(*itEdge, 1);
              if ( !isMarked(t1, toDelete1) )
              {
                t2 = *itEdge;
                while ( isMarked(t2, toDelete1) )
                  {
                    t2 = alpha21(t2);
                  }

                if ( t2 != alpha(t1, 1) )
                {
                  unsew(t1, 1);
                  if (!isFree(t2, 1)) unsew(t2, 1);
                  if (t1!=t2) sew(t1, t2, 1);
                }
                }*/

              if (!dangling)
              {
                uftrees.unify_sets(get_uftree(uftrees, faces, currentdart),
                                   get_uftree(uftrees, faces,
                                            m_map.template beta<2>(currentdart)));
              }

              m_map.remove_cell<1>(currentdart);
              if (!m_map.is_dart_used(it))
              { ++it; }

             /* m_map.display_characteristics(std::cout) << ", valid="
                                                       << m_map.is_valid() << std::endl;
              m_map.display_darts(std::cout);
              std::cout<<"\n*****************************************************************\n";
            */
            }
          }
        }
      }
      
      m_map.set_automatic_attributes_management(true);
      m_map.free_mark(treated);
    }
    
    void surface_simplification_in_one_vertex()
    {
      UFTree uftrees; // uftree of vertices; one tree for each vertex, contains one dart of the vertex
      boost::unordered_map<Dart_const_handle, UFTree_handle> vertices;
      initialize_vertices(uftrees, vertices);

      m_map.set_automatic_attributes_management(false);

      for (typename Map::Dart_range::iterator it=m_map.darts().begin(),
           itend=m_map.darts().end(); it!=itend; ++it)
      {
        if (get_uftree(uftrees, vertices, it)!=
            get_uftree(uftrees, vertices, m_map.template beta<2>(it)))
        {
          uftrees.unify_sets(get_uftree(uftrees, vertices, it),
                             get_uftree(uftrees, vertices, m_map.template beta<2>(it)));
          m_map.contract_cell<1>(it);
        }
      }

      m_map.set_automatic_attributes_management(true);
    }
    
    void surface_quadrangulate()
    {
      // Here the map has only one face and one vertex.
      typename Map::size_type oldedges=m_map.get_new_mark();
      m_map.negate_mark(oldedges); // now all edges are marked
      
      // 1) We insert a vertex in the face (note that all points have the same geometry).
      //    New edges created by the operation are not marked.
      m_map.insert_point_in_cell_2(m_map.darts().begin(), m_map.point(m_map.darts().begin()));

      // 2) We remove all the old edges.
      for (typename Map::Dart_range::iterator it=m_map.darts().begin(), itend=m_map.darts().end();
           it!=itend; ++it)
      {
        if (m_map.is_marked(it, oldedges))
        { m_map.remove_cell<1>(it); }
      }

      m_map.free_mark(oldedges);
    }

  protected:
    Map& m_map;
  };
  
} // namespace CGAL

#endif // CGAL_COMBINATORIAL_MAP_FUNCTIONALITIES_H //
// EOF //
