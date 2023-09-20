#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;
class spell_effect;
enum class military_unit_category;
enum class spell_target;

class spell final : public named_data_entry, public data_type<spell>
{
	Q_OBJECT

	Q_PROPERTY(metternich::spell_target target MEMBER target READ get_target NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(int mana_cost MEMBER mana_cost READ get_mana_cost NOTIFY changed)
	Q_PROPERTY(int range MEMBER range READ get_range NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "spell";
	static constexpr const char property_class_identifier[] = "metternich::spell*";
	static constexpr const char database_folder[] = "spells";

	explicit spell(const std::string &identifier);
	~spell();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	spell_target get_target() const
	{
		return this->target;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	int get_mana_cost() const
	{
		return this->mana_cost;
	}

	int get_range() const
	{
		return this->range;
	}

	const std::vector<military_unit_category> get_military_unit_categories() const
	{
		return this->military_unit_categories;
	}

	bool is_available_for_military_unit_category(const military_unit_category military_unit_category) const;

signals:
	void changed();

private:
	spell_target target;
	metternich::icon *icon = nullptr;
	int mana_cost = 0;
	int range = 0;
	std::vector<military_unit_category> military_unit_categories;
	std::vector<qunique_ptr<spell_effect>> effects;
};

}
