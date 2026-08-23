#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "profile.h"
#include "toml-c.h"

/* toml-c.h poisons calloc for its implementation; do not leak that internally. */
#undef calloc

static int parse_project_media(toml_table_t *table,
			       struct project_media **result, size_t *count);

static int get_string(toml_table_t *table, const char *key, char **result)
{
	toml_value_t value;

	value = toml_table_string(table, key);
	*result = value.ok ? value.u.s : NULL;
	return 0;
}

static int get_required_string(toml_table_t *table, const char *key,
			       char **result)
{
	get_string(table, key, result);
	if (*result)
		return 0;

	fprintf(stderr, "megen: missing or invalid required string '%s'\n", key);
	return -1;
}

static void free_string_array(char **strings, size_t count)
{
	size_t i;

	for (i = 0; i < count; i++)
		free((void *)strings[i]);
	free(strings);
}

static int get_string_array(toml_table_t *table, const char *key,
			    char ***result, size_t *count)
{
	toml_array_t *array;
	char **strings;
	int length;
	int i;

	*result = NULL;
	*count = 0;
	array = toml_table_array(table, key);
	if (!array && (toml_table_table(table, key) ||
		       toml_table_unparsed(table, key))) {
		fprintf(stderr, "megen: '%s' must be an array of strings\n", key);
		return -1;
	}
	if (!array)
		return 0;

	length = toml_array_len(array);
	if (length <= 0)
		return 0;

	strings = calloc((size_t)length, sizeof(*strings));
	if (!strings)
		return -1;

	for (i = 0; i < length; i++) {
		toml_value_t value = toml_array_string(array, i);

		if (!value.ok) {
			fprintf(stderr, "megen: '%s' must be an array of strings\n", key);
			free_string_array(strings, (size_t)i);
			return -1;
		}
		strings[i] = value.u.s;
	}

	*result = strings;
	*count = (size_t)length;
	return 0;
}

static int parse_date(const char *str, struct date *date)
{
	unsigned int year;
	unsigned int month;
	char trailing;

	if (!str || strlen(str) != 7 || str[4] != '-' ||
	    !isdigit((unsigned char)str[0]) || !isdigit((unsigned char)str[1]) ||
	    !isdigit((unsigned char)str[2]) || !isdigit((unsigned char)str[3]) ||
	    !isdigit((unsigned char)str[5]) || !isdigit((unsigned char)str[6]) ||
	    sscanf(str, "%4u-%2u%c", &year, &month, &trailing) != 2 ||
	    year == 0 || month < 1 || month > 12)
		return -1;

	date->year = year;
	date->month = month;
	return 0;
}

static int parse_date_range(toml_table_t *table, struct date_range *range)
{
	char *start = NULL;
	char *end = NULL;
	int status = -1;

	memset(range, 0, sizeof(*range));
	if (get_required_string(table, "start", &start))
		goto out;
	get_string(table, "end", &end);

	if (parse_date(start, &range->start)) {
		fprintf(stderr, "megen: invalid date '%s' for 'start' (expected YYYY-MM)\n",
			start);
		goto out;
	}

	if (!end) {
		range->ongoing = true;
		status = 0;
		goto out;
	}

	if (parse_date(end, &range->end)) {
		fprintf(stderr, "megen: invalid date '%s' for 'end' (expected YYYY-MM)\n",
			end);
		goto out;
	}

	status = 0;
out:
	free((void *)start);
	free((void *)end);
	return status;
}

static int parse_year_or_date(const char *value, struct date *date)
{
	memset(date, 0, sizeof(*date));
	if (strlen(value) == 4 && isdigit((unsigned char)value[0]) &&
	    isdigit((unsigned char)value[1]) &&
	    isdigit((unsigned char)value[2]) &&
	    isdigit((unsigned char)value[3])) {
		date->year = (unsigned int)strtoul(value, NULL, 10);
		return date->year > 0 ? 0 : -1;
	}
	return parse_date(value, date);
}

