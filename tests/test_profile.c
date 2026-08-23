#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "profile.h"

static void test_education_array(void)
{
	struct profile profile;

	assert(profile_load(&profile, "tests/fixtures/education.toml") == 0);
	assert(profile.education_count == 2);
	assert(strcmp(profile.education[0].institution, "Example University") == 0);
	assert(profile.education[0].degree == DEGREE_BACHELOR);
	assert(profile.education[0].period.start.year == 2020);
	assert(profile.education[0].period.start.month == 9);
	assert(profile.education[0].period.end.year == 2025);
	assert(profile.education[0].period.end.month == 6);
	assert(!profile.education[0].period.ongoing);
	assert(strcmp(profile.education[1].institution, "Graduate School") == 0);
	assert(profile.education[1].degree == DEGREE_MASTER);
	assert(profile.education[1].period.ongoing);
	assert(profile.education[1].description == NULL);

	profile_free(&profile);
	assert(profile.education == NULL);
	assert(profile.education_count == 0);
}

static void test_invalid_date_cleans_partial_profile(void)
{
	struct profile profile;

	assert(profile_load(&profile, "tests/fixtures/invalid-date.toml") == -1);
	assert(profile.education == NULL);
	assert(profile.education_count == 0);
}

static void test_experience_array(void)
{
	struct profile profile;
	const struct experience *experience;

	assert(profile_load(&profile, "tests/fixtures/experience.toml") == 0);
	assert(profile.experience_count == 2);
	experience = &profile.experiences[0];
	assert(strcmp(experience->company, "HAVELSAN") == 0);
	assert(strcmp(experience->title, "Software Engineer") == 0);
	assert(strcmp(experience->location, "Ankara, Türkiye") == 0);
	assert(strcmp(experience->employment_type, "Full-time") == 0);
	assert(experience->period.start.year == 2025);
	assert(experience->period.start.month == 11);
	assert(experience->period.ongoing);
	assert(experience->highlight_count == 1);
	experience = &profile.experiences[1];
	assert(experience->location == NULL);
	assert(experience->employment_type == NULL);
	assert(!experience->period.ongoing);
	assert(experience->period.end.month == 10);
	assert(experience->highlights == NULL);

	profile_free(&profile);
	assert(profile.experiences == NULL);
	assert(profile.experience_count == 0);
}

static void test_missing_experience_company_is_rejected(void)
{
	struct profile profile;

	assert(profile_load(&profile,
			    "tests/fixtures/missing-experience-company.toml") == -1);
	assert(profile.experiences == NULL);
	assert(profile.experience_count == 0);
}

static void test_awards_and_certificates(void)
{
	struct profile profile;

	assert(profile_load(&profile,
			    "tests/fixtures/awards-certificates.toml") == 0);
	assert(profile.award_count == 2);
	assert(strcmp(profile.awards[0].title, "Best Paper") == 0);
	assert(strcmp(profile.awards[0].issuer, "Example Conference") == 0);
	assert(profile.awards[0].date.year == 2024);
	assert(profile.awards[0].date.month == 5);
	assert(profile.awards[0].highlight_count == 2);
	assert(strcmp(profile.awards[0].highlights[0],
		      "Recognized by the program committee.") == 0);
	assert(profile.awards[0].link_count == 2);
	assert(strcmp(profile.awards[0].links[0].label, "Announcement") == 0);
	assert(strcmp(profile.awards[0].links[1].url,
		      "https://example.com/paper") == 0);
	assert(profile.awards[1].link_count == 0);
	assert(profile.awards[1].highlights == NULL);
	assert(profile.awards[1].date.year == 2022);
	assert(profile.awards[1].date.month == 0);

	assert(profile.certificate_count == 2);
	assert(strcmp(profile.certificates[0].title, "C Programming") == 0);
	assert(profile.certificates[0].period.start.year == 2023);
	assert(profile.certificates[0].period.start.month == 11);
	assert(profile.certificates[0].period.end.year == 2023);
	assert(profile.certificates[0].period.end.month == 11);
	assert(profile.certificates[0].show_in_cv);
	assert(profile.certificates[0].link_count == 1);
	assert(strcmp(profile.certificates[0].links[0].url,
		      "https://example.com/credential") == 0);
	assert(profile.certificates[1].period.start.year == 2023);
	assert(profile.certificates[1].period.start.month == 0);
	assert(profile.certificates[1].period.end.year == 2024);
	assert(profile.certificates[1].period.end.month == 0);
	assert(!profile.certificates[1].show_in_cv);

	profile_free(&profile);
	assert(profile.awards == NULL);
	assert(profile.award_count == 0);
	assert(profile.certificates == NULL);
	assert(profile.certificate_count == 0);
	profile_free(&profile);
}

