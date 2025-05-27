#pragma once

#include "database/named_data_entry.h"
#include "language/name_variant.h"

namespace archimedes {
	class gendered_name_generator;
	class name_generator;
	enum class gender;
}

namespace metternich {

class taxon;
enum class taxonomic_rank;

class taxon_base : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(metternich::taxon* supertaxon MEMBER supertaxon READ get_supertaxon)
	Q_PROPERTY(bool ethereal MEMBER ethereal)

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

	const name_generator *get_given_name_generator(const gender gender) const;

	void add_given_name(const gender gender, const name_variant &name);
	void add_given_names_from(const taxon_base *other);

private:
	taxon *supertaxon = nullptr;
	bool ethereal = false;
	std::unique_ptr<gendered_name_generator> given_name_generator; //given names, mapped to the gender they pertain to (use gender::none for names which should be available for both genders)
};

}