static int parse_year_or_date_field(toml_table_t *table, const char *key,
				    struct date *date)
{
	char *value = NULL;
	int status = -1;

	memset(date, 0, sizeof(*date));
	if (get_required_string(table, key, &value))
		return -1;

	if (parse_year_or_date(value, date) == 0) {
		status = 0;
	} else {
		fprintf(stderr,
			"megen: invalid date '%s' for '%s' (expected YYYY or YYYY-MM)\n",
			value, key);
	}

	free((void *)value);
	return status;
}

static int parse_certificate_period(toml_table_t *table,
				    struct date_range *period)
{
	char *date = NULL;
	char *start = NULL;
	char *end = NULL;
	int status = -1;

	memset(period, 0, sizeof(*period));
	get_string(table, "date", &date);
	get_string(table, "start", &start);
	get_string(table, "end", &end);

	if (date) {
		if (start || end) {
			fprintf(stderr,
				"megen: certificate must use either 'date' or 'start'/'end'\n");
			goto out;
		}
		if (parse_year_or_date(date, &period->start) == 0) {
			period->end = period->start;
			status = 0;
		} else {
			fprintf(stderr,
				"megen: invalid certificate date '%s' (expected YYYY or YYYY-MM)\n",
				date);
		}
		goto out;
	}

	if (!start || !end) {
		fprintf(stderr,
			"megen: certificate requires 'date' or both 'start' and 'end'\n");
		goto out;
	}
	if (parse_year_or_date(start, &period->start) != 0 ||
	    parse_year_or_date(end, &period->end) != 0) {
		fprintf(stderr,
			"megen: invalid certificate period (expected YYYY or YYYY-MM)\n");
		goto out;
	}

	status = 0;
out:
	free((void *)date);
	free((void *)start);
	free((void *)end);
	return status;
}

static int parse_flexible_date_range(toml_table_t *table,
				     struct date_range *range)
{
	char *start = NULL;
	char *end = NULL;
	int status = -1;

	memset(range, 0, sizeof(*range));
	if (get_required_string(table, "start", &start))
		return -1;
	get_string(table, "end", &end);

	if (parse_year_or_date(start, &range->start) != 0) {
		fprintf(stderr,
			"megen: invalid research start '%s' (expected YYYY or YYYY-MM)\n",
			start);
		goto out;
	}
	if (!end) {
		range->ongoing = true;
		status = 0;
		goto out;
	}
	if (parse_year_or_date(end, &range->end) != 0) {
		fprintf(stderr,
			"megen: invalid research end '%s' (expected YYYY or YYYY-MM)\n",
			end);
		goto out;
	}

	status = 0;
out:
	free((void *)start);
	free((void *)end);
	return status;
}

static void free_links(struct link *links, size_t count)
{
	size_t i;

	for (i = 0; i < count; i++) {
		free((void *)links[i].label);
		free((void *)links[i].url);
	}
	free(links);
}

static void free_project_media(struct project_media *media, size_t count)
{
	size_t i;

	for (i = 0; i < count; i++) {
		free((void *)media[i].src);
		free((void *)media[i].caption);
		free((void *)media[i].alt);
		free((void *)media[i].link);
		free((void *)media[i].poster);
	}
	free(media);
}

static int parse_links(toml_table_t *parent, struct link **result,
		       size_t *count)
{
	toml_array_t *array;
	struct link *links;
	int length;
	int i;

	*result = NULL;
	*count = 0;
	array = toml_table_array(parent, "links");
	if (!array && (toml_table_table(parent, "links") ||
		       toml_table_unparsed(parent, "links"))) {
		fprintf(stderr, "megen: 'links' must be an array of tables\n");
		return -1;
	}
	if (!array)
		return 0;

	length = toml_array_len(array);
	if (length <= 0)
		return 0;

	links = calloc((size_t)length, sizeof(*links));
	if (!links)
		return -1;

	for (i = 0; i < length; i++) {
		toml_table_t *table = toml_array_table(array, i);

		if (!table) {
			fprintf(stderr, "megen: link entry %d must be a table\n", i + 1);
			goto fail;
		}

		*count = (size_t)i + 1;
		if (get_required_string(table, "label", &links[i].label) ||
		    get_required_string(table, "url", &links[i].url))
			goto fail;
	}

	*result = links;
	return 0;

fail:
	free_links(links, *count);
	*count = 0;
	return -1;
}

