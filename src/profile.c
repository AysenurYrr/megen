#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "profile.h"
#include "toml-c.h"

/* toml-c.h poisons calloc for its implementation; do not leak that internally. */
#undef calloc

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

static int parse_date_ranges(toml_table_t *parent, const char *key,
			     struct date_range **result, size_t *count)
{
	toml_array_t *array;
	struct date_range *ranges;
	int length;
	int i;

	*result = NULL;
	*count = 0;
	array = toml_table_array(parent, key);
	if (!array && (toml_table_table(parent, key) ||
		       toml_table_unparsed(parent, key))) {
		fprintf(stderr, "megen: '%s' must be an array of tables\n", key);
		return -1;
	}
	if (!array)
		return 0;

	length = toml_array_len(array);
	if (length <= 0)
		return 0;

	ranges = calloc((size_t)length, sizeof(*ranges));
	if (!ranges)
		return -1;

	for (i = 0; i < length; i++) {
		toml_table_t *table = toml_array_table(array, i);

		if (!table) {
			fprintf(stderr, "megen: %s entry %d must be a table\n",
				key, i + 1);
			free(ranges);
			return -1;
		}
		if (parse_date_range(table, &ranges[i])) {
			free(ranges);
			return -1;
		}
	}

	*result = ranges;
	*count = (size_t)length;
	return 0;
}

static int parse_date_field(toml_table_t *table, const char *key,
			    struct date *date)
{
	char *value = NULL;
	int status = -1;

	memset(date, 0, sizeof(*date));
	if (get_required_string(table, key, &value))
		return -1;

	if (parse_date(value, date))
		fprintf(stderr, "megen: invalid date '%s' for '%s' (expected YYYY-MM)\n",
			value, key);
	else
		status = 0;

	free((void *)value);
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

		if (!table) {
			fprintf(stderr, "megen: award entry %d must be a table\n", i + 1);
			return -1;
		}

		profile->award_count = (size_t)i + 1;
		if (get_required_string(table, "title", &award->title) ||
		    get_required_string(table, "issuer", &award->issuer) ||
		    parse_date_field(table, "date", &award->date) ||
		    parse_links(table, &award->links, &award->link_count))
			return -1;

		get_string(table, "description", &award->description);
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

		if (!table) {
			fprintf(stderr, "megen: certificate entry %d must be a table\n",
				i + 1);
			return -1;
		}

		profile->certificate_count = (size_t)i + 1;
		if (get_required_string(table, "title", &certificate->title) ||
		    get_required_string(table, "issuer", &certificate->issuer) ||
		    parse_date_field(table, "date", &certificate->date) ||
		    parse_links(table, &certificate->links,
				&certificate->link_count))
			return -1;

		get_string(table, "description", &certificate->description);
	}

	return 0;
}

static int parse_volunteer_activities(toml_table_t *root,
				      struct profile *profile)
{
	toml_array_t *array;
	int length;
	int i;

	array = toml_table_array(root, "volunteer_activity");
	if (!array && (toml_table_table(root, "volunteer_activity") ||
		       toml_table_unparsed(root, "volunteer_activity"))) {
		fprintf(stderr, "megen: 'volunteer_activity' must be an array of tables\n");
		return -1;
	}
	if (!array)
		return 0;

	length = toml_array_len(array);
	if (length <= 0)
		return 0;

	profile->volunteer_activities =
		calloc((size_t)length, sizeof(*profile->volunteer_activities));
	if (!profile->volunteer_activities)
		return -1;

	for (i = 0; i < length; i++) {
		struct volunteer_activity *activity =
			&profile->volunteer_activities[i];
		toml_table_t *table = toml_array_table(array, i);

		if (!table) {
			fprintf(stderr,
				"megen: volunteer_activity entry %d must be a table\n",
				i + 1);
			return -1;
		}

		profile->volunteer_activity_count = (size_t)i + 1;
		if (get_required_string(table, "organization", &activity->organization) ||
		    get_required_string(table, "role", &activity->role) ||
		    parse_date_range(table, &activity->period) ||
		    get_string_array(table, "highlights", &activity->highlights,
				     &activity->highlight_count) ||
		    parse_links(table, &activity->links, &activity->link_count))
			return -1;
	}

	return 0;
}

