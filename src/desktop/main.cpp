#include "gbe_emulator.h"

int main(int argc, char* args[])
{
	gbe::Emulator emulator;
	return emulator.Run() ? 0 : -1;
}