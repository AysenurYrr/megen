#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "profile.h"
#include "latex.h"

static const char *month_abbr(unsigned int month)
{
	static const char *months[] = {
		"", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};

	if (month < 1 || month > 12)
		return "???";

	return months[month];
}

static int latex_write_escaped(FILE *f, const char *str)
{
	if (!str)
		return 0;

	for (; *str; str++) {
		switch (*str) {
		case '&':
			if (fprintf(f, "\\&") < 0)
				return -1;
			break;
		case '%':
			if (fprintf(f, "\\%%") < 0)
				return -1;
			break;
		case '$':
			if (fprintf(f, "\\$") < 0)
				return -1;
			break;
		case '#':
			if (fprintf(f, "\\#") < 0)
				return -1;
			break;
		case '_':
			if (fprintf(f, "\\_") < 0)
				return -1;
			break;
		case '{':
			if (fprintf(f, "\\{") < 0)
				return -1;
			break;
		case '}':
			if (fprintf(f, "\\}") < 0)
				return -1;
			break;
		case '\\':
			if (fprintf(f, "\\textbackslash{}") < 0)
				return -1;
			break;
		case '~':
			if (fprintf(f, "\\textasciitilde{}") < 0)
				return -1;
			break;
		case '^':
			if (fprintf(f, "\\textasciicircum{}") < 0)
				return -1;
			break;
		default:
			if (fputc(*str, f) == EOF)
				return -1;
		}
	}

	return 0;
}

static int latex_render_date(FILE *f, const struct date date)
{
	return fprintf(f, "%s %04u", month_abbr(date.month), date.year);
}

static int render_highlights(FILE *f, char *const *highlights, size_t count)
{
	size_t i;

	if (count == 0)
		return 0;

	if (fprintf(f, "\\begin{itemize}\n") < 0)
		return -1;

	for (i = 0; i < count; i++) {
		if (fprintf(f, "\\item ") < 0)
			return -1;

		if (latex_write_escaped(f, highlights[i]) < 0)
			return -1;

		if (fprintf(f, "\n") < 0)
			return -1;
	}

	if (fprintf(f, "\\end{itemize}\n") < 0)
		return -1;

	return 0;
}

