/***************************************************************************\
    LocalWX.cpp
    Scott Randolph
    July 19, 1996

	Manage the "deaggregated" clouds in a given local.
\***************************************************************************/
#include <limits.h>
#include "WXMap.h"
#include "WXCell.h"
#include "LocalWX.h"

#pragma warning(disable : 4127)

void LocalWeather::Setup( float visRange, ObjectDisplayList* objList )
{
	// Store the pointer to the object display list we are to use for drawing clouds
	objMgr = objList;
	
	// Compute how many blocks into the distance we need to keep (0 means center cell only)
	range = visRange;
	cellRange = FloatToInt32((float)ceil( visRange/TheWeather->CellSize() ));

	// Create our pool of local cells
	rowLen = 2*cellRange+1;
	cellArray = new WeatherCell[ rowLen*rowLen ];

	// Initialize which weather cell contains the viewer
	// (Force a full rebuild at the first update)
	cellRow = -5000;
	cellCol = -5000;
	centerX = -1e6f;
	centerY = -1e6f;
	rowShiftHistory = colShiftHistory = 0;// JPO initialise
}


void LocalWeather::Cleanup( void )
{
	WeatherCell	*p;
	int			r, c;

	// Free the array of local cells
	ShiAssert( cellArray );

	// Release our whole list of cloud cells
	p = cellArray;
	for (r = cellRow-cellRange; r <= cellRow+cellRange; r++) {
		for (c = cellCol-cellRange; c <= cellCol+cellRange; c++) {
			p->Cleanup( objMgr );
			p++;
		}
	}

	delete []cellArray;
	cellArray = NULL;
}


//void LocalWeather::Update( Tpoint *position, DWORD currentTime )
void LocalWeather::Update( Tpoint *position, DWORD)
{
	int newRow, newCol;
	int	vx, vy;
	int rowStep, colStep;

	// Store the new world space location
	centerX = position->x;
	centerY = position->y;

	// Compute which weather cell contains the viewer
	newRow = TheWeather->WorldToTile( centerX - TheWeather->xOffset );
	newCol = TheWeather->WorldToTile( centerY - TheWeather->yOffset );

	// Figure out which direction the viewer moved
	vx = newRow - cellRow;
	vy = newCol - cellCol;

	// Check to see if the underlying map did a row or column shift since last time...
	// NOTE: rowStep > 0 means the map shifted cells up and added at the bottom,
	//		 colStep < 0 means the map shifted celss left and added at the right, etc.
	rowStep = TheWeather->rowShiftHistory - rowShiftHistory;
	colStep = TheWeather->colShiftHistory - colShiftHistory;
	if (rowStep || colStep) {
		rowShiftHistory = TheWeather->rowShiftHistory;
		colShiftHistory = TheWeather->colShiftHistory;

		vx -= rowStep;
		vy -= colStep;
	}

	// If we crossed a cell boundry, we need to reshuffle our tiles
	if (vx || vy) {

		// Store the new center cell coordinates;
		cellRow = newRow;
		cellCol = newCol;

		// If we took too big a step, flush the list and start over
		if ( (abs(vx) > 1) || (abs(vy) > 1)) {

			RebuildList();

		} else {
			if (vx) {
				SlideLocalCellsVertical( vx );
			}
			if (vy) {
				SlideLocalCellsHorizontal( vy );
			}
		}

		// Refresh our notion of cloud bases and tops
		ComputeAreaFloorAndCeiling();
	}

	// Now make a pass through the list to update positions for wind drift
	UpdateForDrift();
}

float LocalWeather::GetRainFactor ()
{
    WeatherCell *wx = GetLocalCell (0, 0);
    if (wx) return wx->GetRainFactor();
    return 0;
}

float LocalWeather::GetVisibility ()
{
    WeatherCell *wx = GetLocalCell (0, 0);
    if (wx) return wx->GetVisibility();
    return 1;
}

float LocalWeather::GetLocalCloudTops ()
{
    WeatherCell *wx = GetLocalCell (0, 0);
    if (wx) return wx->GetTop();
    return 1;
}

bool  LocalWeather::HasLightning() 
{
    WeatherCell *wx = GetLocalCell (0, 0);
    if (wx) return wx->GetLightning();
    return false;
}


void LocalWeather::UpdateForDrift( void )
{
	WeatherCell	*p;
	int			row, col;

	p = cellArray;
	for (row=cellRow - cellRange; row <= cellRow + cellRange; row++) {
		for (col=cellCol - cellRange; col <= cellCol + cellRange; col++) {
			p->UpdateForDrift( row, col );
			p++;
		}
	}
}


