#include "metternich.h"

#include "infrastructure/holding_type.h"

#include "database/database.h"
#include "economy/commodity.h"
#include "map/tile_image_provider.h"
#include "population/population_class.h"
#include "population/population_type.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

const std::set<std::string> holding_type::database_dependencies = {
	//so that commodity units are present
	commodity::class_identifier
};

holding_type::holding_type(const std::string &identifier) : named_data_entry(identifier)
{
}

holding_type::~holding_type()
{
}

void holding_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "level_commodity_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->level_commodity_costs[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "level_commodity_costs_per_level") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->level_commodity_costs_per_level[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "fortification_level_commodity_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->fortification_level_commodity_costs[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "conditional_names") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			auto conditions = std::make_unique<and_condition<site>>();
			conditions->process_gsml_data(child_scope);
			this->conditional_names[child_tag] = std::move(conditions);
		});
	} else if (tag == "tier_levels") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->tier_levels[key] = std::stoi(value);
		});
	} else if (tag == "base_holding_types") {
		for (const std::string &value : values) {
			holding_type *holding_type = holding_type::get(value);
			this->base_holding_types.push_back(holding_type);
			holding_type->upgraded_holding_types.push_back(this);
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

void holding_type::initialize()
{
	QTimer::singleShot(0, [this]() -> QCoro::Task<void> {
		co_await tile_image_provider::get()->load_image("settlement/" + this->get_identifier() + "/0");
	});

	this->calculate_level();

	named_data_entry::initialize();
}

void holding_type::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Holding type \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Holding type \"{}\" has no portrait.", this->get_identifier()));
	}

	if (this->get_image_filepath().empty()) {
		throw std::runtime_error(std::format("Holding type \"{}\" has no image filepath.", this->get_identifier()));
	}

	if (vector::contains(this->get_base_holding_types(), this)) {
		throw std::runtime_error(std::format("Holding type \"{}\" is an upgrade of itself.", this->get_identifier()));
	}

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->get_build_conditions() != nullptr) {
		this->get_build_conditions()->check_validity();
	}
}

void holding_type::set_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_image_filepath()) {
		return;
	}

	this->image_filepath = database::get()->get_graphics_path(this->get_module()) / filepath;
}

void holding_type::calculate_level()
{
	if (this->get_level() != 0) {
		return;
	}

	if (!this->get_base_holding_types().empty()) {
		for (const holding_type *base_holding_type : this->get_base_holding_types()) {
			const_cast<holding_type *>(base_holding_type)->calculate_level();

			this->level = std::max(this->level, base_holding_type->get_level() + 1);
		}
	} else {
		this->level = 1;
	}
}

bool holding_type::can_have_population_type(const population_type *population_type) const
{
	assert_throw(population_type != nullptr);

	return vector::contains(this->get_population_classes(), population_type->get_population_class());
}

}
