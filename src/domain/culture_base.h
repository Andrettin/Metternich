#pragma once

#include "database/data_entry_container.h"
#include "database/named_data_entry.h"
#include "domain/domain_container.h"
#include "infrastructure/building_class_container.h"
#include "language/name_variant.h"
#include "population/population_class_container.h"
#include "species/phenotype_container.h"
#include "unit/civilian_unit_class_container.h"
#include "unit/military_unit_class_container.h"
#include "unit/transporter_class_container.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("species/phenotype.h")

namespace archimedes {
	class gendered_name_generator;
	class name_generator;
	enum class gender;
}

namespace metternich {

class building_type;
class civilian_unit_type;
class cultural_group;
class culture_history;
class domain;
class government_group;
class government_type;
class military_unit_type;
class office;
class phenotype;
class population_type;
class transporter_type;
enum class domain_tier;

class culture_base : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(metternich::cultural_group* group MEMBER group NOTIFY changed)
	Q_PROPERTY(metternich::cultural_group* upper_group MEMBER group NOTIFY changed)
	Q_PROPERTY(metternich::phenotype* default_phenotype MEMBER default_phenotype)

public:
	using government_variant = std::variant<const government_type *, const government_group *>;
	using title_name_map = std::map<government_variant, std::map<domain_tier, std::string>>;
	using site_title_name_map = std::map<government_variant, std::map<int, std::string>>;
	using office_title_name_map = data_entry_map<office, std::map<government_variant, std::map<domain_tier, std::map<gender, std::string>>>>;

	explicit culture_base(const std::string &identifier);
	~culture_base();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	culture_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	const cultural_group *get_group() const
	{
		return this->group;
	}

	bool is_part_of_group(const cultural_group *group) const;

	phenotype *get_default_phenotype() const;

	const std::string &get_title_name(const government_type *government_type, const domain_tier tier) const;
	const std::string &get_site_title_name(const government_type *government_type, const int tier) const;
	const std::string &get_office_title_name(const office *office, const government_type *government_type, const domain_tier tier, const gender gender) const;

	const building_type *get_building_class_type(const building_class *building_class) const;
	void set_building_class_type(const building_class *building_class, const building_type *building_type);

	const population_type *get_population_class_type(const population_class *population_class) const;
	void set_population_class_type(const population_class *population_class, const population_type *population_type);

	const civilian_unit_type *get_civilian_class_unit_type(const civilian_unit_class *unit_class) const;
	void set_civilian_class_unit_type(const civilian_unit_class *unit_class, const civilian_unit_type *unit_type);

	const military_unit_type *get_military_class_unit_type(const military_unit_class *unit_class) const;
	void set_military_class_unit_type(const military_unit_class *unit_class, const military_unit_type *unit_type);

	const transporter_type *get_transporter_class_type(const transporter_class *transporter_class) const;
	void set_transporter_class_type(const transporter_class *transporter_class, const transporter_type *transporter_type);

	std::string generate_personal_name(const gender gender, const std::map<std::string, int> &used_name_counts) const;
	std::string generate_military_unit_name(const military_unit_type *type, const std::map<std::string, int> &used_name_counts) const;
	std::string generate_transporter_name(const transporter_type *type, const std::map<std::string, int> &used_name_counts) const;

	const name_generator *get_personal_name_generator(const gender gender) const;
	void add_personal_name(const gender gender, const name_variant &name);

	const name_generator *get_surname_generator(const gender gender) const;
	void add_surname(const gender gender, const name_variant &surname);

	const name_generator *get_military_unit_class_name_generator(const military_unit_class *unit_class) const;
	void add_military_unit_class_name(const military_unit_class *unit_class, const name_variant &name);

	const name_generator *get_transporter_class_name_generator(const transporter_class *transporter_class) const;
	void add_transporter_class_name(const transporter_class *transporter_class, const name_variant &name);

	void add_ship_name(const name_variant &ship_name);

	void add_names_from(const culture_base *other);

	const phenotype_map<int64_t> &get_phenotype_weights() const;
	void change_phenotype_weight(const phenotype *phenotype, const int64_t change);

	const domain_set &get_domains() const
	{
		return this->domains;
	}

	void add_domain(const domain *domain);

signals:
	void changed();

private:
	cultural_group *group = nullptr;
	phenotype *default_phenotype = nullptr;
	title_name_map title_names;
	site_title_name_map site_title_names;
	office_title_name_map office_title_names;
	building_class_map<const building_type *> building_class_types;
	population_class_map<const population_type *> population_class_types;
	civilian_unit_class_map<const civilian_unit_type *> civilian_class_unit_types;
	military_unit_class_map<const military_unit_type *> military_class_unit_types;
	transporter_class_map<const transporter_type *> transporter_class_types;
	std::unique_ptr<gendered_name_generator> personal_name_generator;
	std::unique_ptr<gendered_name_generator> surname_generator;
	military_unit_class_map<std::unique_ptr<name_generator>> military_unit_class_name_generators;
	transporter_class_map<std::unique_ptr<name_generator>> transporter_class_name_generators;
	std::unique_ptr<name_generator> ship_name_generator;
	phenotype_map<int64_t> phenotype_weights;
	domain_set domains;
	qunique_ptr<culture_history> history;
};

}
