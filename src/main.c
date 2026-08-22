#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#include "profile.h"
#include "latex.h"
#include "html.h"

int main(void)
{
	struct profile profile;

	if (profile_load(&profile, "profile.toml"))
		return 1;

	if (mkdir("build", 0755) && errno != EEXIST) {
		fprintf(stderr, "megen: failed to create build directory\n");
		profile_free(&profile);
		return 1;
	}
	if (mkdir("build/site", 0755) && errno != EEXIST) {
		fprintf(stderr, "megen: failed to create site output directory\n");
		profile_free(&profile);
		return 1;
	}

	if (latex_render(&profile, "build/cv.tex")) {
		profile_free(&profile);
		return 1;
	}

	if (latex_compile_pdf("build/cv.tex")) {
		fprintf(stderr, "megen: failed to compile PDF\n");
		profile_free(&profile);
		return 1;
	}

	if (static_html_render(&profile, "build/site/index.html") ||
	    static_html_copy_asset("web/style.css", "build/site/style.css")) {
		fprintf(stderr, "megen: failed to generate static site\n");
		profile_free(&profile);
		return 1;
	}

	profile_free(&profile);

	return 0;
}