static void test_invalid_nested_link_cleans_profile(void)
{
	struct profile profile;

	assert(profile_load(&profile, "tests/fixtures/invalid-link.toml") == -1);
	assert(profile.personal.name == NULL);
	assert(profile.awards == NULL);
	assert(profile.award_count == 0);
}

static void test_invalid_award_date_cleans_profile(void)
{
	struct profile profile;

	assert(profile_load(&profile,
			    "tests/fixtures/invalid-award-date.toml") == -1);
	assert(profile.personal.name == NULL);
	assert(profile.awards == NULL);
	assert(profile.award_count == 0);
}

static void test_certificate_failure_cleans_previous_awards(void)
{
	struct profile profile;

	assert(profile_load(&profile,
			    "tests/fixtures/invalid-certificate-date.toml") == -1);
	assert(profile.personal.name == NULL);
	assert(profile.awards == NULL);
	assert(profile.award_count == 0);
	assert(profile.certificates == NULL);
	assert(profile.certificate_count == 0);
}

static void test_invalid_link_shape_is_rejected(void)
{
	struct profile profile;

	assert(profile_load(&profile,
			    "tests/fixtures/invalid-link-shape.toml") == -1);
	assert(profile.awards == NULL);
	assert(profile.award_count == 0);
}

static void test_remaining_sections(void)
{
	struct profile profile;
	const struct project *project;
	const struct research_project *research;

	assert(profile_load(&profile, "tests/fixtures/remaining.toml") == 0);

	assert(profile.project_count == 2);
	assert(profile.note_count == 1);
	assert(strcmp(profile.notes[0].title, "Pipeline Notes") == 0);
	assert(strcmp(profile.notes[0].category, "computer_architecture") == 0);
	assert(strcmp(profile.notes[0].summary, "Hazards and forwarding.") == 0);
	assert(strcmp(profile.notes[0].url, "notes/pipeline.pdf") == 0);
	project = &profile.projects[0];
	assert(strcmp(project->address, "0x00003100") == 0);
	assert(strcmp(project->name, "megen") == 0);
	assert(strcmp(project->summary, "Profile generator") == 0);
	assert(strcmp(project->description, "A TOML-driven C application") == 0);
	assert(project->category == PROJECT_CATEGORY_SYSTEMS);
	assert(project->show_in_cv);
	assert(project->technology_count == 2);
	assert(strcmp(project->technologies[1], "TOML") == 0);
	assert(project->highlight_count == 2);
	assert(project->link_count == 1);
	assert(strcmp(project->github, "https://example.com/source") == 0);
	assert(strcmp(project->blog, "/projects/megen") == 0);
	assert(project->image_count == 2);
	assert(strcmp(project->images[0].caption,
		      "fig_01 // generated index") == 0);
	assert(project->images[0].show_on_index);
	assert(!project->images[1].show_on_index);
	project = &profile.projects[1];
	assert(strcmp(project->name, "Tiny Tool") == 0);
	assert(project->description == NULL);
	assert(project->category == PROJECT_CATEGORY_OTHER);
	assert(!project->show_in_cv);
	assert(project->technologies == NULL);
	assert(project->highlight_count == 0);
	assert(project->link_count == 0);

	assert(profile.research_project_count == 1);
	research = &profile.research_projects[0];
	assert(strcmp(research->title, "Autonomous Systems Research") == 0);
	assert(strcmp(research->organization, "Example University Lab") == 0);
	assert(strcmp(research->role, "Research Assistant") == 0);
	assert(research->sponsor_count == 2);
	assert(strcmp(research->sponsors[0], "NVIDIA") == 0);
	assert(research->technology_count == 2);
	assert(strcmp(research->technologies[0], "C++") == 0);
	assert(research->period.start.year == 2022);
	assert(research->period.end.month == 6);
	assert(!research->period.ongoing);
	assert(research->highlight_count == 2);
	assert(research->link_count == 2);
	assert(strcmp(research->links[1].label, "Publication") == 0);

	profile_free(&profile);
	assert(profile.projects == NULL);
	assert(profile.project_count == 0);
	assert(profile.notes == NULL);
	assert(profile.note_count == 0);
	assert(profile.research_projects == NULL);
	assert(profile.research_project_count == 0);
	profile_free(&profile);
}

