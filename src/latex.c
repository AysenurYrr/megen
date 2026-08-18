#include <stdio.h>
#include <stdlib.h>

#include "profile.h"
#include "latex.h"

static int render_personal(FILE *f, const struct personal_info *personal)
{
	if (fprintf(f, "\\section*{%s}\n\n", personal->name) < 0)
		return -1;

	if (personal->email) {
		if (fprintf(f, "%s\n\n", personal->email) < 0)
			return -1;
	}

	return 0;
}

static int render_education(FILE *f, const struct profile *profile)
{
	size_t i;

	if (fprintf(f, "\\section*{Education}\n\n") < 0)
		return -1;

	for (i = 0; i < profile->education_count; i++) {
		const struct education *edu = &profile->education[i];

		if (fprintf(f, "\\textbf{%s}\\\\\n", edu->institution) < 0)
			return -1;

		if (edu->department) {
			if (fprintf(f, "%s\\\\\n", edu->department) < 0)
				return -1;
		}

		if (fprintf(f, "%04u-%02u -- ", edu->period.start.year,
			    edu->period.start.month) < 0)
			return -1;

		if (edu->period.ongoing) {
			if (fprintf(f, "Present\n\n") < 0)
				return -1;
		} else {
			if (fprintf(f, "%04u-%02u\n\n", edu->period.end.year,
				    edu->period.end.month) < 0)
				return -1;
		}
	}

	return 0;
}

int latex_render(const struct profile *profile, const char *path)
{
	FILE *f;

	f = fopen(path, "w");
	if (!f)
		return -1;

	if (fprintf(f, "\\documentclass[11pt,a4paper]{article}\n\n") < 0)
		goto error;

	if (fprintf(f, "\\begin{document}\n\n") < 0)
		goto error;

	if (render_personal(f, &profile->personal) < 0)
		goto error;

	if (render_education(f, profile) < 0)
		goto error;

	if (fprintf(f, "\\end{document}\n") < 0)
		goto error;

	if (fclose(f) != 0)
		return -1;

	return 0;

error:
	fclose(f);
	return -1;
}