static enum degree parse_degree(const char *str)
{
	if (!strcmp(str, "bachelor"))
		return DEGREE_BACHELOR;
	if (!strcmp(str, "master"))
		return DEGREE_MASTER;
	if (!strcmp(str, "phd"))
		return DEGREE_PHD;
	return DEGREE_OTHER;
}

static int parse_project_category(const char *value,
				  enum project_category *category)
{
	if (!strcmp(value, "low_level"))
		*category = PROJECT_CATEGORY_LOW_LEVEL;
	else if (!strcmp(value, "systems"))
		*category = PROJECT_CATEGORY_SYSTEMS;
	else if (!strcmp(value, "robotics"))
		*category = PROJECT_CATEGORY_ROBOTICS;
	else if (!strcmp(value, "ai_ml"))
		*category = PROJECT_CATEGORY_AI_ML;
	else if (!strcmp(value, "web"))
		*category = PROJECT_CATEGORY_WEB;
	else if (!strcmp(value, "other"))
		*category = PROJECT_CATEGORY_OTHER;
	else {
		fprintf(stderr, "megen: unknown project category '%s'\n", value);
		return -1;
	}
	return 0;
}

static int parse_education(toml_table_t *root, struct profile *profile)
{
	toml_array_t *array;
	int length;
	int i;

	array = toml_table_array(root, "education");
	if (!array)
		return 0;

	length = toml_array_len(array);
	if (length <= 0)
		return 0;

	profile->education = calloc((size_t)length, sizeof(*profile->education));
	if (!profile->education)
		return -1;

	for (i = 0; i < length; i++) {
		struct education *education = &profile->education[i];
		toml_table_t *table = toml_array_table(array, i);
		char *degree = NULL;

		if (!table) {
			fprintf(stderr, "megen: education entry %d must be a table\n", i + 1);
			return -1;
		}
		/* Include the current, possibly partial entry in failure cleanup. */
		profile->education_count = (size_t)i + 1;

		if (get_required_string(table, "institution", &education->institution) ||
		    get_required_string(table, "department", &education->department) ||
		    get_required_string(table, "degree", &degree) ||
		    parse_date_range(table, &education->period)) {
			free((void *)degree);
			return -1;
		}

		education->degree = parse_degree(degree);
		free((void *)degree);
		get_string(table, "description", &education->description);
	}

	return 0;
}

static int parse_awards(toml_table_t *root, struct profile *profile)
{
	toml_array_t *array;
	int length;
	int i;

	array = toml_table_array(root, "award");
	if (!array)
		return 0;

	length = toml_array_len(array);
	if (length <= 0)
		return 0;

	profile->awards = calloc((size_t)length, sizeof(*profile->awards));
	if (!profile->awards)
		return -1;

	for (i = 0; i < length; i++) {
		struct award *award = &profile->awards[i];
		toml_table_t *table = toml_array_table(array, i);
		toml_value_t show_in_website_projects;

		if (!table) {
			fprintf(stderr, "megen: award entry %d must be a table\n", i + 1);
			return -1;
		}

		profile->award_count = (size_t)i + 1;
		if (get_required_string(table, "title", &award->title) ||
		    get_required_string(table, "issuer", &award->issuer) ||
		    parse_year_or_date_field(table, "date", &award->date) ||
		    get_string_array(table, "highlights", &award->highlights,
				     &award->highlight_count) ||
		    parse_links(table, &award->links, &award->link_count) ||
		    parse_project_media(table, &award->media,
					&award->media_count))
			return -1;
		show_in_website_projects =
			toml_table_bool(table, "show_in_website_projects");
		award->show_in_website_projects =
			show_in_website_projects.ok && show_in_website_projects.u.b;
	}

	return 0;
}

