/* fast89-poly2tri - svn://repo.hu/projects/fast89_poly2tri

  A C89 port (by Tibor 'Igor2' Palinkas in 2019) of the rewrite of the
  poly2tri library (https://github.com/jhasse/poly2tri) by
  Unspongeful (@unspongeful).

  Based on the Sweep-line, Constrained Delauney Triangulation (CDT) See: Domiter,
  V. and Zalik, B.(2008)'Sweep-line algorithm for constrained Delaunay
  triangulation', International Journal of Geographical Information Science
  "FlipScan" Constrained Edge Algorithm by Thomas hln, thahlen@gmail.com

  This library's focus is speed. No validation is performed, other than
  basic debug-time assert.
  Passing malformed data will result in a crash, _you must_ pre-validate
  the data.

  NOTE: Unlike unlike poly2tri's original implementation I removed
  the ability to add Steiner points explicitly, but they can be added
  by manipulating the internal state of the context.

  -------------------------------
  Configuration defines:

  #define FP2T_POLY2TRI_USE_DOUBLE
     To use double precision floating point for calculations

  #define FP2T_POLY2TRI_USE_CUSTOM_SORT
     To use the a custom merge sort implementation. Enabling this option will
     require more working memory.

  -------------------------------
  Standard library overrides

  #define FP2T_MemorySet, FP2T_MemoryCopy
    To avoid including string.h

  #define FP2T_APIFUNC
    Prefix for declaring API functions.
    Defaults to static but can be #define'd on the caller side, e.g. to empty
    for linking.

  LICENSE - MIT (same as original license)
*/

#ifndef FP2T_POLY2TRI_HEADER
#define FP2T_POLY2TRI_HEADER

/*SECTION: Engine define overrides*/

#include <math.h>	/* fabs fabs */

#ifndef FP2T_MemorySet
#include <string.h>	/*memset */
#define FP2T_MemorySet memset
#define FP2T_MemoryCopy memcpy
#endif

#ifndef FP2T_Assert
#include <assert.h>
#define FP2T_Assert assert
#endif

#ifdef FP2T_STDINT_TYPES
#include <stdint.h>	/* uint32_t */
typedef uint8_t fp2t_u8_t;
typedef int32_t fp2t_i32_t;
typedef int32_t fp2t_b32_t;
typedef uint32_t fp2t_u32_t;
typedef size_t fp2t_uxx_t;
typedef int fp2t_bxx_t;
typedef intptr_t fp2t_imm_t;
typedef uintptr_t fp2t_umm_t;
#endif

#ifdef FP2T_POLY2TRI_USE_DOUBLE
typedef double fp2t_float_t;
#else
typedef float fp2t_float_t;
#endif

#ifndef FP2T_APIFUNC
#define FP2T_APIFUNC static
#endif
#ifndef FP2T_INLINE
#define FP2T_INLINE static
#endif

#ifdef __cplusplus
	extern "C" {
#endif

/*///////////////////*/

typedef struct fp2t_allocator_s {
	fp2t_u8_t *Memory;
	fp2t_umm_t Size;
	fp2t_umm_t Used;
} fp2t_allocator_t;

typedef struct fp2t_point_s {
	struct fp2t_edge_s *FirstEdge;
	fp2t_float_t X;
	fp2t_float_t Y;
} fp2t_point_t;

typedef struct fp2t_triangle_s {
	struct fp2t_triangle_s *Neighbors[3];
	fp2t_point_t *Points[3];
	fp2t_uxx_t Flags;
} fp2t_triangle_t;

typedef struct fp2t_node_s {
	struct fp2t_node_s *Next;
	struct fp2t_node_s *Prev;
	fp2t_point_t *Point;
	fp2t_triangle_t *Triangle;
	fp2t_float_t Value;
#ifndef FP2T_POLY2TRI_USE_DOUBLE
	fp2t_u32_t _PADDING;
#endif
} fp2t_node_t;

typedef struct fp2t_basin_s {
	fp2t_node_t *LeftNode;
	fp2t_node_t *BottomNode;
	fp2t_node_t *RightNode;
	fp2t_float_t Width;
#ifdef FP2T_POLY2TRI_USE_DOUBLE
	fp2t_bxx_t LeftHighest;
#else
	fp2t_b32_t LeftHighest;
#endif
} fp2t_basin_t;

typedef struct fp2t_edge_s {
	fp2t_point_t *P;
	fp2t_point_t *Q;
	struct fp2t_edge_s *Next;
} fp2t_edge_t;

typedef struct fp2t_edge_event_t {
	fp2t_edge_t *ConstrainedEdge;
	fp2t_bxx_t Right;
} fp2t_edge_event_t;


typedef struct fp2t_s {
	void *UserData;
	void *Memory;
	fp2t_allocator_t Allocator;
	fp2t_basin_t Basin;
	fp2t_edge_event_t EdgeEvent;
	fp2t_node_t *HeadNode;
	fp2t_node_t *TailNode;
	fp2t_node_t *SearchNode;

	fp2t_triangle_t *TrianglePool;
	fp2t_triangle_t **Triangles;
	fp2t_triangle_t **TempTriangles;
	fp2t_point_t **Points;
	fp2t_point_t *PointsPool;
	fp2t_node_t *Nodes;

	fp2t_point_t *HeadPoint;
	fp2t_point_t *TailPoint;

	fp2t_u32_t MaxPointCount;
	fp2t_u32_t PointPoolCount;
	fp2t_u32_t PointCount;
	fp2t_u32_t TrianglePoolCount;
	fp2t_u32_t TriangleCount;
	fp2t_u32_t NodeCount;
	fp2t_b32_t Valid;
	fp2t_u32_t _PADDING;
} fp2t_t;


/*///////////////////*/
/* PUBLIC API*/

FP2T_APIFUNC fp2t_umm_t fp2t_memory_required(fp2t_u32_t MaxPointCount);

FP2T_APIFUNC fp2t_b32_t fp2t_init(fp2t_t * PolyContext, void *Memory, fp2t_u32_t MaxPointCount);

FP2T_APIFUNC fp2t_point_t * fp2t_push_point(fp2t_t * PolyContext);

FP2T_APIFUNC fp2t_point_t * fp2t_push_point_array(fp2t_t * PolyContext, fp2t_u32_t Count);

FP2T_APIFUNC void fp2t_add_edge(fp2t_t * PolyContext);

FP2T_APIFUNC void fp2t_triangulate(fp2t_t * PolyContext);

FP2T_APIFUNC void fp2t_add_hole(fp2t_t * PolyContext);

FP2T_APIFUNC void fp2t_add_points(fp2t_t * PolyContext, fp2t_point_t * Points, fp2t_u32_t PointCount);

/*SECTION: Required overrides*/

#ifdef __cplusplus
	}
#endif

#endif /* FP2T_POLY2TRI_HEADER */


#ifdef FP2T_POLY2TRI_IMPLEMENTATION

/*////////////////////*/
/* IMPLEMENTATION*/

#define FP2T_POLY2TRI_EPSILON (fp2t_float_t)1e-12f
#define FP2T_POLY2TRI_POINT_COUNT_EPSILON 16

#ifndef FP2T_InvalidCodePath
#define FP2T_InvalidCodePath FP2T_Assert(0)
#endif

#ifndef FP2T_TRUE
#define FP2T_FALSE 0
#define FP2T_TRUE 1
#endif


#ifndef FP2T_Assert
#error "Please #define FP2T_Assert to assert from <assert.h> or custom implementation"
#endif
#ifndef FP2T_MemorySet
#error "Please #define FP2T_MemorySet to memset from <string.h> or custom implementation"
#endif


#define FP2T_PolyPush(Allocator, Type) ((Type*)FP2T_PolyPushRaw(&Allocator, sizeof(Type)))
#define FP2T_PolyPushArray(Allocator, Type, Count) ((Type*)FP2T_PolyPushRaw(&Allocator, sizeof(Type) * Count))

FP2T_APIFUNC fp2t_u8_t * FP2T_PolyPushRaw(fp2t_allocator_t * Allocator, fp2t_umm_t Size);

typedef struct fp2t_edge_ts {
	fp2t_u8_t ConstrainedCW;
	fp2t_u8_t ConstrainedCCW;
	fp2t_u8_t DelaunayCW;
	fp2t_u8_t DelaunayCCW;
} fp2t_edge_ts;

enum fp2t_orientation {
	fp2t_orientation_CCW,
	fp2t_orientation_CW,
	fp2t_orientation_Collinear
};

enum fp2t_tri_flag {
	fp2t_tri_flag_DelaunayEdge0 = 1 << 0,
	fp2t_tri_flag_DelaunayEdge1 = 1 << 1,
	fp2t_tri_flag_DelaunayEdge2 = 1 << 2,
	fp2t_tri_flag_ConstrainedEdge0 = 1 << 3,
	fp2t_tri_flag_ConstrainedEdge1 = 1 << 4,
	fp2t_tri_flag_ConstrainedEdge2 = 1 << 5,

	fp2t_tri_flag_IsInterior = 1 << 30,

	fp2t_tri_flag_DelaunayEdgeMask = fp2t_tri_flag_DelaunayEdge0 | fp2t_tri_flag_DelaunayEdge1 | fp2t_tri_flag_DelaunayEdge2,
	fp2t_tri_flag_ConstrainedEdgeMask = fp2t_tri_flag_ConstrainedEdge0 | fp2t_tri_flag_ConstrainedEdge1 | fp2t_tri_flag_ConstrainedEdge2
};


static fp2t_u32_t fp2t_edge_tLUT[] = { 0, 1, 2, 0, 1, 2, 3 };

FP2T_APIFUNC fp2t_u8_t * FP2T_PolyPushRaw(fp2t_allocator_t * Allocator, fp2t_umm_t Size)
{
	fp2t_u8_t *Result = 0;
	if (Allocator->Used + Size <= Allocator->Size) {
		Result = Allocator->Memory + Allocator->Used;
		Allocator->Used += Size;
	}
	else {
		FP2T_InvalidCodePath;
	}
	return Result;
}


/*
  Formula to calculate signed area
  Positive if CCW
  Negative if CW
  0 if collinear
  A[P1,P2,P3]  =  (x1*y2 - y1*x2) + (x2*y3 - y2*x3) + (x3*y1 - y3*x1)
               =  (x1-x3)*(y2-y3) - (y1-y3)*(x2-x3)
 */

FP2T_APIFUNC fp2t_uxx_t FP2T_PolyOrient2D(fp2t_point_t * PointA, fp2t_point_t * PointB, fp2t_point_t * PointC)
{
	fp2t_uxx_t Result;
	fp2t_float_t DeltaLeft = (PointA->X - PointC->X) * (PointB->Y - PointC->Y);
	fp2t_float_t DeltaRight = (PointA->Y - PointC->Y) * (PointB->X - PointC->X);
	fp2t_float_t Value = DeltaLeft - DeltaRight;

	FP2T_Assert(PointA != PointB && PointA != PointC && PointB != PointC);

	if (Value > -FP2T_POLY2TRI_EPSILON && Value < FP2T_POLY2TRI_EPSILON) {
		Result = fp2t_orientation_Collinear;
	}
	else if (Value > 0) {
		Result = fp2t_orientation_CCW;
	}
	else {
		Result = fp2t_orientation_CW;
	}
	return Result;
}