static int parse_projects(toml_table_t *root, struct profile *profile)
{
	toml_array_t *array;
	int length;
	int i;

	array = toml_table_array(root, "project");
	if (!array && (toml_table_table(root, "project") ||
		       toml_table_unparsed(root, "project"))) {
		fprintf(stderr, "megen: 'project' must be an array of tables\n");
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

		if (!table) {
			fprintf(stderr, "megen: project entry %d must be a table\n", i + 1);
			return -1;
		}

		profile->project_count = (size_t)i + 1;
		if (get_required_string(table, "name", &project->name) ||
		    get_required_string(table, "summary", &project->summary))
			return -1;

		get_string(table, "description", &project->description);
		if (get_string_array(table, "technologies", &project->technologies,
				     &project->technology_count) ||
		    get_string_array(table, "highlights", &project->highlights,
				     &project->highlight_count) ||
		    parse_links(table, &project->links, &project->link_count))
			return -1;
	}

	return 0;
}

static int parse_academic_works(toml_table_t *root, struct profile *profile)
{
	toml_array_t *array;
	int length;
	int i;

	array = toml_table_array(root, "academic_work");
	if (!array && (toml_table_table(root, "academic_work") ||
		       toml_table_unparsed(root, "academic_work"))) {
		fprintf(stderr, "megen: 'academic_work' must be an array of tables\n");
		return -1;
	}
	if (!array)
		return 0;

	length = toml_array_len(array);
	if (length <= 0)
		return 0;

	profile->academic_works =
		calloc((size_t)length, sizeof(*profile->academic_works));
	if (!profile->academic_works)
		return -1;

	for (i = 0; i < length; i++) {
		struct academic_work *work = &profile->academic_works[i];
		toml_table_t *table = toml_array_table(array, i);

		if (!table) {
			fprintf(stderr, "megen: academic_work entry %d must be a table\n",
				i + 1);
			return -1;
		}

		profile->academic_work_count = (size_t)i + 1;
		if (get_required_string(table, "title", &work->title) ||
		    get_required_string(table, "organization", &work->organization) ||
		    get_string_array(table, "technologies", &work->technologies,
				     &work->technology_count) ||
		    get_string_array(table, "highlights", &work->highlights,
				     &work->highlight_count) ||
		    parse_links(table, &work->links, &work->link_count) ||
		    parse_date_ranges(table, "periods", &work->periods,
				      &work->period_count))
			return -1;

		if (work->period_count == 0) {
			fprintf(stderr,
				"megen: academic_work entry %d requires at least one period\n",
				 i + 1);
			return -1;
		}
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
	get_string(personal, "email", &profile->personal.email);
	get_string(personal, "phone", &profile->personal.phone);
	get_string(personal, "linkedin", &profile->personal.linkedin);
	get_string(personal, "github", &profile->personal.github);
	get_string(personal, "website", &profile->personal.website);

	if (parse_education(root, profile) ||
	    parse_awards(root, profile) ||
	    parse_certificates(root, profile) ||
	    parse_volunteer_activities(root, profile) ||
	    parse_projects(root, profile) ||
	    parse_academic_works(root, profile)) {
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
	free((void *)profile->personal.email);
	free((void *)profile->personal.phone);
	free((void *)profile->personal.linkedin);
	free((void *)profile->personal.github);
	free((void *)profile->personal.website);

	for (i = 0; i < profile->education_count; i++) {
		free((void *)profile->education[i].institution);
		free((void *)profile->education[i].department);
		free((void *)profile->education[i].description);
	}
	free(profile->education);

	for (i = 0; i < profile->award_count; i++) {
		free((void *)profile->awards[i].title);
		free((void *)profile->awards[i].issuer);
		free((void *)profile->awards[i].description);
		free_links(profile->awards[i].links, profile->awards[i].link_count);
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

	for (i = 0; i < profile->volunteer_activity_count; i++) {
		struct volunteer_activity *activity =
			&profile->volunteer_activities[i];

		free((void *)activity->organization);
		free((void *)activity->role);
		free_string_array(activity->highlights, activity->highlight_count);
		free_links(activity->links, activity->link_count);
	}
	free(profile->volunteer_activities);

	for (i = 0; i < profile->project_count; i++) {
		struct project *project = &profile->projects[i];

		free((void *)project->name);
		free((void *)project->summary);
		free((void *)project->description);
		free_string_array(project->technologies, project->technology_count);
		free_string_array(project->highlights, project->highlight_count);
		free_links(project->links, project->link_count);
	}
	free(profile->projects);

	for (i = 0; i < profile->academic_work_count; i++) {
		struct academic_work *work = &profile->academic_works[i];

		free((void *)work->title);
		free((void *)work->organization);
		free_string_array(work->technologies, work->technology_count);
		free(work->periods);
		free_string_array(work->highlights, work->highlight_count);
		free_links(work->links, work->link_count);
	}
	free(profile->academic_works);

	memset(profile, 0, sizeof(*profile));
}
