#include "Maze.Validation.h"

bool isValidMazeSize(const MazeSize& size)
{
	if (size.rows >= MIN_MAZE_SIZE &&
		size.rows <= MAX_MAZE_SIZE &&
		size.cols >= MIN_MAZE_SIZE &&
		size.cols <= MAX_MAZE_SIZE)
	{
		return true;
	}
	return false;
}