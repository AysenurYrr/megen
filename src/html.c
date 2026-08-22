#include <stdio.h>

#include "html.h"
#include "profile.h"

static int html_write_escaped(FILE *f, const char *text, int attribute)
{
	if (!text)
		return 0;

	for (; *text; text++) {
		switch (*text) {
		case '&':
			if (fputs("&amp;", f) == EOF)
				return -1;
			break;
		case '<':
			if (fputs("&lt;", f) == EOF)
				return -1;
			break;
		case '>':
			if (fputs("&gt;", f) == EOF)
				return -1;
			break;
		case '"':
			if (attribute && fputs("&quot;", f) == EOF)
				return -1;
			if (!attribute && fputc(*text, f) == EOF)
				return -1;
			break;
		case '\'':
			if (attribute && fputs("&#39;", f) == EOF)
				return -1;
			if (!attribute && fputc(*text, f) == EOF)
				return -1;
			break;
		default:
			if (fputc((unsigned char)*text, f) == EOF)
				return -1;
		}
	}
	return 0;
}

static int render_external_link(FILE *f, const char *class_name,
				const char *url, const char *label,
				const char *suffix)
{
	if (!url)
		return 0;
	if (fprintf(f, "        <a class=\"%s\" href=\"", class_name) < 0 ||
	    html_write_escaped(f, url, 1) < 0 ||
	    fputs("\" target=\"_blank\" rel=\"noreferrer noopener\">", f) == EOF ||
	    html_write_escaped(f, label, 0) < 0 ||
	    (suffix && html_write_escaped(f, suffix, 0) < 0) ||
	    fputs("</a>\n", f) == EOF)
		return -1;
	return 0;
}

static int render_skills(FILE *f, const struct personal_info *personal)
{
	size_t i;

	if (personal->skill_count == 0)
		return 0;
	if (fputs("      <p class=\"hero-skills\" aria-label=\"Technical skills\">", f) == EOF)
		return -1;
	for (i = 0; i < personal->skill_count; i++) {
		if (i > 0 && fputs("<span aria-hidden=\"true\">·</span>", f) == EOF)
			return -1;
		if (fputs("<span>", f) == EOF ||
		    html_write_escaped(f, personal->skills[i], 0) < 0 ||
		    fputs("</span>", f) == EOF)
			return -1;
	}
	return fputs("</p>\n", f) == EOF ? -1 : 0;
}

int static_html_render(const struct profile *profile, const char *path)
{
	const struct personal_info *personal;
	FILE *f;

	if (!profile || !path)
		return -1;
	personal = &profile->personal;
	f = fopen(path, "w");
	if (!f)
		return -1;

	if (fputs("<!doctype html>\n<html lang=\"en\">\n<head>\n"
		  "  <meta charset=\"utf-8\">\n"
		  "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
		  "  <meta name=\"color-scheme\" content=\"dark\">\n"
		  "  <meta name=\"description\" content=\"", f) == EOF ||
	    html_write_escaped(f, personal->summary ? personal->summary : personal->title, 1) < 0 ||
	    fputs("\">\n  <title>", f) == EOF ||
	    html_write_escaped(f, personal->name, 0) < 0 ||
	    fputs("</title>\n  <link rel=\"stylesheet\" href=\"style.css\">\n"
		  "</head>\n<body>\n"
		  "  <a class=\"skip-link\" href=\"#hero-content\">Skip to content</a>\n"
		  "  <header class=\"site-hero\">\n"
		  "    <nav class=\"site-nav\" aria-label=\"Primary navigation\">\n"
		  "      <a href=\"#projects\">projects</a>\n"
		  "      <a href=\"#notes\">notes</a>\n"
		  "      <a href=\"#about\">about</a>\n", f) == EOF)
		goto error;

	if (personal->github &&
	    render_external_link(f, "nav-github", personal->github,
				 "github", " ↗") < 0)
		goto error;

	if (fputs("    </nav>\n"
		  "    <div class=\"hero-content\" id=\"hero-content\">\n"
		  "      <p class=\"hero-meta\">0x00000000 &lt;_start&gt;</p>\n"
		  "      <h1 class=\"hero-name\">", f) == EOF ||
	    html_write_escaped(f, personal->name, 0) < 0 ||
	    fputs("</h1>\n      <p class=\"hero-title\">", f) == EOF ||
	    html_write_escaped(f, personal->title, 0) < 0 ||
	    fputs("</p>\n", f) == EOF)
		goto error;

	if (personal->location &&
	    (fputs("      <p class=\"hero-location\">", f) == EOF ||
	     html_write_escaped(f, personal->location, 0) < 0 ||
	     fputs("</p>\n", f) == EOF))
		goto error;
	if (render_skills(f, personal) < 0 ||
	    fputs("      <div class=\"hero-actions\" aria-label=\"Profile links\">\n", f) == EOF)
		goto error;
	if (render_external_link(f, "hero-action", personal->github,
				 "github", " ↗") < 0 ||
	    render_external_link(f, "hero-action", personal->linkedin,
				 "linkedin", " ↗") < 0 ||
	    fputs("        <a class=\"hero-action\" href=\"../cv.pdf\" download>resume ↓</a>\n"
		  "      </div>\n"
		  "    </div>\n"
		  "  </header>\n"
		  "</body>\n</html>\n", f) == EOF)
		goto error;

	if (fclose(f) != 0)
		return -1;
	return 0;

error:
	fclose(f);
	return -1;
}

int static_html_copy_asset(const char *source_path, const char *output_path)
{
	unsigned char buffer[8192];
	FILE *source;
	FILE *output;
	size_t count;
	int status = -1;

	if (!source_path || !output_path)
		return -1;
	source = fopen(source_path, "rb");
	if (!source)
		return -1;
	output = fopen(output_path, "wb");
	if (!output) {
		fclose(source);
		return -1;
	}
	while ((count = fread(buffer, 1, sizeof(buffer), source)) > 0) {
		if (fwrite(buffer, 1, count, output) != count)
			goto out;
	}
	if (ferror(source))
		goto out;
	status = 0;
out:
	if (fclose(output) != 0)
		status = -1;
	fclose(source);
	return status;
}