static int render_personal(FILE *f, const struct personal_info *personal)
{
	if (fprintf(f, "\\section*{") < 0)
		return -1;

	if (latex_write_escaped(f, personal->name) < 0)
		return -1;

	if (fprintf(f, "}\n\n") < 0)
		return -1;

	if (personal->email) {
		if (latex_write_escaped(f, personal->email) < 0)
			return -1;

		if (fprintf(f, "\n\n") < 0)
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

		if (fprintf(f, "\\textbf{") < 0)
			return -1;

		if (latex_write_escaped(f, edu->institution) < 0)
			return -1;

		if (fprintf(f, "}\\\\\n") < 0)
			return -1;

		if (edu->department) {
			if (latex_write_escaped(f, edu->department) < 0)
				return -1;

			if (fprintf(f, "\\\\\n") < 0)
				return -1;
		}

		if (latex_render_date(f, edu->period.start) < 0)
			return -1;

		if (fprintf(f, " -- ") < 0)
			return -1;

		if (edu->period.ongoing) {
			if (fprintf(f, "Present\n\n") < 0)
				return -1;
		} else {
			if (latex_render_date(f, edu->period.end) < 0)
				return -1;

			if (fprintf(f, "\n\n") < 0)
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

		if (fprintf(f, "\\textbf{") < 0)
			return -1;

		if (latex_write_escaped(f, work->title) < 0)
			return -1;

		if (fprintf(f, "}\\\\\n") < 0)
			return -1;

		if (work->organization) {
			if (latex_write_escaped(f, work->organization) < 0)
				return -1;

			if (fprintf(f, "\\\\\n") < 0)
				return -1;
		}

		for (j = 0; j < work->period_count; j++) {
			if (latex_render_date(f, work->periods[j].start) < 0)
				return -1;

			if (fprintf(f, " -- ") < 0)
				return -1;

			if (work->periods[j].ongoing) {
				if (fprintf(f, "Present") < 0)
					return -1;
			} else {
				if (latex_render_date(f, work->periods[j].end) < 0)
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
				if (latex_write_escaped(f, work->technologies[j]) < 0)
					return -1;

				if (j < work->technology_count - 1) {
					if (fprintf(f, ", ") < 0)
						return -1;
				}
			}
			if (fprintf(f, "}\\\\\n") < 0)
				return -1;
		}

		if (render_highlights(f, work->highlights, work->highlight_count) < 0)
			return -1;

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

		if (fprintf(f, "\\textbf{") < 0)
			return -1;

		if (latex_write_escaped(f, award->title) < 0)
			return -1;

		if (fprintf(f, "}\\\\\n") < 0)
			return -1;

		if (latex_write_escaped(f, award->issuer) < 0)
			return -1;

		if (fprintf(f, ", ") < 0)
			return -1;

		if (latex_render_date(f, award->date) < 0)
			return -1;

		if (fprintf(f, "\n") < 0)
			return -1;

		if (award->description) {
			if (latex_write_escaped(f, award->description) < 0)
				return -1;

			if (fprintf(f, "\n") < 0)
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

		if (fprintf(f, "\\textbf{") < 0)
			return -1;

		if (latex_write_escaped(f, cert->title) < 0)
			return -1;

		if (fprintf(f, "}\\\\\n") < 0)
			return -1;

		if (latex_write_escaped(f, cert->issuer) < 0)
			return -1;

		if (fprintf(f, ", ") < 0)
			return -1;

		if (latex_render_date(f, cert->date) < 0)
			return -1;

		if (fprintf(f, "\n") < 0)
			return -1;

		if (cert->description) {
			if (latex_write_escaped(f, cert->description) < 0)
				return -1;

			if (fprintf(f, "\n") < 0)
				return -1;
		}

		if (fprintf(f, "\n") < 0)
			return -1;
	}

	return 0;
}

static int render_volunteer_activities(FILE *f, const struct profile *profile)
{
	size_t i;

	if (profile->volunteer_activity_count == 0)
		return 0;

	if (fprintf(f, "\\section*{Volunteer Activities}\n\n") < 0)
		return -1;

	for (i = 0; i < profile->volunteer_activity_count; i++) {
		const struct volunteer_activity *activity =
			&profile->volunteer_activities[i];

		if (fprintf(f, "\\textbf{") < 0)
			return -1;

		if (latex_write_escaped(f, activity->organization) < 0)
			return -1;

		if (fprintf(f, "}\\\\\n") < 0)
			return -1;

		if (latex_write_escaped(f, activity->role) < 0)
			return -1;

		if (fprintf(f, "\\\\\n") < 0)
			return -1;

		if (latex_render_date(f, activity->period.start) < 0)
			return -1;

		if (fprintf(f, " -- ") < 0)
			return -1;

		if (activity->period.ongoing) {
			if (fprintf(f, "Present\n") < 0)
				return -1;
		} else {
			if (latex_render_date(f, activity->period.end) < 0)
				return -1;

			if (fprintf(f, "\n") < 0)
				return -1;
		}

		if (render_highlights(f, activity->highlights, activity->highlight_count) < 0)
			return -1;

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

		if (fprintf(f, "\\textbf{") < 0)
			return -1;

		if (latex_write_escaped(f, proj->name) < 0)
			return -1;

		if (fprintf(f, "}\\\\\n") < 0)
			return -1;

		if (proj->summary) {
			if (latex_write_escaped(f, proj->summary) < 0)
				return -1;

			if (fprintf(f, "\n") < 0)
				return -1;
		}

		if (proj->description) {
			if (latex_write_escaped(f, proj->description) < 0)
				return -1;

			if (fprintf(f, "\\\\\n") < 0)
				return -1;
		}

		if (proj->technology_count > 0) {
			if (fprintf(f, "\\textit{") < 0)
				return -1;
			for (j = 0; j < proj->technology_count; j++) {
				if (latex_write_escaped(f, proj->technologies[j]) < 0)
					return -1;

				if (j < proj->technology_count - 1) {
					if (fprintf(f, ", ") < 0)
						return -1;
				}
			}
			if (fprintf(f, "}\\\\\n") < 0)
				return -1;
		}

		if (render_highlights(f, proj->highlights, proj->highlight_count) < 0)
			return -1;

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