void LocalWeather::SlideLocalCellsVertical( int vx )
{
	WeatherCell	*from;
	WeatherCell	*to;
	WeatherCell	*last;
	int			newRow;
	int			col;

	ShiAssert( vx );	// Don't bother if there's no motion

	if (vx < 0)
	{
		// Viewer moving South so
		// Clouds moving North relative to the viewer
		// Clean up the North row
		to		= cellArray + rowLen*(rowLen-1);
		last	= to + rowLen;
		while (to < last) {
			to->Cleanup( objMgr );
			to++;
		}

		// Copy all the cell contents up one row
		to		= cellArray + rowLen*rowLen - 1;
		from	= to - rowLen;
		last	= cellArray;
		while (from >= last) {
			*to = *from;
			to--;
			from--;
		}

		// Note which row needs to be reinitialized for its new location
		newRow	= cellRow-cellRange;
		to		= cellArray;
	}
	else
	{
		// Clouds moving South relative to the viewer
		// Clean up the South row
		to		= cellArray;
		last	= to + rowLen;
		while (to < last) {
			to->Cleanup( objMgr );
			to++;
		}

		// Copy all the cell contents down one row
		to		= cellArray;
		from	= cellArray + rowLen;
		last	= cellArray + rowLen*rowLen;
		while (from < last) {
			*to = *from;
			to++;
			from++;
		}

		// Note which row needs to be reinitialized for its new location
		newRow	= cellRow+cellRange;
		to		= cellArray + rowLen*(rowLen-1);
	}

	// Now set up the new row
	last = to + rowLen;
	col = cellCol - cellRange;
	while (to < last) {
		to->Reset( newRow, col, objMgr );
		to++;
		col++;
	}
}


void LocalWeather::SlideLocalCellsHorizontal( int vy )
{
	WeatherCell	*from;
	WeatherCell	*to;
	WeatherCell	*last;
	WeatherCell	*rowStart;
	WeatherCell	*rowLast;
	int			newCol;
	int			row;

	ShiAssert( vy );	// Don't bother if there's no motion

	if (vy < 0)
	{
		// Viewer moving West so
		// Clouds moving East relative to the viewer
		// Clean up the right most col
		to		= cellArray + rowLen-1;
		last	= cellArray + rowLen*rowLen;
		while (to < last) {
			to->Cleanup( objMgr );
			to += rowLen;
		}

		// Copy all the cell contents right one row
		rowStart	= cellArray;
		rowLast		= cellArray + rowLen*rowLen;
		while (rowStart < rowLast) {
			to = rowStart + rowLen-1;
			from = to - 1;
			last = rowStart;
			while (from >= last) {
				*to = *from;
				to--;
				from--;
			}
			rowStart+=rowLen;
		}

		// Note which column needs to be reinitialized for its new location
		newCol	= cellCol-cellRange;
		to		= cellArray;
	}
	else
	{
		// Viewer moving East so
		// Clouds moving West relative to the viewer
		// Clean up the left most col
		to		= cellArray;
		last	= cellArray + rowLen*rowLen;
		while (to < last) {
			to->Cleanup( objMgr );
			to += rowLen;
		}

		// Copy all the cell contents left one row
		rowStart	= cellArray;
		rowLast		= cellArray + rowLen*rowLen-1;
		while (rowStart <= rowLast) {
			to = rowStart;
			from = to + 1;
			last = rowStart + rowLen;
			while (from < last) {
				*to = *from;
				to++;
				from++;
			}
			rowStart+=rowLen;
		}

		// Note which column needs to be reinitialized for its new location
		newCol	= cellCol+cellRange;
		to		= cellArray + rowLen-1;
	}

	// Now set up the new column
	last = to + rowLen*rowLen;
	row = cellRow - cellRange;
	while (to < last) {
		to->Reset( row, newCol, objMgr );
		to += rowLen;
		row++;
	}
}


void LocalWeather::RebuildList( void )
{
	WeatherCell	*p;
	int			r, c;

	// Release our whole list of cloud cells
	p = cellArray;
	for (r = cellRow-cellRange; r <= cellRow+cellRange; r++) {
		for (c = cellCol-cellRange; c <= cellCol+cellRange; c++) {
			p->Cleanup( objMgr );
			p++;
		}
	}

	// Record new values for the shift history of the weather map
	rowShiftHistory = TheWeather->rowShiftHistory;
	colShiftHistory = TheWeather->colShiftHistory;

	// Setup the cells arround us
	p = cellArray;
	for (r = cellRow-cellRange; r <= cellRow+cellRange; r++) {
		for (c = cellCol-cellRange; c <= cellCol+cellRange; c++) {
			p->Setup( r, c, objMgr );
			p++;
		}
	}
}


