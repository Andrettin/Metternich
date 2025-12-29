#include "metternich.h"

#include "domain/culture_base.h"

#include "domain/cultural_group.h"
#include "domain/culture_history.h"
#include "domain/domain_tier.h"
#include "domain/government_type.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_type.h"
#include "language/fallback_name_generator.h"
#include "language/gendered_name_generator.h"
#include "language/grammatical_gender.h"
#include "language/language.h"
#include "language/name_generator.h"
#include "language/word.h"
#include "population/population_class.h"
#include "population/population_type.h"
#include "unit/civilian_unit_class.h"
#include "unit/civilian_unit_type.h"
#include "unit/military_unit_class.h"
#include "unit/military_unit_type.h"
#include "unit/transporter_class.h"
#include "unit/transporter_type.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/string_util.h"
#include "util/vector_util.h"

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_utility.hpp>

namespace metternich {

const std::set<std::string> culture_base::database_dependencies = {
	//so that languages and words are fully initialized when initializing cultures
	language::class_identifier,
	word::class_identifier
};

culture_base::culture_base(const std::string &identifier) : named_data_entry(identifier)
{
}

culture_base::~culture_base()
{
}

void culture_base::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "given_name_markov_chain_size") {
		assert_throw(property.get_operator() == gsml_operator::assignment);
		if (this->given_name_generator == nullptr) {
			this->given_name_generator = std::make_unique<gendered_name_generator>();
		}
		this->given_name_generator->set_markov_chain_size(std::stoull(value));
	} else if (key == "surname_markov_chain_size") {
		assert_throw(property.get_operator() == gsml_operator::assignment);
		if (this->surname_generator == nullptr) {
			this->surname_generator = std::make_unique<gendered_name_generator>();
		}
		this->surname_generator->set_markov_chain_size(std::stoull(value));
	} else if (key == "patronym") {
		assert_throw(property.get_operator() == gsml_operator::assignment);
		this->patronyms[gender::none] = value;
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

void culture_base::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "patronyms") {
		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->patronyms[magic_enum::enum_cast<gender>(key).value()] = value;
		});
	} else if (tag == "title_names") {
		government_type::process_title_name_scope(this->title_names, scope);
	} else if (tag == "site_title_names") {
		government_type::process_site_title_name_scope(this->site_title_names, scope);
	} else if (tag == "office_title_names") {
		government_type::process_office_title_name_scope(this->office_title_names, scope);
	} else if (tag == "building_class_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const building_class *building_class = building_class::get(key);
			const building_type *building_type = building_type::get(value);
			this->set_building_class_type(building_class, building_type);
		});
	} else if (tag == "population_class_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const population_class *population_class = population_class::get(key);
			const population_type *population_type = population_type::get(value);
			this->set_population_class_type(population_class, population_type);
		});
	} else if (tag == "civilian_class_unit_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const civilian_unit_class *unit_class = civilian_unit_class::get(key);
			const civilian_unit_type *unit_type = civilian_unit_type::get(value);
			this->set_civilian_class_unit_type(unit_class, unit_type);
		});
	} else if (tag == "military_class_unit_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const military_unit_class *unit_class = military_unit_class::get(key);
			const military_unit_type *unit_type = military_unit_type::get(value);
			this->set_military_class_unit_type(unit_class, unit_type);
		});
	} else if (tag == "transporter_class_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const transporter_class *transporter_class = transporter_class::get(key);
			const transporter_type *transporter_type = transporter_type::get(value);
			this->set_transporter_class_type(transporter_class, transporter_type);
		});
	} else if (tag == "given_names") {
		if (this->given_name_generator == nullptr) {
			this->given_name_generator = std::make_unique<gendered_name_generator>();
		}

		if (!values.empty()) {
			this->given_name_generator->add_names(gender::none, values);
		}

		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const gender gender = enum_converter<archimedes::gender>::to_enum(tag);

			this->given_name_generator->add_names(gender, child_scope.get_values());
		});
	} else if (tag == "surnames") {
		if (this->surname_generator == nullptr) {
			this->surname_generator = std::make_unique<gendered_name_generator>();
		}

		if (!values.empty()) {
			this->surname_generator->add_names(gender::none, values);
		}

		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const gender gender = enum_converter<archimedes::gender>::to_enum(tag);

			this->surname_generator->add_names(gender, child_scope.get_values());
		});
	} else if (tag == "military_unit_class_names") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const military_unit_class *unit_class = military_unit_class::get(tag);

			if (!this->military_unit_class_name_generators.contains(unit_class)) {
				this->military_unit_class_name_generators[unit_class] = std::make_unique<name_generator>();
			}

			this->military_unit_class_name_generators[unit_class]->add_names(child_scope.get_values());
		});
	} else if (tag == "transporter_class_names") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const std::string &tag = child_scope.get_tag();

			const transporter_class *unit_class = transporter_class::get(tag);

			if (!this->transporter_class_name_generators.contains(unit_class)) {
				this->transporter_class_name_generators[unit_class] = std::make_unique<name_generator>();
			}

			this->transporter_class_name_generators[unit_class]->add_names(child_scope.get_values());
		});
	} else if (tag == "ship_names") {
		if (this->ship_name_generator == nullptr) {
			this->ship_name_generator = std::make_unique<name_generator>();
		}

		this->ship_name_generator->add_names(values);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void culture_base::initialize()
{
	if (this->get_language() != nullptr && (this->given_name_generator == nullptr || !this->given_name_generator->has_enough_data())) {
		assert_throw(this->get_language()->is_initialized());

		std::set<gender> enough_data_genders;
		if (this->given_name_generator != nullptr) {
			if (this->given_name_generator->has_enough_data(gender::female)) {
				enough_data_genders.insert(gender::female);
			}
			if (this->given_name_generator->has_enough_data(gender::male)) {
				enough_data_genders.insert(gender::male);
			}
		}

		for (const word *front_compound_element : this->get_language()->get_name_front_compound_elements()) {
			for (const word *rear_compound_element : this->get_language()->get_name_rear_compound_elements()) {
				if (rear_compound_element == front_compound_element) {
					continue;
				}

				if (this->given_name_generator == nullptr) {
					this->given_name_generator = std::make_unique<gendered_name_generator>();
				}

				const gender gender = grammatical_gender_to_gender(rear_compound_element->get_gender());

				if (enough_data_genders.contains(gender)) {
					continue;
				}

				this->given_name_generator->add_name(gender, front_compound_element->get_anglicized_name() + string::lowered(rear_compound_element->get_anglicized_name()));
			}
		}
	}

	if (!this->get_patronyms().empty() && this->given_name_generator != nullptr && this->given_name_generator->get_name_generator(gender::male) != nullptr) {
		magic_enum::enum_for_each<gender>([this](const gender gender) {
			const std::string &patronym = this->get_patronym(gender);
			if (patronym.empty()) {
				return;
			}

			if (this->surname_generator != nullptr && this->surname_generator->get_name_generator(gender) != nullptr && this->surname_generator->get_name_generator(gender)->has_enough_data()) {
				return;
			}

			if (this->surname_generator == nullptr) {
				this->surname_generator = std::make_unique<gendered_name_generator>();
			}

			for (const auto &name_variant : this->given_name_generator->get_name_generator(gender::male)->get_names()) {
				const std::string male_name = get_name_variant_string(name_variant);
				this->surname_generator->add_name(gender, male_name + patronym);
			}
		});
	}

	if (this->group != nullptr) {
		if (!this->group->is_initialized()) {
			this->group->initialize();
		}

		this->group->add_names_from(this);

		for (const domain *domain : this->get_domains()) {
			this->group->add_domain(domain);
		}
	}

	if (this->given_name_generator != nullptr) {
		fallback_name_generator::get()->add_given_names(this->given_name_generator);

		//add words from the language for Markov generation
		if (this->get_language() != nullptr && this->given_name_generator->uses_markov_generation() && this->uses_language_data_for_markov_generation()) {
			for (const word *word : this->get_language()->get_words()) {
				const std::string word_str = word->get_anglicized_name();
				const gender gender = grammatical_gender_to_gender(word->get_gender());
				this->given_name_generator->add_name(gender, word);
			}
		}

		this->given_name_generator->propagate_ungendered_names();
	}

	if (this->surname_generator != nullptr) {
		fallback_name_generator::get()->add_surnames(this->surname_generator);

		//add words from the language for Markov generation
		if (this->get_language() != nullptr && this->surname_generator->uses_markov_generation() && this->uses_language_data_for_markov_generation()) {
			for (const word *word : this->get_language()->get_words()) {
				const std::string word_str = word->get_anglicized_name();
				const gender gender = grammatical_gender_to_gender(word->get_gender());
				this->surname_generator->add_name(gender, word);
			}
		}

		this->surname_generator->propagate_ungendered_names();
	}

	fallback_name_generator::get()->add_military_unit_class_names(this->military_unit_class_name_generators);
	military_unit_class::propagate_names(this->military_unit_class_name_generators, this->ship_name_generator);

	fallback_name_generator::get()->add_transporter_class_names(this->transporter_class_name_generators);
	transporter_class::propagate_names(this->transporter_class_name_generators, this->ship_name_generator);

	if (this->ship_name_generator != nullptr) {
		fallback_name_generator::get()->add_ship_names(this->ship_name_generator->get_names());
	}

	named_data_entry::initialize();
}

void culture_base::check() const
{
	for (const auto &[building_class, building_type] : this->building_class_types) {
		assert_throw(building_type->get_building_class() == building_class);
	}

	for (const auto &[population_class, population_type] : this->population_class_types) {
		assert_throw(population_type->get_population_class() == population_class);
	}

	for (const auto &[unit_class, unit_type] : this->civilian_class_unit_types) {
		assert_throw(unit_type->get_unit_class() == unit_class);
	}

	for (const auto &[unit_class, unit_type] : this->military_class_unit_types) {
		assert_throw(unit_type->get_unit_class() == unit_class);
	}
}

data_entry_history *culture_base::get_history_base()
{
	return this->history.get();
}

void culture_base::reset_history()
{
	this->history = make_qunique<culture_history>();
}

const std::string &culture_base::get_patronym(const gender gender) const
{
	const auto find_iterator = this->patronyms.find(gender);
	if (find_iterator != this->patronyms.end()) {
		return find_iterator->second;
	}

	return string::empty_str;
}

phenotype *culture_base::get_default_phenotype() const
{
	if (this->default_phenotype != nullptr) {
		return this->default_phenotype;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_default_phenotype();
	}

	return nullptr;
}

bool culture_base::is_part_of_group(const cultural_group *group) const
{
	if (this->get_group() == nullptr) {
		return false;
	}

	if (this->get_group() == group) {
		return true;
	}

	//not the same group, and has a rank lesser than or equal to that of our group, so it can't be an upper group of ours
	if (group->get_rank() <= this->get_group()->get_rank()) {
		return false;
	}

	return this->get_group()->is_part_of_group(group);
}

const std::string &culture_base::get_title_name(const government_type *government_type, const domain_tier tier) const
{
	auto find_iterator = this->title_names.find(government_type);
	if (find_iterator == this->title_names.end()) {
		find_iterator = this->title_names.find(government_type->get_group());
	}

	if (find_iterator != this->title_names.end()) {
		auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator == find_iterator->second.end()) {
			sub_find_iterator = find_iterator->second.find(domain_tier::none);
		}

		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_title_name(government_type, tier);
	}

	return string::empty_str;
}

