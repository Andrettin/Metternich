#include "metternich.h"

#include "fallback_name_generator.h"

#include "language/gendered_name_generator.h"
#include "language/name_generator.h"
#include "unit/military_unit_class.h"
#include "unit/transporter_class.h"
#include "util/gender.h"
#include "util/vector_util.h"

namespace metternich {

fallback_name_generator::fallback_name_generator()
{
}

fallback_name_generator::~fallback_name_generator()
{
}

const name_generator *fallback_name_generator::get_personal_name_generator(const gender gender) const
{
	if (this->personal_name_generator != nullptr) {
		return this->personal_name_generator->get_name_generator(gender);
	}

	return nullptr;
}

void fallback_name_generator::add_personal_names(const std::unique_ptr<gendered_name_generator> &source_name_generator)
{
	if (this->personal_name_generator == nullptr) {
		this->personal_name_generator = std::make_unique<gendered_name_generator>();
	}

	this->personal_name_generator->add_names_from(source_name_generator);
}

const name_generator *fallback_name_generator::get_surname_generator(const gender gender) const
{
	if (this->surname_generator != nullptr) {
		return this->surname_generator->get_name_generator(gender);
	}

	return nullptr;
}

void fallback_name_generator::add_surnames(const std::unique_ptr<gendered_name_generator> &source_name_generator)
{
	if (this->surname_generator == nullptr) {
		this->surname_generator = std::make_unique<gendered_name_generator>();
	}

	this->surname_generator->add_names_from(source_name_generator);
}

const name_generator *fallback_name_generator::get_military_unit_class_name_generator(const military_unit_class *unit_class) const
{
	const auto find_iterator = this->military_unit_class_name_generators.find(unit_class);
	if (find_iterator != this->military_unit_class_name_generators.end()) {
		return find_iterator->second.get();
	}

	if (unit_class->is_ship() && this->ship_name_generator != nullptr) {
		return this->ship_name_generator.get();
	}

	return nullptr;
}

void fallback_name_generator::add_military_unit_class_names(const military_unit_class_map<std::unique_ptr<name_generator>> &unit_class_names)
{
	for (const auto &kv_pair : unit_class_names) {
		if (!this->military_unit_class_name_generators.contains(kv_pair.first)) {
			this->military_unit_class_name_generators[kv_pair.first] = std::make_unique<name_generator>();
		}

		this->military_unit_class_name_generators[kv_pair.first]->add_names(kv_pair.second->get_names());
	}

	military_unit_class::propagate_names(unit_class_names, this->ship_name_generator);
}

const name_generator *fallback_name_generator::get_transporter_class_name_generator(const transporter_class *transporter_class) const
{
	const auto find_iterator = this->transporter_class_name_generators.find(transporter_class);
	if (find_iterator != this->transporter_class_name_generators.end()) {
		return find_iterator->second.get();
	}

	if (transporter_class->is_ship() && this->ship_name_generator != nullptr) {
		return this->ship_name_generator.get();
	}

	return nullptr;
}

void fallback_name_generator::add_transporter_class_names(const transporter_class_map<std::unique_ptr<name_generator>> &transporter_class_names)
{
	for (const auto &kv_pair : transporter_class_names) {
		if (!this->transporter_class_name_generators.contains(kv_pair.first)) {
			this->transporter_class_name_generators[kv_pair.first] = std::make_unique<name_generator>();
		}

		this->transporter_class_name_generators[kv_pair.first]->add_names(kv_pair.second->get_names());
	}

	transporter_class::propagate_names(transporter_class_names, this->ship_name_generator);
}

void fallback_name_generator::add_ship_names(const std::vector<name_variant> &ship_names)
{
	if (this->ship_name_generator == nullptr) {
		this->ship_name_generator = std::make_unique<name_generator>();
	}

	this->ship_name_generator->add_names(ship_names);
}

}
