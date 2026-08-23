#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "html.h"
#include "profile.h"

static const char *hero_title(const char *title)
{
	const char *separator;

	if (!title)
		return "";
	separator = strstr(title, " — ");
	return separator ? separator + strlen(" — ") : title;
}

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

static int render_address(FILE *f, size_t base, size_t index)
{
	return fprintf(f, "0x%08zX", base + (index + 1) * 0x100) < 0 ? -1 : 0;
}

static int render_notes(FILE *f, const struct profile *profile)
{
	size_t i;

	if (profile->note_count == 0)
		return 0;
	if (fputs("    <section class=\"site-section notes-section\" id=\"notes\" aria-labelledby=\"notes-title\">\n"
		  "      <div class=\"section-heading\">\n"
		  "        <span class=\"section-address\">0x00002000</span>\n"
		  "        <span class=\"section-node\" aria-hidden=\"true\">●</span>\n"
		  "        <h2 id=\"notes-title\">.lecture_notes</h2>\n"
		  "      </div>\n"
		  "      <div class=\"memory-group notes-list\">\n", f) == EOF)
		return -1;

	for (i = 0; i < profile->note_count; i++) {
		const struct note *note = &profile->notes[i];

		if (fputs("        <article class=\"memory-entry\">\n"
			  "          <span class=\"entry-address\">", f) == EOF ||
		    render_address(f, 0x2000, i) < 0 ||
		    fputs("</span>\n"
			  "          <div class=\"entry-content\">\n"
			  "            <div class=\"entry-heading\"><h4>", f) == EOF ||
		    html_write_escaped(f, note->title, 0) < 0 ||
		    fputs("</h4><a href=\"", f) == EOF ||
		    html_write_escaped(f, note->url, 1) < 0 ||
		    fputs("\">PDF ↗</a></div>\n            <p>", f) == EOF ||
		    html_write_escaped(f, note->summary, 0) < 0 ||
		    fputs("</p>\n          </div>\n        </article>\n", f) == EOF)
			return -1;
	}
	return fputs("      </div>\n    </section>\n", f) == EOF ? -1 : 0;
}

