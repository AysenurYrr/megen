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
	const char *label;
	const char *url;
};

struct personal_info {
	const char *name;
	const char *email;
	const char *phone;
	const char *linkedin;
	const char *github;
	const char *website;
};

enum degree {
	DEGREE_BACHELOR,
	DEGREE_MASTER,
	DEGREE_PHD,
	DEGREE_OTHER,
};

struct education {
	const char *institution;
	const char *department;
	enum degree degree;

	struct date_range period;

	const char *description;
};

struct academic_work {
	const char *title;
	const char *organization;

	const char **technologies;
	size_t technology_count;

	struct date_range *periods;
	size_t period_count;

	const char **highlights;
	size_t highlight_count;

	struct link *links;
	size_t link_count;
};

struct award {
	const char *title;
	const char *issuer;
	struct date date;

	const char *description;

	struct link *links;
	size_t link_count;
};

struct volunteer_activity {
	const char *organization;
	const char *role;

	struct date_range period;

	const char **highlights;
	size_t highlight_count;

	struct link *links;
	size_t link_count;
};

struct project {
	const char *name;
	const char *summary;
	const char *description;

	const char **technologies;
	size_t technology_count;

	const char **highlights;
	size_t highlight_count;

	struct link *links;
	size_t link_count;
};

struct certificate {
	const char *title;
	const char *issuer;
	struct date date;

	const char *description;

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
void profile_free(struct profile *profile);

#endif /* MEGEN_PROFILE_H */
