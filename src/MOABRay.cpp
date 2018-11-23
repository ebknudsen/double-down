
#include "ray.h"
#include "primitives.hpp"
#include "MOABRay.h"

double dot_prod(RTCDRay ray) {
  moab::CartVect ray_dir(ray.dir[0], ray.dir[1], ray.dir[2]);
  moab::CartVect tri_norm(ray.Ng[0], ray.Ng[1], ray.Ng[2]);

  return ray_dir % tri_norm;
}

bool in_facets(MBRay ray, moab::EntityHandle tri) {
  if (ray.rh) {
    return ray.rh->in_history(tri);
  } else {
    return false;
  }
}

void backface_cull(MBRay &ray, void*) {

  if( in_facets(ray, ray.primID) ) {
    ray.geomID = RTC_INVALID_GEOMETRY_ID;
    ray.primID = RTC_INVALID_GEOMETRY_ID;
  }
      
  if(dot_prod(ray) < 0.0) {
    ray.geomID = RTC_INVALID_GEOMETRY_ID;
    ray.primID = RTC_INVALID_GEOMETRY_ID;
  }

  return;
}

void frontface_cull(MBRay &ray, void*) {

  if( in_facets(ray, ray.primID) ) {
    ray.geomID = RTC_INVALID_GEOMETRY_ID;
    ray.primID = RTC_INVALID_GEOMETRY_ID;
  }

  if(dot_prod(ray) > 0.0) {
    ray.geomID = RTC_INVALID_GEOMETRY_ID;
    ray.primID = RTC_INVALID_GEOMETRY_ID;
  }

  return;
}

void count_hits(MBRayAccumulate &ray, void*) {

  if (dot_prod(ray) > 0.0) {
    ray.sum -= 1; // leaving
  }
  else {
    ray.sum += 1; // entering
  }

  ray.geomID = RTC_INVALID_GEOMETRY_ID;
  ray.primID = RTC_INVALID_GEOMETRY_ID;

  ray.num_hit++;
  return;
}


void MBDblTriIntersectFunc(void* tris_i, MBRay& ray, size_t item) {

  DblTriIntersectFunc(tris_i, ray, item);

  const DblTri* tris = (const DblTri*) tris_i;
  const DblTri& this_tri = tris[item];

  // set surf and tri handle if hit was found
  if (ray.geomID != RTC_INVALID_GEOMETRY_ID) {
    if (ray.rh) {
      if (!ray.rh->in_history(this_tri.handle)) {
        ray.prim_handle = this_tri.handle;
      } else {
        ray.geomID = RTC_INVALID_GEOMETRY_ID;
        ray.primID = RTC_INVALID_GEOMETRY_ID;
      }
    }
    
    if (ray.orientation == 1) {
      frontface_cull(ray);
    } else if (ray.orientation == -1) {
      backface_cull(ray);
    }
  }
  
  return;
}