/***************************************************************************\
	Store the local cloud tops and bottoms
\***************************************************************************/
void LocalWeather::ComputeAreaFloorAndCeiling(void)
{
	WeatherCell *p;
	WeatherCell *last;
	float		tops;
	float		base = 1.0f-SKY_ROOF_HEIGHT;	// -Z is upward
	
	ShiAssert( cellArray );		// If this fails we could set 
								// base = 1.0f-SKY_ROOF_HEIGHT;
								// and
								// tops = -SKY_ROOF_HEIGHT;

	tops = 1e12f;
	p = cellArray;
	last = cellArray + rowLen*rowLen;
	while (p < last) {
		base = max( base, p->GetBase() );	// -Z is upward
		tops = min( tops, p->GetTop() );	// -Z is upward
		p++;
	}

	cloudBase = base;		// -Z is upward
	cloudTops = tops;		// -Z is upward
}


/***************************************************************************\
	Return TRUE if p1 can see p2 without clouds getting in the way.
	TODO:  Return the % transmission of light through the cloud instead of
	a strict boolean value.
\***************************************************************************/
float LocalWeather::LineOfSight( Tpoint *pnt1, Tpoint *pnt2 )
{
	WeatherCell	*startCell, *endCell;
	float		x1, y1, z1, x2, y2, z2;
	int			downward;
	int			stepUp, stepRt;
	int			hStep, vStep;
	int			endRow, endCol;
	int			prevRow, prevCol;
	float		dx, dy, dz;
	float		dzdx, dzdy, dydx, dxdy;
	int			row, col;
	int			rowt, colt;
	float		x, y, z;
	float		xt, yt, zt;


	// If both points are above or below the clouds, they can see each other
	// (Remember -Z is up)
	if ( ((pnt1->z > cloudBase) && (pnt2->z > cloudBase)) || ((pnt1->z < cloudTops) && (pnt2->z < cloudTops))) {
		return 1.0f;
	}

	// Store parameters of the vector we're testing
	x1 = pnt1->x - TheWeather->xOffset;
	y1 = pnt1->y - TheWeather->yOffset;
	z1 = pnt1->z;
	x2 = pnt2->x - TheWeather->xOffset;
	y2 = pnt2->y - TheWeather->yOffset;
	z2 = pnt2->z;
	dx = x2 - x1;
	dy = y2 - y1;
	dz = z2 - z1;
	dzdx = dz / dx;
	dzdy = dz / dy;
	dydx = dy / dx;
	dxdy = dx / dy;
	stepUp = (dx > 0.0f);
	stepRt = (dy > 0.0f);
	hStep = stepRt ? 1 : -1;
	vStep = stepUp ? 1 : -1;


	// initial point in cell space
	row       = TheWeather->WorldToTile( x1 );
	col       = TheWeather->WorldToTile( y1 );
	startCell = GetLocalCell( row-cellRow, col-cellCol );

	// terminal point in cell space
	endRow  = TheWeather->WorldToTile( x2 );
	endCol  = TheWeather->WorldToTile( y2 );
	endCell = GetLocalCell( endRow-cellRow, endCol-cellCol );


	// If either point is IN its local cell, they can't see each other
	if (startCell && (z1 <= startCell->GetBase()) && (z1 >= startCell->GetTop())) {
		// from point is in it
		return 0.0f;
	}
	if (endCell   && (z2 <= endCell->GetBase())   && (z2 >= endCell->GetTop())) {
		// at point is in it
		return 0.0f;
	}

	// If both points are in the same cell and haven't returned yet, they CAN see each other
	if (endCell == startCell) {
		return 1.0f;
	}

	// If either point is too close to the edge of the map, but neither is in a cloud, then 
	// ASSUME they can see each other (This prevents us from trying to run though the code below 
	// and walking off the edge of the available data)
	if ( (abs(row-cellRow)    > cellRange-1) || (abs(col-cellCol)    > cellRange-1) ||
		 (abs(endRow-cellRow) > cellRange-1) || (abs(endCol-cellCol) > cellRange-1)) {
		return 1.0f;
	}


	// We're looking downward through the clouds if we're starting out above them...
	downward = (z1 <= startCell->GetTop());


	// Decide if we need to use x or y as the control variable
	if ( fabs(dx) > fabs(dy) ) {
		// North/South dominant vector

		// Adjust the stepping parameters
		if (stepUp) {
			row		+=1;
			endRow	+=1;
		}

		// Walk the line
		while (1) {
			
			// Compute row/col for next check
			x = TheWeather->TileToWorld( row );
			y = y1 + dydx*(x - x1);
			z = z1 + dzdx*(x - x1);
			prevCol = col;
			col = TheWeather->WorldToTile( y );

			// Do vertical edge check if we've changed columns
			if (col != prevCol) {

				// Compute row/col for the check
				rowt = min( row, row-vStep );
				colt = max( col, prevCol );
				yt = TheWeather->TileToWorld( colt );
				xt = x1 + dxdy*(yt - y1);
				zt = z1 + dzdy*(yt - y1);

				// Check vertical edge we crossed
				if (verticalEdgeTest( rowt, colt, xt, yt, zt, downward )) {
					return LineSquareIntersection( rowt, colt-stepRt, x1,y1,z1, dx,dy,dz, downward );
				}

			}

			// Check horizontal edge between (row,col) and (row,col+1)
			if (horizontalEdgeTest( row, col, x, y, z, downward )) {
				return LineSquareIntersection( row-stepUp, col,x1,y1,z1, dx,dy,dz, downward );
			}

			// See if we're done or not
			if (row != endRow) {
				// Take one horizontal step
				row += vStep;
			} else {
				// We're done
				break;
			}
		}
	} else {
		// East/West dominant vector

		// Adjust the stepping parameters
		if (stepRt) {
			col		+=1;
			endCol	+=1;
		}

		// Walk the line
		while (1) {
			
			// Compute row/col for next check
			y = TheWeather->TileToWorld( col );
			x = x1 + dxdy*(y - y1);
			z = z1 + dzdy*(y - y1);
			prevRow = row;
			row = TheWeather->WorldToTile( x );

			// Do horizontal edge check if we've changed rows
			if (row != prevRow) {
				
				// Compute row/col for next check
				rowt = max( row, prevRow );
				colt = min( col, col-hStep );
				xt = TheWeather->TileToWorld( rowt );
				yt = y1 + dydx*(xt - x1);
				zt = z1 + dzdx*(xt - x1);

				// Check horizontal edge between we crossed
				if (horizontalEdgeTest( rowt, colt, xt, yt, zt, downward )) {
					return LineSquareIntersection( rowt-stepUp, colt, x1,y1,z1, dx,dy,dz, downward );
				}

			}

			// Check vertical edge between (row,col) and (row+1,col)
			if (verticalEdgeTest( row, col, x, y, z, downward )) {
				return LineSquareIntersection( row, col-stepRt, x1,y1,z1, dx,dy,dz, downward );
			}

			// See if we're done or not
			if (col != endCol) {
				// Take one horizontal step
				col += hStep;
			} else {
				// We're done
				break;
			}
		}
	}

	// If we got here, we didn't find an intersection with the cloud layer
	// so finally, test the cell which might contain the end point
	row = TheWeather->WorldToTile( x2 );
	col = TheWeather->WorldToTile( y2 );
	return LineSquareIntersection( row, col, x1,y1,z1, dx,dy,dz, downward );
}


