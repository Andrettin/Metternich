#include "util/translator.h"

#include "database/database.h"
#include "database/gsml_data.h"
#include "database/gsml_operator.h"
#include "database/gsml_parser.h"
#include "util/string_util.h"

#include <filesystem>

namespace metternich {

std::string translator::translate(const std::vector<std::string> &base_tags, const std::vector<std::vector<std::string>> &suffix_list_with_fallbacks, const std::string &final_suffix) const
{
	std::vector<std::string> suffix_combinations = string::get_suffix_combinations(suffix_list_with_fallbacks);

	for (const std::string &suffix : suffix_combinations) {
		for (const std::string &base_tag : base_tags) {
			const auto &suffix_find_iterator = this->translations.find(base_tag + suffix + final_suffix);
			if (suffix_find_iterator != this->translations.end())  {
				return suffix_find_iterator->second;
			}
		}
	}

	return base_tags.front();
}

QString translator::translate(const char *context, const char *source_text, const char *disambiguation, int n) const
{
	Q_UNUSED(context)
	Q_UNUSED(n)
	Q_UNUSED(disambiguation)

	return QString::fromStdString(this->translate(source_text));
}

void translator::load()
{
	this->translations.clear();

	for (const std::filesystem::path &path : database::get()->get_localization_paths()) {
		std::filesystem::path localization_path(path / this->get_locale());

		if (!std::filesystem::exists(localization_path)) {
			continue;
		}

		std::filesystem::recursive_directory_iterator dir_iterator(localization_path);

		for (const std::filesystem::directory_entry &dir_entry : dir_iterator) {
			if (!dir_entry.is_regular_file()) {
				continue;
			}

			this->load_file(dir_entry.path());
		}
	}
}

void translator::load_file(const std::filesystem::path &filepath)
{
	gsml_parser parser(filepath);
	const gsml_data gsml_data = parser.parse();

	gsml_data.for_each_property([&](const gsml_property &property) {
		if (property.get_operator() != gsml_operator::assignment) {
			throw std::runtime_error("Only the assignment operator is allowed for translation files.");
		}

		this->add_translation(property.get_key(), property.get_value());
	});
}

}
