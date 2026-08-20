#include <stdio.h>

#include "profile.h"
#include "latex.h"

int main(void)
{
	struct profile profile;

	if (profile_load(&profile, "profile.toml"))
		return 1;

	if (latex_render(&profile, "build/cv.tex")) {
		profile_free(&profile);
		return 1;
	}

	if (latex_compile_pdf("build/cv.tex")) {
		fprintf(stderr, "megen: failed to compile PDF\n");
		profile_free(&profile);
		return 1;
	}

	profile_free(&profile);

	return 0;
}
