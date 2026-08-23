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

static int latex_write_visual_escaped(FILE *f, const char *str)
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

static int latex_write_actual_text(FILE *f, const char *str)
{
	const unsigned char *p = (const unsigned char *)str;
	unsigned int codepoint;

	if (fprintf(f, "\\BeginAccSupp{method=hex,unicode,ActualText=") < 0)
		return -1;

	while (*p) {
		if (*p < 0x80) {
			codepoint = *p++;
		} else if ((*p & 0xe0) == 0xc0 && p[1]) {
			codepoint = ((unsigned int)(p[0] & 0x1f) << 6) |
				    (unsigned int)(p[1] & 0x3f);
			p += 2;
		} else if ((*p & 0xf0) == 0xe0 && p[1] && p[2]) {
			codepoint = ((unsigned int)(p[0] & 0x0f) << 12) |
				    ((unsigned int)(p[1] & 0x3f) << 6) |
				    (unsigned int)(p[2] & 0x3f);
			p += 3;
		} else {
			return -1;
		}

		if (fprintf(f, "%04X", codepoint) < 0)
			return -1;
	}

	if (fprintf(f, "}") < 0 || latex_write_visual_escaped(f, str) < 0 ||
	    fprintf(f, "\\EndAccSupp{}") < 0)
		return -1;

	return 0;
}

static int latex_write_escaped(FILE *f, const char *str)
{
	const unsigned char *p = (const unsigned char *)str;

	if (!str)
		return 0;
	for (; *p; p++) {
		if (*p >= 0x80)
			return latex_write_actual_text(f, str);
	}
	return latex_write_visual_escaped(f, str);
}

static const char *short_url(const char *url)
{
	const char *display = url;

	if (strncmp(display, "https://", 8) == 0)
		display += 8;
	else if (strncmp(display, "http://", 7) == 0)
		display += 7;

	if (strncmp(display, "www.", 4) == 0)
		display += 4;
	if (strncmp(display, "tr.linkedin.com/", 16) == 0)
		display += 3;

	return display;
}

static int render_url(FILE *f, const char *url)
{
	const char *display = short_url(url);
	const char *scheme = strstr(url, "://") ? "" : "https://";

	if (fprintf(f, "\\href{\\detokenize{%s%s}}{", scheme, url) < 0 ||
	    latex_write_escaped(f, display) < 0 || fprintf(f, "}") < 0)
		return -1;

	return 0;
}

static int render_labeled_url(FILE *f, const struct link *link)
{
	const char *scheme = strstr(link->url, "://") ? "" : "https://";

	if (fprintf(f, "\\href{\\detokenize{%s%s}}{", scheme, link->url) < 0 ||
	    latex_write_escaped(f, link->label) < 0 || fprintf(f, "}") < 0)
		return -1;

	return 0;
}

static int render_email(FILE *f, const char *email)
{
	if (fprintf(f, "\\href{mailto:\\detokenize{%s}}{", email) < 0 ||
	    latex_write_escaped(f, email) < 0 || fprintf(f, "}") < 0)
		return -1;

	return 0;
}

static int latex_render_date(FILE *f, const struct date date)
{
	if (date.month == 0)
		return fprintf(f, "%04u", date.year);
	return fprintf(f, "%s %04u", month_abbr(date.month), date.year);
}

