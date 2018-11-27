#include <gbe.h>

int main(int argc, char* args[])
{
	gbe_context_t context = gbe_create();
	int32_t	res = gbe_main_loop(context);
	gbe_destroy(context);
	return res;
}