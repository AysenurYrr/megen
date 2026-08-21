#include <stdio.h>

#include "profile.h"

static const char *degree_name(enum degree degree)
{
	switch (degree) {
	case DEGREE_BACHELOR:
		return "bachelor";
	case DEGREE_MASTER:
		return "master";
	case DEGREE_PHD:
		return "phd";
	case DEGREE_OTHER:
		return "other";
	}

	return "unknown";
}

static const char *project_category_name(enum project_category category)
{
	switch (category) {
	case PROJECT_CATEGORY_LOW_LEVEL: return "low_level";
	case PROJECT_CATEGORY_SYSTEMS: return "systems";
	case PROJECT_CATEGORY_ROBOTICS: return "robotics";
	case PROJECT_CATEGORY_AI_ML: return "ai_ml";
	case PROJECT_CATEGORY_WEB: return "web";
	case PROJECT_CATEGORY_OTHER: return "other";
	}
	return "unknown";
}

static void dump_string(const char *label, const char *value)
{
	printf("  %s: %s\n", label, value ? value : "(none)");
}

static void dump_string_array(const char *label,
			      char *const *strings, size_t count)
{
	size_t i;

	if (!strings || count == 0) {
		printf("  %s: []\n", label);
		return;
	}

	printf("  %s:\n", label);
	for (i = 0; i < count; i++)
		printf("    - %s\n", strings[i] ? strings[i] : "(none)");
}

static void dump_links(const struct link *links, size_t count)
{
	size_t i;

	if (!links || count == 0) {
		puts("  links: []");
		return;
	}

	puts("  links:");
	for (i = 0; i < count; i++)
		printf("    - %s -> %s\n",
		       links[i].label ? links[i].label : "(none)",
		       links[i].url ? links[i].url : "(none)");
}

static void dump_date(const struct date *date)
{
	if (date->month == 0)
		printf("%04u", date->year);
	else
		printf("%04u-%02u", date->year, date->month);
}

static void dump_date_range(const struct date_range *range)
{
	dump_date(&range->start);
	printf(" -> ");
	if (range->ongoing)
		printf("present");
	else
		dump_date(&range->end);
}

void profile_dump(const struct profile *profile)
{
	size_t i;

	if (!profile)
		return;

	puts("personal:");
	dump_string("name", profile->personal.name);
	dump_string("email", profile->personal.email);
	dump_string("phone", profile->personal.phone);
	dump_string("linkedin", profile->personal.linkedin);
	dump_string("github", profile->personal.github);
	dump_string("website", profile->personal.website);

	for (i = 0; i < profile->education_count; i++) {
		const struct education *education = &profile->education[i];

		putchar('\n');
		printf("education[%zu]:\n", i);
		dump_string("institution", education->institution);
		dump_string("department", education->department);
		printf("  degree: %s\n", degree_name(education->degree));
		printf("  period: ");
		dump_date_range(&education->period);
		putchar('\n');
		dump_string("description", education->description);
	}

	for (i = 0; i < profile->experience_count; i++) {
		const struct experience *experience = &profile->experiences[i];

		putchar('\n');
		printf("experience[%zu]:\n", i);
		dump_string("company", experience->company);
		dump_string("title", experience->title);
		dump_string("location", experience->location);
		dump_string("employment_type", experience->employment_type);
		printf("  period: ");
		dump_date_range(&experience->period);
		putchar('\n');
		dump_string_array("highlights", experience->highlights,
				  experience->highlight_count);
	}

	for (i = 0; i < profile->award_count; i++) {
		const struct award *award = &profile->awards[i];

		putchar('\n');
		printf("award[%zu]:\n", i);
		dump_string("title", award->title);
		dump_string("issuer", award->issuer);
		printf("  date: ");
		dump_date(&award->date);
		putchar('\n');
		dump_string_array("highlights", award->highlights,
				  award->highlight_count);
		dump_links(award->links, award->link_count);
	}

	for (i = 0; i < profile->certificate_count; i++) {
		const struct certificate *certificate = &profile->certificates[i];

		putchar('\n');
		printf("certificate[%zu]:\n", i);
		dump_string("title", certificate->title);
		dump_string("issuer", certificate->issuer);
		printf("  period: ");
		dump_date_range(&certificate->period);
		putchar('\n');
		dump_string("description", certificate->description);
		dump_links(certificate->links, certificate->link_count);
	}

	for (i = 0; i < profile->project_count; i++) {
		const struct project *project = &profile->projects[i];

		putchar('\n');
		printf("project[%zu]:\n", i);
		dump_string("name", project->name);
		dump_string("summary", project->summary);
		dump_string("description", project->description);
		printf("  category: %s\n", project_category_name(project->category));
		printf("  show_in_cv: %s\n", project->show_in_cv ? "true" : "false");
		dump_string_array("technologies", project->technologies,
				  project->technology_count);
		dump_string_array("highlights", project->highlights,
				  project->highlight_count);
		dump_links(project->links, project->link_count);
	}

	for (i = 0; i < profile->research_project_count; i++) {
		const struct research_project *project = &profile->research_projects[i];

		putchar('\n');
		printf("research_project[%zu]:\n", i);
		dump_string("title", project->title);
		dump_string("organization", project->organization);
		dump_string("role", project->role);
		dump_string_array("sponsors", project->sponsors,
				  project->sponsor_count);
		dump_string_array("technologies", project->technologies,
				  project->technology_count);
		printf("  period: ");
		dump_date_range(&project->period);
		putchar('\n');
		dump_string_array("highlights", project->highlights,
				  project->highlight_count);
		dump_links(project->links, project->link_count);
	}
}
