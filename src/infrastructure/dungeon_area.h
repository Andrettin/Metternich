#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class domain_event;
class portrait;
class site;

template <typename scope_type>
class and_condition;

class dungeon_area final : public named_data_entry, public data_type<dungeon_area>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::portrait* portrait MEMBER portrait READ get_portrait NOTIFY changed)
	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)
	Q_PROPERTY(bool entrance MEMBER entrance READ is_entrance NOTIFY changed)
	Q_PROPERTY(bool allow_retreat MEMBER allow_retreat READ allows_retreat NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "dungeon_area";
	static constexpr const char property_class_identifier[] = "metternich::dungeon_area*";
	static constexpr const char database_folder[] = "dungeon_areas";

	explicit dungeon_area(const std::string &identifier);
	~dungeon_area();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	const std::string &get_description() const
	{
		return this->description;
	}

	Q_INVOKABLE void set_description(const std::string &description)
	{
		this->description = description;
	}

	QString get_description_qstring() const
	{
		return QString::fromStdString(this->get_description());
	}

	bool is_entrance() const
	{
		return this->entrance;
	}

	bool allows_retreat() const
	{
		return this->allow_retreat;
	}

	const and_condition<site> *get_conditions() const
	{
		return this->conditions.get();
	}

	const domain_event *get_event() const
	{
		return this->event;
	}

signals:
	void changed();

private:
	const metternich::portrait *portrait = nullptr;
	std::string description;
	bool entrance = false;
	bool allow_retreat = true;
	std::unique_ptr<const and_condition<site>> conditions;
	domain_event *event = nullptr;
};

}
