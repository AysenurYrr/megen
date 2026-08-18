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

	if (profile->education_count == 0)
		return 0;

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

static int render_academic_work(FILE *f, const struct profile *profile)
{
	size_t i, j;

	if (profile->academic_work_count == 0)
		return 0;

	if (fprintf(f, "\\section*{Academic Work}\n\n") < 0)
		return -1;

	for (i = 0; i < profile->academic_work_count; i++) {
		const struct academic_work *work = &profile->academic_works[i];

		if (fprintf(f, "\\textbf{%s}\\\\\n", work->title) < 0)
			return -1;

		if (work->organization) {
			if (fprintf(f, "%s\\\\\n", work->organization) < 0)
				return -1;
		}

		for (j = 0; j < work->period_count; j++) {
			if (fprintf(f, "%04u-%02u -- ", work->periods[j].start.year,
				    work->periods[j].start.month) < 0)
				return -1;

			if (work->periods[j].ongoing) {
				if (fprintf(f, "Present") < 0)
					return -1;
			} else {
				if (fprintf(f, "%04u-%02u", work->periods[j].end.year,
					    work->periods[j].end.month) < 0)
					return -1;
			}

			if (j < work->period_count - 1) {
				if (fprintf(f, "; ") < 0)
					return -1;
			}
		}

		if (fprintf(f, "\n") < 0)
			return -1;

		if (work->technology_count > 0) {
			if (fprintf(f, "\\textit{") < 0)
				return -1;
			for (j = 0; j < work->technology_count; j++) {
				if (fprintf(f, "%s", work->technologies[j]) < 0)
					return -1;
				if (j < work->technology_count - 1) {
					if (fprintf(f, ", ") < 0)
						return -1;
				}
			}
			if (fprintf(f, "}\\\\\n") < 0)
				return -1;
		}

		for (j = 0; j < work->highlight_count; j++) {
			if (fprintf(f, "\\item %s\n", work->highlights[j]) < 0)
				return -1;
		}

		if (fprintf(f, "\n") < 0)
			return -1;
	}

	return 0;
}

static int render_awards(FILE *f, const struct profile *profile)
{
	size_t i;

	if (profile->award_count == 0)
		return 0;

	if (fprintf(f, "\\section*{Awards}\n\n") < 0)
		return -1;

	for (i = 0; i < profile->award_count; i++) {
		const struct award *award = &profile->awards[i];

		if (fprintf(f, "\\textbf{%s}\\\\\n", award->title) < 0)
			return -1;

		if (fprintf(f, "%s, %04u-%02u\n", award->issuer, award->date.year,
			    award->date.month) < 0)
			return -1;

		if (award->description) {
			if (fprintf(f, "%s\n", award->description) < 0)
				return -1;
		}

		if (fprintf(f, "\n") < 0)
			return -1;
	}

	return 0;
}

static int render_certificates(FILE *f, const struct profile *profile)
{
	size_t i;

	if (profile->certificate_count == 0)
		return 0;

	if (fprintf(f, "\\section*{Certificates}\n\n") < 0)
		return -1;

	for (i = 0; i < profile->certificate_count; i++) {
		const struct certificate *cert = &profile->certificates[i];

		if (fprintf(f, "\\textbf{%s}\\\\\n", cert->title) < 0)
			return -1;

		if (fprintf(f, "%s, %04u-%02u\n", cert->issuer, cert->date.year,
			    cert->date.month) < 0)
			return -1;

		if (cert->description) {
			if (fprintf(f, "%s\n", cert->description) < 0)
				return -1;
		}

		if (fprintf(f, "\n") < 0)
			return -1;
	}

	return 0;
}

static int render_volunteer_activities(FILE *f, const struct profile *profile)
{
	size_t i, j;

	if (profile->volunteer_activity_count == 0)
		return 0;

	if (fprintf(f, "\\section*{Volunteer Activities}\n\n") < 0)
		return -1;

	for (i = 0; i < profile->volunteer_activity_count; i++) {
		const struct volunteer_activity *activity =
			&profile->volunteer_activities[i];

		if (fprintf(f, "\\textbf{%s}\\\\\n", activity->organization) < 0)
			return -1;

		if (fprintf(f, "%s\\\\\n", activity->role) < 0)
			return -1;

		if (fprintf(f, "%04u-%02u -- ", activity->period.start.year,
			    activity->period.start.month) < 0)
			return -1;

		if (activity->period.ongoing) {
			if (fprintf(f, "Present\n") < 0)
				return -1;
		} else {
			if (fprintf(f, "%04u-%02u\n", activity->period.end.year,
				    activity->period.end.month) < 0)
				return -1;
		}

		for (j = 0; j < activity->highlight_count; j++) {
			if (fprintf(f, "\\item %s\n", activity->highlights[j]) < 0)
				return -1;
		}

		if (fprintf(f, "\n") < 0)
			return -1;
	}

	return 0;
}

static int render_projects(FILE *f, const struct profile *profile)
{
	size_t i, j;

	if (profile->project_count == 0)
		return 0;

	if (fprintf(f, "\\section*{Projects}\n\n") < 0)
		return -1;

	for (i = 0; i < profile->project_count; i++) {
		const struct project *proj = &profile->projects[i];

		if (fprintf(f, "\\textbf{%s}\\\\\n", proj->name) < 0)
			return -1;

		if (proj->summary) {
			if (fprintf(f, "%s\n", proj->summary) < 0)
				return -1;
		}

		if (proj->description) {
			if (fprintf(f, "%s\\\\\n", proj->description) < 0)
				return -1;
		}

		if (proj->technology_count > 0) {
			if (fprintf(f, "\\textit{") < 0)
				return -1;
			for (j = 0; j < proj->technology_count; j++) {
				if (fprintf(f, "%s", proj->technologies[j]) < 0)
					return -1;
				if (j < proj->technology_count - 1) {
					if (fprintf(f, ", ") < 0)
						return -1;
				}
			}
			if (fprintf(f, "}\\\\\n") < 0)
				return -1;
		}

		for (j = 0; j < proj->highlight_count; j++) {
			if (fprintf(f, "\\item %s\n", proj->highlights[j]) < 0)
				return -1;
		}

		if (fprintf(f, "\n") < 0)
			return -1;
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

	if (render_academic_work(f, profile) < 0)
		goto error;

	if (render_awards(f, profile) < 0)
		goto error;

	if (render_certificates(f, profile) < 0)
		goto error;

	if (render_volunteer_activities(f, profile) < 0)
		goto error;

	if (render_projects(f, profile) < 0)
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