FP2T_APIFUNC fp2t_bxx_t FP2T_PolyInScanArea(fp2t_point_t * PointA, fp2t_point_t * PointB, fp2t_point_t * PointC, fp2t_point_t * PointD)
{
	fp2t_float_t oadb = (PointA->X - PointB->X) * (PointD->Y - PointB->Y) - (PointD->X - PointB->X) * (PointA->Y - PointB->Y);
	fp2t_bxx_t Result;

	FP2T_Assert(PointA != PointB && PointA != PointC && PointA != PointD && PointB != PointC && PointB != PointD && PointC != PointD);

	if (oadb >= -FP2T_POLY2TRI_EPSILON) {
		Result = FP2T_FALSE;
	}
	else {
		fp2t_float_t oadc = (PointA->X - PointC->X) * (PointD->Y - PointC->Y) - (PointD->X - PointC->X) * (PointA->Y - PointC->Y);
		if (oadc <= FP2T_POLY2TRI_EPSILON) {
			Result = FP2T_FALSE;
		}
		else {
			Result = FP2T_TRUE;
		}
	}
	return Result;
}

FP2T_INLINE fp2t_bxx_t FP2T_PolyContainsPoint(fp2t_triangle_t * Triangle, fp2t_point_t * Point)
{
	fp2t_bxx_t Result = Point == Triangle->Points[0] || Point == Triangle->Points[1] || Point == Triangle->Points[2];
	FP2T_Assert(Point);
	return Result;
}

FP2T_INLINE fp2t_bxx_t FP2T_PolyContainsPoints(fp2t_triangle_t * Triangle, fp2t_point_t * Point, fp2t_point_t * OtherPoint)
{
	fp2t_bxx_t Result = FP2T_PolyContainsPoint(Triangle, Point) && FP2T_PolyContainsPoint(Triangle, OtherPoint);
	return Result;
}

FP2T_APIFUNC void FP2T_PolySwapNeighbors(fp2t_triangle_t * Triangle, fp2t_triangle_t * OldNeighbor, fp2t_triangle_t * NewNeighbor)
{
	fp2t_u32_t NeighborIndex;
	for(NeighborIndex = 0;; ++NeighborIndex) {
		FP2T_Assert(NeighborIndex < 3);
		if (Triangle->Neighbors[NeighborIndex] == OldNeighbor) {
			Triangle->Neighbors[NeighborIndex] = NewNeighbor;
			break;
		}
	}
}

FP2T_APIFUNC void FP2T_PolyMarkNeighborTri(fp2t_triangle_t * Triangle, fp2t_triangle_t * Neighbor)
{
	fp2t_u32_t EdgeIndex;
	for(EdgeIndex = 0; EdgeIndex < 3; ++EdgeIndex) {
		fp2t_u32_t NextEdgeIndex = fp2t_edge_tLUT[EdgeIndex + 1];
		fp2t_u32_t NeighborIndex;
		for(NeighborIndex = 0; NeighborIndex < 3; ++NeighborIndex) {
			fp2t_u32_t NextNeighborIndex = fp2t_edge_tLUT[NeighborIndex + 1];
			if ((Triangle->Points[EdgeIndex] == Neighbor->Points[NeighborIndex] &&
					 Triangle->Points[NextEdgeIndex] == Neighbor->Points[NextNeighborIndex]) || (Triangle->Points[EdgeIndex] == Neighbor->Points[NextNeighborIndex] && Triangle->Points[NextEdgeIndex] == Neighbor->Points[NeighborIndex])) {
				Triangle->Neighbors[fp2t_edge_tLUT[NextEdgeIndex + 1]] = Neighbor;
				Neighbor->Neighbors[fp2t_edge_tLUT[NextNeighborIndex + 1]] = Triangle;
				return;
			}
		}
	}
}

FP2T_INLINE void FP2T_ClearDelunayEdges(fp2t_triangle_t * Triangle)
{
	Triangle->Flags &= ~(fp2t_uxx_t) fp2t_tri_flag_DelaunayEdgeMask;
}

FP2T_APIFUNC fp2t_point_t * FP2T_PointCW(fp2t_triangle_t * Triangle, fp2t_point_t * Point, fp2t_i32_t * PointIndex)
{
	fp2t_point_t *Result;
	if (Point == Triangle->Points[0]) {
		Result = Triangle->Points[2];
		*PointIndex = 2;
	}
	else if (Point == Triangle->Points[1]) {
		Result = Triangle->Points[0];
		*PointIndex = 0;
	}
	else if (Point == Triangle->Points[2]) {
		Result = Triangle->Points[1];
		*PointIndex = 1;
	}
	else {
		FP2T_InvalidCodePath;
		Result = 0;
		*PointIndex = -1;
	}
	return Result;
}

FP2T_APIFUNC fp2t_triangle_t * FP2T_NeighborCCW(fp2t_triangle_t * Triangle, fp2t_point_t * Point)
{
	fp2t_triangle_t *Result;
	if (Point == Triangle->Points[0]) {
		Result = Triangle->Neighbors[2];
	}
	else if (Point == Triangle->Points[1]) {
		Result = Triangle->Neighbors[0];
	}
	else {
		FP2T_Assert(Point == Triangle->Points[2]);
		Result = Triangle->Neighbors[1];
	}
	return Result;
}

#define FLAG(Tri, Name) (Tri->Flags & fp2t_tri_flag_##Name) == fp2t_tri_flag_## Name

FP2T_APIFUNC void FP2T_SetAdjacentEdges(fp2t_triangle_t * Triangle, fp2t_point_t * Point, fp2t_edge_ts Edges, fp2t_bxx_t Clockwise)
{
	fp2t_uxx_t Mask;
	fp2t_uxx_t NewFlags;
	if (Point == Triangle->Points[0]) {
		if (Clockwise) {
			Mask = fp2t_tri_flag_ConstrainedEdge1 | fp2t_tri_flag_DelaunayEdge1;
			NewFlags = (Edges.ConstrainedCW & fp2t_tri_flag_ConstrainedEdge1) | (Edges.DelaunayCW & fp2t_tri_flag_DelaunayEdge1);
		}
		else {
			Mask = fp2t_tri_flag_ConstrainedEdge2 | fp2t_tri_flag_DelaunayEdge2;
			NewFlags = (Edges.ConstrainedCCW & fp2t_tri_flag_ConstrainedEdge2) | (Edges.DelaunayCCW & fp2t_tri_flag_DelaunayEdge2);
		}
	}
	else if (Point == Triangle->Points[1]) {
		if (Clockwise) {
			Mask = fp2t_tri_flag_ConstrainedEdge2 | fp2t_tri_flag_DelaunayEdge2;
			NewFlags = (Edges.ConstrainedCW & fp2t_tri_flag_ConstrainedEdge2) | (Edges.DelaunayCW & fp2t_tri_flag_DelaunayEdge2);
		}
		else {
			Mask = fp2t_tri_flag_ConstrainedEdge0 | fp2t_tri_flag_DelaunayEdge0;
			NewFlags = (Edges.ConstrainedCCW & fp2t_tri_flag_ConstrainedEdge0) | (Edges.DelaunayCCW & fp2t_tri_flag_DelaunayEdge0);
		}
	}
	else {
		if (Clockwise) {
			Mask = fp2t_tri_flag_ConstrainedEdge0 | fp2t_tri_flag_DelaunayEdge0;
			NewFlags = (Edges.ConstrainedCW & fp2t_tri_flag_ConstrainedEdge0) | (Edges.DelaunayCW & fp2t_tri_flag_DelaunayEdge0);
		}
		else {
			Mask = fp2t_tri_flag_ConstrainedEdge1 | fp2t_tri_flag_DelaunayEdge1;
			NewFlags = (Edges.ConstrainedCCW & fp2t_tri_flag_ConstrainedEdge1) | (Edges.DelaunayCCW & fp2t_tri_flag_DelaunayEdge1);
		}
	}
	Triangle->Flags = (Triangle->Flags & ~Mask) | NewFlags;
}


FP2T_APIFUNC fp2t_uxx_t FP2T_GetConstrainedEdgeCW(fp2t_triangle_t * Triangle, fp2t_point_t * Point)
{
	fp2t_uxx_t Result;
	if (Point == Triangle->Points[0]) {
		Result = Triangle->Flags & fp2t_tri_flag_ConstrainedEdge1;
	}
	else if (Point == Triangle->Points[1]) {
		Result = Triangle->Flags & fp2t_tri_flag_ConstrainedEdge2;
	}
	else {
		FP2T_Assert(Point == Triangle->Points[2]);
		Result = Triangle->Flags & fp2t_tri_flag_ConstrainedEdge0;
	}
	return Result;
}

FP2T_INLINE fp2t_triangle_t * FP2T_PolyNeighborAcross(fp2t_triangle_t * Triangle, fp2t_point_t * OppositePoint)
{
	fp2t_triangle_t *Result;
	if (OppositePoint == Triangle->Points[0]) {
		Result = Triangle->Neighbors[0];
	}
	else if (OppositePoint == Triangle->Points[1]) {
		Result = Triangle->Neighbors[1];
	}
	else {
		FP2T_Assert(OppositePoint == Triangle->Points[2]);
		Result = Triangle->Neighbors[2];
	}
	return Result;
}

#if FP2T_UNUSED_INTERNAL
FP2T_INLINE void FP2T_PolyLegalizePoint(fp2t_triangle_t * Triangle, fp2t_point_t * Point)
{
	Triangle->Points[1] = Triangle->Points[0];
	Triangle->Points[0] = Triangle->Points[2];
	Triangle->Points[2] = Point;
}
#endif

FP2T_APIFUNC fp2t_i32_t FP2T_PolyPointIndex(fp2t_triangle_t * Triangle, fp2t_point_t * Point)
{
	fp2t_i32_t Result;
	if (Point == Triangle->Points[0]) {
		Result = 0;
	}
	else if (Point == Triangle->Points[1]) {
		Result = 1;
	}
	else if (Point == Triangle->Points[2]) {
		Result = 2;
	}
	else {
		Result = -1;
		FP2T_InvalidCodePath;
	}
	return Result;
}

