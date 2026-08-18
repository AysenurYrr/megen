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

	profile_free(&profile);

	return 0;
}
