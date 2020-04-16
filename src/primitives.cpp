
#include "primitives.hpp"


void intersectionFilter(void* ptr, RTCDRayHit &rayhit)
{
  switch(rayhit.ray.rf_type)
    {
    case 0: //if this is a typical ray_fire, check the dot_product
      if ( 0 > rayhit.dot_prod() )
	rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
      break;
    case 1: //if this is a point_in_vol fire, do nothing
      break;
    }
}


void DblTriBounds(const RTCBoundsFunctionArguments* args)
{
  void* tris_i = args->geometryUserPtr;
  size_t item = args->primID;
  RTCBounds& bounds_o = *args->bounds_o;

  const DblTri* tris = (const DblTri*) tris_i;
  const DblTri& this_tri = tris[item];

  MBDirectAccess* mdam = (MBDirectAccess*) this_tri.mdam;

  bounds_o = DblTriBounds(mdam, this_tri.handle);

  return;
}

void DblTriIntersectFunc(RTCIntersectFunctionNArguments* args) {

  void* tris_i = args->geometryUserPtr;
  size_t item = args->primID;
  RTCDRayHit* rayhit = (RTCDRayHit*)args->rayhit;
  RTCDRay& ray = rayhit->ray;
  RTCDHit& hit = rayhit->hit;

  const DblTri* tris = (const DblTri*) tris_i;
  const DblTri& this_tri = tris[item];

  // moab::Interface* mbi = (moab::Interface*) this_tri.moab_instance;
  // moab::ErrorCode rval;

  MBDirectAccess* mdam = (MBDirectAccess*) this_tri.mdam;

  std::array<Vec3da, 3> coords = mdam->get_coords(this_tri.handle);

  double dist;
  double nonneg_ray_len = 1e17;
  const double* ptr = &nonneg_ray_len;
  Vec3da ray_org(ray.dorg);
  Vec3da ray_dir(ray.ddir);

  bool hit_tri = plucker_ray_tri_intersect(&(coords[0]), ray_org, ray_dir, dist, ptr);

  if ( hit_tri ) {
    if (dist > ray.dtfar) {
      hit.geomID = -1;
      return;
    }

    ray.dtfar = dist;
    ray.tfar = dist;
    hit.u = 0.0f;
    hit.v = 0.0f;
    hit.geomID = this_tri.geomID;
    hit.primID = (unsigned int) item;

    Vec3da normal = cross((coords[1] - coords[0]),(coords[2] - coords[0]));
    normal.normalize();

    if( -1 == this_tri.sense ) normal *= -1;

    hit.dNg[0] = normal[0];
    hit.dNg[1] = normal[1];
    hit.dNg[2] = normal[2];
  } else {
    hit.geomID = RTC_INVALID_GEOMETRY_ID;
  }

  return;
}

void DblTriOccludedFunc(RTCOccludedFunctionNArguments* args) {

  void* tris_i = args->geometryUserPtr;
  size_t item = args->primID;
  RTCDRay* ray = (RTCDRay*)&(args->ray);

  const DblTri* tris = (const DblTri*) tris_i;
  const DblTri& this_tri = tris[item];

  MBDirectAccess* mdam = (MBDirectAccess*) mdam;

  std::array<Vec3da, 3> coords = mdam->get_coords(this_tri.handle);

  double dist;
  double nonneg_ray_len = 1e37;
  double* ptr = &nonneg_ray_len;
  Vec3da ray_org(ray->dorg);
  Vec3da ray_dir(ray->ddir);

  bool hit_tri = plucker_ray_tri_intersect(&(coords[0]), ray_org, ray_dir, dist, ptr);
  if ( hit_tri ) {
    ray->set_len(neg_inf);
  }
}

bool DblTriPointQueryFunc(RTCPointQueryFunctionArguments* args) {

  RTCDPointQuery* pq = (RTCDPointQuery*) args->query;

  double pnt[3];
  pnt[0] = pq->dx;
  pnt[1] = pq->dy;
  pnt[2] = pq->dz;

  size_t item = args->primID;

  RTCGeometry g = rtcGetGeometry(*(RTCScene*)args->userPtr, args->geomID);

  void* tris_i = rtcGetGeometryUserData(g);
  const DblTri* tris = (const DblTri*) tris_i;
  const DblTri& this_tri = tris[item];

  double dist = DblTriClosestFunc(this_tri, pnt);
  if (dist < pq->dradius) {
    pq->radius = dist;
    pq->dradius = dist;
    pq->primID = args->primID;
    pq->geomID = args->geomID;
    return true;
  } else {
    return false;
  }

}

double DblTriClosestFunc(const DblTri& tri, const double loc[3]) {

  MBDirectAccess* mdam = (MBDirectAccess*) tri.mdam;

  std::array<Vec3da, 3> coords = mdam->get_coords(tri.handle);

  Vec3da location(loc);

  Vec3da result;
  closest_location_on_tri(location, &(coords[0]), result);

  return (result - location).length();
}
