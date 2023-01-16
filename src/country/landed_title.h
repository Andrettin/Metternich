#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/qunique_ptr.h"

namespace archimedes {
	enum class gender;
}

namespace metternich {

class country;
class landed_title_game_data;
class site;
enum class landed_title_tier;

class landed_title final : public named_data_entry, public data_type<landed_title>
{
	Q_OBJECT

	Q_PROPERTY(metternich::landed_title_tier default_tier MEMBER default_tier READ get_default_tier)
	Q_PROPERTY(metternich::landed_title_tier min_tier MEMBER min_tier READ get_min_tier)
	Q_PROPERTY(metternich::landed_title_tier max_tier MEMBER max_tier READ get_max_tier)
	Q_PROPERTY(metternich::landed_title_game_data* game_data READ get_game_data NOTIFY game_data_changed)

public:
	using title_name_map = std::map<landed_title_tier, std::string>;
	using ruler_title_name_map = std::map<landed_title_tier, std::map<gender, std::string>>;

	static constexpr const char class_identifier[] = "landed_title";
	static constexpr const char property_class_identifier[] = "metternich::landed_title*";
	static constexpr const char database_folder[] = "landed_titles";

	static const std::set<std::string> database_dependencies;

	static void process_title_names(title_name_map &title_names, const gsml_data &scope);
	static void process_character_title_names(ruler_title_name_map &character_title_names, const gsml_data &scope);
	static void process_character_title_name_scope(std::map<gender, std::string> &character_title_names, const gsml_data &scope);

	explicit landed_title(const std::string &identifier);

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	void reset_game_data();

	landed_title_game_data *get_game_data() const
	{
		return this->game_data.get();
	}

	const metternich::country *get_country() const
	{
		return this->country;
	}

	void set_country(const metternich::country *country);

	const metternich::site *get_site() const
	{
		return this->site;
	}

	void set_site(const metternich::site *site);

	landed_title_tier get_default_tier() const
	{
		return this->default_tier;
	}

	landed_title_tier get_min_tier() const
	{
		return this->min_tier;
	}

	landed_title_tier get_max_tier() const
	{
		return this->max_tier;
	}

	const std::string &get_title_name(const landed_title_tier tier) const;
	const std::string &get_ruler_title_name(const landed_title_tier tier, const gender gender) const;

signals:
	void game_data_changed();

private:
	const metternich::country *country = nullptr;
	const metternich::site *site = nullptr;
	landed_title_tier default_tier;
	landed_title_tier min_tier;
	landed_title_tier max_tier;
	title_name_map title_names;
	ruler_title_name_map ruler_title_names;
	qunique_ptr<landed_title_game_data> game_data;
};

}