static int dates_equal(const struct date left, const struct date right)
{
	return left.year == right.year && left.month == right.month;
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
	int first = 1;
	size_t i;

	/* Dominant name with a subtle, selectable low-level signature. */
	if (fprintf(f, "\\noindent{\\fontsize{24}{27}\\selectfont\\bfseries\\color{ink} ") < 0)
		return -1;

	if (latex_write_actual_text(f, personal->name) < 0)
		return -1;

	if (fprintf(f,
		    "}%%\n\\hfill%%\n"
		    "{\\color{accent}\\raisebox{0.6ex}{\\rule{1.4cm}{0.6pt}}}%%\n"
		    "\\hspace{0.65em}%%\n"
		    "{\\ttfamily\\bfseries\\fontsize{14}{16}\\selectfont\\color{accent} 0x41595345}\\par\n") < 0)
		return -1;

	if (personal->title) {
		if (fprintf(f, "{\\fontsize{12.5}{15}\\selectfont\\bfseries\\color{accent} ") < 0 ||
		    latex_write_escaped(f, personal->title) < 0 ||
		    fprintf(f, "}\\\\[4pt]\n") < 0)
			return -1;
	}

	if (personal->location) {
		if (latex_write_actual_text(f, personal->location) < 0)
			return -1;
		first = 0;
	}

	/* Contact info on one line with separators */
	if (personal->email) {
		if (!first && fprintf(f, " \\textcolor{muted}{|} ") < 0)
			return -1;
		if (render_email(f, personal->email) < 0)
			return -1;
		first = 0;
	}

	if (personal->github) {
		if (!first && fprintf(f, " \\textcolor{muted}{|} ") < 0)
			return -1;
		if (render_url(f, personal->github) < 0)
			return -1;
		first = 0;
	}

	if (personal->linkedin) {
		if (!first && fprintf(f, " \\textcolor{muted}{|} ") < 0)
			return -1;
		if (render_url(f, personal->linkedin) < 0)
			return -1;
		first = 0;
	}

	if (personal->website) {
		if (!first && fprintf(f, " \\textcolor{muted}{|} ") < 0)
			return -1;
		if (render_url(f, personal->website) < 0)
			return -1;
		first = 0;
	}

	if (!first && fprintf(f, "\\par\\vspace{4pt}\n") < 0)
		return -1;

	if (personal->skill_count > 0) {
		for (i = 0; i < personal->skill_count; i++) {
			if (latex_write_escaped(f, personal->skills[i]) < 0)
				return -1;
			if (i + 1 < personal->skill_count &&
			    fprintf(f, " \\textcolor{accent}{\\textbullet} ") < 0)
				return -1;
		}
		if (fprintf(f, "\\par\\vspace{3pt}\n") < 0)
			return -1;
	}

	if (fprintf(f, "\\vspace{2pt}\n") < 0)
		return -1;

	return 0;
}

static int render_summary(FILE *f, const struct personal_info *personal)
{
	if (!personal->summary)
		return 0;
	if (fprintf(f, "\\cvsection{Summary}\n") < 0 ||
	    latex_write_escaped(f, personal->summary) < 0 ||
	    fprintf(f, "\n\n") < 0)
		return -1;
	return 0;
}

static int render_education(FILE *f, const struct profile *profile)
{
	size_t i;

	if (profile->education_count == 0)
		return 0;

	if (fprintf(f, "\\cvsection{Education}\n") < 0)
		return -1;

	for (i = 0; i < profile->education_count; i++) {
		const struct education *edu = &profile->education[i];

		/* Institution name on left, date on right */
		if (fprintf(f, "\\textbf{") < 0)
			return -1;

		if (latex_write_escaped(f, edu->institution) < 0)
			return -1;

		if (fprintf(f, "} \\hfill ") < 0)
			return -1;

		if (latex_render_date(f, edu->period.start) < 0)
			return -1;

		if (fprintf(f, " -- ") < 0)
			return -1;

		if (edu->period.ongoing) {
			if (fprintf(f, "Present\\\\\n") < 0)
				return -1;
		} else {
			if (latex_render_date(f, edu->period.end) < 0)
				return -1;

			if (fprintf(f, "\\\\\n") < 0)
				return -1;
		}

		/* Department on separate line */
		if (edu->department) {
			if (latex_write_escaped(f, edu->department) < 0)
				return -1;

			if (fprintf(f, "\\par\n") < 0)
				return -1;
		}

		if (edu->description) {
			if (fprintf(f, "{\\small\\color{muted} ") < 0 ||
			    latex_write_escaped(f, edu->description) < 0 ||
			    fprintf(f, "}\\par\n") < 0)
				return -1;
		}

		if (fprintf(f, "\n") < 0)
			return -1;
	}

	return 0;
}

