#include <cstdio>
#include "Go.h"

int main(const int argc, const char* argv[])
{
	if (argc > 1)
		printf("argv[1] => %s\n", argv[1]);
	else
		printf("argc => %d\n", argc);

	// If launched from the editor, pass the scene path from argv[1], otherwise Go() defaults to scene.leveldata
	Go(argc > 1 ? argv[1] : nullptr);
	return 0;
}