static int parse_experiences(toml_table_t *root, struct profile *profile)
{
	toml_array_t *array;
	int length;
	int i;

	array = toml_table_array(root, "experience");
	if (!array && (toml_table_table(root, "experience") ||
		       toml_table_unparsed(root, "experience"))) {
		fprintf(stderr, "megen: 'experience' must be an array of tables\n");
		return -1;
	}
	if (!array)
		return 0;

	length = toml_array_len(array);
	if (length <= 0)
		return 0;

	profile->experiences = calloc((size_t)length,
				      sizeof(*profile->experiences));
	if (!profile->experiences)
		return -1;

	for (i = 0; i < length; i++) {
		struct experience *experience = &profile->experiences[i];
		toml_table_t *table = toml_array_table(array, i);

		if (!table) {
			fprintf(stderr, "megen: experience entry %d must be a table\n",
				i + 1);
			return -1;
		}

		profile->experience_count = (size_t)i + 1;
		if (get_required_string(table, "company", &experience->company) ||
		    get_required_string(table, "title", &experience->title) ||
		    parse_date_range(table, &experience->period) ||
		    get_string_array(table, "highlights", &experience->highlights,
				     &experience->highlight_count))
			return -1;

		get_string(table, "location", &experience->location);
		get_string(table, "employment_type", &experience->employment_type);
	}

	return 0;
}

static int parse_certificates(toml_table_t *root, struct profile *profile)
{
	toml_array_t *array;
	int length;
	int i;

	array = toml_table_array(root, "certificate");
	if (!array)
		return 0;

	length = toml_array_len(array);
	if (length <= 0)
		return 0;

	profile->certificates = calloc((size_t)length, sizeof(*profile->certificates));
	if (!profile->certificates)
		return -1;

	for (i = 0; i < length; i++) {
		struct certificate *certificate = &profile->certificates[i];
		toml_table_t *table = toml_array_table(array, i);
		toml_value_t show_in_cv;

		if (!table) {
			fprintf(stderr, "megen: certificate entry %d must be a table\n",
				i + 1);
			return -1;
		}

		profile->certificate_count = (size_t)i + 1;
		if (get_required_string(table, "title", &certificate->title) ||
		    get_required_string(table, "issuer", &certificate->issuer) ||
		    parse_certificate_period(table, &certificate->period) ||
		    parse_links(table, &certificate->links,
				&certificate->link_count))
			return -1;

		show_in_cv = toml_table_bool(table, "show_in_cv");
		if (!show_in_cv.ok) {
			fprintf(stderr,
				"megen: certificate entry %d requires boolean 'show_in_cv'\n",
				i + 1);
			return -1;
		}
		certificate->show_in_cv = show_in_cv.u.b;

		get_string(table, "description", &certificate->description);
	}

	return 0;
}

static int parse_project_media(toml_table_t *table,
			       struct project_media **result, size_t *count)
{
	toml_array_t *array = toml_table_array(table, "media");
	int length;
	int i;

	*result = NULL;
	*count = 0;
	if (toml_table_array(table, "images") ||
	    toml_table_table(table, "images") ||
	    toml_table_unparsed(table, "images")) {
		fprintf(stderr,
			"megen: project images are not supported; use media entries\n");
		return -1;
	}
	if (!array && (toml_table_table(table, "media") ||
		       toml_table_unparsed(table, "media"))) {
		fprintf(stderr, "megen: project media must be an array of tables\n");
		return -1;
	}
	length = array ? toml_array_len(array) : 0;
	if (length <= 0)
		return 0;
	*result = calloc((size_t)length, sizeof(**result));
	if (!*result)
		return -1;