static void test_invalid_academic_period_cleans_previous_sections(void)
{
	struct profile profile;

	assert(profile_load(&profile,
			    "tests/fixtures/invalid-academic-period.toml") == -1);
	assert(profile.personal.name == NULL);
	assert(profile.projects == NULL);
	assert(profile.project_count == 0);
	assert(profile.research_projects == NULL);
	assert(profile.research_project_count == 0);
	profile_free(&profile);
}

static void test_invalid_period_shape_is_rejected(void)
{
	struct profile profile;

	assert(profile_load(&profile,
			    "tests/fixtures/invalid-period-shape.toml") == -1);
	assert(profile.research_projects == NULL);
	assert(profile.research_project_count == 0);
}

static void test_project_summary_is_optional(void)
{
	struct profile profile;

	assert(profile_load(&profile,
			    "tests/fixtures/missing-project-summary.toml") == 0);
	assert(profile.project_count == 1);
	assert(profile.projects[0].summary == NULL);
	assert(!profile.projects[0].show_in_cv);
	profile_free(&profile);
}

static void test_website_project_schema(void)
{
	struct profile profile;
	const struct project *project;

	assert(profile_load(&profile, "tests/fixtures/website-projects.toml") == 0);
	assert(profile.project_count == 1);
	project = &profile.projects[0];
	assert(strcmp(project->address, "0x00002100") == 0);
	assert(strcmp(project->name, "Offline Shader Compiler") == 0);
	assert(project->category == PROJECT_CATEGORY_OTHER);
	assert(!project->show_in_cv);
	assert(project->technology_count == 3);
	assert(strcmp(project->technologies[2], "NIR") == 0);
	assert(strcmp(project->github,
		      "https://github.com/example/compiler") == 0);
	assert(strcmp(project->blog,
		      "/projects/offline-shader-compiler") == 0);
	assert(strcmp(project->demo, "https://example.com/compiler") == 0);
	assert(project->image_count == 2);
	assert(strcmp(project->images[0].alt,
		      "Compiler pipeline architecture diagram") == 0);
	assert(strcmp(project->images[0].link,
		      "/diagrams/compiler-pipeline.svg") == 0);
	assert(project->images[0].show_on_index);
	assert(!project->images[1].show_on_index);
	profile_free(&profile);
}

int main(void)
{
	test_education_array();
	test_invalid_date_cleans_partial_profile();
	test_experience_array();
	test_missing_experience_company_is_rejected();
	test_awards_and_certificates();
	test_invalid_nested_link_cleans_profile();
	test_invalid_award_date_cleans_profile();
	test_certificate_failure_cleans_previous_awards();
	test_invalid_link_shape_is_rejected();
	test_remaining_sections();
	test_invalid_academic_period_cleans_previous_sections();
	test_invalid_period_shape_is_rejected();
	test_project_summary_is_optional();
	test_website_project_schema();
	puts("profile tests passed");
	return 0;
}