const std::string &culture_base::get_site_title_name(const government_type *government_type, const int tier) const
{
	auto find_iterator = this->site_title_names.find(government_type);
	if (find_iterator == this->site_title_names.end()) {
		find_iterator = this->site_title_names.find(government_type->get_group());
	}

	if (find_iterator != this->site_title_names.end()) {
		auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator == find_iterator->second.end()) {
			sub_find_iterator = find_iterator->second.find(0);
		}

		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_site_title_name(government_type, tier);
	}

	return string::empty_str;
}

const std::string &culture_base::get_office_title_name(const office *office, const government_type *government_type, const domain_tier tier, const gender gender) const
{
	const auto office_find_iterator = this->office_title_names.find(office);
	if (office_find_iterator != this->office_title_names.end()) {
		auto find_iterator = office_find_iterator->second.find(government_type);
		if (find_iterator == office_find_iterator->second.end()) {
			find_iterator = office_find_iterator->second.find(government_type->get_group());
		}

		if (find_iterator != office_find_iterator->second.end()) {
			auto sub_find_iterator = find_iterator->second.find(tier);
			if (sub_find_iterator == find_iterator->second.end()) {
				sub_find_iterator = find_iterator->second.find(domain_tier::none);
			}

			if (sub_find_iterator != find_iterator->second.end()) {
				auto sub_sub_find_iterator = sub_find_iterator->second.find(gender);
				if (sub_sub_find_iterator == sub_find_iterator->second.end()) {
					sub_sub_find_iterator = sub_find_iterator->second.find(gender::none);
				}

				if (sub_sub_find_iterator != sub_find_iterator->second.end()) {
					return sub_sub_find_iterator->second;
				}
			}
		}
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_office_title_name(office, government_type, tier, gender);
	}

	return string::empty_str;
}

