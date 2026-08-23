#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "html.h"
#include "latex.h"
#include "profile.h"

static int create_directory(const char *path)
{
	if (mkdir(path, 0755) == 0 || errno == EEXIST)
		return 0;

	fprintf(stderr, "megen: failed to create directory '%s'\n", path);
	return -1;
}

static int prepare_output(void)
{
	return create_directory("build") ||
	       create_directory("build/site") ||
	       create_directory("build/site/assets");
}

static int copy_site_assets(void)
{
	return static_html_copy_asset("web/style.css", "build/site/style.css") ||
	       static_html_copy_asset("web/gallery.js", "build/site/gallery.js") ||
	       static_html_copy_asset_tree("web/assets/images",
					   "build/site/assets/images") ||
	       static_html_copy_asset_tree("web/assets/notes",
					   "build/site/assets/notes") ||
	       static_html_copy_asset_tree("web/assets/fonts",
					   "build/site/assets/fonts");
}

static int generate_outputs(const struct profile *profile)
{
	if (latex_render(profile, "build/cv.tex"))
		return -1;
	if (latex_compile_pdf("build/cv.tex")) {
		fprintf(stderr, "megen: failed to compile PDF\n");
		return -1;
	}
	if (static_html_render(profile, "build/site/index.html") ||
	    copy_site_assets()) {
		fprintf(stderr, "megen: failed to generate static site\n");
		return -1;
	}
	return 0;
}

int main(void)
{
	struct profile profile;

	if (profile_load(&profile, "profile.toml"))
		return EXIT_FAILURE;

	if (prepare_output() || generate_outputs(&profile)) {
		profile_free(&profile);
		return EXIT_FAILURE;
	}

	profile_free(&profile);
	return EXIT_SUCCESS;
}
