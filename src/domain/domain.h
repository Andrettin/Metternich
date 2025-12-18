#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("domain/country_ai.h")
Q_MOC_INCLUDE("domain/country_turn_data.h")
Q_MOC_INCLUDE("domain/domain_game_data.h")
Q_MOC_INCLUDE("domain/domain_tier.h")
Q_MOC_INCLUDE("domain/government_type.h")
Q_MOC_INCLUDE("map/site.h")
Q_MOC_INCLUDE("religion/religion.h")

namespace archimedes {
	class era;
	enum class gender;
}

namespace metternich {

class character;
class country_ai;
class country_economy;
class country_government;
class country_military;
class country_technology;
class country_turn_data;
class culture;
class domain_game_data;
class domain_history;
class government_group;
class government_type;
class office;
class population_class;
class province;
class religion;
class site;
enum class country_type;
enum class domain_tier;

template <typename scope_type>
class and_condition;

class domain final : public named_data_entry, public data_type<domain>
{
	Q_OBJECT

	Q_PROPERTY(metternich::country_type type MEMBER type READ get_type NOTIFY changed)
	Q_PROPERTY(bool tribe READ is_tribe CONSTANT)
	Q_PROPERTY(bool clade READ is_clade CONSTANT)
	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)
	Q_PROPERTY(QString flag READ get_flag_qstring NOTIFY changed)
	Q_PROPERTY(metternich::domain_tier default_tier MEMBER default_tier READ get_default_tier)
	Q_PROPERTY(metternich::domain_tier min_tier MEMBER min_tier READ get_min_tier)
	Q_PROPERTY(metternich::domain_tier max_tier MEMBER max_tier READ get_max_tier)
	Q_PROPERTY(metternich::religion* default_religion MEMBER default_religion NOTIFY changed)
	Q_PROPERTY(metternich::government_type* default_government_type MEMBER default_government_type NOTIFY changed)
	Q_PROPERTY(metternich::site* default_capital MEMBER default_capital NOTIFY changed)
	Q_PROPERTY(bool short_name MEMBER short_name READ has_short_name NOTIFY changed)
	Q_PROPERTY(bool definite_article MEMBER definite_article NOTIFY changed)
	Q_PROPERTY(QVariantList available_technologies READ get_available_technologies_qvariant_list NOTIFY changed)
	Q_PROPERTY(metternich::domain_game_data* game_data READ get_game_data NOTIFY game_data_changed)
	Q_PROPERTY(metternich::country_turn_data* turn_data READ get_turn_data NOTIFY turn_data_changed)
	Q_PROPERTY(metternich::country_ai* ai READ get_ai NOTIFY ai_changed)

public:
	using government_variant = std::variant<const government_type *, const government_group *>;
	using title_name_map = std::map<government_variant, std::map<domain_tier, std::string>>;
	using office_title_name_map = data_entry_map<office, std::map<government_variant, std::map<domain_tier, std::map<gender, std::string>>>>;

	static constexpr const char class_identifier[] = "domain";
	static constexpr const char property_class_identifier[] = "metternich::domain*";
	static constexpr const char database_folder[] = "domains";
	static constexpr bool history_enabled = true;

	static constexpr int min_opinion = -200;
	static constexpr int max_opinion = 200;

	explicit domain(const std::string &identifier);
	~domain();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;
	virtual data_entry_history *get_history_base() override;

	domain_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	void reset_game_data();

	domain_game_data *get_game_data() const
	{
		return this->game_data.get();
	}

	country_economy *get_economy() const;
	country_government *get_government() const;
	country_military *get_military() const;
	country_technology *get_technology() const;

	void reset_turn_data();

	country_turn_data *get_turn_data() const
	{
		return this->turn_data.get();
	}

	void reset_ai();

	country_ai *get_ai() const
	{
		return this->ai.get();
	}

	country_type get_type() const
	{
		return this->type;
	}

	bool is_playable() const;
	bool is_tribe() const;
	bool is_clade() const;

	const QColor &get_color() const;

	const std::string &get_flag() const
	{
		return this->flag;
	}

	Q_INVOKABLE void set_flag(const std::string &flag)
	{
		this->flag = flag;
	}

	QString get_flag_qstring() const
	{
		return QString::fromStdString(this->get_flag());
	}

	domain_tier get_default_tier() const
	{
		return this->default_tier;
	}

	domain_tier get_min_tier() const
	{
		return this->min_tier;
	}

	domain_tier get_max_tier() const
	{
		return this->max_tier;
	}

	using named_data_entry::get_name;

	const std::string &get_name(const government_type *government_type, const domain_tier tier) const;
	std::string get_titled_name(const government_type *government_type, const domain_tier tier, const culture *culture, const religion *religion) const;
	const std::string &get_title_name(const government_type *government_type, const domain_tier tier, const culture *culture, const religion *religion) const;
	const std::string &get_office_title_name(const office *office, const government_type *government_type, const domain_tier tier, const gender gender, const culture *culture, const religion *religion) const;

	const std::vector<const culture *> &get_cultures() const
	{
		return this->cultures;
	}

	bool is_culture_allowed(const culture *culture) const;

	const culture *get_default_culture() const
	{
		if (this->get_cultures().empty()) {
			return nullptr;
		}

		return this->get_cultures().at(0);
	}

	const religion *get_default_religion() const
	{
		return this->default_religion;
	}

	const government_type *get_default_government_type() const
	{
		return this->default_government_type;
	}

	const site *get_default_capital() const
	{
		return this->default_capital;
	}

	bool has_short_name() const
	{
		return this->short_name;
	}

	const std::map<std::string, std::unique_ptr<const and_condition<domain>>> &get_conditional_flags() const
	{
		return this->conditional_flags;
	}

	const std::vector<const era *> &get_eras() const
	{
		return this->eras;
	}

	const std::vector<province *> &get_core_provinces() const
	{
		return this->core_provinces;
	}

	std::vector<const province *> get_core_provinces_for_tier(const domain_tier tier) const;

	const std::vector<const site *> &get_core_holdings() const
	{
		return this->core_holdings;
	}

	std::vector<const site *> get_core_holdings_for_tier(const domain_tier tier) const;

	bool can_declare_war() const;

	std::vector<const technology *> get_available_technologies() const;
	QVariantList get_available_technologies_qvariant_list() const;

signals:
	void changed();
	void game_data_changed() const;
	void turn_data_changed() const;
	void ai_changed() const;

private:
	country_type type{};
	QColor color;
	std::string flag;
	domain_tier default_tier{};
	domain_tier min_tier{};
	domain_tier max_tier{};
	std::vector<const culture *> cultures;
	religion *default_religion = nullptr;
	government_type *default_government_type = nullptr;
	site *default_capital = nullptr;
	bool short_name = false;
	bool definite_article = false;
	std::map<std::string, std::unique_ptr<const and_condition<domain>>> conditional_flags;
	std::vector<const era *> eras; //eras this country appears in at start, for random maps
	title_name_map short_names;
	title_name_map title_names;
	office_title_name_map office_title_names;
	std::vector<province *> core_provinces;
	std::vector<const site *> core_holdings;
	std::map<domain_tier, std::vector<const province *>> tier_core_provinces;
	std::map<domain_tier, std::vector<const site *>> tier_core_holdings;
	qunique_ptr<domain_history> history;
	qunique_ptr<domain_game_data> game_data;
	qunique_ptr<country_turn_data> turn_data;
	qunique_ptr<country_ai> ai;
};

}