const building_type *culture_base::get_building_class_type(const building_class *building_class) const
{
	const auto find_iterator = this->building_class_types.find(building_class);
	if (find_iterator != this->building_class_types.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_building_class_type(building_class);
	}

	return building_class->get_default_building_type();
}

void culture_base::set_building_class_type(const building_class *building_class, const building_type *building_type)
{
	if (building_type == nullptr) {
		this->building_class_types.erase(building_class);
		return;
	}

	this->building_class_types[building_class] = building_type;
}

const population_type *culture_base::get_population_class_type(const population_class *population_class) const
{
	const auto find_iterator = this->population_class_types.find(population_class);
	if (find_iterator != this->population_class_types.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_population_class_type(population_class);
	}

	return population_class->get_default_population_type();
}

void culture_base::set_population_class_type(const population_class *population_class, const population_type *population_type)
{
	if (population_type == nullptr) {
		this->population_class_types.erase(population_class);
		return;
	}

	this->population_class_types[population_class] = population_type;
}

const civilian_unit_type *culture_base::get_civilian_class_unit_type(const civilian_unit_class *unit_class) const
{
	const auto find_iterator = this->civilian_class_unit_types.find(unit_class);
	if (find_iterator != this->civilian_class_unit_types.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_civilian_class_unit_type(unit_class);
	}

	return unit_class->get_default_unit_type();
}

