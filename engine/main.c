#include "plat/plat.h"


int main(void) {
	plat_init();
	while (plat_loop());
	return 0;
}
