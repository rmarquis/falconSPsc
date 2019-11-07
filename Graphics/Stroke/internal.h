
typedef struct {
  UInt count; // Number of paths
  UInt start; // First path
  UInt width; // Character width
} StrokeGlyph;

typedef struct {
  UInt count;  // Number of verticles
  UInt start;  // First vertex
} StrokePath_;

typedef struct {
  Int x;
  Int y;
} StrokeVertex;

//Currently unused..one font
typedef struct {
  StrokeGlyph*  glyph;
  StrokePath_*   path;
  StrokeVertex* vertex;
} StrokeFont;