static int render_experiences(FILE *f, const struct profile *profile)
{
	size_t i;

	if (profile->experience_count == 0)
		return 0;

	if (fprintf(f, "\\cvsection{Experience}\n") < 0)
		return -1;

	for (i = 0; i < profile->experience_count; i++) {
		const struct experience *experience = &profile->experiences[i];

		if (fprintf(f, "\\textbf{") < 0 ||
		    latex_write_escaped(f, experience->company) < 0 ||
		    fprintf(f, "} \\hfill ") < 0 ||
		    latex_render_date(f, experience->period.start) < 0 ||
		    fprintf(f, " -- ") < 0)
			return -1;

		if (experience->period.ongoing) {
			if (fprintf(f, "Present\\\\\n") < 0)
				return -1;
		} else {
			if (latex_render_date(f, experience->period.end) < 0 ||
			    fprintf(f, "\\\\\n") < 0)
				return -1;
		}

		if (latex_write_escaped(f, experience->title) < 0 ||
		    fprintf(f, "\\par\n") < 0)
			return -1;

		if (experience->location) {
			if (fprintf(f, "{\\small\\color{muted} ") < 0 ||
			    latex_write_escaped(f, experience->location) < 0 ||
			    fprintf(f, "}\\par\n") < 0)
				return -1;
		}
		if (render_highlights(f, experience->highlights,
				      experience->highlight_count) < 0 ||
		    fprintf(f, "\\vspace{4pt}\n") < 0)
			return -1;
	}

	return 0;
}

static int render_research_projects(FILE *f, const struct profile *profile)
{
	size_t i, j;

	if (profile->research_project_count == 0)
		return 0;

	if (fprintf(f, "\\cvsection{Research Experience}\n") < 0)
		return -1;

	for (i = 0; i < profile->research_project_count; i++) {
		const struct research_project *project =
			&profile->research_projects[i];
		struct link video_link = { "Video", project->video };
		if (fprintf(f, "\\Needspace{12\\baselineskip}\\begin{samepage}\n") < 0)
			return -1;

		if (fprintf(f, "\\textbf{") < 0 ||
		    latex_write_escaped(f, project->title) < 0 ||
		    fprintf(f, "}") < 0)
			return -1;
		if (project->video &&
		    (fprintf(f, " {\\small\\color{muted}\\textbullet{} ") < 0 ||
		     render_labeled_url(f, &video_link) < 0 ||
		     fprintf(f, "}") < 0))
			return -1;
		if (fprintf(f, "\\par\n") < 0 ||
		    latex_write_escaped(f, project->organization) < 0 ||
		    fprintf(f, " \\hfill ") < 0 ||
		    latex_render_date(f, project->period.start) < 0 ||
		    fprintf(f, " -- ") < 0)
			return -1;

		if (project->period.ongoing) {
			if (fprintf(f, "Present\\par\n") < 0)
				return -1;
		} else if (latex_render_date(f, project->period.end) < 0 ||
			   fprintf(f, "\\par\n") < 0) {
			return -1;
		}

		if (project->role &&
		    (fprintf(f, "{\\small\\color{muted} ") < 0 ||
		     latex_write_escaped(f, project->role) < 0 ||
		     fprintf(f, "}\\par\n") < 0))
			return -1;

		if (project->sponsor_count > 0) {
			if (fprintf(f, "Supported by ") < 0)
				return -1;
			for (j = 0; j < project->sponsor_count; j++) {
				if (latex_write_escaped(f, project->sponsors[j]) < 0 ||
				    (j + 1 < project->sponsor_count &&
				     fprintf(f, ", ") < 0))
					return -1;
			}
			if (fprintf(f, "\\par\n") < 0)
				return -1;
		}

		if (project->technology_count > 0) {
			for (j = 0; j < project->technology_count; j++) {
				if (latex_write_escaped(f, project->technologies[j]) < 0)
					return -1;
				if (j + 1 < project->technology_count &&
				    fprintf(f, " \\textcolor{accent}{\\textbullet} ") < 0)
						return -1;
			}
			if (fprintf(f, "\\par\n") < 0)
				return -1;
		}

		if (render_highlights(f, project->highlights,
				      project->highlight_count) < 0)
			return -1;

		if (fprintf(f, "\\end{samepage}\\vspace{4pt}\n") < 0)
			return -1;
	}

	return 0;
}

