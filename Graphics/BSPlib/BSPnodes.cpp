/***************************************************************************\
    BSPnodes.cpp
    Scott Randolph
    January 30, 1998

    This provides the structure for the runtime BSP trees.
\***************************************************************************/
#include "stdafx.h"
#include "StateStack.h"
#include "ClipFlags.h"
#include "TexBank.h"
#include "ObjectInstance.h"
#include "Scripts.h"
#include "BSPnodes.h"
#include "falclib\include\mltrig.h"
#include "falclib\include\IsBad.h"

#pragma warning(disable : 4291)

/***************************************************************\
	To improve performance, these classes use several global
	variables to store command data instead of passing it
	through the call stack.
	The global variables are maintained by the StackState
	module.
\***************************************************************/

// Determine the type of an encoded node and intialize and contruct
// it appropriatly.
BNode* BNode::RestorePointers( BYTE *baseAddress, int offset, BNodeType **tagListPtr )
{
	BNode		*node;
	BNodeType	tag;

	// Get this record's tag then advance the pointer to the next tag we'll need
	tag = **tagListPtr;
	*tagListPtr = (*tagListPtr)+1;
	

	// Apply the proper virtual table setup and constructor
	switch( tag ) {
	  case tagBSubTree:
		node = new (baseAddress+offset) BSubTree( baseAddress, tagListPtr );
		break;
	  case tagBRoot:
		node = new (baseAddress+offset) BRoot( baseAddress, tagListPtr );
		break;
	  case tagBSpecialXform:
		node = new (baseAddress+offset) BSpecialXform( baseAddress, tagListPtr );
		break;
	  case tagBSlotNode:
		node = new (baseAddress+offset) BSlotNode( baseAddress, tagListPtr );
		break;
	  case tagBDofNode:
		node = new (baseAddress+offset) BDofNode( baseAddress, tagListPtr );
		break;
	  case tagBSwitchNode:
		node = new (baseAddress+offset) BSwitchNode( baseAddress, tagListPtr );
		break;
	  case tagBSplitterNode:
		node = new (baseAddress+offset) BSplitterNode( baseAddress, tagListPtr );
		break;
	  case tagBPrimitiveNode:
		node = new (baseAddress+offset) BPrimitiveNode( baseAddress, tagListPtr );
		break;
	  case tagBLitPrimitiveNode:
		node = new (baseAddress+offset) BLitPrimitiveNode( baseAddress, tagListPtr );
		break;
	  case tagBCulledPrimitiveNode:
		node = new (baseAddress+offset) BCulledPrimitiveNode( baseAddress, tagListPtr );
		break;
	  case tagBLightStringNode:
		node = new (baseAddress+offset) BLightStringNode( baseAddress, tagListPtr );
		break;
	  default:
		ShiError("Decoding unrecognized BSP node type.");
	}

	return node;
}


// Convert from file offsets back to pointers
BNode::BNode( BYTE *baseAddress, BNodeType **tagListPtr )
{
	// Fixup our sibling, if any
	if ((int)sibling >= 0) {
		sibling = RestorePointers( baseAddress, (int)sibling, tagListPtr );
	} else {
		sibling = NULL;
	}
}


// Convert from file offsets back to pointers
BSubTree::BSubTree( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Fixup our dependents
	subTree		= RestorePointers( baseAddress, (int)subTree, tagListPtr );

	// Fixup our data pointers
	pCoords		= (Ppoint*)(baseAddress + (int)pCoords);
	pNormals	= (Pnormal*)(baseAddress + (int)pNormals);
}


// Convert from file offsets back to pointers
BRoot::BRoot( BYTE *baseAddress, BNodeType **tagListPtr )
:BSubTree( baseAddress, tagListPtr )
{
	// Fixup our extra data pointers
	pTexIDs		= (int*)(baseAddress + (int)pTexIDs);
}


// Convert from file offsets back to pointers
BSpecialXform::BSpecialXform( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Fixup our dependents
	subTree		= RestorePointers( baseAddress, (int)subTree, tagListPtr );

	// Fixup our data pointers
	pCoords		= (Ppoint*)(baseAddress + (int)pCoords);
}


// Convert from file offsets back to pointers
BSlotNode::BSlotNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
}


// Convert from file offsets back to pointers
BDofNode::BDofNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BSubTree( baseAddress, tagListPtr )
{
}


// Convert from file offsets back to pointers
BSwitchNode::BSwitchNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Fixup our table of children
	subTrees = (BSubTree**)(baseAddress + (int)subTrees);

	// Now fixup each child tree
	for (int i=0; i<numChildren; i++) {
		subTrees[i] = (BSubTree*)RestorePointers( baseAddress, (int)subTrees[i], tagListPtr );
	}
}


