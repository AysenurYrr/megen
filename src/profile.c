#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "profile.h"
#include "toml-c.h"

/* toml-c.h poisons calloc for its implementation; do not leak that internally. */
#undef calloc

static int get_string(toml_table_t *table, const char *key, const char **result)
{
	toml_value_t value;

	value = toml_table_string(table, key);
	*result = value.ok ? value.u.s : NULL;
	return 0;
}

static int get_required_string(toml_table_t *table, const char *key,
			       const char **result)
{
	get_string(table, key, result);
	if (*result)
		return 0;

	fprintf(stderr, "megen: missing or invalid required string '%s'\n", key);
	return -1;
}

static int __attribute__((unused))
get_string_array(toml_table_t *table, const char *key,
		 const char ***result, size_t *count)
{
	toml_array_t *array;
	const char **strings;
	int length;
	int i;

	*result = NULL;
	*count = 0;
	array = toml_table_array(table, key);
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
			while (i-- > 0)
				free((void *)strings[i]);
			free(strings);
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
	const char *start = NULL;
	const char *end = NULL;
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
		const char *degree = NULL;

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

	if (parse_education(root, profile)) {
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

	memset(profile, 0, sizeof(*profile));
}