FP2T_APIFUNC fp2t_i32_t FP2T_EdgeIndex(fp2t_triangle_t * Triangle, fp2t_point_t * P1, fp2t_point_t * P2)
{
	fp2t_i32_t Result = -1;
	if (Triangle->Points[0] == P1) {
		if (Triangle->Points[1] == P2) {
			Result = 2;
		}
		else if (Triangle->Points[2] == P2) {
			Result = 1;
		}
	}
	else if (Triangle->Points[1] == P1) {
		if (Triangle->Points[2] == P2) {
			Result = 0;
		}
		else if (Triangle->Points[0] == P2) {
			Result = 2;
		}
	}
	else if (Triangle->Points[2] == P1) {
		if (Triangle->Points[0] == P2) {
			Result = 1;
		}
		else if (Triangle->Points[1] == P2) {
			Result = 0;
		}
	}
	return Result;
}


FP2T_INLINE void FP2T_PolyMarkConstrainedEdgeIndex(fp2t_triangle_t * Triangle, fp2t_i32_t Index)
{
	FP2T_Assert(Index >= 0);
	Triangle->Flags |= (fp2t_uxx_t) fp2t_tri_flag_ConstrainedEdge0 << (fp2t_u32_t) Index;
}

FP2T_INLINE void FP2T_PolyMarkDelaunayEdge(fp2t_triangle_t * Triangle, fp2t_i32_t Index)
{
	FP2T_Assert(Index >= 0);
	Triangle->Flags |= (fp2t_uxx_t) fp2t_tri_flag_DelaunayEdge0 << (fp2t_u32_t) Index;
}

FP2T_APIFUNC void FP2T_PolyMarkConstrainedEdgePoints(fp2t_triangle_t * Triangle, fp2t_point_t * Point, fp2t_point_t * OtherPoint)
{
	if ((OtherPoint == Triangle->Points[0] && Point == Triangle->Points[1]) || (OtherPoint == Triangle->Points[1] && Point == Triangle->Points[0])) {
		FP2T_PolyMarkConstrainedEdgeIndex(Triangle, 2);
	}
	else if ((OtherPoint == Triangle->Points[0] && Point == Triangle->Points[2]) || (OtherPoint == Triangle->Points[2] && Point == Triangle->Points[0])) {
		FP2T_PolyMarkConstrainedEdgeIndex(Triangle, 1);
	}
	else if ((OtherPoint == Triangle->Points[1] && Point == Triangle->Points[2]) || (OtherPoint == Triangle->Points[2] && Point == Triangle->Points[1])) {
		FP2T_PolyMarkConstrainedEdgeIndex(Triangle, 0);
	}
}

#if FP2T_UNUSED_INTERNAL
FP2T_INLINE void FP2T_PolyMarkConstrainedEdge(fp2t_triangle_t * Triangle, fp2t_edge_t * Edge)
{
	FP2T_PolyMarkConstrainedEdgePoints(Triangle, Edge->P, Edge->Q);
}
#endif

FP2T_INLINE fp2t_node_t * FP2T_PolyNode(fp2t_t * PolyContext, fp2t_point_t * Point, fp2t_triangle_t * Triangle)
{
	fp2t_node_t *Result = PolyContext->Nodes + PolyContext->NodeCount++;
	Result->Point = Point;
	Result->Triangle = Triangle;
	Result->Value = Point->X;
	return Result;
}

FP2T_APIFUNC fp2t_point_t * fp2t_push_point(fp2t_t * PolyContext)
{
	fp2t_point_t *Result;
	FP2T_Assert(PolyContext->MaxPointCount > PolyContext->PointPoolCount);
	Result = PolyContext->PointsPool + PolyContext->PointPoolCount++;
	return Result;
}

FP2T_APIFUNC fp2t_point_t * fp2t_push_point_array(fp2t_t * PolyContext, fp2t_u32_t Count)
{
	fp2t_point_t *Result = PolyContext->PointsPool + PolyContext->PointPoolCount;
	FP2T_Assert(PolyContext->MaxPointCount > (PolyContext->PointPoolCount + Count));
	PolyContext->PointPoolCount += Count;
	return Result;
}


FP2T_INLINE fp2t_triangle_t * FP2T_PushTriangle(fp2t_t * PolyContext, fp2t_point_t * A, fp2t_point_t * B, fp2t_point_t * C)
{
	fp2t_triangle_t *Result = PolyContext->TrianglePool + PolyContext->TrianglePoolCount++;
	Result->Points[0] = A;
	Result->Points[1] = B;
	Result->Points[2] = C;
	return Result;
}

FP2T_INLINE fp2t_node_t * FP2T_LocatePoint(fp2t_t * PolyContext, fp2t_point_t * Point)
{
	fp2t_float_t PX = Point->X;
	fp2t_node_t *Node = PolyContext->SearchNode;
	fp2t_float_t NX = Node->Point->X;

	if (PX < NX) {
		while((Node = Node->Prev)) {
			if (Point == Node->Point) {
				break;
			}
		}
	}
	else if ((PX - NX) < FP2T_POLY2TRI_EPSILON) {
		if (Point != Node->Point) {
			/* We might have two nodes with same X Value for a short time */
			if (Point == Node->Prev->Point) {
				Node = Node->Prev;
			}
			else {
				FP2T_Assert(Point == Node->Next->Point);
				Node = Node->Next;
			}
		}
	}
	else {
		while((Node = Node->Next)) {
			if (Point == Node->Point) {
				break;
			}
		}
	}
	if (Node) {
		PolyContext->SearchNode = Node;
	}
	return Node;
}

FP2T_APIFUNC fp2t_bxx_t FP2T_IsEdgeSideOfTriangle(fp2t_triangle_t * Triangle, fp2t_point_t * EdgeP, fp2t_point_t * EdgeQ)
{
	fp2t_i32_t EdgeIndex = FP2T_EdgeIndex(Triangle, EdgeP, EdgeQ);
	fp2t_bxx_t Result;
	if (EdgeIndex != -1) {
		fp2t_triangle_t *NeighborTriangle;
		FP2T_PolyMarkConstrainedEdgeIndex(Triangle, EdgeIndex);
		NeighborTriangle = Triangle->Neighbors[EdgeIndex];
		if (NeighborTriangle) {
			FP2T_PolyMarkConstrainedEdgePoints(NeighborTriangle, EdgeP, EdgeQ);
		}
		Result = FP2T_TRUE;
	}
	else {
		Result = FP2T_FALSE;
	}
	return Result;
}

FP2T_APIFUNC fp2t_bxx_t FP2T_AngleExceeds90Degrees(fp2t_point_t * Origin, fp2t_point_t * PointA, fp2t_point_t * PointB)
{
	fp2t_float_t AX = PointA->X - Origin->X;
	fp2t_float_t AY = PointA->Y - Origin->Y;
	fp2t_float_t BX = PointB->X - Origin->X;
	fp2t_float_t BY = PointB->Y - Origin->Y;
	fp2t_float_t DotProduct = AX * BX + AY * BY;
	fp2t_bxx_t Result = DotProduct < 0;
	return Result;
}

FP2T_APIFUNC fp2t_bxx_t FP2T_AngleExceedsPlus90DegreesOrIsNegative(fp2t_point_t * Origin, fp2t_point_t * PointA, fp2t_point_t * PointB)
{
	fp2t_float_t AX = PointA->X - Origin->X;
	fp2t_float_t AY = PointA->Y - Origin->Y;
	fp2t_float_t BX = PointB->X - Origin->X;
	fp2t_float_t BY = PointB->Y - Origin->Y;
	fp2t_float_t DotProduct = AX * BX + AY * BY;
	fp2t_float_t Direction = AX * BY - AY * BX;
	fp2t_bxx_t Result = DotProduct < 0 || Direction < 0;
	return Result;
}


FP2T_APIFUNC fp2t_bxx_t FP2T_LargeHole_DontFill(fp2t_node_t * Node)
{
	fp2t_bxx_t Result;
	fp2t_node_t *NextNode = Node->Next;
	fp2t_node_t *PrevNode = Node->Prev;
	if (!FP2T_AngleExceeds90Degrees(Node->Point, NextNode->Point, PrevNode->Point)) {
		Result = FP2T_FALSE;
	}
	else {
		/* Check additional points on front. */
		fp2t_node_t *Next2Node = NextNode->Next;
		/* "..Plus.." because only want angles on same side as fp2t_point_t being added. */
		if (Next2Node && !FP2T_AngleExceedsPlus90DegreesOrIsNegative(Node->Point, Next2Node->Point, PrevNode->Point)) {
			Result = FP2T_FALSE;
		}
		else {
			fp2t_node_t *Prev2Node = PrevNode->Prev;
			/* "..Plus.." because only want angles on same side as fp2t_point_t being added. */
			if (Prev2Node && !FP2T_AngleExceedsPlus90DegreesOrIsNegative(Node->Point, NextNode->Point, Prev2Node->Point)) {
				Result = FP2T_FALSE;
			}
			else {
				Result = FP2T_TRUE;
			}
		}
	}
	return Result;
}

FP2T_INLINE fp2t_bxx_t FP2T_PolyShouldFillBasin(fp2t_node_t * Node)
{
	fp2t_float_t AX = Node->Point->X;
	fp2t_float_t AY = Node->Point->Y;
	fp2t_float_t BX = Node->Next->Next->Point->X;
	fp2t_float_t BY = Node->Next->Next->Point->Y;
	fp2t_float_t DotProduct = AX * BX + AY * BY;
	fp2t_float_t Direction = AX * BY - AY * BX;
	fp2t_bxx_t Result = DotProduct < 0 || Direction > 0;
	return Result;
}

/**
  * Requirement:
  * 1. a,b and c form a triangle.
  * 2. a and d is know to be on opposite side of bc
  *                a
  *                +
  *               / \
  *              /   \
  *            b/     \c
  *            +-------+
  *           /    d    \
  *          /           \
  * Fact: d has to be in area B to have a chance to be inside the circle formed by
  *  a,b and c
  *  d is outside B if FP2T_PolyOrient2D(a,b,d) or FP2T_PolyOrient2D(c,a,d) is CW
  *  This preknowledge gives us a way to optimize the incircle test
  */

FP2T_APIFUNC fp2t_bxx_t FP2T_PolyInCircle(fp2t_point_t * PointA, fp2t_point_t * PointB, fp2t_point_t * pc, fp2t_point_t * pd)
{
	fp2t_bxx_t Result;
	fp2t_float_t adx = PointA->X - pd->X;
	fp2t_float_t ady = PointA->Y - pd->Y;
	fp2t_float_t bdx = PointB->X - pd->X;
	fp2t_float_t bdy = PointB->Y - pd->Y;

	fp2t_float_t adxbdy = adx * bdy;
	fp2t_float_t bdxady = bdx * ady;
	fp2t_float_t oabd = adxbdy - bdxady;

	if (oabd <= 0) {
		Result = FP2T_FALSE;
	}
	else {
		fp2t_float_t cdx = pc->X - pd->X;
		fp2t_float_t cdy = pc->Y - pd->Y;

		fp2t_float_t cdxady = cdx * ady;
		fp2t_float_t adxcdy = adx * cdy;
		fp2t_float_t ocad = cdxady - adxcdy;

		if (ocad <= 0) {
			Result = FP2T_FALSE;
		}
		else {
			fp2t_float_t bdxcdy = bdx * cdy;
			fp2t_float_t cdxbdy = cdx * bdy;

			fp2t_float_t alift = adx * adx + ady * ady;
			fp2t_float_t blift = bdx * bdx + bdy * bdy;
			fp2t_float_t clift = cdx * cdx + cdy * cdy;

			fp2t_float_t det = alift * (bdxcdy - cdxbdy) + blift * ocad + clift * oabd;
			Result = det > 0;
		}
	}
	return Result;
}

