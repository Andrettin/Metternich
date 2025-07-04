#pragma once

#include "trait_base.h"

namespace metternich {

class country;
enum class idea_type;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class modifier;

class idea_trait : public trait_base
{
	Q_OBJECT

public:
	explicit idea_trait(const std::string &identifier);
	~idea_trait();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	virtual idea_type get_idea_type() const = 0;

	const and_condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

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
	std::unique_ptr<const and_condition<country>> conditions; //conditions for the country to have the idea with this trait
	std::unique_ptr<const metternich::modifier<const country>> modifier;
	std::unique_ptr<const metternich::modifier<const country>> scaled_modifier;
};

}