static int render_awards(FILE *f, const struct profile *profile)
{
	size_t i, j;

	if (profile->award_count == 0)
		return 0;

	if (fprintf(f, "\\cvsection{Awards}\n") < 0)
		return -1;

	for (i = 0; i < profile->award_count; i++) {
		const struct award *award = &profile->awards[i];

		if (fprintf(f, "\\begin{samepage}\n") < 0)
			return -1;

		/* Title on left, date on right */
		if (fprintf(f, "\\textbf{") < 0)
			return -1;

		if (latex_write_escaped(f, award->title) < 0)
			return -1;

		if (fprintf(f, "} \\hfill ") < 0)
			return -1;

		if (latex_render_date(f, award->date) < 0)
			return -1;

		if (fprintf(f, "\\\\\n") < 0)
			return -1;

		/* Issuer on next line */
		if (latex_write_escaped(f, award->issuer) < 0)
			return -1;
		for (j = 0; j < award->link_count; j++) {
			if (fprintf(f, " \\textcolor{accent}{\\textbullet} ") < 0 ||
			    render_labeled_url(f, &award->links[j]) < 0)
				return -1;
		}

		if (fprintf(f, "\n") < 0)
			return -1;

		/* CV intentionally shows at most the first award highlight. */
		if (award->highlight_count > 0 &&
		    render_highlights(f, award->highlights, 1) < 0)
			return -1;

		if (fprintf(f, "\\end{samepage}\\vspace{4pt}\n") < 0)
			return -1;
	}

	return 0;
}

static int render_certificates(FILE *f, const struct profile *profile)
{
	size_t i, j;
	int has_visible_certificate = 0;

	for (i = 0; i < profile->certificate_count; i++) {
		if (profile->certificates[i].show_in_cv) {
			has_visible_certificate = 1;
			break;
		}
	}
	if (!has_visible_certificate)
		return 0;

	if (fprintf(f, "\\cvsection{Certificates}\n") < 0)
		return -1;

	for (i = 0; i < profile->certificate_count; i++) {
		const struct certificate *cert = &profile->certificates[i];
		if (!cert->show_in_cv)
			continue;

		/* Title on left, date on right */
		if (fprintf(f, "\\textbf{") < 0)
			return -1;

		if (latex_write_escaped(f, cert->title) < 0)
			return -1;

		if (fprintf(f, "} \\hfill ") < 0)
			return -1;

		if (latex_render_date(f, cert->period.start) < 0)
			return -1;
		if (!dates_equal(cert->period.start, cert->period.end) &&
		    (fprintf(f, " -- ") < 0 ||
		     latex_render_date(f, cert->period.end) < 0))
			return -1;

		if (fprintf(f, "\\\\\n") < 0)
			return -1;

		/* Issuer on next line */
		if (latex_write_escaped(f, cert->issuer) < 0)
			return -1;
		for (j = 0; j < cert->link_count; j++) {
			if (fprintf(f, " \\textcolor{accent}{\\textbullet} ") < 0)
				return -1;
			if (render_labeled_url(f, &cert->links[j]) < 0)
				return -1;
		}

		if (fprintf(f, "\n") < 0)
			return -1;

		/* Description if present */
		if (cert->description) {
			if (latex_write_escaped(f, cert->description) < 0)
				return -1;

			if (fprintf(f, "\n") < 0)
				return -1;
		}

		if (fprintf(f, "\\par\\vspace{3pt}\n") < 0)
			return -1;
	}

	return 0;
}

