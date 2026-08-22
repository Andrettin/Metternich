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
#include "util/log_util.h"
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
		scope.for_each_property([this](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->level_commodity_costs[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "level_commodity_costs_per_level") {
		scope.for_each_property([this](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->level_commodity_costs_per_level[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "fortification_level_commodity_costs") {
		scope.for_each_property([this](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->fortification_level_commodity_costs[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "conditional_names") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			auto conditions = std::make_unique<and_condition<site>>();
			conditions->process_gsml_data(child_scope);
			this->conditional_names[child_tag] = std::move(conditions);
		});
	} else if (tag == "tier_levels") {
		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->tier_levels[key] = std::stoi(value);
		});
	} else if (tag == "population_classes") {
		for (const std::string &value : values) {
			const population_class *population_class = population_class::get(value);
			this->population_classes.push_back(population_class);
		}
	} else if (tag == "income_per_level_and_province_level") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int level = std::stoi(child_tag);

			child_scope.for_each_property([this, level](const gsml_property &property) {
				const std::string &key = property.get_key();
				const std::string &value = property.get_value();
				const int province_level = std::stoi(key);

				this->income_per_level_and_province_level[level][province_level] = dice(value);
			});
		});
	} else if (tag == "taxation_per_level_difference_and_income") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int level_difference = std::stoi(child_tag);

			child_scope.for_each_property([this, level_difference](const gsml_property &property) {
				const std::string &key = property.get_key();
				const std::string &value = property.get_value();
				const int income = std::stoi(key);

				this->taxation_per_level_difference_and_income[level_difference][income] = dice(value);
			});
		});
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

	if (this->get_domain_skill() == nullptr) {
		log::log_error(std::format("Holding type \"{}\" has no domain skill.", this->get_identifier()));
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

bool holding_type::can_have_population_type(const population_type *population_type) const
{
	assert_throw(population_type != nullptr);

	return vector::contains(this->get_population_classes(), population_type->get_population_class());
}

const dice &holding_type::get_income(const int level, const int province_level) const
{
	if (this->income_per_level_and_province_level.empty()) {
		static constexpr dice null_dice(0, 0);
		return null_dice;
	}

	auto find_iterator = this->income_per_level_and_province_level.upper_bound(level);
	assert_throw(find_iterator != this->income_per_level_and_province_level.begin());
	--find_iterator;

	auto sub_find_iterator = find_iterator->second.upper_bound(province_level);
	if (sub_find_iterator == find_iterator->second.begin()) {
		throw std::runtime_error(std::format("Could not find income for holding type \"{}\" for holding level {} and province level {}.", this->get_identifier(), level, province_level));
	}
	--sub_find_iterator;

	return sub_find_iterator->second;
}

const dice &holding_type::get_taxation(const int level_difference, const int income) const
{
	assert_throw(this->can_tax());
	assert_throw(income > 0);

	auto find_iterator = this->taxation_per_level_difference_and_income.find(level_difference);
	if (find_iterator == this->taxation_per_level_difference_and_income.end()) {
		if (level_difference < this->taxation_per_level_difference_and_income.begin()->first) {
			find_iterator = this->taxation_per_level_difference_and_income.begin();
		} else if (level_difference > this->taxation_per_level_difference_and_income.rbegin()->first) {
			find_iterator = this->taxation_per_level_difference_and_income.end();
			--find_iterator;
		}
	}

	auto sub_find_iterator = find_iterator->second.upper_bound(income);
	if (sub_find_iterator == find_iterator->second.begin()) {
		throw std::runtime_error(std::format("Could not find taxation for holding type \"{}\" for level difference {} and income {}.", this->get_identifier(), level_difference, income));
	}
	--sub_find_iterator;

	return sub_find_iterator->second;
}

}