/**
  * Rotates a triangle pair one vertex CW
  *       n2                    n2
  *  P +-----+             P +-----+
  *    | t  /|               |\  t |
  *    |   / |               | \   |
  *  n1|  /  |n3           n1|  \  |n3
  *    | /   |    after CW   |   \ |
  *    |/ oT |               | oT \|
  *    +-----+ oP            +-----+
  *       n4                    n4
  */
FP2T_APIFUNC void FP2T_PolyRotateTrianglePair(fp2t_triangle_t * Triangle, fp2t_point_t * Point, fp2t_i32_t PointIndex, fp2t_triangle_t * OtherTriangle, fp2t_point_t * OtherPoint, fp2t_i32_t OtherPointIndex)
{
	fp2t_uxx_t Flags = Triangle->Flags;
	fp2t_uxx_t OtherFlags = OtherTriangle->Flags;
	fp2t_edge_ts AdjacentEdges;
	fp2t_edge_ts OtherAdjacentEdges;

	fp2t_u32_t RotateAmount = fp2t_edge_tLUT[PointIndex + 1];
	fp2t_u32_t OtherRotateAmount = fp2t_edge_tLUT[OtherPointIndex + 1];

	fp2t_triangle_t *Neighbor1 = Triangle->Neighbors[fp2t_edge_tLUT[1 + RotateAmount]];
	fp2t_triangle_t *Neighbor2 = Triangle->Neighbors[RotateAmount];
	fp2t_triangle_t *Neighbor3 = OtherTriangle->Neighbors[fp2t_edge_tLUT[1 + OtherRotateAmount]];
	fp2t_triangle_t *Neighbor4 = OtherTriangle->Neighbors[OtherRotateAmount];

	FP2T_Assert(PointIndex >= 0 && OtherPointIndex >= 0);

	Triangle->Neighbors[RotateAmount] = Neighbor3;
	Triangle->Neighbors[fp2t_edge_tLUT[RotateAmount + 1]] = Neighbor2;
	Triangle->Neighbors[fp2t_edge_tLUT[RotateAmount + 2]] = OtherTriangle;
	OtherTriangle->Neighbors[OtherRotateAmount] = Neighbor1;
	OtherTriangle->Neighbors[fp2t_edge_tLUT[OtherRotateAmount + 1]] = Neighbor4;
	OtherTriangle->Neighbors[fp2t_edge_tLUT[OtherRotateAmount + 2]] = Triangle;

	if (Neighbor1) {
		FP2T_PolySwapNeighbors(Neighbor1, Triangle, OtherTriangle);
	}
	if (Neighbor3) {
		FP2T_PolySwapNeighbors(Neighbor3, OtherTriangle, Triangle);
	}

	Triangle->Points[RotateAmount] = Triangle->Points[fp2t_edge_tLUT[2 + RotateAmount]];
	Triangle->Points[fp2t_edge_tLUT[2 + RotateAmount]] = Triangle->Points[fp2t_edge_tLUT[1 + RotateAmount]];
	Triangle->Points[fp2t_edge_tLUT[1 + RotateAmount]] = OtherPoint;
	OtherTriangle->Points[OtherRotateAmount] = OtherTriangle->Points[fp2t_edge_tLUT[2 + OtherRotateAmount]];
	OtherTriangle->Points[fp2t_edge_tLUT[2 + OtherRotateAmount]] = OtherTriangle->Points[fp2t_edge_tLUT[1 + OtherRotateAmount]];
	OtherTriangle->Points[fp2t_edge_tLUT[1 + OtherRotateAmount]] = Point;

	AdjacentEdges.ConstrainedCW = Flags & ((fp2t_uxx_t) fp2t_tri_flag_ConstrainedEdge0 << ((RotateAmount))) ? 0xFF : 0;
	AdjacentEdges.ConstrainedCCW = Flags & ((fp2t_uxx_t) fp2t_tri_flag_ConstrainedEdge0 << (fp2t_edge_tLUT[RotateAmount + 1])) ? 0xFF : 0;
	AdjacentEdges.DelaunayCW = Flags & ((fp2t_uxx_t) fp2t_tri_flag_DelaunayEdge0 << ((RotateAmount))) ? 0xFF : 0;
	AdjacentEdges.DelaunayCCW = Flags & ((fp2t_uxx_t) fp2t_tri_flag_DelaunayEdge0 << (fp2t_edge_tLUT[RotateAmount + 1])) ? 0xFF : 0;

	OtherAdjacentEdges.ConstrainedCW = OtherFlags & ((fp2t_uxx_t) fp2t_tri_flag_ConstrainedEdge0 << (fp2t_edge_tLUT[OtherRotateAmount])) ? 0xFF : 0;
	OtherAdjacentEdges.ConstrainedCCW = OtherFlags & ((fp2t_uxx_t) fp2t_tri_flag_ConstrainedEdge0 << (fp2t_edge_tLUT[OtherRotateAmount + 1])) ? 0xFF : 0;
	OtherAdjacentEdges.DelaunayCW = OtherFlags & ((fp2t_uxx_t) fp2t_tri_flag_DelaunayEdge0 << (fp2t_edge_tLUT[OtherRotateAmount])) ? 0xFF : 0;
	OtherAdjacentEdges.DelaunayCCW = OtherFlags & ((fp2t_uxx_t) fp2t_tri_flag_DelaunayEdge0 << (fp2t_edge_tLUT[OtherRotateAmount + 1])) ? 0xFF : 0;

	FP2T_SetAdjacentEdges(Triangle, Point, AdjacentEdges, FP2T_TRUE);
	FP2T_SetAdjacentEdges(Triangle, OtherPoint, OtherAdjacentEdges, FP2T_FALSE);
	FP2T_SetAdjacentEdges(OtherTriangle, Point, AdjacentEdges, FP2T_FALSE);
	FP2T_SetAdjacentEdges(OtherTriangle, OtherPoint, OtherAdjacentEdges, FP2T_TRUE);
}

FP2T_APIFUNC fp2t_bxx_t FP2T_PolyIsShallow(fp2t_t * PolyContext, fp2t_node_t * Node)
{
	fp2t_float_t Height;
	if (PolyContext->Basin.LeftHighest) {
		Height = PolyContext->Basin.LeftNode->Point->Y - Node->Point->Y;
	}
	else {
		Height = PolyContext->Basin.RightNode->Point->Y - Node->Point->Y;
	}

	{
		fp2t_bxx_t Result = PolyContext->Basin.Width > Height;
		return Result;
	}
}

FP2T_APIFUNC fp2t_bxx_t FP2T_PolyLegalize(fp2t_t * PolyContext, fp2t_triangle_t * Triangle);

FP2T_APIFUNC void FP2T_PolyMapTriangleToNodes(fp2t_t * PolyContext, fp2t_triangle_t * Triangle);

FP2T_APIFUNC void FP2T_PolyFill(fp2t_t * PolyContext, fp2t_node_t * Node)
{
	fp2t_triangle_t *Triangle = FP2T_PushTriangle(PolyContext, Node->Prev->Point, Node->Point, Node->Next->Point);

	/* TODO: should copy the ConstrainedEdge Value from neighbor Triangles */
	/*       for now ConstrainedEdge values are copied during the legalize */
	/* static int count; */
	/* printf("POLY %i\n", count++); */
	FP2T_PolyMarkNeighborTri(Triangle, Node->Prev->Triangle);
	FP2T_PolyMarkNeighborTri(Triangle, Node->Triangle);

	/* Update the advancing front */
	Node->Prev->Next = Node->Next;
	Node->Next->Prev = Node->Prev;

	/* If it was legalized the Triangle has already been mapped */
	if (!FP2T_PolyLegalize(PolyContext, Triangle)) {
		FP2T_PolyMapTriangleToNodes(PolyContext, Triangle);
	}
}


FP2T_APIFUNC void FP2T_FillBasinReq(fp2t_t * PolyContext, fp2t_node_t * Node)
{
	/* if shallow stop filling */
	if (FP2T_PolyIsShallow(PolyContext, Node)) {
		return;
	}

	FP2T_PolyFill(PolyContext, Node);

	if (Node->Prev == PolyContext->Basin.LeftNode && Node->Next == PolyContext->Basin.RightNode) {
		return;
	}
	else if (Node->Prev == PolyContext->Basin.LeftNode) {
		fp2t_uxx_t Orientation = FP2T_PolyOrient2D(Node->Point, Node->Next->Point, Node->Next->Next->Point);
		if (Orientation == fp2t_orientation_CW) {
			return;
		}
		Node = Node->Next;
	}
	else if (Node->Next == PolyContext->Basin.RightNode) {
		fp2t_uxx_t Orientation = FP2T_PolyOrient2D(Node->Point, Node->Prev->Point, Node->Prev->Prev->Point);
		if (Orientation == fp2t_orientation_CCW) {
			return;
		}
		Node = Node->Prev;
	}
	else {
		/* Continue with the neighbor Node with lowest Y Value */
		if (Node->Prev->Point->Y < Node->Next->Point->Y) {
			Node = Node->Prev;
		}
		else {
			Node = Node->Next;
		}
	}

	FP2T_FillBasinReq(PolyContext, Node);
}

FP2T_APIFUNC void FP2T_FillBasin(fp2t_t * PolyContext, fp2t_node_t * Node)
{
	if (FP2T_PolyOrient2D(Node->Point, Node->Next->Point, Node->Next->Next->Point) == fp2t_orientation_CCW) {
		PolyContext->Basin.LeftNode = Node->Next->Next;
	}
	else {
		PolyContext->Basin.LeftNode = Node->Next;
	}

	/* Find the bottom and Right Node */
	PolyContext->Basin.BottomNode = PolyContext->Basin.LeftNode;
	while(PolyContext->Basin.BottomNode->Next && PolyContext->Basin.BottomNode->Point->Y >= PolyContext->Basin.BottomNode->Next->Point->Y) {
		PolyContext->Basin.BottomNode = PolyContext->Basin.BottomNode->Next;
	}
	if (PolyContext->Basin.BottomNode != PolyContext->Basin.LeftNode) {
		PolyContext->Basin.RightNode = PolyContext->Basin.BottomNode;
		while(PolyContext->Basin.RightNode->Next && PolyContext->Basin.RightNode->Point->Y < PolyContext->Basin.RightNode->Next->Point->Y) {
			PolyContext->Basin.RightNode = PolyContext->Basin.RightNode->Next;
		}
		if (PolyContext->Basin.RightNode != PolyContext->Basin.BottomNode) {
			PolyContext->Basin.Width = PolyContext->Basin.RightNode->Point->X - PolyContext->Basin.LeftNode->Point->X;
			PolyContext->Basin.LeftHighest = PolyContext->Basin.LeftNode->Point->Y > PolyContext->Basin.RightNode->Point->Y;

			FP2T_FillBasinReq(PolyContext, PolyContext->Basin.BottomNode);
		}
	}
}

