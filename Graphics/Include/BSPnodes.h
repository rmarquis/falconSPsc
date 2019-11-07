/***************************************************************************\
    BSPnodes.h
    Scott Randolph
    January 30, 1998

    This provides the structure for the runtime BSP trees.
\***************************************************************************/
#ifndef _BSPNODES_H_
#define _BSPNODES_H_

#include "PolyLib.h"


typedef enum {
	tagBNode,
	tagBSubTree,
	tagBRoot,
	tagBSlotNode,
	tagBDofNode,
	tagBSwitchNode,
	tagBSplitterNode,
	tagBPrimitiveNode,
	tagBLitPrimitiveNode,
	tagBCulledPrimitiveNode,
	tagBSpecialXform,
	tagBLightStringNode,
} BNodeType;


typedef enum {
	Normal,
	Billboard,
	Tree
} BTransformType;


class BNode {
  public:
	BNode( BYTE *baseAddress, BNodeType **tagListPtr );
	BNode()												{ sibling = NULL; };
	virtual ~BNode()									{ delete sibling; };

	// We have special new and delete operators which don't do any memory operations
	void *operator new(size_t, void *p)	{ return p; };
	void *operator new(size_t n)			{ return malloc(n); };
	void operator delete(void *)			{ };
//	void operator delete(void *, void *) { };

	// Function to identify the type of an encoded node and call the appropriate constructor
	static BNode* RestorePointers( BYTE *baseAddress, int offset, BNodeType **tagListPtr );

	BNode	*sibling;

	virtual void		Draw(void) = 0;
	virtual BNodeType	Type(void)	{ return tagBNode; };
};

class BSubTree: public BNode {
  public:
	BSubTree( BYTE *baseAddress, BNodeType **tagListPtr );
	BSubTree()			{ subTree = NULL; };
	virtual ~BSubTree()	{ delete subTree; };

	Ppoint	*pCoords;
	int		nCoords;

	int		nDynamicCoords;
	int		DynamicCoordOffset;

	Pnormal	*pNormals;
	int		nNormals;


	BNode	*subTree;

	virtual void		Draw(void);
	virtual BNodeType	Type(void)	{ return tagBSubTree; };
};

class BRoot: public BSubTree {
  public:
	BRoot( BYTE *baseAddress, BNodeType **tagListPtr );
	BRoot() : BSubTree()		{ pTexIDs = NULL; nTexIDs = -1; ScriptNumber = -1; };
	virtual ~BRoot()	{};

	void	LoadTextures(void);
	void	UnloadTextures(void);

	int		*pTexIDs;
	int		nTexIDs;
	int		ScriptNumber;

	virtual void		Draw(void);
	virtual BNodeType	Type(void)	{ return tagBRoot; };
};

class BSpecialXform: public BNode {
  public:
	BSpecialXform( BYTE *baseAddress, BNodeType **tagListPtr );
	BSpecialXform()				{ subTree = NULL; };
	virtual ~BSpecialXform()	{ delete subTree; };

	Ppoint			*pCoords;
	int				nCoords;

	BTransformType	type;

	BNode			*subTree;

	virtual void		Draw(void);
	virtual BNodeType	Type(void)	{ return tagBSpecialXform; };
};

class BSlotNode: public BNode {
  public:
	BSlotNode( BYTE *baseAddress, BNodeType **tagListPtr );
	BSlotNode()				{ slotNumber = -1; };
	virtual ~BSlotNode()	{};

	Pmatrix			rotation;
	Ppoint			origin;
	int				slotNumber;

	virtual void		Draw(void);
	virtual BNodeType	Type(void)	{ return tagBSlotNode; };
};

class BDofNode: public BSubTree {
  public:
	BDofNode( BYTE *baseAddress, BNodeType **tagListPtr );
	BDofNode() : BSubTree()		{ dofNumber = -1; };
	virtual ~BDofNode()		{};

	int			dofNumber;
	Pmatrix		rotation;
	Ppoint		translation;

	virtual void		Draw(void);
	virtual BNodeType	Type(void)	{ return tagBDofNode; };
};

class BSwitchNode: public BNode {
  public:
	BSwitchNode( BYTE *baseAddress, BNodeType **tagListPtr );
	BSwitchNode()			{ subTrees = NULL; numChildren = 0; switchNumber = -1; };
	virtual ~BSwitchNode()	{ while (numChildren--) delete subTrees[numChildren]; };

	int			switchNumber;
	int			numChildren;
	BSubTree	**subTrees;

	virtual void		Draw(void);
	virtual BNodeType	Type(void)	{ return tagBSwitchNode; };
};

class BSplitterNode: public BNode {
  public:
	BSplitterNode( BYTE *baseAddress, BNodeType **tagListPtr );
	BSplitterNode()				{ front = back = NULL; };
	virtual ~BSplitterNode()	{ delete front; delete back; };

	float	A, B, C, D;
	BNode	*front;
	BNode	*back;

	virtual void		Draw(void);
	virtual BNodeType	Type(void)	{ return tagBSplitterNode; };
};

class BPrimitiveNode: public BNode {
  public:
	BPrimitiveNode( BYTE *baseAddress, BNodeType **tagListPtr );
	BPrimitiveNode()			{ prim = NULL; };
	virtual ~BPrimitiveNode()	{};

	Prim		*prim;

	virtual void		Draw(void);
	virtual BNodeType	Type(void)	{ return tagBPrimitiveNode; };
};

class BLitPrimitiveNode: public BNode {
  public:
	BLitPrimitiveNode( BYTE *baseAddress, BNodeType **tagListPtr );
	BLitPrimitiveNode()				{ poly = NULL; backpoly = NULL; };
	virtual ~BLitPrimitiveNode()	{};

	Poly		*poly;
	Poly		*backpoly;

	virtual void		Draw(void);
	virtual BNodeType	Type(void)	{ return tagBLitPrimitiveNode; };
};

class BCulledPrimitiveNode: public BNode {
  public:
	BCulledPrimitiveNode( BYTE *baseAddress, BNodeType **tagListPtr );
	BCulledPrimitiveNode()			{ poly = NULL; };
	virtual~BCulledPrimitiveNode()	{};

	Poly		*poly;

	virtual void		Draw(void);
	virtual BNodeType	Type(void)	{ return tagBCulledPrimitiveNode; };
};

class BLightStringNode: public BPrimitiveNode {
  public:
	BLightStringNode( BYTE *baseAddress, BNodeType **tagListPtr );
	BLightStringNode() : BPrimitiveNode()		{ rgbaFront = -1; rgbaBack = -1; A=B=C=D=0.0f;};
	virtual~BLightStringNode()	{};

	// For directional lights
	float		A, B, C, D;
	int			rgbaFront;
	int			rgbaBack;

	virtual void		Draw(void);
	virtual BNodeType	Type(void)	{ return tagBLightStringNode; };
};


#endif //_BSPNODES_H_
