#pragma once

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("map/province.h")
Q_MOC_INCLUDE("map/site.h")

namespace metternich {

class character;
class country;
class military_unit;
class province;
class site;

//collection of military units which are moving to or attacking a province, or visiting a site
class army final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::country* country READ get_country CONSTANT)
	Q_PROPERTY(const metternich::province* target_province READ get_target_province CONSTANT)
	Q_PROPERTY(const metternich::site* target_site READ get_target_site CONSTANT)
	Q_PROPERTY(QVariantList military_units READ get_military_units_qvariant_list NOTIFY military_units_changed)

public:
	using target_variant = std::variant<std::monostate, const province *, const site *>;

	explicit army(const std::vector<military_unit *> &military_units, target_variant &&target);
	~army();

	void do_turn();

	const metternich::country *get_country() const
	{
		return this->country;
	}

	const province *get_target_province() const
	{
		if (std::holds_alternative<const province *>(this->target)) {
			return std::get<const province *>(this->target);
		}

		return nullptr;
	}

	const site *get_target_site() const
	{
		if (std::holds_alternative<const site *>(this->target)) {
			return std::get<const site *>(this->target);
		}

		return nullptr;
	}

	const std::vector<military_unit *> &get_military_units() const
	{
		return this->military_units;
	}

	QVariantList get_military_units_qvariant_list() const;
	void add_military_unit(military_unit *military_unit);
	void remove_military_unit(military_unit *military_unit);

	int get_score() const;

	const character *get_commander() const;

signals:
	void military_units_changed();

private:
	const metternich::country *country = nullptr;
	std::vector<military_unit *> military_units;
	target_variant target; //the province or site the army is moving to
};

}
