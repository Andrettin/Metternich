#pragma once

#include "language/name_variant.h"
#include "unit/military_unit_class_container.h"
#include "unit/transporter_class_container.h"
#include "util/singleton.h"

namespace archimedes {
	class gendered_name_generator;
	class name_generator;
	enum class gender;
}

namespace metternich {

class fallback_name_generator final : public singleton<fallback_name_generator>
{
public:
	fallback_name_generator();
	~fallback_name_generator();

	const name_generator *get_personal_name_generator(const gender gender) const;
	void add_personal_names(const std::unique_ptr<gendered_name_generator> &source_name_generator);

	const name_generator *get_surname_generator(const gender gender) const;
	void add_surnames(const std::unique_ptr<gendered_name_generator> &source_name_generator);

	const name_generator *get_military_unit_class_name_generator(const military_unit_class *unit_class) const;
	void add_military_unit_class_names(const military_unit_class_map<std::unique_ptr<name_generator>> &unit_class_names);

	const name_generator *get_transporter_class_name_generator(const transporter_class *transporter_class) const;
	void add_transporter_class_names(const transporter_class_map<std::unique_ptr<name_generator>> &transporter_class_names);

	void add_ship_names(const std::vector<name_variant> &ship_names);

private:
	//name generation lists containing all names (i.e. from each culture)
	std::unique_ptr<gendered_name_generator> personal_name_generator;
	std::unique_ptr<gendered_name_generator> surname_generator;
	military_unit_class_map<std::unique_ptr<name_generator>> military_unit_class_name_generators;
	transporter_class_map<std::unique_ptr<name_generator>> transporter_class_name_generators;
	std::unique_ptr<name_generator> ship_name_generator;
};

}
