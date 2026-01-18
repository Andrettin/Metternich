#pragma once

#include "culture/culture_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("character/character.h")

namespace archimedes {
	class gsml_data;
}

namespace metternich {

class character;

class bloodline final : public named_data_entry, public data_type<bloodline>
{
	Q_OBJECT

	Q_PROPERTY(metternich::character* founder MEMBER founder WRITE set_founder NOTIFY changed)
	Q_PROPERTY(int weight MEMBER weight READ get_weight NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "bloodline";
	static constexpr const char property_class_identifier[] = "metternich::bloodline*";
	static constexpr const char database_folder[] = "bloodlines";

	static constexpr int max_bloodline_strength = 100;

	explicit bloodline(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const character *get_founder() const
	{
		return this->founder;
	}

	void set_founder(character *founder);

	int get_weight() const
	{
		return this->weight;
	}

	const culture_set &get_cultures() const
	{
		return this->cultures;
	}

signals:
	void changed();

private:
	character *founder = nullptr;
	int weight = 1;
	culture_set cultures; //cultures for which this bloodline can be generated
};

}