void culture_base::set_civilian_class_unit_type(const civilian_unit_class *unit_class, const civilian_unit_type *unit_type)
{
	if (unit_type == nullptr) {
		this->civilian_class_unit_types.erase(unit_class);
		return;
	}

	this->civilian_class_unit_types[unit_class] = unit_type;
}

const military_unit_type *culture_base::get_military_class_unit_type(const military_unit_class *unit_class) const
{
	const auto find_iterator = this->military_class_unit_types.find(unit_class);
	if (find_iterator != this->military_class_unit_types.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_military_class_unit_type(unit_class);
	}

	return unit_class->get_default_unit_type();
}

void culture_base::set_military_class_unit_type(const military_unit_class *unit_class, const military_unit_type *unit_type)
{
	if (unit_type == nullptr) {
		this->military_class_unit_types.erase(unit_class);
		return;
	}

	this->military_class_unit_types[unit_class] = unit_type;
}

const transporter_type *culture_base::get_transporter_class_type(const transporter_class *transporter_class) const
{
	const auto find_iterator = this->transporter_class_types.find(transporter_class);
	if (find_iterator != this->transporter_class_types.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_transporter_class_type(transporter_class);
	}

	return transporter_class->get_default_transporter_type();
}

void culture_base::set_transporter_class_type(const transporter_class *transporter_class, const transporter_type *transporter_type)
{
	if (transporter_type == nullptr) {
		this->transporter_class_types.erase(transporter_class);
		return;
	}

	this->transporter_class_types[transporter_class] = transporter_type;
}

