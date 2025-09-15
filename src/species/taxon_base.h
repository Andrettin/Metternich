#pragma once

#include "database/data_entry_container.h"
#include "database/named_data_entry.h"
#include "language/name_variant.h"
#include "util/dice.h"

namespace archimedes {
	class gendered_name_generator;
	class name_generator;
	enum class gender;
}

namespace metternich {

class item_slot;
class taxon;
enum class starting_age_category;
enum class taxonomic_rank;

class taxon_base : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(metternich::taxon* supertaxon MEMBER supertaxon READ get_supertaxon NOTIFY changed)
	Q_PROPERTY(bool ethereal MEMBER ethereal NOTIFY changed)
	Q_PROPERTY(int adulthood_age MEMBER adulthood_age NOTIFY changed)
	Q_PROPERTY(int middle_age MEMBER middle_age NOTIFY changed)
	Q_PROPERTY(int old_age MEMBER old_age NOTIFY changed)
	Q_PROPERTY(int venerable_age MEMBER venerable_age NOTIFY changed)
	Q_PROPERTY(archimedes::dice maximum_age_modifier MEMBER maximum_age_modifier NOTIFY changed)

protected:
	explicit taxon_base(const std::string &identifier);
	~taxon_base();

public:
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;

	virtual taxonomic_rank get_rank() const = 0;

	taxon *get_supertaxon() const
	{
		return this->supertaxon;
	}

	const taxon *get_supertaxon_of_rank(const taxonomic_rank rank) const;
	bool is_subtaxon_of(const taxon *other_taxon) const;

	bool is_ethereal() const;

	virtual const std::string &get_common_name() const = 0;

	int get_adulthood_age() const;
	int get_middle_age() const;
	int get_old_age() const;
	int get_venerable_age() const;
	const dice &get_maximum_age_modifier() const;
	const dice &get_starting_age_modifier(const starting_age_category category) const;

	const data_entry_map<item_slot, int> &get_item_slot_counts() const;
	int get_item_slot_count(const item_slot *slot) const;

	const name_generator *get_given_name_generator(const gender gender) const;

	void add_given_name(const gender gender, const name_variant &name);
	void add_given_names_from(const taxon_base *other);

signals:
	void changed();

private:
	taxon *supertaxon = nullptr;
	bool ethereal = false;
	int adulthood_age = 0;
	int middle_age = 0;
	int old_age = 0;
	int venerable_age = 0;
	dice maximum_age_modifier;
	std::map<starting_age_category, dice> starting_age_modifiers;
	data_entry_map<item_slot, int> item_slot_counts;
	std::unique_ptr<gendered_name_generator> given_name_generator; //given names, mapped to the gender they pertain to (use gender::none for names which should be available for both genders)
};

}
