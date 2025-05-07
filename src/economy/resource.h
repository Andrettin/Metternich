#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("economy/commodity.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class commodity;
class icon;
class improvement;
class technology;
class terrain_type;
enum class site_type;

template <typename scope_type>
class modifier;

//resources are present on tiles, allowing the tile to produce a given commodity
//multiple resources can produce the same commodity
class resource final : public named_data_entry, public data_type<resource>
{
	Q_OBJECT

	Q_PROPERTY(QString plural_name READ get_plural_name_qstring NOTIFY changed)
	Q_PROPERTY(metternich::commodity* commodity MEMBER commodity NOTIFY changed)
	Q_PROPERTY(const metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(const metternich::icon* tiny_icon MEMBER tiny_icon READ get_tiny_icon NOTIFY changed)
	Q_PROPERTY(bool natural_wonder MEMBER natural_wonder READ is_natural_wonder NOTIFY changed)
	Q_PROPERTY(bool coastal MEMBER coastal READ is_coastal NOTIFY changed)
	Q_PROPERTY(bool near_water MEMBER near_water READ is_near_water NOTIFY changed)
	Q_PROPERTY(bool prospectable MEMBER prospectable READ is_prospectable NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(metternich::technology* discovery_technology MEMBER discovery_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "resource";
	static constexpr const char property_class_identifier[] = "metternich::resource*";
	static constexpr const char database_folder[] = "resources";

	explicit resource(const std::string &identifier);
	~resource();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const std::string &get_plural_name() const
	{
		return this->plural_name;
	}

	Q_INVOKABLE void set_plural_name(const std::string &plural_name)
	{
		this->plural_name = plural_name;
	}

	QString get_plural_name_qstring() const
	{
		return QString::fromStdString(this->get_plural_name());
	}

	const metternich::commodity *get_commodity() const
	{
		return this->commodity;
	}

	const metternich::icon *get_icon() const;

	const metternich::icon *get_tiny_icon() const
	{
		return this->tiny_icon;
	}

	bool is_natural_wonder() const
	{
		return this->natural_wonder;
	}

	bool is_coastal() const
	{
		return this->coastal;
	}

	bool is_near_water() const
	{
		return this->near_water;
	}

	bool is_prospectable() const
	{
		return this->prospectable;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const technology *get_discovery_technology() const
	{
		return this->discovery_technology;
	}

	const std::vector<const terrain_type *> &get_terrain_types() const
	{
		return this->terrain_types;
	}

	const std::set<site_type> &get_site_types() const
	{
		return this->site_types;
	}

	const terrain_type *get_fallback_terrain(const terrain_type *terrain) const;

	const std::vector<const improvement *> &get_improvements() const
	{
		return this->improvements;
	}

	void add_improvement(const improvement *improvement)
	{
		this->improvements.push_back(improvement);
	}

	const modifier<const site> *get_modifier() const
	{
		return this->modifier.get();
	}

	const modifier<const site> *get_improved_modifier() const
	{
		return this->improved_modifier.get();
	}

signals:
	void changed();

private:
	std::string plural_name;
	metternich::commodity *commodity = nullptr;
	const metternich::icon *icon = nullptr;
	const metternich::icon *tiny_icon = nullptr;
	bool natural_wonder = false;
	bool coastal = false;
	bool near_water = false;
	bool prospectable = false;
	technology *required_technology = nullptr; //technology which is required to see the resource on the tile
	technology *discovery_technology = nullptr; //technology which is obtained when exploring this resource tile
	std::vector<const terrain_type *> terrain_types;
	std::set<site_type> site_types;
	std::vector<const improvement *> improvements;
	std::unique_ptr<metternich::modifier<const site>> modifier;
	std::unique_ptr<metternich::modifier<const site>> improved_modifier;
};

}