FP2T_APIFUNC void FP2T_FillRightConcaveEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node);

FP2T_APIFUNC void FP2T_FillRightConvexEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node);

FP2T_APIFUNC void FP2T_FillLeftBelowEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node);

FP2T_APIFUNC void FP2T_FillLeftConcaveEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node);

FP2T_APIFUNC void FP2T_FillLeftConvexEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node);

FP2T_APIFUNC void FP2T_FillRightBelowEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node)
{
	if (Node->Point->X < Edge->P->X) {
		if (FP2T_PolyOrient2D(Node->Point, Node->Next->Point, Node->Next->Next->Point) == fp2t_orientation_CCW) {
			/* Concave */
			FP2T_FillRightConcaveEdgeEvent(PolyContext, Edge, Node);
		}
		else {
			/* Convex */
			FP2T_FillRightConvexEdgeEvent(PolyContext, Edge, Node);
			/* Retry this one */
			FP2T_FillRightBelowEdgeEvent(PolyContext, Edge, Node);
		}
	}
}

FP2T_APIFUNC void FP2T_FillRightAboveEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node)
{
	while(Node->Next->Point->X < Edge->P->X) {
		/* Check if Next Node is below the Edge */
		if (FP2T_PolyOrient2D(Edge->Q, Node->Next->Point, Edge->P) == fp2t_orientation_CCW) {
			FP2T_FillRightBelowEdgeEvent(PolyContext, Edge, Node);
		}
		else {
			Node = Node->Next;
		}
	}
}

FP2T_APIFUNC void FP2T_FillRightConcaveEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node)
{
	FP2T_PolyFill(PolyContext, Node->Next);
	if (Node->Next->Point != Edge->P) {
		/* Next above or below Edge? */
		if (FP2T_PolyOrient2D(Edge->Q, Node->Next->Point, Edge->P) == fp2t_orientation_CCW) {
			/* Below */
			if (FP2T_PolyOrient2D(Node->Point, Node->Next->Point, Node->Next->Next->Point) == fp2t_orientation_CCW) {
				/* Next is concave */
				FP2T_FillRightConcaveEdgeEvent(PolyContext, Edge, Node);
			}
			else {
				/* Next is convex */
			}
		}
	}
}

FP2T_APIFUNC void FP2T_FillRightConvexEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node)
{
	/* Next concave or convex? */
	if (FP2T_PolyOrient2D(Node->Next->Point, Node->Next->Next->Point, Node->Next->Next->Next->Point) == fp2t_orientation_CCW) {
		/* Concave */
		FP2T_FillRightConcaveEdgeEvent(PolyContext, Edge, Node->Next);
	}
	else {
		/* Convex */
		/* Next above or below Edge? */
		if (FP2T_PolyOrient2D(Edge->Q, Node->Next->Next->Point, Edge->P) == fp2t_orientation_CCW) {
			/* Below */
			FP2T_FillRightConvexEdgeEvent(PolyContext, Edge, Node->Next);
		}
		else {
			/* Above */
		}
	}
}

FP2T_APIFUNC void FP2T_FillLeftAboveEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node)
{
	while(Node->Prev->Point->X > Edge->P->X) {
		/* Check if Next Node is below the Edge */
		if (FP2T_PolyOrient2D(Edge->Q, Node->Prev->Point, Edge->P) == fp2t_orientation_CW) {
			FP2T_FillLeftBelowEdgeEvent(PolyContext, Edge, Node);
		}
		else {
			Node = Node->Prev;
		}
	}
}

FP2T_APIFUNC void FP2T_FillEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node)
{
	if (PolyContext->EdgeEvent.Right) {
		FP2T_FillRightAboveEdgeEvent(PolyContext, Edge, Node);
	}
	else {
		FP2T_FillLeftAboveEdgeEvent(PolyContext, Edge, Node);
	}
}

FP2T_APIFUNC void FP2T_FillLeftBelowEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node)
{
	if (Node->Point->X > Edge->P->X) {
		if (FP2T_PolyOrient2D(Node->Point, Node->Prev->Point, Node->Prev->Prev->Point) == fp2t_orientation_CW) {
			/* Concave */
			FP2T_FillLeftConcaveEdgeEvent(PolyContext, Edge, Node);
		}
		else {
			/* Convex */
			FP2T_FillLeftConvexEdgeEvent(PolyContext, Edge, Node);
			/* Retry this one */
			FP2T_FillLeftBelowEdgeEvent(PolyContext, Edge, Node);
		}
	}
}

FP2T_APIFUNC void FP2T_FillLeftConvexEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node)
{
	/* Next concave or convex? */
	if (FP2T_PolyOrient2D(Node->Prev->Point, Node->Prev->Prev->Point, Node->Prev->Prev->Prev->Point) == fp2t_orientation_CW) {
		/* Concave */
		FP2T_FillLeftConcaveEdgeEvent(PolyContext, Edge, Node->Prev);
	}
	else {
		/* Convex */
		/* Next above or below Edge? */
		if (FP2T_PolyOrient2D(Edge->Q, Node->Prev->Prev->Point, Edge->P) == fp2t_orientation_CW) {
			/* Below */
			FP2T_FillLeftConvexEdgeEvent(PolyContext, Edge, Node->Prev);
		}
		else {
			/* Above */
		}
	}
}

FP2T_APIFUNC void FP2T_FillLeftConcaveEdgeEvent(fp2t_t * PolyContext, fp2t_edge_t * Edge, fp2t_node_t * Node)
{
	FP2T_PolyFill(PolyContext, Node->Prev);
	if (Node->Prev->Point != Edge->P) {
		/* Next above or below Edge? */
		if (FP2T_PolyOrient2D(Edge->Q, Node->Prev->Point, Edge->P) == fp2t_orientation_CW) {
			/* Below */
			if (FP2T_PolyOrient2D(Node->Point, Node->Prev->Point, Node->Prev->Prev->Point) == fp2t_orientation_CW) {
				/* Next is concave */
				FP2T_FillLeftConcaveEdgeEvent(PolyContext, Edge, Node);
			}
			else {
				/* Next is convex */
			}
		}
	}
}

FP2T_APIFUNC
	fp2t_triangle_t * FP2T_NextFlipTriangle(fp2t_t * PolyContext, fp2t_uxx_t Orientation, fp2t_triangle_t * Triangle, fp2t_triangle_t * OtherTriangle, fp2t_point_t * Point, fp2t_i32_t PointIndex, fp2t_i32_t OtherPointIndex, fp2t_i32_t * NewPointIndex)
{
	fp2t_triangle_t *Result;
	FP2T_Assert(PointIndex >= 0 && OtherPointIndex >= 0);
	if (Orientation == fp2t_orientation_CCW) {
		/* OtherTriangle is not crossing Edge after flip */
		/* fp2t_i32_t EdgeIndex = FP2T_EdgeIndex(OtherTriangle, Point, OtherPoint); */
		fp2t_i32_t EdgeIndex = OtherPointIndex;
		/* FP2T_Assert(EdgeIndex == OtherPointIndex); */
		FP2T_Assert(EdgeIndex != -1);
		FP2T_PolyMarkDelaunayEdge(OtherTriangle, EdgeIndex);
		FP2T_PolyLegalize(PolyContext, OtherTriangle);
		FP2T_ClearDelunayEdges(OtherTriangle);
		Result = Triangle;
	}
	else {
		/* Triangle is not crossing Edge after flip */
		/* fp2t_i32_t EdgeIndex = PointIndex - OtherPointIndex; */
		/* if (EdgeIndex < 0) */
		/* { */
		/*   EdgeIndex = -EdgeIndex; */
		/* } */
		/* fp2t_i32_t EdgeIndex = FP2T_EdgeIndex(Triangle, Point, OtherPoint); */
		fp2t_i32_t EdgeIndex = PointIndex;
		FP2T_Assert(EdgeIndex == PointIndex);
		FP2T_Assert(EdgeIndex != -1);
		FP2T_PolyMarkDelaunayEdge(Triangle, EdgeIndex);
		FP2T_PolyLegalize(PolyContext, Triangle);
		FP2T_ClearDelunayEdges(Triangle);
		Result = OtherTriangle;
	}
	*NewPointIndex = FP2T_PolyPointIndex(Result, Point);

	/* FP2T_Assert(*NewPointIndex >= 0); */
	FP2T_Assert(Result->Points[*NewPointIndex] == Point);
	return Result;
}

FP2T_INLINE fp2t_i32_t FP2T_NextFlipPoint(fp2t_point_t * EdgeP, fp2t_point_t * EdgeQ, fp2t_point_t * OtherPoint, fp2t_i32_t OtherPointIndex)
{
	fp2t_uxx_t Orientation = FP2T_PolyOrient2D(EdgeQ, OtherPoint, EdgeP);
	fp2t_i32_t Result;
	FP2T_Assert(OtherPoint != EdgeP);
	if (Orientation == fp2t_orientation_CW) {
		Result = (fp2t_i32_t) fp2t_edge_tLUT[OtherPointIndex + 1];
	}
	else {
		FP2T_Assert(Orientation == fp2t_orientation_CCW);
		Result = (fp2t_i32_t) fp2t_edge_tLUT[OtherPointIndex + 2];
	}
	return Result;
}


FP2T_APIFUNC void FP2T_FlipScanEdgeEvent(fp2t_t * PolyContext, fp2t_point_t * EdgeP, fp2t_point_t * EdgeQ, fp2t_i32_t EdgeQIndex, fp2t_triangle_t * FlipTriangle, fp2t_triangle_t * Triangle, fp2t_i32_t PointIndex);

FP2T_APIFUNC void FP2T_EdgeEventPoints(fp2t_t * PolyContext, fp2t_point_t * EdgeP, fp2t_point_t * EdgeQ, fp2t_i32_t EdgeQIndex, fp2t_triangle_t * Triangle, fp2t_point_t * Point, fp2t_i32_t PointIndex);

