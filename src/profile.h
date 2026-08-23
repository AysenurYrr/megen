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

struct project_media;

struct personal_info {
	char *name;
	char *nickname;
	char *title;
	char *location;
	char *email;
	char *phone;
	char *linkedin;
	char *github;
	char *website;
	char *summary;
	char **skills;
	size_t skill_count;
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

struct experience {
	char *company;
	char *title;
	char *location;
	char *employment_type;
	struct date_range period;
	char **highlights;
	size_t highlight_count;
};

struct research_project {
	char *title;
	char *organization;
	char *role;
	char *video;
	char *description;

	char **sponsors;
	size_t sponsor_count;
	char **technologies;
	size_t technology_count;

	struct date_range period;

	char **highlights;
	size_t highlight_count;

	struct link *links;
	size_t link_count;
	struct project_media *media;
	size_t media_count;
	bool show_in_website_projects;
};

struct award {
	char *title;
	char *issuer;
	struct date date;

	char **highlights;
	size_t highlight_count;

	struct link *links;
	size_t link_count;
	struct project_media *media;
	size_t media_count;
	bool show_in_website_projects;
};

enum project_category {
	PROJECT_CATEGORY_LOW_LEVEL,
	PROJECT_CATEGORY_SYSTEMS,
	PROJECT_CATEGORY_ROBOTICS,
	PROJECT_CATEGORY_AI_ML,
	PROJECT_CATEGORY_WEB,
	PROJECT_CATEGORY_OTHER,
};

enum project_media_type {
	PROJECT_MEDIA_IMAGE,
	PROJECT_MEDIA_VIDEO,
};

struct project_media {
	enum project_media_type type;
	char *src;
	char *caption;
	char *alt;
	char *link;
	char *poster;
	bool show_on_index;
};

struct project {
	char *address;
	char *name;
	char *summary;
	char *description;
	enum project_category category;

	char **technologies;
	size_t technology_count;

	char **highlights;
	size_t highlight_count;

	struct link *links;
	size_t link_count;
	char *github;
	char *blog;
	char *demo;
	char *video;

	struct project_media *media;
	size_t media_count;
	bool show_in_cv;
};

struct note {
	char *title;
	char *category;
	char *summary;
	char *url;
};

struct certificate {
	char *title;
	char *issuer;
	struct date_range period;
	bool show_in_cv;

	char *description;

	struct link *links;
	size_t link_count;
};

struct profile {
	struct personal_info personal;

	struct education *education;
	size_t education_count;

	struct experience *experiences;
	size_t experience_count;

	struct research_project *research_projects;
	size_t research_project_count;

	struct award *awards;
	size_t award_count;

	struct note *notes;
	size_t note_count;

	struct project *projects;
	size_t project_count;

	struct certificate *certificates;
	size_t certificate_count;
};

int profile_load(struct profile *profile, const char *path);
void profile_dump(const struct profile *profile);
void profile_free(struct profile *profile);

#endif /* MEGEN_PROFILE_H */