std::string culture_base::generate_given_name(const gender gender, const std::map<std::string, int> &used_name_counts) const
{
	const name_generator *name_generator = this->get_given_name_generator(gender);
	const archimedes::name_generator *surname_generator = this->get_surname_generator(gender);

	if (name_generator != nullptr) {
		if (surname_generator != nullptr) {
			const size_t potential_full_name_count = name_generator->get_name_count() * surname_generator->get_name_count();

			if (potential_full_name_count > used_name_counts.size()) {
				std::string full_name;
				do {
					full_name = std::format("{} {}", name_generator->generate_name(), surname_generator->generate_name());
				} while (used_name_counts.contains(full_name));
				return full_name;
			}
		} else {
			return name_generator->generate_name(used_name_counts);
		}
	}

	return std::string();
}

std::string culture_base::generate_military_unit_name(const military_unit_type *type, const std::map<std::string, int> &used_name_counts) const
{
	const military_unit_class *unit_class = type->get_unit_class();

	if (unit_class->is_leader()) {
		return this->generate_given_name(gender::male, used_name_counts);
	} else {
		const name_generator *name_generator = this->get_military_unit_class_name_generator(unit_class);

		if (name_generator != nullptr) {
			return name_generator->generate_name(used_name_counts);
		}
	}

	return std::string();
}

std::string culture_base::generate_transporter_name(const transporter_type *type, const std::map<std::string, int> &used_name_counts) const
{
	const transporter_class *transporter_class = type->get_transporter_class();

	const name_generator *name_generator = this->get_transporter_class_name_generator(transporter_class);

	if (name_generator != nullptr) {
		return name_generator->generate_name(used_name_counts);
	}

	return std::string();
}

const name_generator *culture_base::get_given_name_generator(const gender gender) const
{
	const name_generator *name_generator = nullptr;

	if (this->given_name_generator != nullptr) {
		name_generator = this->given_name_generator->get_name_generator(gender);
	}

	if (name_generator != nullptr && name_generator->has_enough_data()) {
		return name_generator;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_given_name_generator(gender);
	}

	return name_generator;
}

void culture_base::add_given_name(const gender gender, const name_variant &name)
{
	if (this->given_name_generator == nullptr) {
		this->given_name_generator = std::make_unique<gendered_name_generator>();
	}

	this->given_name_generator->add_name(gender, name);

	if (gender == gender::none) {
		this->given_name_generator->add_name(gender::male, name);
		this->given_name_generator->add_name(gender::female, name);
	}

	if (this->group != nullptr) {
		this->group->add_given_name(gender, name);
	}
}

const name_generator *culture_base::get_surname_generator(const gender gender) const
{
	const name_generator *name_generator = nullptr;

	if (this->surname_generator != nullptr) {
		name_generator = this->surname_generator->get_name_generator(gender);
	}

	if (name_generator != nullptr && name_generator->has_enough_data()) {
		return name_generator;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_surname_generator(gender);
	}

	return name_generator;
}

void culture_base::add_surname(const gender gender, const name_variant &surname)
{
	if (this->surname_generator == nullptr) {
		this->surname_generator = std::make_unique<gendered_name_generator>();
	}

	this->surname_generator->add_name(gender, surname);

	if (gender == gender::none) {
		this->surname_generator->add_name(gender::male, surname);
		this->surname_generator->add_name(gender::female, surname);
	}

	if (this->group != nullptr) {
		this->group->add_surname(gender, surname);
	}
}

const name_generator *culture_base::get_military_unit_class_name_generator(const military_unit_class *unit_class) const
{
	const auto find_iterator = this->military_unit_class_name_generators.find(unit_class);
	if (find_iterator != this->military_unit_class_name_generators.end() && find_iterator->second->has_enough_data()) {
		return find_iterator->second.get();
	}

	if (unit_class->is_ship() && this->ship_name_generator != nullptr && this->ship_name_generator->has_enough_data()) {
		return this->ship_name_generator.get();
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_military_unit_class_name_generator(unit_class);
	}

	return fallback_name_generator::get()->get_military_unit_class_name_generator(unit_class);
}

void culture_base::add_military_unit_class_name(const military_unit_class *unit_class, const name_variant &name)
{
	if (!this->military_unit_class_name_generators.contains(unit_class)) {
		this->military_unit_class_name_generators[unit_class] = std::make_unique<name_generator>();
	}

	this->military_unit_class_name_generators[unit_class]->add_name(name);

	if (this->group != nullptr) {
		this->group->add_military_unit_class_name(unit_class, name);
	}
}