	for (i = 0; i < length; i++) {
		struct project_media *media = &(*result)[i];
		toml_table_t *media_table = toml_array_table(array, i);
		toml_value_t selected;
		char *type = NULL;

		*count = (size_t)i + 1;
		if (!media_table ||
		    get_required_string(media_table, "src", &media->src) ||
		    get_required_string(media_table, "caption", &media->caption))
			return -1;
		get_string(media_table, "type", &type);
		if (!type || (strcmp(type, "image") && strcmp(type, "video"))) {
			fprintf(stderr,
				"megen: project media type must be 'image' or 'video'\n");
			free((void *)type);
			return -1;
		}
		media->type = !strcmp(type, "video") ?
			PROJECT_MEDIA_VIDEO : PROJECT_MEDIA_IMAGE;
		free((void *)type);
		get_string(media_table, "alt", &media->alt);
		get_string(media_table, "link", &media->link);
		get_string(media_table, "poster", &media->poster);
		selected = toml_table_bool(media_table, "show_on_index");
		media->show_on_index = selected.ok && selected.u.b;
	}
	return 0;
}

static int parse_projects(toml_table_t *root, struct profile *profile)
{
	toml_array_t *array;
	int plural_schema = 0;
	int length;
	int i;

	array = toml_table_array(root, "project");
	if (!array) {
		array = toml_table_array(root, "projects");
		plural_schema = array != NULL;
	}
	if (toml_table_array(root, "project") &&
	    toml_table_array(root, "projects")) {
		fprintf(stderr, "megen: use either 'project' or 'projects', not both\n");
		return -1;
	}
	if (!array && (toml_table_table(root, "project") ||
		       toml_table_unparsed(root, "project") ||
		       toml_table_table(root, "projects") ||
		       toml_table_unparsed(root, "projects"))) {
		fprintf(stderr, "megen: projects must be an array of tables\n");
		return -1;
	}
	if (!array)
		return 0;

	length = toml_array_len(array);
	if (length <= 0)
		return 0;

	profile->projects = calloc((size_t)length, sizeof(*profile->projects));
	if (!profile->projects)
		return -1;

	for (i = 0; i < length; i++) {
		struct project *project = &profile->projects[i];
		toml_table_t *table = toml_array_table(array, i);
		toml_value_t show_in_cv;
		char *category = NULL;

		if (!table) {
			fprintf(stderr, "megen: project entry %d must be a table\n", i + 1);
			return -1;
		}

		profile->project_count = (size_t)i + 1;
		if (plural_schema)
			get_string(table, "title", &project->name);
		else
			get_string(table, "name", &project->name);
		if (!project->name) {
			fprintf(stderr, "megen: missing or invalid required string '%s'\n",
				plural_schema ? "title" : "name");
			return -1;
		}
		get_string(table, "category", &category);
		project->category = PROJECT_CATEGORY_OTHER;
		if (category && parse_project_category(category, &project->category)) {
			free((void *)category);
			return -1;
		}
		free((void *)category);

		show_in_cv = toml_table_bool(table, "show_in_cv");
		if (!show_in_cv.ok && !plural_schema) {
			fprintf(stderr,
				"megen: project entry %d requires boolean 'show_in_cv'\n",
				i + 1);
			return -1;
		}
		project->show_in_cv = show_in_cv.ok && show_in_cv.u.b;

		get_string(table, "address", &project->address);
		get_string(table, "summary", &project->summary);
		get_string(table, "description", &project->description);
		get_string(table, "github", &project->github);
		get_string(table, "blog", &project->blog);
		get_string(table, "demo", &project->demo);
		get_string(table, "video", &project->video);
		if (get_string_array(table, plural_schema ? "tech" : "technologies",
				     &project->technologies,
				     &project->technology_count) ||
		    get_string_array(table, "highlights", &project->highlights,
				     &project->highlight_count) ||
		    parse_links(table, &project->links, &project->link_count) ||
		    parse_project_media(table, &project->media,
					&project->media_count))
			return -1;
	}

	return 0;
}