// Convert from file offsets back to pointers
BSplitterNode::BSplitterNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Fixup our dependents
	front	= RestorePointers( baseAddress, (int)front, tagListPtr );
	back	= RestorePointers( baseAddress, (int)back,  tagListPtr );
}


// Convert from file offsets back to pointers
BPrimitiveNode::BPrimitiveNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Now fixup our polygon
	prim		= RestorePrimPointers( baseAddress, (int)prim );
}


// Convert from file offsets back to pointers
BLitPrimitiveNode::BLitPrimitiveNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Now fixup our polygons
	poly		= (Poly*)RestorePrimPointers( baseAddress, (int)poly );
	backpoly	= (Poly*)RestorePrimPointers( baseAddress, (int)backpoly );
}


// Convert from file offsets back to pointers
BCulledPrimitiveNode::BCulledPrimitiveNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BNode( baseAddress, tagListPtr )
{
	// Now fixup our polygon
	poly		= (Poly*)RestorePrimPointers( baseAddress, (int)poly );
}


// Convert from file offsets back to pointers
BLightStringNode::BLightStringNode( BYTE *baseAddress, BNodeType **tagListPtr )
:BPrimitiveNode( baseAddress, tagListPtr )
{
}


void BSubTree::Draw(void)
{
	BNode	*child;

	TheStateStack.Light( pNormals, nNormals );

	if (nDynamicCoords == 0) {
		TheStateStack.Transform( pCoords, nCoords );
	} else {
		TheStateStack.Transform( pCoords, nCoords-nDynamicCoords );
		TheStateStack.Transform( TheStateStack.CurrentInstance->DynamicCoords+DynamicCoordOffset, 
								 nDynamicCoords );
	}

	child = subTree;
	ShiAssert(FALSE == F4IsBadReadPtr( child, sizeof*child) );
	do
	{
		child->Draw();
		child = child->sibling;
	} while (child); // JB 010306 CTD
	//} while (child && !F4IsBadReadPtr(child, sizeof(BNode))); // JB 010306 CTD (too much CPU)
}

void BRoot::Draw(void)
{
	// Compute the offset to the first texture in the texture set
	int texOffset = TheStateStack.CurrentInstance->TextureSet * 
		(nTexIDs/TheStateStack.CurrentInstance->ParentObject->nTextureSets);
	TheStateStack.SetTextureTable( pTexIDs + texOffset );
								   
	if (ScriptNumber > 0) {
		ShiAssert( ScriptNumber < ScriptArrayLength );
		if (ScriptNumber < ScriptArrayLength) {
			ScriptArray[ScriptNumber]();
		}
	}

	BSubTree::Draw();
}

void BSpecialXform::Draw(void)
{
	ShiAssert( subTree );

	TheStateStack.PushVerts();
	TheStateStack.TransformBillboardWithClip( pCoords, nCoords, type );
	subTree->Draw();
	TheStateStack.PopVerts();
}

void BSlotNode::Draw(void)
{
	ShiAssert( slotNumber < TheStateStack.CurrentInstance->ParentObject->nSlots );
	if (slotNumber >= TheStateStack.CurrentInstance->ParentObject->nSlots )
		return; // JPO fix
	ObjectInstance *subObject = TheStateStack.CurrentInstance->SlotChildren[slotNumber];

	if (subObject) {
		TheStateStack.DrawSubObject( subObject, &rotation, &origin );
	}
}

void BDofNode::Draw(void)
{
	Pmatrix	dofRot;
	Pmatrix	R;
	Ppoint	T;
	mlTrig trig;

	ShiAssert( dofNumber < TheStateStack.CurrentInstance->ParentObject->nDOFs );
	if (dofNumber >= TheStateStack.CurrentInstance->ParentObject->nDOFs )
		return;
	// Set up our free rotation
	mlSinCos (&trig, TheStateStack.CurrentInstance->DOFValues[dofNumber].rotation);
	dofRot.M11 = 1.0f;	dofRot.M12 = 0.0f;	dofRot.M13 = 0.0f;
	dofRot.M21 = 0.0f;	dofRot.M22 = trig.cos;	dofRot.M23 = -trig.sin;
	dofRot.M31 = 0.0f;	dofRot.M32 = trig.sin;	dofRot.M33 = trig.cos;


	// Now compose this with the rotation into our parents coordinate system
	MatrixMult( &rotation, &dofRot, &R );

	// Now do our free translation
	// SCR 10/28/98:  THIS IS WRONG FOR TRANSLATION DOFs.  "DOFValues" is supposed to 
	// translate along the local x axis, but this will translate along the parent's x axis.
	// To fix this would require a bit more math (and/or thought).  Since it
	// only happens once in Falcon, I'll leave it broken and put a workaround into
	// the KC10 object so that the parent's and child's x axis are forced into alignment
	// by inserting an extra dummy DOF bead.
	T.x = translation.x + TheStateStack.CurrentInstance->DOFValues[dofNumber].translation;
	T.y = translation.y;
	T.z = translation.z;

	// Draw our subtree
	TheStateStack.PushAll();
	TheStateStack.CompoundTransform( &R, &T );
	BSubTree::Draw();
	TheStateStack.PopAll();
}

