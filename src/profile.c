#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "profile.h"
#include "toml-c.h"

static char *table_string(toml_table_t *table, const char *key)
{
	toml_value_t value;

	value = toml_table_string(table, key);
	if (!value.ok)
		return NULL;

	return value.u.s;
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

	profile->personal.name = table_string(personal, "name");
	profile->personal.email = table_string(personal, "email");
	profile->personal.phone = table_string(personal, "phone");
	profile->personal.linkedin = table_string(personal, "linkedin");
	profile->personal.github = table_string(personal, "github");
	profile->personal.website = table_string(personal, "website");

	toml_free(root);

	return 0;
}

void profile_free(struct profile *profile)
{
	free(profile->personal.name);
	free(profile->personal.email);
	free(profile->personal.phone);
	free(profile->personal.linkedin);
	free(profile->personal.github);
	free(profile->personal.website);

	memset(profile, 0, sizeof(*profile));
}