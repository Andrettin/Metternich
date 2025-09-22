#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class portrait;
class site;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class modifier;

class dungeon final : public named_data_entry, public data_type<dungeon>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::portrait* portrait MEMBER portrait READ get_portrait NOTIFY changed)
	Q_PROPERTY(int level MEMBER level READ get_level NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "dungeon";
	static constexpr const char property_class_identifier[] = "metternich::dungeon*";
	static constexpr const char database_folder[] = "dungeons";

	explicit dungeon(const std::string &identifier);
	~dungeon();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	int get_level() const
	{
		return this->level;
	}

	const and_condition<site> *get_conditions() const
	{
		return this->conditions.get();
	}

signals:
	void changed();

private:
	const metternich::portrait *portrait = nullptr;
	int level = 0; //expected level of characters to take on this dungeon
	std::unique_ptr<const and_condition<site>> conditions;
};

}