void BSwitchNode::Draw(void)
{
	UInt32		mask;
	int			i = 0;

	ShiAssert( switchNumber < TheStateStack.CurrentInstance->ParentObject->nSwitches );
	if (switchNumber >= TheStateStack.CurrentInstance->ParentObject->nSwitches)
		return;
	mask = TheStateStack.CurrentInstance->SwitchValues[switchNumber];

#if 0	// This will generally be faster due to early termination
	// Go until all ON switch children have been drawn.
	while (mask) {
#else	// This will work even if the mask is set for non-existent children
	// Go until all children have been considered for drawing.
	while (i < numChildren) {
#endif
		ShiAssert( subTrees[i] );

		// Only draw this subtree if the corresponding switch bit is set
		if (mask & 1) {
			TheStateStack.PushVerts();
			subTrees[i]->Draw();
			TheStateStack.PopVerts();
		}
		mask >>= 1;
		i++;
	}
}

void BSplitterNode::Draw(void)
{
	BNode	*child;

	ShiAssert( front );
	ShiAssert( back );

	if (A*TheStateStack.ObjSpaceEye.x + 
		B*TheStateStack.ObjSpaceEye.y + 
		C*TheStateStack.ObjSpaceEye.z + D > 0.0f) {

		child = front;
		do
		{
			child->Draw();
			child = child->sibling;
		} while (child);

		child = back;
		do
		{
			child->Draw();
			child = child->sibling;
		} while (child);
	}
	else
	{
		child = back;
		do
		{
			child->Draw();
			child = child->sibling;
		} while (child);

		child = front;
		do
		{
			child->Draw();
			child = child->sibling;
		} while (child);
	}
}

void BPrimitiveNode::Draw(void)
{
	// Call the appropriate draw function for this primitive
	DrawPrimJumpTable[prim->type]( prim );
}

void BLitPrimitiveNode::Draw(void)
{
	// Choose the front facing polygon so that lighting is correct
	if ((poly->A*TheStateStack.ObjSpaceEye.x + poly->B*TheStateStack.ObjSpaceEye.y + poly->C*TheStateStack.ObjSpaceEye.z + poly->D) >= 0) {
		DrawPrimJumpTable[poly->type]( poly );
	} else {
		DrawPrimJumpTable[poly->type]( backpoly );
	}
}

void BCulledPrimitiveNode::Draw(void)
{
	// Only draw front facing polygons
	if ((poly->A*TheStateStack.ObjSpaceEye.x + poly->B*TheStateStack.ObjSpaceEye.y + poly->C*TheStateStack.ObjSpaceEye.z + poly->D) >= 0) {
		// Call the appropriate draw function for this polygon
		DrawPrimJumpTable[poly->type]( poly );
	}
}

void BLightStringNode::Draw(void)
{
	// Clobber the primitive color with the appropriate front or back color
	if ((A*TheStateStack.ObjSpaceEye.x + B*TheStateStack.ObjSpaceEye.y + C*TheStateStack.ObjSpaceEye.z + D) >= 0) {
		((PrimPointFC*)prim)->rgba = rgbaFront;
	} else {
		((PrimPointFC*)prim)->rgba = rgbaBack;
	}

	// Call the appropriate draw function for this polygon
	DrawPrimJumpTable[prim->type]( prim );
}


void BRoot::LoadTextures(void)
{
	for (int i=0; i<nTexIDs; i++) {
		// Skip unsed texture indices
		if (pTexIDs[i] >= 0) {
			TheTextureBank.Reference( pTexIDs[i] );
		}
	}
}


void BRoot::UnloadTextures(void)
{
	for (int i=0; i<nTexIDs; i++) {
		if (pTexIDs[i] >= 0) {
			TheTextureBank.Release( pTexIDs[i] );
		}
	}
}