const name_generator *culture_base::get_transporter_class_name_generator(const transporter_class *transporter_class) const
{
	const auto find_iterator = this->transporter_class_name_generators.find(transporter_class);
	if (find_iterator != this->transporter_class_name_generators.end() && find_iterator->second->has_enough_data()) {
		return find_iterator->second.get();
	}

	if (transporter_class->is_ship() && this->ship_name_generator != nullptr && this->ship_name_generator->has_enough_data()) {
		return this->ship_name_generator.get();
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_transporter_class_name_generator(transporter_class);
	}

	return fallback_name_generator::get()->get_transporter_class_name_generator(transporter_class);
}

void culture_base::add_transporter_class_name(const transporter_class *transporter_class, const name_variant &name)
{
	if (!this->transporter_class_name_generators.contains(transporter_class)) {
		this->transporter_class_name_generators[transporter_class] = std::make_unique<name_generator>();
	}

	this->transporter_class_name_generators[transporter_class]->add_name(name);

	if (this->group != nullptr) {
		this->group->add_transporter_class_name(transporter_class, name);
	}
}

void culture_base::add_ship_name(const name_variant &ship_name)
{
	if (this->ship_name_generator == nullptr) {
		this->ship_name_generator = std::make_unique<name_generator>();
	}

	this->ship_name_generator->add_name(ship_name);

	if (this->group != nullptr) {
		this->group->add_ship_name(ship_name);
	}
}

void culture_base::add_names_from(const culture_base *other)
{
	if (other->given_name_generator != nullptr) {
		if (this->given_name_generator == nullptr) {
			this->given_name_generator = std::make_unique<gendered_name_generator>();
		}

		this->given_name_generator->add_names_from(other->given_name_generator);
	}

	if (other->surname_generator != nullptr) {
		if (this->surname_generator == nullptr) {
			this->surname_generator = std::make_unique<gendered_name_generator>();
		}

		this->surname_generator->add_names_from(other->surname_generator);
	}

	for (const auto &kv_pair : other->military_unit_class_name_generators) {
		if (!this->military_unit_class_name_generators.contains(kv_pair.first)) {
			this->military_unit_class_name_generators[kv_pair.first] = std::make_unique<name_generator>();
		}

		this->military_unit_class_name_generators[kv_pair.first]->add_names(kv_pair.second->get_names());
	}

	military_unit_class::propagate_names(other->military_unit_class_name_generators, this->ship_name_generator);

	for (const auto &kv_pair : other->transporter_class_name_generators) {
		if (!this->transporter_class_name_generators.contains(kv_pair.first)) {
			this->transporter_class_name_generators[kv_pair.first] = std::make_unique<name_generator>();
		}

		this->transporter_class_name_generators[kv_pair.first]->add_names(kv_pair.second->get_names());
	}

	transporter_class::propagate_names(other->transporter_class_name_generators, this->ship_name_generator);

	if (other->ship_name_generator != nullptr) {
		if (this->ship_name_generator == nullptr) {
			this->ship_name_generator = std::make_unique<name_generator>();
		}

		this->ship_name_generator->add_names(other->ship_name_generator->get_names());
	}

	if (this->group != nullptr) {
		this->group->add_names_from(other);
	}
}

const phenotype_map<int64_t> &culture_base::get_phenotype_weights() const
{
	if (!this->phenotype_weights.empty()) {
		return this->phenotype_weights;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_phenotype_weights();
	}

	return this->phenotype_weights;
}

void culture_base::change_phenotype_weight(const phenotype *phenotype, const int64_t change)
{
	const int64_t weight = (this->phenotype_weights[phenotype] += change);

	if (weight == 0) {
		this->phenotype_weights.erase(phenotype);
	}

	if (this->group != nullptr) {
		this->group->change_phenotype_weight(phenotype, change);
	}
}

void culture_base::add_domain(const domain *domain)
{
	this->domains.insert(domain);

	if (this->group != nullptr) {
		this->group->add_domain(domain);
	}
}

}