FP2T_APIFUNC void FP2T_FlipEdgeEvent(fp2t_t * PolyContext, fp2t_point_t * EdgeP, fp2t_point_t * EdgeQ, fp2t_i32_t EdgeQIndex, fp2t_triangle_t * Triangle, fp2t_point_t * Point, fp2t_i32_t PointIndex)
{
	fp2t_triangle_t *OtherTriangle;
	fp2t_i32_t OtherPointIndex;
	fp2t_point_t *CWPoint;
	fp2t_point_t *CCWPoint;
	fp2t_point_t *OtherPoint;

	FP2T_Assert(PointIndex >= 0 && EdgeQIndex >= 0);
	FP2T_Assert(Triangle->Points[PointIndex] == Point);
	FP2T_Assert(OtherTriangle);

	OtherTriangle = FP2T_PolyNeighborAcross(Triangle, Point);
	CWPoint = Triangle->Points[fp2t_edge_tLUT[PointIndex + 2]];
	CCWPoint = Triangle->Points[fp2t_edge_tLUT[PointIndex + 1]];
	OtherPoint = FP2T_PointCW(OtherTriangle, CWPoint, &OtherPointIndex);

	if (FP2T_PolyInScanArea(Point, CCWPoint, CWPoint, OtherPoint)) {
		/* Lets rotate shared Edge one vertex CW */
		FP2T_PolyRotateTrianglePair(Triangle, Point, PointIndex, OtherTriangle, OtherPoint, OtherPointIndex);
		FP2T_PolyMapTriangleToNodes(PolyContext, Triangle);
		FP2T_PolyMapTriangleToNodes(PolyContext, OtherTriangle);

		if (Point == EdgeQ && OtherPoint == EdgeP) {
			if (EdgeQ == PolyContext->EdgeEvent.ConstrainedEdge->Q && EdgeP == PolyContext->EdgeEvent.ConstrainedEdge->P) {
				FP2T_PolyMarkConstrainedEdgePoints(Triangle, EdgeP, EdgeQ);
				FP2T_PolyMarkConstrainedEdgePoints(OtherTriangle, EdgeP, EdgeQ);
				FP2T_PolyLegalize(PolyContext, Triangle);
				FP2T_PolyLegalize(PolyContext, OtherTriangle);
			}
			else {
				/* XXX: I think one of the Triangles should be legalized here? */
			}
		}
		else {
			FP2T_Assert(OtherPoint != EdgeP && OtherPoint != EdgeQ && OtherPoint != Point);
			{
				fp2t_uxx_t Orientation = FP2T_PolyOrient2D(EdgeQ, OtherPoint, EdgeP);
				fp2t_i32_t NewPointIndex;
				Triangle = FP2T_NextFlipTriangle(PolyContext, Orientation, Triangle, OtherTriangle, Point, PointIndex, OtherPointIndex, &NewPointIndex);
				FP2T_FlipEdgeEvent(PolyContext, EdgeP, EdgeQ, EdgeQIndex, Triangle, Point, NewPointIndex);
			}
		}
	}
	else {
		/* FP2T_BREAK; */
		FP2T_Assert(OtherPoint != EdgeP);
		{
			fp2t_i32_t NewPointIndex = FP2T_NextFlipPoint(EdgeP, EdgeQ, OtherPoint, OtherPointIndex);
			EdgeQIndex = FP2T_PolyPointIndex(Triangle, EdgeQ);
			FP2T_FlipScanEdgeEvent(PolyContext, EdgeP, EdgeQ, EdgeQIndex, Triangle, OtherTriangle, NewPointIndex);
			PointIndex = FP2T_PolyPointIndex(Triangle, Point);
			FP2T_EdgeEventPoints(PolyContext, EdgeP, EdgeQ, EdgeQIndex, Triangle, Point, PointIndex);
		}
	}
}

FP2T_APIFUNC void FP2T_FlipScanEdgeEvent(fp2t_t * PolyContext, fp2t_point_t * EdgeP, fp2t_point_t * EdgeQ, fp2t_i32_t EdgeQIndex, fp2t_triangle_t * FlipTriangle, fp2t_triangle_t * Triangle, fp2t_i32_t PointIndex)
{
	fp2t_triangle_t *OtherTriangle = Triangle->Neighbors[PointIndex];
	fp2t_i32_t OtherPointIndex;
	fp2t_point_t *CWPoint = Triangle->Points[fp2t_edge_tLUT[PointIndex + 2]];
	/* fp2t_point_t* CCWPoint = Triangle->Points[fp2t_edge_tLUT[PointIndex+1]]; */
	fp2t_point_t *FlipCWPoint = FlipTriangle->Points[fp2t_edge_tLUT[EdgeQIndex + 2]];
	fp2t_point_t *FlipCCWPoint = FlipTriangle->Points[fp2t_edge_tLUT[EdgeQIndex + 1]];
	fp2t_point_t *OtherPoint = FP2T_PointCW(OtherTriangle, CWPoint, &OtherPointIndex);

	FP2T_Assert(PointIndex >= 0 && EdgeQIndex >= 0);
	FP2T_Assert(EdgeQ == FlipTriangle->Points[EdgeQIndex]);

	if (FP2T_PolyInScanArea(EdgeQ, FlipCCWPoint, FlipCWPoint, OtherPoint)) {
		/* flip with new edge OtherPoint->EdgeQ */
		FP2T_FlipEdgeEvent(PolyContext, EdgeQ, OtherPoint, PointIndex, OtherTriangle, OtherPoint, OtherPointIndex);
		/* TODO: Actually I just figured out that it should be possible to */
		/*       improve this by getting the Next OtherTriangle and op before the the above */
		/*       flip and continue the flipScanEdgeEvent here */
		/* set new OtherTriangle and op here and fp2t_uxx_t back to inScanArea test */
		/* also need to set a new FlipTriangle first */
		/* Turns out at first glance that this is somewhat complicated */
		/* so it will have to wait. */
	}
	else {
		fp2t_i32_t NewPointIndex = FP2T_NextFlipPoint(EdgeP, EdgeQ, OtherPoint, OtherPointIndex);
		FP2T_FlipScanEdgeEvent(PolyContext, EdgeP, EdgeQ, EdgeQIndex, FlipTriangle, OtherTriangle, NewPointIndex);
	}
}

FP2T_APIFUNC void FP2T_PolyMapTriangleToNodes(fp2t_t * PolyContext, fp2t_triangle_t * Triangle)
{
	fp2t_uxx_t TriangleIndex;
	for(TriangleIndex = 0; TriangleIndex < 3; ++TriangleIndex) {
		if (!Triangle->Neighbors[TriangleIndex]) {
			fp2t_point_t *NextPoint = Triangle->Points[fp2t_edge_tLUT[TriangleIndex + 2]];
			fp2t_node_t *Node = FP2T_LocatePoint(PolyContext, NextPoint);
			if (Node) {
				Node->Triangle = Triangle;
			}
		}
	}
}

FP2T_APIFUNC void FP2T_PolyFinalizeTriangles(fp2t_t * PolyContext, fp2t_triangle_t * StartTriangle)
{
	fp2t_u32_t TriangleCount = 0;
	PolyContext->Triangles = FP2T_PolyPush(PolyContext->Allocator, fp2t_triangle_t *);

	PolyContext->TempTriangles[TriangleCount++] = StartTriangle;
	while(TriangleCount) {
		fp2t_triangle_t *TempTriangle = PolyContext->TempTriangles[--TriangleCount];
		if (!(TempTriangle->Flags & fp2t_tri_flag_IsInterior)) {
			fp2t_uxx_t NeighborIndex;
			TempTriangle->Flags |= fp2t_tri_flag_IsInterior;
			PolyContext->Triangles[PolyContext->TriangleCount++] = TempTriangle;

			for(NeighborIndex = 0; NeighborIndex < 3; ++NeighborIndex) {
				fp2t_uxx_t BitMask = (fp2t_uxx_t) fp2t_tri_flag_ConstrainedEdge0 << NeighborIndex;
				if (!(TempTriangle->Flags & BitMask)) {
					if (TempTriangle->Neighbors[NeighborIndex]) {
						PolyContext->TempTriangles[TriangleCount++] = TempTriangle->Neighbors[NeighborIndex];
					}
				}
			}
		}
	}
}

FP2T_APIFUNC fp2t_bxx_t FP2T_PolyLegalize(fp2t_t * PolyContext, fp2t_triangle_t * Triangle)
{
	fp2t_bxx_t Result = FP2T_FALSE;
	fp2t_u32_t EdgeIndex;
	/* To legalize a Triangle we start by finding if any of the three edges */
	/* violate the Delaunay condition */
	for(EdgeIndex = 0; EdgeIndex < 3; ++EdgeIndex) {
		fp2t_triangle_t *OtherTriangle;

		if (Triangle->Flags & ((fp2t_uxx_t) fp2t_tri_flag_DelaunayEdge0 << EdgeIndex)) {
			continue;
		}

		OtherTriangle = Triangle->Neighbors[EdgeIndex];

		if (OtherTriangle) {
			fp2t_point_t *CCWPoint;
			fp2t_bxx_t IsInside;
			fp2t_point_t *Point = Triangle->Points[EdgeIndex];
			fp2t_i32_t OtherPointIndex;
			fp2t_point_t *CWPoint = Triangle->Points[fp2t_edge_tLUT[EdgeIndex + 2]];
			fp2t_point_t *OtherPoint = FP2T_PointCW(OtherTriangle, CWPoint, &OtherPointIndex);
			/* If this is a Constrained Edge or a Delaunay Edge(only during recursive */
			/* legalization) */
			/* then we should not try to legalize */
			fp2t_uxx_t CombinedMask = (((fp2t_uxx_t) fp2t_tri_flag_ConstrainedEdge0 << OtherPointIndex) | ((fp2t_uxx_t) fp2t_tri_flag_DelaunayEdge0 << OtherPointIndex));
			if (OtherTriangle->Flags & CombinedMask) {
				fp2t_uxx_t Mask = (fp2t_uxx_t) fp2t_tri_flag_ConstrainedEdge0 << EdgeIndex;
				fp2t_uxx_t NewFlag = (OtherTriangle->Flags & ((fp2t_uxx_t) fp2t_tri_flag_ConstrainedEdge0 << OtherPointIndex)) ? Mask : 0;
				Triangle->Flags = (Triangle->Flags & ~Mask) | NewFlag;
				continue;
			}

			CCWPoint = Triangle->Points[fp2t_edge_tLUT[EdgeIndex + 1]];
			IsInside = FP2T_PolyInCircle(Point, CCWPoint, CWPoint, OtherPoint);
			if (IsInside) {
				fp2t_bxx_t NotLegalized;

				/* Lets mark this shared edge as Delaunay */
				Triangle->Flags |= (fp2t_uxx_t) fp2t_tri_flag_DelaunayEdge0 << EdgeIndex;
				OtherTriangle->Flags |= (fp2t_uxx_t) fp2t_tri_flag_DelaunayEdge0 << OtherPointIndex;

				/* Lets rotate shared edge one vertex CW to legalize it */
				FP2T_PolyRotateTrianglePair(Triangle, Point, (fp2t_i32_t) EdgeIndex, OtherTriangle, OtherPoint, OtherPointIndex);

				/* We now got one valid Delaunay Edge shared by two Triangles */
				/* This gives us 4 new edges to check for Delaunay */

				/* Make sure that Triangle to Node mapping is done only one time for a */
				/* specific Triangle */
				NotLegalized = !FP2T_PolyLegalize(PolyContext, Triangle);
				if (NotLegalized) {
					FP2T_PolyMapTriangleToNodes(PolyContext, Triangle);
				}

				NotLegalized = !FP2T_PolyLegalize(PolyContext, OtherTriangle);
				if (NotLegalized) {
					FP2T_PolyMapTriangleToNodes(PolyContext, OtherTriangle);
				}

				/* Reset the Delaunay edges, since they only are valid Delaunay edges */
				/* until we add a new Triangle or fp2t_point_t. */
				/* XXX: need to think about this. Can these edges be tried after we */
				/*      return to previous recursive level? */
				Triangle->Flags &= ~(fp2t_uxx_t) fp2t_tri_flag_DelaunayEdge0 << EdgeIndex;
				OtherTriangle->Flags &= ~(fp2t_uxx_t) fp2t_tri_flag_DelaunayEdge0 << OtherPointIndex;

				/* If Triangle have been legalized no need to check the other edges */
				/* since */
				/* the recursive legalization will handles those so we can end here. */
				Result = FP2T_TRUE;
				break;
			}
		}
	}
	return Result;
}