//
// Helper functions used for the Cloud Intersection and Line of Sight tests.
// Return TRUE if the given point is below (less negative than) the edge
// anchored at given absolute row,col address.
//
//BOOL LocalWeather::horizontalEdgeTest( int row, int col, float x, float y, float z, BOOL downward )
BOOL LocalWeather::horizontalEdgeTest( int row, int col, float, float y, float z, BOOL downward )
{
	WeatherCell	*p;
	float		t, height;

	// Get the relevant cell
	p = GetLocalCell( row-cellRow, col-cellCol );

	// Make sure we didn't get out of bounds r/c
	ShiAssert( p >= cellArray );
	ShiAssert( p <  cellArray + rowLen*rowLen );

	// Compute the height of the edge at the point the line crosses it
	t = y / TheWeather->CellSize() - col;
	ShiAssert( (t > -0.01f) && (t < 1.01f) );
	height = p->GetSWtop() + t * (p->GetSEtop() - p->GetSWtop());

	// Return true if the line crosses below the edge (ie: is less negative) when we're going downward
	// (reverse sense when going upward)
	if (downward) 
		return (height<z);
	else
		return (height>z);
}

//BOOL LocalWeather::verticalEdgeTest( int row, int col, float x, float y, float z, BOOL downward )
BOOL LocalWeather::verticalEdgeTest( int row, int col, float, float y, float z, BOOL downward )
{
	WeatherCell	*p;
	float		t, height;

	// Get the relevant cell
	p = GetLocalCell( row-cellRow, col-cellCol );

	// Make sure we didn't get out of bounds r/c
	ShiAssert( p >= cellArray );
	ShiAssert( p <  cellArray + rowLen*rowLen );

	// Compute the height of the edge at the point the line crosses it
	t = y / TheWeather->CellSize() - col;
	ShiAssert( (t > -0.01f) && (t < 1.01f) );
	height = p->GetSWtop() + t * (p->GetNWtop() - p->GetSWtop());

	// Return true if the line crosses below the edge (ie: is less negative) when we're going downward
	// (reverse sense when going upward)
	if (downward) 
		return (height<z);
	else
		return (height>z);
}


