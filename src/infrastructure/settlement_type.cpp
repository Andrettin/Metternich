#include "metternich.h"

#include "infrastructure/settlement_type.h"

#include "database/database.h"
#include "map/tile_image_provider.h"
#include "population/population_class.h"
#include "population/population_type.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

settlement_type::settlement_type(const std::string &identifier) : named_data_entry(identifier)
{
}

settlement_type::~settlement_type()
{
}

void settlement_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "base_settlement_types") {
		for (const std::string &value : values) {
			settlement_type *settlement_type = settlement_type::get(value);
			this->base_settlement_types.push_back(settlement_type);
			settlement_type->upgraded_settlement_types.push_back(this);
		}
	} else if (tag == "population_classes") {
		for (const std::string &value : values) {
			const population_class *population_class = population_class::get(value);
			this->population_classes.push_back(population_class);
		}
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<site>>();
		conditions->process_gsml_data(scope);
		this->conditions = std::move(conditions);
	} else if (tag == "build_conditions") {
		auto conditions = std::make_unique<and_condition<site>>();
		conditions->process_gsml_data(scope);
		this->build_conditions = std::move(conditions);
	} else if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const site>>();
		this->modifier->process_gsml_data(scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void settlement_type::initialize()
{
	tile_image_provider::get()->load_image("settlement/" + this->get_identifier() + "/0");

	this->calculate_level();

	named_data_entry::initialize();
}

void settlement_type::check() const
{
	if (this->get_image_filepath().empty()) {
		throw std::runtime_error(std::format("Settlement type \"{}\" has no image filepath.", this->get_identifier()));
	}

	if (vector::contains(this->get_base_settlement_types(), this)) {
		throw std::runtime_error(std::format("Settlement type \"{}\" is an upgrade of itself.", this->get_identifier()));
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->get_build_conditions() != nullptr) {
		this->get_build_conditions()->check_validity();
	}
}

void settlement_type::set_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_image_filepath()) {
		return;
	}

	this->image_filepath = database::get()->get_graphics_path(this->get_module()) / filepath;
}

void settlement_type::calculate_level()
{
	if (this->get_level() != 0) {
		return;
	}

	if (!this->get_base_settlement_types().empty()) {
		for (const settlement_type *base_settlement_type : this->get_base_settlement_types()) {
			const_cast<settlement_type *>(base_settlement_type)->calculate_level();

			this->level = std::max(this->level, base_settlement_type->get_level() + 1);
		}
	} else {
		this->level = 1;
	}
}

bool settlement_type::can_have_population_type(const population_type *population_type) const
{
	assert_throw(population_type != nullptr);

	return vector::contains(this->get_population_classes(), population_type->get_population_class());
}

}