static int render_projects(FILE *f, const struct profile *profile)
{
	size_t i, j;
	int has_visible_project = 0;

	for (i = 0; i < profile->project_count; i++) {
		if (profile->projects[i].show_in_cv) {
			has_visible_project = 1;
			break;
		}
	}
	if (!has_visible_project)
		return 0;

	if (fprintf(f, "\\cvsection{Projects}\n") < 0)
		return -1;

	for (i = 0; i < profile->project_count; i++) {
		const struct project *proj = &profile->projects[i];
		struct link video_link = { "Video", proj->video };
		if (!proj->show_in_cv)
			continue;

		if (fprintf(f, "\\Needspace{8\\baselineskip}\\textbf{") < 0)
			return -1;

		if (latex_write_escaped(f, proj->name) < 0)
			return -1;

		if (fprintf(f, "}") < 0)
			return -1;
		if (proj->link_count > 0 || proj->video) {
			if (fprintf(f, " {\\small\\color{muted}\\textbullet{} ") < 0)
				return -1;
			for (j = 0; j < proj->link_count; j++) {
				if (render_labeled_url(f, &proj->links[j]) < 0 ||
				    (j + 1 < proj->link_count &&
				     fprintf(f, " \\textbullet{} ") < 0))
					return -1;
			}
			if (proj->video &&
			    ((proj->link_count > 0 &&
			      fprintf(f, " \\textbullet{} ") < 0) ||
			     render_labeled_url(f, &video_link) < 0))
				return -1;
			if (fprintf(f, "}") < 0)
				return -1;
		}
		if (fprintf(f, " \\hfill ") < 0)
			return -1;

		if (proj->technology_count > 0) {
			if (fprintf(f, "{\\small\\color{accent} ") < 0)
				return -1;
			for (j = 0; j < proj->technology_count; j++) {
				if (latex_write_escaped(f, proj->technologies[j]) < 0)
					return -1;

				if (j + 1 < proj->technology_count) {
					if (fprintf(f, " \\textbullet{} ") < 0)
						return -1;
				}
			}
			if (fprintf(f, "}") < 0)
				return -1;
		}
		if (fprintf(f, "\\par\n") < 0)
			return -1;

		if (render_highlights(f, proj->highlights,
				      proj->highlight_count) < 0)
			return -1;

		if (fprintf(f, "\\vspace{4pt}\n") < 0)
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

	if (fprintf(f, "\\documentclass[10pt,a4paper]{article}\n\n") < 0)
		goto error;

	/* ATS-safe encoding with explicit, precomposed Turkish mappings. */
	if (fprintf(f, "\\usepackage[T1]{fontenc}\n\\usepackage[utf8]{inputenc}\n") < 0)
		goto error;
	if (fprintf(f, "\\usepackage[scaled=0.95]{helvet}\n\\renewcommand{\\familydefault}{\\sfdefault}\n") < 0)
		goto error;
	if (fprintf(f, "\\input{glyphtounicode}\n\\pdfgentounicode=1\n") < 0)
		goto error;
	if (fprintf(f,
		    "\\pdfglyphtounicode{Gbreve}{011E}\n"
		    "\\pdfglyphtounicode{gbreve}{011F}\n"
		    "\\pdfglyphtounicode{Idotaccent}{0130}\n"
		    "\\pdfglyphtounicode{dotlessi}{0131}\n"
		    "\\pdfglyphtounicode{Scedilla}{015E}\n"
		    "\\pdfglyphtounicode{scedilla}{015F}\n\n") < 0)
		goto error;

	/* Simple single-column layout; all content remains ordinary PDF text. */
	if (fprintf(f, "\\usepackage[a4paper,top=1.35cm,bottom=1.35cm,left=1.65cm,right=1.65cm]{geometry}\n") < 0)
		goto error;
	if (fprintf(f, "\\usepackage{enumitem}\n\\usepackage{microtype}\n") < 0)
		goto error;
	if (fprintf(f, "\\usepackage{needspace}\n") < 0)
		goto error;
	if (fprintf(f, "\\usepackage{accsupp}\n") < 0)
		goto error;
	if (fprintf(f, "\\usepackage{xcolor}\n") < 0)
		goto error;
	if (fprintf(f, "\\usepackage[hidelinks]{hyperref}\n") < 0)
		goto error;
	if (fprintf(f, "\\definecolor{ink}{HTML}{20252B}\n\\definecolor{muted}{HTML}{7A828A}\n\\definecolor{accent}{HTML}{657681}\n") < 0)
		goto error;
	if (fprintf(f, "\\color{ink}\\setlength{\\parindent}{0pt}\\setlength{\\parskip}{0pt}\n") < 0)
		goto error;
	if (fprintf(f, "\\linespread{1.04}\\selectfont\n\n") < 0)
		goto error;

	/* Remove page numbers */
	if (fprintf(f, "\\pagestyle{empty}\n\n") < 0)
		goto error;

	/* Monospace prefix, sans-serif label, and an inline continuation rule. */
	if (fprintf(f,
		    "\\newcommand{\\cvsection}[1]{%%\n"
		    "  \\vspace{6pt}\\noindent\\color{accent}%%\n"
		    "  \\makebox[\\linewidth][l]{{\\ttfamily\\bfseries\\fontsize{12.5}{14}\\selectfont //}\\hspace{0.25em}%%\n"
		    "  {\\sffamily\\bfseries\\large #1}\\hspace{0.75em}%%\n"
		    "  \\leaders\\hrule height 0.35pt\\hfill}%%\n"
		    "  \\par\\vspace{3pt}\\color{ink}}\n\n") < 0)
		goto error;

	/* Bullet point spacing */
	if (fprintf(f, "\\setlist[itemize]{leftmargin=1.25em,label=\\textbullet,itemsep=1.5pt,topsep=2pt,parsep=0pt,partopsep=0pt}\n\n") < 0)
		goto error;

	if (fprintf(f, "\\begin{document}\n\n") < 0)
		goto error;

	if (render_personal(f, &profile->personal) < 0)
		goto error;

	if (render_summary(f, &profile->personal) < 0)
		goto error;

	if (render_education(f, profile) < 0)
		goto error;

	if (render_experiences(f, profile) < 0)
		goto error;

	if (render_projects(f, profile) < 0)
		goto error;

	if (render_research_projects(f, profile) < 0)
		goto error;

	if (render_awards(f, profile) < 0)
		goto error;

	if (render_certificates(f, profile) < 0)
		goto error;

	/* Selectable end-of-program signature at the bottom of the final page. */
	if (fprintf(f,
		    "\\vfill\\noindent\\hfill%%\n"
		    "{\\color{accent}\\raisebox{0.6ex}{\\rule{1.4cm}{0.6pt}}}%%\n"
		    "\\hspace{0.65em}%%\n"
		    "{\\ttfamily\\bfseries\\fontsize{14}{16}\\selectfont\\color{accent} s\\_endpgm}\\par\n") < 0)
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

int latex_compile_pdf(const char *tex_path)
{
	char cmd[512];
	int ret;

	if (!tex_path)
		return -1;

	/* Compile in the same directory with deterministic ToUnicode mappings.
	   -interaction=nonstopmode: don't stop on errors
	   -halt-on-error: stop on fatal errors
	 */
	ret = snprintf(cmd, sizeof(cmd),
		       "pdflatex -interaction=nonstopmode -halt-on-error "
		       "-output-directory=$(dirname '%s') '%s' > /dev/null 2>&1",
		       tex_path, tex_path);

	if (ret < 0 || ret >= (int)sizeof(cmd))
		return -1;

	return system(cmd) == 0 ? 0 : -1;
}
