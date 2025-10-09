#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/dice.h"

Q_MOC_INCLUDE("character/saving_throw_type.h")

namespace metternich {

class saving_throw_type;

template <typename scope_type>
class effect_list;

class status_effect final : public named_data_entry, public data_type<status_effect>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::saving_throw_type* saving_throw_type MEMBER saving_throw_type READ get_saving_throw_type NOTIFY changed)
	Q_PROPERTY(int saving_throw_modifier MEMBER saving_throw_modifier READ get_saving_throw_modifier NOTIFY changed)
	Q_PROPERTY(archimedes::dice duration_rounds_dice MEMBER duration_rounds_dice READ get_duration_rounds_dice NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "status_effect";
	static constexpr const char property_class_identifier[] = "metternich::status_effect*";
	static constexpr const char database_folder[] = "status_effects";

	explicit status_effect(const std::string &identifier);
	~status_effect();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::saving_throw_type *get_saving_throw_type() const
	{
		return this->saving_throw_type;
	}

	int get_saving_throw_modifier() const
	{
		return this->saving_throw_modifier;
	}

	const dice &get_duration_rounds_dice() const
	{
		return this->duration_rounds_dice;
	}

	const effect_list<const character> *get_end_effects() const
	{
		return this->end_effects.get();
	}

signals:
	void changed();

private:
	const metternich::saving_throw_type *saving_throw_type = nullptr;
	int saving_throw_modifier = 0;
	dice duration_rounds_dice;
	std::unique_ptr<effect_list<const character>> end_effects; //effects after the duration has passed
};

}