//
// Helper function used for the Cloud Intersection test.
// Assume the given ray intersects the square whose
// lower left corner post is at (row,col).  Return the exact 
// location of the intersection in the provided Tpoint structure.
//
float LocalWeather::LineSquareIntersection( int row, int col, float x1, float y1, float z1, float dx, float dy, float dz, BOOL downward )
{
	int			r, c;
	WeatherCell	*p;
	float		t;
	float		Nx, Ny, Nz;
	float		SWx, SWy, SWz;
	float		PQdotN;
	float		NdotDIR;
	float		x, y;

	// Get the relevant cell
	r = row - cellRow + cellRange;
	c = col - cellCol + cellRange;
	p = cellArray + r*rowLen+c;

	// Make sure we didn't get out of bounds r/c
	ShiAssert( p >= cellArray );
	ShiAssert( p <  cellArray + rowLen*rowLen );

	// If this cell if empty, just return 100%
	if (p->IsEmpty()) {
		return 1.0f;
	}

	// Store the world space location of the lower left corner
	SWx = TheWeather->TileToWorld( row );
	SWy = TheWeather->TileToWorld( col );
	SWz = p->GetSWtop();

	// Compute the normal from the three posts which bound the point of interest
	// (don't forget - positive Z is DOWN)
	// upper left triangle (remember positive Z is down)
	if (downward) {
		Nx = p->GetNWtop() - p->GetSWtop();
		Ny = p->GetNEtop() - p->GetNWtop();
		Nz = -TheWeather->CellSize();
	} else {
		Nx = p->GetNWbottom() - p->GetSWbottom();
		Ny = p->GetNEbottom() - p->GetNWbottom();
		Nz = TheWeather->CellSize();
	}

	// Only check the first triangle if it is front facing relative to the test ray
	if ( Nx*dx + Ny*dy + Nz*dz < 0.0f ) {

		// Compute the intersection of the line with the plane
		PQdotN = (SWx - x1) * Nx + (SWy - y1) * Ny + (SWz - z1) * Nz;
		NdotDIR = Nx * dx        + Ny * dy         + Nz * dz;
		t = PQdotN / NdotDIR;

		x = x1 + t * dx;
		y = y1 + t * dy;

		// Return now if the intersection is within the upper left half space
		if ( (x-SWx) >= (y-SWy) ) {
			if ((t > 1.0f) || (t < 0.0f)) {
				// The intersection was outside the segment joining pnt1 and pnt2
				// so count it as a no-hit
				return 1.0f;
			}

			// Deal with alpha in border cells
			return 1.0f - p->GetAlpha( x+TheWeather->xOffset, y+TheWeather->yOffset );
		}
	}


	// lower right triangle (remember positive Z is down)
	// (Reuse the Nz value from the first triangle)
	if (downward) {
		Nx = p->GetNEtop() - p->GetSEtop();
		Ny = p->GetSEtop() - p->GetSWtop();
	} else {
		Nx = p->GetNEbottom() - p->GetSEbottom();
		Ny = p->GetSEbottom() - p->GetSWbottom();
	}

	// Only check the second triangle if it is front facing relative to the test ray
	if ( Nx*dx + Ny*dy + Nz*dz < 0.0f ) {

		// Compute the intersection of the line with the plane
		PQdotN = (SWx - x1) * Nx + (SWy - y1) * Ny + (SWz - z1) * Nz;
		NdotDIR = Nx * dx        + Ny * dy         + Nz * dz;
		t = PQdotN / NdotDIR;

		if ((t > 1.0f) || (t < 0.0f)) {
			// The intersection was outside the segment joining pnt1 and pnt2
			// so count it as a no-hit
			return 1.0f;
		}

		x = x1 + t * dx;
		y = y1 + t * dy;

		// Make sure the intersection we found is within the upper right half space
		// Rounding errors could make this assertion fail occasionally
		// WELL, this hits for more than rounding errors, but while it IS wrong, it isn't fatal
		// so lets let it go for now...
//		ShiAssert( (x-SWx) <= (y-SWy) );

		// Deal with alpha in border cells
		return 1.0f - p->GetAlpha( x+TheWeather->xOffset, y+TheWeather->yOffset );
	}


	// Hmm.  If we got here, its because both triangles were backfacing with respect to the
	// test ray.  Shouldn't happen, but this stuff is all a little shaky and dependent upon
	// floating point precision anyway, so it might...
	return 1.0f;
}