FP2T_APIFUNC void FP2T_EdgeEventPoints(fp2t_t * PolyContext, fp2t_point_t * EdgeP, fp2t_point_t * EdgeQ, fp2t_i32_t EdgeQIndex, fp2t_triangle_t * Triangle, fp2t_point_t * Point, fp2t_i32_t PointIndex)
{
	fp2t_i32_t Point1Index;
	fp2t_point_t *Point1;
	fp2t_uxx_t Orientation1;
	fp2t_i32_t Point2Index;
	fp2t_point_t *Point2;
	fp2t_uxx_t Orientation2;

	if (FP2T_IsEdgeSideOfTriangle(Triangle, EdgeP, EdgeQ)) {
		return;
	}
	Point1Index = (fp2t_i32_t) fp2t_edge_tLUT[PointIndex + 1];
	Point1 = Triangle->Points[Point1Index];
	Orientation1 = FP2T_PolyOrient2D(EdgeQ, Point1, EdgeP);

	FP2T_Assert(PointIndex >= 0 && EdgeQIndex >= 0);
	FP2T_Assert(Triangle->Points[PointIndex] == Point);

	if (Orientation1 == fp2t_orientation_Collinear) {
		if (FP2T_PolyContainsPoints(Triangle, EdgeQ, Point1)) {
			FP2T_PolyMarkConstrainedEdgePoints(Triangle, EdgeQ, Point1);
			/* We are modifying the maybe it would be better to */
			/* not change the given and just keep a variable for the new */
			 /**/ PolyContext->EdgeEvent.ConstrainedEdge->Q = Point1;
			Triangle = FP2T_PolyNeighborAcross(Triangle, Point);
			FP2T_EdgeEventPoints(PolyContext, EdgeP, Point1, Point1Index, Triangle, Point1, Point1Index);
		}
		else {
			FP2T_InvalidCodePath;
		}
		return;
	}

	Point2Index = (fp2t_i32_t) fp2t_edge_tLUT[PointIndex + 2];
	Point2 = Triangle->Points[Point2Index];
	Orientation2 = FP2T_PolyOrient2D(EdgeQ, Point2, EdgeP);

	if (Orientation2 == fp2t_orientation_Collinear) {
		if (FP2T_PolyContainsPoints(Triangle, EdgeQ, Point2)) {
			FP2T_PolyMarkConstrainedEdgePoints(Triangle, EdgeQ, Point2);
			/* We are modifying the maybe it would be better to */
			/* not change the given and just keep a variable for the new */
			 /**/ PolyContext->EdgeEvent.ConstrainedEdge->Q = Point2;
			Triangle = FP2T_PolyNeighborAcross(Triangle, Point);
			FP2T_EdgeEventPoints(PolyContext, EdgeP, Point2, Point2Index, Triangle, Point2, Point2Index);
		}
		else {
			FP2T_InvalidCodePath;
		}
		return;
	}

	if (Orientation1 == Orientation2) {
		/* Need to decide if we are rotating CW or CCW to get to a Triangle */
		/* that will cross edge */
		fp2t_triangle_t *NewTriangle;
		fp2t_i32_t NewPointIndex = (fp2t_i32_t) fp2t_edge_tLUT[PointIndex + (fp2t_i32_t) Orientation1 + 1];
		NewTriangle = Triangle->Neighbors[NewPointIndex];
		FP2T_Assert(NewTriangle);
		NewPointIndex = FP2T_PolyPointIndex(NewTriangle, Point);
		FP2T_Assert(NewPointIndex == FP2T_PolyPointIndex(NewTriangle, Point));
		FP2T_EdgeEventPoints(PolyContext, EdgeP, EdgeQ, EdgeQIndex, NewTriangle, Point, NewPointIndex);
	}
	else {
		/* This Triangle crosses so lets flippin start! */
		FP2T_FlipEdgeEvent(PolyContext, EdgeP, EdgeQ, EdgeQIndex, Triangle, Point, PointIndex);
	}
}

FP2T_APIFUNC void fp2t_add_edge(fp2t_t * PolyContext)
{
	fp2t_point_t *Points = PolyContext->PointsPool + PolyContext->PointCount;
	fp2t_u32_t PointCount = PolyContext->PointPoolCount - PolyContext->PointCount;
	fp2t_uxx_t PointIndex;
	FP2T_Assert(PointCount && PointCount < PolyContext->MaxPointCount);
	for(PointIndex = 0; PointIndex < PointCount; ++PointIndex) {
		fp2t_edge_t *Edge;
		fp2t_uxx_t NextPointIndex = PointIndex < PointCount - 1 ? PointIndex + 1 : 0;
		fp2t_point_t *A = Points + PointIndex;
		fp2t_point_t *B = Points + NextPointIndex;
		PolyContext->Points[PolyContext->PointCount++] = A;
		Edge = FP2T_PolyPush(PolyContext->Allocator, fp2t_edge_t);

		if ((A->Y > B->Y) || ((B->Y - A->Y) < FP2T_POLY2TRI_EPSILON && (A->X > B->X))) {
			Edge->Q = A;
			Edge->P = B;
		}
		else {
			Edge->P = A;
			Edge->Q = B;
		}

		FP2T_Assert(fabs(A->Y - B->Y) > FP2T_POLY2TRI_EPSILON || fabs(A->X - B->X) > FP2T_POLY2TRI_EPSILON);

		if (Edge->Q->FirstEdge) {
			fp2t_edge_t *LastEdge = Edge->Q->FirstEdge;
			while(LastEdge && LastEdge->Next) {
				LastEdge = LastEdge->Next;
			}
			LastEdge->Next = Edge;
		}
		else {
			Edge->Q->FirstEdge = Edge;
		}
	}
}


FP2T_APIFUNC void fp2t_add_hole(fp2t_t * PolyContext)
{
	fp2t_add_edge(PolyContext);
}

FP2T_APIFUNC void fp2t_add_points(fp2t_t * PolyContext, fp2t_point_t * Points, fp2t_u32_t PointCount)
{
	fp2t_uxx_t PointIndex;
	for(PointIndex = 0; PointIndex < PointCount; ++PointIndex) {
		fp2t_point_t *Point = Points + PointIndex;
		PolyContext->Points[PolyContext->PointCount++] = Point;
	}
}


/*SECTION: Sort*/

#define FP2T_POLY_COMPARE(A, B) \
  (((A)->Y < (B)->Y) || (((A)->Y - (B)->Y) < FP2T_POLY2TRI_EPSILON && ((A)->X < (B)->X)))

#ifdef FP2T_POLY2TRI_USE_CUSTOM_SORT
FP2T_APIFUNC void FP2T_PolySort(fp2t_point_t ** Points, fp2t_point_t ** Temp, fp2t_u32_t Count)
{
	FP2T_Assert(Count);
	if (Count <= 1) {

	}
	else if (Count == 2) {
		fp2t_point_t *A = Points[0];
		fp2t_point_t *B = Points[1];
		if ((A->Y > B->Y) || ((B->Y - A->Y) < FP2T_POLY2TRI_EPSILON && A->X >= B->X)) {
			fp2t_point_t *Swap = A;
			Points[0] = B;
			Points[1] = Swap;
		}
	}
	else {
		fp2t_u32_t Half0 = Count / 2;
		fp2t_u32_t Half1 = Count - Half0;

		fp2t_point_t **InHalf0 = Points;
		fp2t_point_t **InHalf1 = Points + Half0;
		fp2t_point_t **End = Points + Count;

		FP2T_PolySort(InHalf0, Temp, Half0);
		FP2T_PolySort(InHalf1, Temp, Half1);

		fp2t_point_t **ReadHalf0 = InHalf0;
		fp2t_point_t **ReadHalf1 = InHalf1;

		/*NOTE: Don't merge if two halves are already sorted */
		if (InHalf1[-1]->Y > InHalf1[0]->Y) {
			fp2t_point_t **Out = Temp;
			fp2t_u32_t WriteIndex;
			for(WriteIndex = 0; WriteIndex < Count; ++WriteIndex) {
				if (ReadHalf0 == InHalf1) {
					*Out++ = *ReadHalf1++;
				}
				else if (ReadHalf1 == End) {
					*Out++ = *ReadHalf0++;
				}
				else {
					fp2t_point_t *A = ReadHalf0[0];
					fp2t_point_t *B = ReadHalf1[0];
					if (FP2T_POLY_COMPARE(A, B)) {
						*Out++ = *ReadHalf0++;
					}
					else {
						*Out++ = *ReadHalf1++;
					}
				}
			}
			FP2T_MemoryCopy(Points, Temp, sizeof(*Points) * Count);
		}
	}
}
#else
FP2T_APIFUNC fp2t_i32_t FP2T_PolySortCompare(fp2t_point_t ** A, fp2t_point_t ** B)
{
	fp2t_i32_t Result;
	if ((*A)->Y < (*B)->Y) {
		Result = -1;
	}
	else if (((*A)->Y - (*B)->Y) < FP2T_POLY2TRI_EPSILON) {
		if ((*A)->X < (*B)->X) {
			Result = -1;
		}
		else {
			Result = 1;
		}
	}
	else {
		Result = 1;
	}
	return Result;
}
#endif