static int parse_notes(toml_table_t *root, struct profile *profile)
{
	toml_array_t *array;
	int length;
	int i;

	array = toml_table_array(root, "note");
	if (!array && (toml_table_table(root, "note") ||
		       toml_table_unparsed(root, "note"))) {
		fprintf(stderr, "megen: 'note' must be an array of tables\n");
		return -1;
	}
	if (!array)
		return 0;

	length = toml_array_len(array);
	if (length <= 0)
		return 0;
	profile->notes = calloc((size_t)length, sizeof(*profile->notes));
	if (!profile->notes)
		return -1;

	for (i = 0; i < length; i++) {
		struct note *note = &profile->notes[i];
		toml_table_t *table = toml_array_table(array, i);

		if (!table) {
			fprintf(stderr, "megen: note entry %d must be a table\n", i + 1);
			return -1;
		}
		profile->note_count = (size_t)i + 1;
		if (get_required_string(table, "title", &note->title) ||
		    get_required_string(table, "category", &note->category) ||
		    get_required_string(table, "summary", &note->summary) ||
		    get_required_string(table, "url", &note->url))
			return -1;
	}

	return 0;
}

static int parse_research_projects(toml_table_t *root, struct profile *profile)
{
	toml_array_t *array;
	int length;
	int i;

	array = toml_table_array(root, "research_project");
	if (!array && (toml_table_table(root, "research_project") ||
		       toml_table_unparsed(root, "research_project"))) {
		fprintf(stderr, "megen: 'research_project' must be an array of tables\n");
		return -1;
	}
	if (!array)
		return 0;

	length = toml_array_len(array);
	if (length <= 0)
		return 0;

	profile->research_projects =
		calloc((size_t)length, sizeof(*profile->research_projects));
	if (!profile->research_projects)
		return -1;

	for (i = 0; i < length; i++) {
		struct research_project *project = &profile->research_projects[i];
		toml_table_t *table = toml_array_table(array, i);
		toml_value_t show_in_website_projects;

		if (!table) {
			fprintf(stderr, "megen: research_project entry %d must be a table\n",
				i + 1);
			return -1;
		}

		profile->research_project_count = (size_t)i + 1;
		if (get_required_string(table, "title", &project->title) ||
		    get_required_string(table, "organization", &project->organization) ||
		    parse_flexible_date_range(table, &project->period) ||
		    get_string_array(table, "sponsors", &project->sponsors,
				     &project->sponsor_count) ||
		    get_string_array(table, "technologies", &project->technologies,
				     &project->technology_count) ||
		    get_string_array(table, "highlights", &project->highlights,
				     &project->highlight_count) ||
		    parse_links(table, &project->links, &project->link_count) ||
		    parse_project_media(table, &project->media,
					&project->media_count))
			return -1;

		get_string(table, "role", &project->role);
		get_string(table, "video", &project->video);
		get_string(table, "description", &project->description);
		show_in_website_projects =
			toml_table_bool(table, "show_in_website_projects");
		project->show_in_website_projects =
			show_in_website_projects.ok && show_in_website_projects.u.b;
	}

	return 0;
}

int profile_load(struct profile *profile, const char *path)
{
	FILE *file;
	toml_table_t *root;
	toml_table_t *personal;
	char errbuf[256];

	memset(profile, 0, sizeof(*profile));

	file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "megen: failed to open %s\n", path);
		return -1;
	}

	root = toml_parse_file(file, errbuf, sizeof(errbuf));
	fclose(file);

	if (!root) {
		fprintf(stderr, "megen: %s\n", errbuf);
		return -1;
	}

	personal = toml_table_table(root, "personal");
	if (!personal) {
		fprintf(stderr, "megen: missing [personal] table\n");
		toml_free(root);
		return -1;
	}

	get_string(personal, "name", &profile->personal.name);
	get_string(personal, "title", &profile->personal.title);
	get_string(personal, "location", &profile->personal.location);
	get_string(personal, "email", &profile->personal.email);
	get_string(personal, "phone", &profile->personal.phone);
	get_string(personal, "linkedin", &profile->personal.linkedin);
	get_string(personal, "github", &profile->personal.github);
	get_string(personal, "website", &profile->personal.website);
	get_string(personal, "summary", &profile->personal.summary);
	if (get_string_array(personal, "skills", &profile->personal.skills,
			     &profile->personal.skill_count)) {
		toml_free(root);
		profile_free(profile);
		return -1;
	}

	if (parse_education(root, profile) ||
	    parse_experiences(root, profile) ||
	    parse_awards(root, profile) ||
	    parse_certificates(root, profile) ||
	    parse_notes(root, profile) ||
	    parse_projects(root, profile) ||
	    parse_research_projects(root, profile)) {
		toml_free(root);
		profile_free(profile);
		return -1;
	}

	toml_free(root);

	return 0;
}

