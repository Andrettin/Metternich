#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "trait_base.h"

namespace metternich {

class country;

template <typename scope_type>
class modifier;

class research_organization_trait final : public trait_base, public data_type<research_organization_trait>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "research_organization_trait";
	static constexpr const char property_class_identifier[] = "metternich::research_organization_trait*";
	static constexpr const char database_folder[] = "traits/research_organization";

	explicit research_organization_trait(const std::string &identifier);
	~research_organization_trait();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

	const metternich::modifier<const country> *get_scaled_modifier() const
	{
		return this->scaled_modifier.get();
	}

signals:
	void changed();

private:
	std::unique_ptr<const metternich::modifier<const country>> modifier;
	std::unique_ptr<const metternich::modifier<const country>> scaled_modifier;
};

}