FP2T_APIFUNC void fp2t_triangulate(fp2t_t * PolyContext)
{
	fp2t_u32_t PointCount = PolyContext->PointCount;
	FP2T_Assert(PolyContext->PointCount);
	/*NOTE: Init triangulation */
	{
		fp2t_float_t Xmax = PolyContext->Points[0]->X;
		fp2t_float_t Xmin = PolyContext->Points[0]->X;
		fp2t_float_t Ymax = PolyContext->Points[0]->Y;
		fp2t_float_t Ymin = PolyContext->Points[0]->Y;
		fp2t_uxx_t PointIndex = 0;
		/*TODO: Calculate min and max during sort */
		for(PointIndex = 0; PointIndex < PolyContext->PointCount; ++PointIndex) {
			fp2t_point_t *Point = PolyContext->Points[PointIndex];
			if (Point->X > Xmax) {
				Xmax = Point->X;
			}
			if (Point->X < Xmin) {
				Xmin = Point->X;
			}
			if (Point->Y > Ymax) {
				Ymax = Point->Y;
			}
			if (Point->Y < Ymin) {
				Ymin = Point->Y;
			}
		}
#define FP2T_INITIAL_TRIANGLE_FACTOR  (fp2t_float_t)0.3f
		{
			fp2t_point_t *Tail;
			fp2t_float_t Dx = FP2T_INITIAL_TRIANGLE_FACTOR * (Xmax - Xmin);
			fp2t_float_t Dy = FP2T_INITIAL_TRIANGLE_FACTOR * (Ymax - Ymin);
			fp2t_point_t *Head = FP2T_PolyPush(PolyContext->Allocator, fp2t_point_t);
			Head->X = Xmax + Dx;
			Head->Y = Ymin - Dy;
			Tail = FP2T_PolyPush(PolyContext->Allocator, fp2t_point_t);
			Tail->X = Xmin - Dx;
			Tail->Y = Ymin - Dy;
			PolyContext->HeadPoint = Head;
			PolyContext->TailPoint = Tail;
		}
	}

	/*NOTE: Sort points */
	{
#ifdef FP2T_POLY2TRI_USE_CUSTOM_SORT
		fp2t_umm_t BeforeRollback = PolyContext->Allocator.Used;
		fp2t_point_t **Temp = FP2T_PolyPushArray(PolyContext->Allocator, fp2t_point_t *, PointCount);
		FP2T_PolySort(PolyContext->Points, Temp, PointCount);
		FP2T_MemorySet(PolyContext->Allocator.Memory + BeforeRollback, 0, PolyContext->Allocator.Used - BeforeRollback);
		PolyContext->Allocator.Used = BeforeRollback;
#else
		qsort(PolyContext->Points, PointCount, sizeof(fp2t_point_t *), (int (*)(const void *, const void *))FP2T_PolySortCompare);
#endif
	}

	/*NOTE: Create advancing front */
	{
		fp2t_triangle_t *Triangle = FP2T_PushTriangle(PolyContext, PolyContext->Points[0], PolyContext->TailPoint, PolyContext->HeadPoint);

		fp2t_node_t *Head = FP2T_PolyNode(PolyContext, Triangle->Points[1], Triangle);
		fp2t_node_t *Middle = FP2T_PolyNode(PolyContext, Triangle->Points[0], Triangle);
		fp2t_node_t *Tail = FP2T_PolyNode(PolyContext, Triangle->Points[2], 0);

		PolyContext->HeadNode = PolyContext->SearchNode = Head;
		PolyContext->TailNode = Tail;
		Head->Next = Middle;
		Middle->Next = Tail;
		Middle->Prev = Head;
		Tail->Prev = Middle;
	}

	/*NOTE: Sweep points */
	{
		fp2t_uxx_t PointIndex;
		fp2t_edge_t *Edge;
		for(PointIndex = 1; PointIndex < PolyContext->PointCount; ++PointIndex) {
			fp2t_point_t *Point = PolyContext->Points[PointIndex];
			fp2t_node_t *Node;
			fp2t_node_t *NewNode;
			{
				Node = PolyContext->SearchNode;
				if (Point->X < Node->Value) {
					while((Node = Node->Prev)) {
						if (Point->X >= Node->Value) {
							PolyContext->SearchNode = Node;
							break;
						}
					}
				}
				else {
					while((Node = Node->Next)) {
						if (Point->X < Node->Value) {
							PolyContext->SearchNode = Node->Prev;
							Node = Node->Prev;
							break;
						}
					}
				}
			}

			/*NOTE: New front triangle */
			{
				fp2t_triangle_t *NewTriangle = FP2T_PushTriangle(PolyContext, Point, Node->Point, Node->Next->Point);
				FP2T_PolyMarkNeighborTri(NewTriangle, Node->Triangle);

				NewNode = FP2T_PolyNode(PolyContext, Point, 0);
				NewNode->Next = Node->Next;
				NewNode->Prev = Node;
				Node->Next->Prev = NewNode;
				Node->Next = NewNode;

				if (!FP2T_PolyLegalize(PolyContext, NewTriangle)) {
					FP2T_PolyMapTriangleToNodes(PolyContext, NewTriangle);
				}
				FP2T_Assert(NewNode->Triangle == NewTriangle);

				/* Only need to check +epsilon since fp2t_point_t never have smaller */
				/* X Value than Node due to how we fetch nodes from the front */
				if (Point->X <= Node->Point->X + FP2T_POLY2TRI_EPSILON) {
					FP2T_PolyFill(PolyContext, Node);
				}

				/*NOTE: FillAdvancingFront */
				{
					/* Fill Right holes */
					fp2t_node_t *NextNode = NewNode->Next;

					while(NextNode->Next) {
						/* if HoleAngle exceeds 90 degrees then break. */
						if (FP2T_LargeHole_DontFill(NextNode)) {
							break;
						}
						FP2T_PolyFill(PolyContext, NextNode);
						NextNode = NextNode->Next;
					}


					/* Fill left holes */
					NextNode = NewNode->Prev;

					while(NextNode->Prev) {
						/* if HoleAngle exceeds 90 degrees then break. */
						if (FP2T_LargeHole_DontFill(NextNode)) {
							break;
						}
						FP2T_PolyFill(PolyContext, NextNode);
						NextNode = NextNode->Prev;
					}

					/* Fill Right basins */
					if (NewNode->Next && NewNode->Next->Next) {
						if (FP2T_PolyShouldFillBasin(NewNode)) {
							FP2T_FillBasin(PolyContext, NewNode);
						}
					}
				}
			}
			for(Edge = Point->FirstEdge; Edge; Edge = Edge->Next) {
				/*NOTE: Edge event */
				PolyContext->EdgeEvent.ConstrainedEdge = Edge;
				PolyContext->EdgeEvent.Right = (Edge->P->X > Edge->Q->X);

				if (!FP2T_IsEdgeSideOfTriangle(NewNode->Triangle, Edge->P, Edge->Q)) {
					fp2t_i32_t EdgeQIndex;
					/* For now we will do all needed filling */
					/* TODO: integrate with flip process might give some better performance */
					/*       but for now this avoid the issue with cases that needs both flips and */
					/*       fills */
					FP2T_FillEdgeEvent(PolyContext, Edge, NewNode);
					EdgeQIndex = FP2T_PolyPointIndex(NewNode->Triangle, Edge->Q);
					FP2T_EdgeEventPoints(PolyContext, Edge->P, Edge->Q, EdgeQIndex, NewNode->Triangle, Edge->Q, EdgeQIndex);
				}
			}
		}
	}

	/*NOTE: Finalize polygon */
	{
		fp2t_triangle_t *Triangle = PolyContext->HeadNode->Next->Triangle;
		fp2t_point_t *Point = PolyContext->HeadNode->Next->Point;
		while(!FP2T_GetConstrainedEdgeCW(Triangle, Point)) {
			Triangle = FP2T_NeighborCCW(Triangle, Point);
		}
		FP2T_PolyFinalizeTriangles(PolyContext, Triangle);
	}
}

FP2T_APIFUNC fp2t_b32_t fp2t_init(fp2t_t * PolyContext, void *Memory, fp2t_u32_t MaxPointCount)
{
	if (Memory && MaxPointCount) {
		fp2t_umm_t AlignmentOffset;
		fp2t_u8_t *MemoryBytes;
		fp2t_umm_t MemoryAddress;

		fp2t_umm_t MemorySize = fp2t_memory_required(MaxPointCount);
		fp2t_u32_t MaxTrianglePoolCount;
		MaxPointCount = MaxPointCount + FP2T_POLY2TRI_POINT_COUNT_EPSILON;
		MaxTrianglePoolCount = MaxPointCount * 2;

		FP2T_MemorySet(PolyContext, 0, sizeof(*PolyContext));
		/* Align to 8 bytes */
		AlignmentOffset = 0;
		MemoryBytes = (fp2t_u8_t *) Memory;
		MemoryAddress = (fp2t_umm_t) (MemoryBytes);
		if (MemoryAddress & 0x7) {
			AlignmentOffset = (0x8) - (MemoryAddress & 0x7);
		}
		FP2T_Assert(AlignmentOffset == 0);
		PolyContext->Memory = Memory;
		PolyContext->Allocator.Size = MemorySize - AlignmentOffset;
		PolyContext->Allocator.Memory = (fp2t_u8_t *) Memory + AlignmentOffset;

		PolyContext->MaxPointCount = MaxPointCount;
		PolyContext->PointsPool = FP2T_PolyPushArray(PolyContext->Allocator, fp2t_point_t, MaxPointCount);
		PolyContext->Points = FP2T_PolyPushArray(PolyContext->Allocator, fp2t_point_t *, MaxPointCount);
		PolyContext->TrianglePool = FP2T_PolyPushArray(PolyContext->Allocator, fp2t_triangle_t, MaxTrianglePoolCount);
		PolyContext->TempTriangles = FP2T_PolyPushArray(PolyContext->Allocator, fp2t_triangle_t *, MaxTrianglePoolCount);
		PolyContext->Nodes = FP2T_PolyPushArray(PolyContext->Allocator, fp2t_node_t, MaxPointCount);
		PolyContext->Valid = FP2T_TRUE;
	}
	else {
		PolyContext->Valid = FP2T_FALSE;
	}

	return PolyContext->Valid;
}

FP2T_APIFUNC fp2t_umm_t fp2t_memory_required(fp2t_u32_t MaxPointCount)
{
	fp2t_umm_t Result;
	fp2t_umm_t EstimatedSizePerPoint;
	MaxPointCount += FP2T_POLY2TRI_POINT_COUNT_EPSILON;
	EstimatedSizePerPoint = sizeof(fp2t_edge_t) + sizeof(fp2t_point_t) + sizeof(fp2t_point_t *) + sizeof(fp2t_point_t *) + sizeof(fp2t_node_t) + sizeof(fp2t_triangle_t) * 2 + sizeof(fp2t_triangle_t *) * 2 + sizeof(fp2t_triangle_t *) * 2;
	Result = EstimatedSizePerPoint * MaxPointCount + 0x8;
	return Result;
}

#endif /* FP2T_POLY2TRI_IMPLEMENTATION */