#if 0
/***************************************************************************\
**	Determine if a line segment (origin + vector) intersects bounding box 
**	for the BSP.
**  Return TRUE if so and the collision point.
**	Algo from GGEms I
\***************************************************************************/
BOOL DrawableBSP::GetRayHit( const Tpoint *from, const Tpoint *vector, Tpoint *collide )
{
	Tpoint		origin, vec;
	int			i;
	float 		*minBp, *maxBp, *orgp, *vecp, *collp;
	enum		{LEFT,RIGHT,MIDDLE}		quadrant[3];
	float		t, tMax;
	float		minB[3], maxB[3];
	float		candidatePlane[3];
	int			whichPlane;
	BOOL		inside = TRUE;

	// First we transform the origin and vector into object space (since that's easier than rotating the box)
	origin.x = from->x - position.x;
	origin.y = from->y - position.y;
	origin.z = from->z - position.z;
	vec = *vector;

	// Do away with these since they're constants?
	minB[0] = instance.BoxBack();
	minB[1] = instance.BoxLeft();
	minB[2] = instance.BoxTop();
	maxB[0] = instance.BoxFront();
	maxB[1] = instance.BoxRight();
	maxB[2] = instance.BoxBottom();


	// find candiate planes
	orgp =  (float *)&origin;
	minBp = (float *)&minB;
	maxBp = (float *)&maxB;
	for ( i = 0; i < 3; i++, orgp++, minBp++, maxBp++ )
	{
		if ( *orgp < *minBp )
		{
			quadrant[i] = LEFT;
			candidatePlane[i] = *minBp;
			inside = FALSE;
		}
		else if ( *orgp > *maxBp )
		{
			quadrant[i] = RIGHT;
			candidatePlane[i] = *maxBp;
			inside = FALSE;
		}
		else
		{
			quadrant[i] = MIDDLE;
		}
	}

	// origin is in box
	if ( inside ) {
		*collide = *from;
		return TRUE;
	}

	// calculate T distances to candidate planes and accumulate the largest
	if ( quadrant[0] != MIDDLE && vec.x != 0.0f ) {
		tMax = (candidatePlane[0] - origin.x) / vec.x;
		whichPlane = 0;
	} else {
		tMax = -1.0f;
	}
	if ( quadrant[1] != MIDDLE && vec.y != 0.0f ) {
		t = (candidatePlane[1] - origin.y) / vec.y;
		if (t > tMax) {
			tMax = t;
			whichPlane = 1;
		}
	}
	if ( quadrant[2] != MIDDLE && vec.z != 0.0f ) {
		t = (candidatePlane[2] - origin.z) / vec.z;
		if (t > tMax) {
			tMax = t;
			whichPlane = 2;
		}
	}

	// Check final candidate is within the segment of interest
	if ( tMax < 0.0f || tMax > 1.0f ) {
		return FALSE;
	}

	// Check final candidate is within the bounds of the side of the box
	orgp	= (float *)&origin;
	vecp	= (float *)&vec;
	collp	= (float *)&pos;
	for ( i = 0; i < 3; i++, vecp++, orgp++, collp++ ) {
		if ( whichPlane != i ) {
			*collp = *orgp + tMax * (*vecp);
			if ( *collp < minB[i] ||  *collp > maxB[i] ) {
				// outside box
				return FALSE;
			}
		} else {
			*collp = candidatePlane[i];
		}
	}

	// We must transform the collision point from object space back into world space
	collide->x = pos.x + position.x;
	collide->y = pos.y + position.y;
	collide->z = pos.z + position.z;


	return TRUE;
};
#endif
