#include "metternich.h"

#include "database/preferences.h"

#include "database/database.h"
#include "database/defines.h"
#include "database/gsml_data.h"
#include "database/gsml_parser.h"
#include "game/game_rules.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"

namespace metternich {

std::filesystem::path preferences::get_path()
{
	return database::get_user_data_path() / "preferences.txt";
}

preferences::preferences()
{
	this->game_rules = make_qunique<metternich::game_rules>();
}

preferences::~preferences()
{
}

void preferences::load()
{
	this->load_file();
}

void preferences::load_file()
{
	std::filesystem::path preferences_path = preferences::get_path();

	if (!std::filesystem::exists(preferences_path)) {
		return;
	}

	gsml_parser parser;
	gsml_data data;

	try {
		data = parser.parse(preferences_path);
	} catch (...) {
		exception::report(std::current_exception());
		log::log_error("Failed to parse preferences file.");
	}

	database::process_gsml_data(this, data);
}

void preferences::save() const
{
	const std::filesystem::path preferences_path = preferences::get_path();

	gsml_data data(preferences_path.filename().stem().string());

	data.add_property("scale_factor", this->get_scale_factor().to_string());
	data.add_property("major_scenarios_only", string::from_bool(this->major_scenarios_only));

	data.add_child("game_rules", this->get_game_rules()->to_gsml_data());

	try {
		data.print_to_file(preferences_path);
	} catch (...) {
		exception::report(std::current_exception());
		log::log_error("Failed to save preferences file.");
	}
}

void preferences::process_gsml_property(const gsml_property &property)
{
	//use a try-catch for the properties, as the property or its value could no longer exist
	try {
		database::get()->process_gsml_property_for_object(this, property);
	} catch (...) {
		exception::report(std::current_exception());
	}
}

void preferences::process_gsml_scope(const gsml_data &scope)
{
	try {
		const std::string &tag = scope.get_tag();

		if (tag == "game_rules") {
			database::process_gsml_data(this->game_rules, scope);
		} else {
			database::get()->process_gsml_scope_for_object(this, scope);
		}
	} catch (...) {
		exception::report(std::current_exception());
	}
}

void preferences::set_scale_factor(const centesimal_int &factor)
{
	if (factor == this->get_scale_factor()) {
		return;
	}

	this->scale_factor = factor;

	emit scale_factor_changed();
}

}
