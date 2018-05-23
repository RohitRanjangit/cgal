// Copyright (c) 2018 GeometryFactory (France).
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
// SPDX-License-Identifier: GPL-3.0+
//
//
// Author(s)     : Sebastien Loriot, Maxime Gimeno


#ifndef CGAL_POLYGON_MESH_PROCESSING_EXTRUDE_H
#define CGAL_POLYGON_MESH_PROCESSING_EXTRUDE_H

#include <CGAL/license/Polygon_mesh_processing/meshing_hole_filling.h>


#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/boost/graph/named_params_helper.h>
#include <CGAL/boost/graph/named_function_params.h>
#include <CGAL/boost/graph/copy_face_graph.h>
#include <CGAL/Kernel_traits.h>
#include <vector>

namespace CGAL {
namespace Polygon_mesh_processing {
namespace internal{

template<typename PMAP, typename Vector>
struct ConstDistTranslation{
  ConstDistTranslation(PMAP map, const Vector& dir, const double d)
    :map(map), dir(dir), d(d){}
  
  template<typename VertexDescriptor, typename U>
  void operator()(const VertexDescriptor vd, const U&)
  {
    typename boost::property_traits<PMAP>::value_type p = get(map, vd) + d*dir;
    put(map, vd, p);
  }
  
  PMAP map;
  Vector dir;
  double d;
};

struct IdentityFunctor
{
  template<typename T, typename U>
  void operator()(const T&, const U&){}
};
}//end internal

/**
 * \ingroup PMP_meshing_grp
 * Extrudes `input` into `output` using `bot` and `top`. It means that 
 * `input` will be copied twice and transformed once using `bot`, once using `top`,
 * and these two copies will be joined with triangle strips.
 * @tparam InputMesh a model of the concept `FaceListGraph`
 * @tparam OutputMesh a model of the concept `FaceListGraph`
 * @tparam NamedParameters1 a sequence of \ref pmp_namedparameters "Named Parameters" for `InputMesh`
 * @tparam NamedParameters2 a sequence of \ref pmp_namedparameters "Named Parameters" for `OutputMesh`
 * @tparam BottomFunctor a functor that will apply a transformation to all points of 
 * `input` in order to create the first offsetted part of the extrusion. It must have a function 
 * `void operator()(boost::graph_traits<InputMesh>::vertex_descriptor,
 * boost::graph_traits<OutputMesh>::vertex_descriptor);
 * @tparam TopFunctor a functor that will apply a transformation to all points of 
 * `input` in order to create the second offsetted part of the extrusion. It must have a function 
 * `void operator()(boost::graph_traits<InputMesh>::vertex_descriptor,
 * boost::graph_traits<OutputMesh>::vertex_descriptor);
 * @param input the open triangulated `InputMesh` to extrude.
 * @param output the `OutputMesh` containing the result of the extrusion.
 * @param np1 an optional sequence of \ref pmp_namedparameters "Named Parameters" among the ones listed below
 *
 * \cgalNamedParamsBegin
 *    \cgalParamBegin{vertex_point_map}
 *    the property map that contains the points associated to the vertices of `input`. 
 * If this parameter is omitted, an internal property map for `CGAL::vertex_point_t` 
 * should be available for the vertices of `input` \cgalParamEnd
 * \cgalNamedParamsEnd
 * 
 * * @param np2 an optional sequence of \ref pmp_namedparameters "Named Parameters" among the ones listed below
 *
 * \cgalNamedParamsBegin
 *    \cgalParamBegin{vertex_point_map}
 *    the property map that will contain the points associated to the vertices of `output`. 
 * If this parameter is omitted, an internal property map for `CGAL::vertex_point_t` 
 * should be available for the vertices of `output` \cgalParamEnd
 * \cgalNamedParamsEnd
 */
template <class InputMesh,
          class OutputMesh,
          class BottomFunctor,
          class TopFunctor = internal::IdentityFunctor,
          class NamedParameters1,
          class NamedParameters2
          >
void generic_extrude_mesh(const InputMesh& input, 
                          OutputMesh& output, 
                          BottomFunctor& bot,
                          TopFunctor& top,
                          const NamedParameters1& np1,
                          const NamedParameters2& np2)
{
  typedef typename boost::graph_traits<InputMesh>::vertex_descriptor input_vertex_descriptor;
  typedef typename boost::graph_traits<InputMesh>::halfedge_descriptor input_halfedge_descriptor;
  
  typedef typename boost::graph_traits<OutputMesh>::vertex_descriptor   output_vertex_descriptor;
  typedef typename boost::graph_traits<OutputMesh>::halfedge_descriptor output_halfedge_descriptor;
  typedef typename boost::graph_traits<OutputMesh>::edge_descriptor     output_edge_descriptor;
  typedef typename boost::graph_traits<OutputMesh>::face_descriptor     output_face_descriptor;
  
  CGAL_assertion(!CGAL::is_closed(input));
  typedef typename GetVertexPointMap < OutputMesh, NamedParameters2>::type VPMap;
  typedef typename GetVertexPointMap < InputMesh, NamedParameters1>::const_type IVPMap;
  
  VPMap output_vpm = choose_param(get_param(np2, internal_np::vertex_point),
                                   get_property_map(vertex_point, output));
  IVPMap input_vpm = choose_param(get_param(np1, internal_np::vertex_point),
                                   get_const_property_map(vertex_point, input));
  
  std::vector<std::pair<input_vertex_descriptor, output_vertex_descriptor> > bottom_v2v;
  std::vector<std::pair<input_halfedge_descriptor, output_halfedge_descriptor> > bottom_h2h;
  copy_face_graph(input, output, std::back_inserter(bottom_v2v),
                        std::back_inserter(bottom_h2h), Emptyset_iterator(),
                  input_vpm, output_vpm);
  
  // create the offset for the other side
  for(std::size_t i = 0; i< bottom_v2v.size(); ++i)
  {
    bot(bottom_v2v[i].first, bottom_v2v[i].second);
  }
  CGAL::Polygon_mesh_processing::reverse_face_orientations(output);
  
  // collect border halfedges for the creation of the triangle strip
  std::vector<std::pair<input_vertex_descriptor, output_vertex_descriptor> > top_v2v;
  std::vector<std::pair<input_halfedge_descriptor, output_halfedge_descriptor> > top_h2h;
  copy_face_graph(input, output, std::inserter(top_v2v, top_v2v.end()),
                        std::inserter(top_h2h, top_h2h.end()), Emptyset_iterator(),
                  input_vpm, output_vpm);
  for(std::size_t i = 0; i< top_v2v.size(); ++i)
  {
    top(top_v2v[i].first, top_v2v[i].second);
  }
  std::vector<output_halfedge_descriptor> border_hedges;
  std::vector<output_halfedge_descriptor> offset_border_hedges;
  for(std::size_t i = 0; i< top_h2h.size(); ++i)
  {
    input_halfedge_descriptor h = top_h2h[i].first;
    if( CGAL::is_border(h, input) )
    {
      border_hedges.push_back(top_h2h[i].second);
      offset_border_hedges.push_back(bottom_h2h[i].second);
      CGAL_assertion(is_border(border_hedges.back(), output));
      CGAL_assertion(is_border(offset_border_hedges.back(), output));
    }
  }
  // now create a quad strip
  for(std::size_t i=0; i< border_hedges.size(); ++i)
  {
    //     before                 after
    // -----  o  -------     -----  o  -------
    // <----     <-----      <----  |   <-----
    //  nh1        h1         nh1   |     h1
    //                              |
    //                        hnew  |  hnew_opp
    //                              |
    //   ph2       h2          ph2  |     h2
    //  ---->    ----->       ----> |   ----->
    // -----  o  -------     -----  o  -------
    output_halfedge_descriptor h1 = border_hedges[i], h2=offset_border_hedges[i],
        nh1 = next(h1, output), ph2 = prev(h2, output);
    output_halfedge_descriptor newh = halfedge(add_edge(output), output),
        newh_opp = opposite(newh, output);
    // set target vertices of the new halfedges
    set_target(newh, target(h1, output), output);
    set_target(newh_opp, target(ph2, output), output);
    // update next/prev pointers
    set_next(h1, newh_opp, output);
    set_next(newh_opp, h2, output);
    set_next(ph2, newh, output);
    set_next(newh, nh1, output);
  }
  
  // create new faces
  for(std::size_t i=0; i< border_hedges.size(); ++i)
  {
    output_halfedge_descriptor h = border_hedges[i];
    
    output_face_descriptor nf1 = add_face(output);
    output_face_descriptor nf2 = add_face(output);
    
    CGAL::cpp11::array<output_halfedge_descriptor, 4> hedges;
    for (int k=0; k<4; ++k)
    {
      hedges[k]=h;
      h = next(h, output);
    }
    
    //add a diagonal
    output_edge_descriptor new_e = add_edge(output);
    output_halfedge_descriptor new_h = halfedge(new_e, output),
        new_h_opp = opposite(new_h, output);
    // set vertex pointers
    set_target(new_h_opp, target(hedges[0], output), output);
    set_target(new_h, target(hedges[2], output), output);
    
    // set next pointers
    set_next(hedges[0], new_h, output);
    set_next(new_h, hedges[3], output);
    set_next(hedges[2], new_h_opp, output);
    set_next(new_h_opp, hedges[1], output);
    
    // set face halfedge pointers
    set_face(hedges[0], nf1, output);
    set_face(hedges[3], nf1, output);
    set_face(new_h, nf1, output);
    set_face(hedges[1], nf2, output);
    set_face(hedges[2], nf2, output);
    set_face(new_h_opp, nf2, output);
    
    // set halfedge face pointers
    set_halfedge(nf1, hedges[0], output);
    set_halfedge(nf2, hedges[2], output);
  }
}


/**
 * \ingroup PMP_meshing_grp
 * Extrudes `input` into `output` following the direction given by `dir` and
 * at a distance `d`.
 * @tparam InputMesh a model of the concept `FaceListGraph`
 * @tparam OutputMesh a model of the concept `FaceListGraph`
 * @tparam NamedParameters1 a sequence of \ref pmp_namedparameters "Named Parameters" for `InputMesh`
 * @tparam NamedParameters2 a sequence of \ref pmp_namedparameters "Named Parameters" for `OutputMesh`
 * @param input the open triangulated `InputMesh` to extrude.
 * @param output the `OutputMesh` containing the result of the extrusion.
 * @param np1 an optional sequence of \ref pmp_namedparameters "Named Parameters" among the ones listed below
 *
 * \cgalNamedParamsBegin
 *    \cgalParamBegin{vertex_point_map}
 *    the property map that will contain the points associated to the vertices of `output`. 
 * If this parameter is omitted, an internal property map for `CGAL::vertex_point_t` 
 * should be available for the vertices of `output` \cgalParamEnd
 * \cgalNamedParamsEnd
 * 
 * * @param np2 an optional sequence of \ref pmp_namedparameters "Named Parameters" among the ones listed below
 *
 * \cgalNamedParamsBegin
 *    \cgalParamBegin{vertex_point_map}
 *    the property map that contains the points associated to the vertices of `input`. 
 * If this parameter is omitted, an internal property map for `CGAL::vertex_point_t` 
 * should be available for the vertices of `input` \cgalParamEnd
 * \cgalNamedParamsEnd
 */
template <class InputMesh,
          class OutputMesh,
          class NamedParameters1,
          class NamedParameters2>
void extrude_mesh(const InputMesh& input, 
                  OutputMesh& output, 
                  #ifdef DOXYGEN_RUNNING
                  Vector_3 dir,
                  const FT d, 
                  #else
                  typename GetGeomTraits<OutputMesh, NamedParameters2>::type::Vector_3 dir, 
                  const typename GetGeomTraits<OutputMesh, NamedParameters2>::type::FT d,
                  #endif
                  const NamedParameters1& np1,
                  const NamedParameters2& np2)
{
  typedef typename GetVertexPointMap < OutputMesh, NamedParameters2>::type VPMap;
  VPMap output_vpm = choose_param(get_param(np2, internal_np::vertex_point),
                                   get_property_map(vertex_point, output));
  
  internal::ConstDistTranslation<
      typename GetVertexPointMap<OutputMesh, NamedParameters2>::type,
      typename GetGeomTraits<OutputMesh, NamedParameters2>::type::Vector_3> bot(output_vpm, 
                                                                                  dir, d);
  internal::IdentityFunctor top;
  generic_extrude_mesh(input, output, bot,top, np1, np2);
}
//convenience overload
template <class InputMesh,
          class OutputMesh,
          typename Vector>
void extrude_mesh(const InputMesh& input, 
                  OutputMesh& output, 
                  Vector dir, 
                  const double d)
{
  extrude_mesh(input, output, dir, d, 
               parameters::all_default(),
               parameters::all_default());
}

template <class InputMesh,
          class OutputMesh,
          class BottomFunctor,
          class TopFunctor>
void generic_extrude_mesh(const InputMesh& input, 
                          OutputMesh& output, 
                          BottomFunctor& bot,
                          TopFunctor& top)
{
  generic_extrude_mesh(input, output, bot, top, 
                       parameters::all_default(), parameters::all_default());
}

template <class InputMesh,
          class OutputMesh,
          class BottomFunctor>
void generic_extrude_mesh(const InputMesh& input, 
                          OutputMesh& output, 
                          BottomFunctor& bot)
{
  internal::IdentityFunctor top;
  generic_extrude_mesh(input, output, bot, top, 
                       parameters::all_default(), parameters::all_default());
}

}} //end CGAL::PMP
#endif //CGAL_POLYGON_MESH_PROCESSING_EXTRUDE_H
