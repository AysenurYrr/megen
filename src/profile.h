#ifndef MEGEN_PROFILE_H
#define MEGEN_PROFILE_H

#include <stdbool.h>
#include <stddef.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

struct date {
	unsigned int month;
	unsigned int year;
};

struct date_range {
	struct date start;
	struct date end;
	bool ongoing;
};

struct link {
	char *label;
	char *url;
};

struct personal_info {
	char *name;
	char *email;
	char *phone;
	char *linkedin;
	char *github;
	char *website;
};

enum degree {
	DEGREE_BACHELOR,
	DEGREE_MASTER,
	DEGREE_PHD,
	DEGREE_OTHER,
};

struct education {
	char *institution;
	char *department;
	enum degree degree;
	struct date_range period;
	char *description;
};

struct academic_work {
	char *title;
	char *organization;

	char **technologies;
	size_t technology_count;

	struct date_range *periods;
	size_t period_count;

	char **highlights;
	size_t highlight_count;

	struct link *links;
	size_t link_count;
};

struct award {
	char *title;
	char *issuer;
	struct date date;

	char *description;

	struct link *links;
	size_t link_count;
};

struct volunteer_activity {
	char *organization;
	char *role;

	struct date_range period;

	char **highlights;
	size_t highlight_count;

	struct link *links;
	size_t link_count;
};

struct project {
	char *name;
	char *summary;
	char *description;

	char **technologies;
	size_t technology_count;

	char **highlights;
	size_t highlight_count;

	struct link *links;
	size_t link_count;
};

struct certificate {
	char *title;
	char *issuer;
	struct date date;

	char *description;

	struct link *links;
	size_t link_count;
};

struct profile {
	struct personal_info personal;

	struct education *education;
	size_t education_count;

	struct academic_work *academic_works;
	size_t academic_work_count;

	struct award *awards;
	size_t award_count;

	struct volunteer_activity *volunteer_activities;
	size_t volunteer_activity_count;

	struct project *projects;
	size_t project_count;

	struct certificate *certificates;
	size_t certificate_count;
};

int profile_load(struct profile *profile, const char *path);
void profile_dump(const struct profile *profile);
void profile_free(struct profile *profile);

#endif /* MEGEN_PROFILE_H */