static int render_projects(FILE *f, const struct profile *profile)
{
	size_t i;
	size_t j;

	if (profile->project_count == 0)
		return 0;
	if (fputs("    <section class=\"site-section projects-section\" id=\"projects\" aria-labelledby=\"projects-title\">\n"
		  "      <div class=\"section-heading\">\n"
		  "        <span class=\"section-address\">0x00003000</span>\n"
		  "        <span class=\"section-node\" aria-hidden=\"true\">●</span>\n"
		  "        <h2 id=\"projects-title\">.projects</h2>\n"
		  "      </div>\n      <div class=\"memory-group project-group\">\n", f) == EOF)
		return -1;

	for (i = 0; i < profile->project_count; i++) {
		const struct project *project = &profile->projects[i];
		const char *project_text = project->description ?
			project->description : project->summary;
		int has_links = project->github || project->blog || project->demo ||
			project->link_count > 0;
		size_t index_image_count = 0;

		for (j = 0; j < project->image_count; j++)
			if (project->images[j].show_on_index)
				index_image_count++;

		if (fputs("        <article class=\"memory-entry project-entry\">\n"
			  "          <span class=\"entry-address\">", f) == EOF ||
		    (project->address ? html_write_escaped(f, project->address, 0) :
		     render_address(f, 0x3000, i)) < 0 ||
		    fputs("</span>\n"
			  "          <div class=\"entry-content\"><div class=\"entry-heading\"><h3>", f) == EOF ||
		    html_write_escaped(f, project->name, 0) < 0 ||
		    fputs("</h3></div>\n", f) == EOF)
			return -1;
		if (project_text &&
		    (fputs("            <p>", f) == EOF ||
		     html_write_escaped(f, project_text, 0) < 0 ||
		     fputs("</p>\n", f) == EOF))
			return -1;
		if (project->technology_count > 0) {
			if (fputs("            <p class=\"entry-technologies\">", f) == EOF)
				return -1;
			for (j = 0; j < project->technology_count; j++) {
				if (j && fputs(" <span aria-hidden=\"true\">·</span> ", f) == EOF)
					return -1;
				if (html_write_escaped(f, project->technologies[j], 0) < 0)
					return -1;
			}
			if (fputs("</p>\n", f) == EOF)
				return -1;
		}
		if (has_links) {
			if (fputs("            <div class=\"entry-links\">", f) == EOF)
				return -1;
			if (project->github &&
			    (fputs("<a href=\"", f) == EOF ||
			     html_write_escaped(f, project->github, 1) < 0 ||
			     fputs("\" target=\"_blank\" rel=\"noreferrer noopener\">repository ↗</a>", f) == EOF))
				return -1;
			if (project->blog &&
			    (fputs("<a href=\"", f) == EOF ||
			     html_write_escaped(f, project->blog, 1) < 0 ||
			     fputs("\">project notes ↗</a>", f) == EOF))
				return -1;
			if (project->demo &&
			    (fputs("<a href=\"", f) == EOF ||
			     html_write_escaped(f, project->demo, 1) < 0 ||
			     fputs("\" target=\"_blank\" rel=\"noreferrer noopener\">live demo ↗</a>", f) == EOF))
				return -1;
			for (j = 0; j < project->link_count; j++) {
				if (fputs("<a href=\"", f) == EOF ||
				    html_write_escaped(f, project->links[j].url, 1) < 0 ||
				    fputs("\" target=\"_blank\" rel=\"noreferrer noopener\">", f) == EOF ||
				    html_write_escaped(f, project->links[j].label, 0) < 0 ||
				    fputs(" ↗</a>", f) == EOF)
					return -1;
			}
			if (fputs("</div>\n", f) == EOF)
				return -1;
		}
		if (index_image_count > 0) {
			if (fputs("            <div class=\"project-gallery\">\n", f) == EOF)
				return -1;
			for (j = 0; j < project->image_count; j++) {
				const struct project_image *image = &project->images[j];
				const char *alt = image->alt ? image->alt : image->caption;
				if (!image->show_on_index)
					continue;
				if (fputs("              <figure class=\"project-image\">\n                ", f) == EOF)
					return -1;
				if (image->link) {
					if (fputs("<a class=\"project-figure-link\" href=\"", f) == EOF ||
					    html_write_escaped(f, image->link, 1) < 0 ||
					    fputs("\">", f) == EOF)
						return -1;
				} else if (fputs("<button class=\"project-figure-expand\" type=\"button\" data-lightbox-src=\"", f) == EOF ||
					   html_write_escaped(f, image->src, 1) < 0 ||
					   fputs("\" data-lightbox-caption=\"", f) == EOF ||
					   html_write_escaped(f, image->caption, 1) < 0 ||
					   fputs("\" aria-label=\"Expand image: ", f) == EOF ||
					   html_write_escaped(f, image->caption, 1) < 0 ||
					   fputs("\">", f) == EOF) {
					return -1;
				}
				if (fputs("<img src=\"", f) == EOF ||
				    html_write_escaped(f, image->src, 1) < 0 ||
				    fputs("\" alt=\"", f) == EOF ||
				    html_write_escaped(f, alt, 1) < 0 ||
				    fputs("\" loading=\"lazy\">", f) == EOF ||
				    fputs(image->link ? "</a>\n" : "</button>\n", f) == EOF ||
				    fputs("                <figcaption>", f) == EOF ||
				    html_write_escaped(f, image->caption, 0) < 0 ||
				    fputs("</figcaption>\n              </figure>\n", f) == EOF)
					return -1;
			}
			if (fputs("            </div>\n", f) == EOF)
				return -1;
		}
		if (fputs("          </div>\n        </article>\n", f) == EOF)
			return -1;
	}
	return fputs("      </div>\n    </section>\n", f) == EOF ? -1 : 0;
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
		  "  <script src=\"gallery.js\" defer></script>\n"
		  "</head>\n<body>\n"
		  "  <a class=\"skip-link\" href=\"#hero-content\">Skip to content</a>\n"
		  "  <header class=\"site-hero\">\n"
		  "    <nav class=\"site-nav\" aria-label=\"Primary navigation\">\n"
		  "      <a href=\"#about\">about</a>\n", f) == EOF)
		goto error;
	if (fputs("      <a href=\"#notes\">notes</a>\n"
		  "      <a href=\"#projects\">projects</a>\n", f) == EOF)
		goto error;

	if (personal->github &&
	    render_external_link(f, "nav-github", personal->github,
				 "github", " ↗") < 0)
		goto error;

	if (fputs("    </nav>\n"
		  "    <div class=\"memory-rail\" aria-hidden=\"true\">\n"
		  "      <span class=\"memory-rail-address\">0x00000000</span>\n"
		  "      <span class=\"memory-rail-node\">●</span>\n"
		  "      <span class=\"memory-rail-label\">&lt;_start&gt;</span>\n"
		  "    </div>\n"
		  "    <div class=\"hero-content\" id=\"hero-content\">\n"
		  "      <h1 class=\"hero-name\">", f) == EOF ||
	    html_write_escaped(f, personal->name, 0) < 0 ||
	    fputs("</h1>\n      <p class=\"hero-title\">", f) == EOF ||
	    html_write_escaped(f, hero_title(personal->title), 0) < 0 ||
	    fputs("</p>\n", f) == EOF)
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
		  "  <main>\n"
		  "    <section class=\"site-section about-section\" id=\"about\" aria-labelledby=\"about-title\">\n"
		  "      <div class=\"section-heading\">\n"
		  "        <span class=\"section-address\">0x00001000</span>\n"
		  "        <span class=\"section-node\" aria-hidden=\"true\">●</span>\n"
		  "        <h2 id=\"about-title\">.about</h2>\n"
		  "      </div>\n"
		  "      <div class=\"about-content\">\n"
		  "        <p class=\"about-summary\">", f) == EOF ||
	    html_write_escaped(f, personal->summary, 0) < 0 ||
	    fputs("</p>\n"
		  "      </div>\n"
		  "    </section>\n", f) == EOF ||
	    render_notes(f, profile) < 0 ||
	    render_projects(f, profile) < 0 ||
	    fputs("  </main>\n"
		  "  <dialog class=\"project-lightbox\" aria-label=\"Expanded project image\">\n"
		  "    <button class=\"lightbox-close\" type=\"button\" aria-label=\"Close image\">×</button>\n"
		  "    <figure><img src=\"\" alt=\"\"><figcaption></figcaption></figure>\n"
		  "  </dialog>\n"
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

int static_html_copy_asset_tree(const char *source_path, const char *output_path)
{
	DIR *directory;
	struct dirent *entry;
	int status = 0;

	if (!source_path || !output_path)
		return -1;
	if (mkdir(output_path, 0755) && errno != EEXIST)
		return -1;
	directory = opendir(source_path);
	if (!directory)
		return -1;
	while ((entry = readdir(directory)) != NULL) {
		char source[4096];
		char output[4096];
		struct stat info;

		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;
		if (snprintf(source, sizeof(source), "%s/%s", source_path,
			     entry->d_name) >= (int)sizeof(source) ||
		    snprintf(output, sizeof(output), "%s/%s", output_path,
			     entry->d_name) >= (int)sizeof(output) ||
		    stat(source, &info) != 0) {
			status = -1;
			break;
		}
		if (S_ISDIR(info.st_mode))
			status = static_html_copy_asset_tree(source, output);
		else if (S_ISREG(info.st_mode))
			status = static_html_copy_asset(source, output);
		if (status)
			break;
	}
	if (closedir(directory) != 0)
		status = -1;
	return status;
}