void profile_free(struct profile *profile)
{
	size_t i;

	free((void *)profile->personal.name);
	free((void *)profile->personal.title);
	free((void *)profile->personal.location);
	free((void *)profile->personal.email);
	free((void *)profile->personal.phone);
	free((void *)profile->personal.linkedin);
	free((void *)profile->personal.github);
	free((void *)profile->personal.website);
	free((void *)profile->personal.summary);
	free_string_array(profile->personal.skills, profile->personal.skill_count);

	for (i = 0; i < profile->education_count; i++) {
		free((void *)profile->education[i].institution);
		free((void *)profile->education[i].department);
		free((void *)profile->education[i].description);
	}
	free(profile->education);

	for (i = 0; i < profile->experience_count; i++) {
		struct experience *experience = &profile->experiences[i];

		free((void *)experience->company);
		free((void *)experience->title);
		free((void *)experience->location);
		free((void *)experience->employment_type);
		free_string_array(experience->highlights,
				  experience->highlight_count);
	}
	free(profile->experiences);

	for (i = 0; i < profile->award_count; i++) {
		free((void *)profile->awards[i].title);
		free((void *)profile->awards[i].issuer);
		free_string_array(profile->awards[i].highlights,
				  profile->awards[i].highlight_count);
		free_links(profile->awards[i].links, profile->awards[i].link_count);
		free_project_media(profile->awards[i].media,
				   profile->awards[i].media_count);
	}
	free(profile->awards);

	for (i = 0; i < profile->certificate_count; i++) {
		free((void *)profile->certificates[i].title);
		free((void *)profile->certificates[i].issuer);
		free((void *)profile->certificates[i].description);
		free_links(profile->certificates[i].links,
			   profile->certificates[i].link_count);
	}
	free(profile->certificates);

	for (i = 0; i < profile->note_count; i++) {
		free((void *)profile->notes[i].title);
		free((void *)profile->notes[i].category);
		free((void *)profile->notes[i].summary);
		free((void *)profile->notes[i].url);
	}
	free(profile->notes);

	for (i = 0; i < profile->project_count; i++) {
		struct project *project = &profile->projects[i];

		free((void *)project->address);
		free((void *)project->name);
		free((void *)project->summary);
		free((void *)project->description);
		free_string_array(project->technologies, project->technology_count);
		free_string_array(project->highlights, project->highlight_count);
		free_links(project->links, project->link_count);
		free((void *)project->github);
		free((void *)project->blog);
		free((void *)project->demo);
		free((void *)project->video);
		free_project_media(project->media, project->media_count);
	}
	free(profile->projects);

	for (i = 0; i < profile->research_project_count; i++) {
		struct research_project *project = &profile->research_projects[i];

		free((void *)project->title);
		free((void *)project->organization);
		free((void *)project->role);
		free((void *)project->video);
		free((void *)project->description);
		free_string_array(project->sponsors, project->sponsor_count);
		free_string_array(project->technologies, project->technology_count);
		free_string_array(project->highlights, project->highlight_count);
		free_links(project->links, project->link_count);
		free_project_media(project->media, project->media_count);
	}
	free(profile->research_projects);

	memset(profile, 0, sizeof(*profile));
}
