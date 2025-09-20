#ifndef INPUT_HPP
#define INPUT_HPP

// Functioh declarations
int actionDown(int[]);			// is action currently down?
int actionPressed(int[]);		// was action pressed this frame?
int actionReleased(int[]);		// was action released this frame?

float actionAxis(int[], int[]);	// get axis from 2 actions
Vector2 actionVector(int[], int[], int[], int[]); // get Vector2 from 4 actions (not normalized)

// (used by input.cpp)
int arrayLen(int[]);
int arrayLen(Vector2[]);

// Built-in actions
const int action_UiLeft[2] = { KEY_A, KEY_LEFT };
const int action_UiRight[2] = { KEY_D, KEY_RIGHT };
const int action_UiUp[2] = { KEY_W, KEY_UP };
const int action_UiDown[2] = { KEY_S, KEY_DOWN };

const int action_UiAccept[2] = { KEY_SPACE, KEY_ENTER };
const int action_UiBack[2] = { KEY_ESCAPE, KEY_BACKSPACE };

#endif 