#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

#include <QColor>

#include <string>
#include <vector>

namespace metternich {

class character;
class culture;
class government_type;
class holding;
class holding_slot;
class law;
class law_group;
class province;
class religion;
class star_system;
class territory;
class world;
enum class landed_title_tier;

class landed_title final : public data_entry, public data_type<landed_title>
{
	Q_OBJECT

	Q_PROPERTY(QString titled_name READ get_titled_name_qstring NOTIFY titled_name_changed)
	Q_PROPERTY(QColor color MEMBER color READ get_color)
	Q_PROPERTY(metternich::star_system* star_system READ get_star_system CONSTANT)
	Q_PROPERTY(metternich::character* holder READ get_holder WRITE set_holder NOTIFY holder_changed)
	Q_PROPERTY(metternich::landed_title* holder_title MEMBER holder_title WRITE set_holder_title)
	Q_PROPERTY(metternich::landed_title* liege_title MEMBER liege_title)
	Q_PROPERTY(metternich::landed_title* de_jure_liege_title READ get_de_jure_liege_title WRITE set_de_jure_liege_title NOTIFY de_jure_liege_title_changed)
	Q_PROPERTY(metternich::landed_title* realm READ get_realm NOTIFY realm_changed)
	Q_PROPERTY(metternich::territory* capital_territory READ get_capital_territory)
	Q_PROPERTY(metternich::province* capital_province READ get_capital_province WRITE set_capital_province NOTIFY capital_province_changed)
	Q_PROPERTY(metternich::world* capital_world MEMBER capital_world READ get_capital_world NOTIFY capital_world_changed)
	Q_PROPERTY(QString flag_tag READ get_flag_tag_qstring WRITE set_flag_tag_qstring)
	Q_PROPERTY(QString flag_path READ get_flag_path_qstring CONSTANT)
	Q_PROPERTY(QVariantList laws READ get_laws_qvariant_list NOTIFY laws_changed)
	Q_PROPERTY(metternich::government_type* government_type READ get_government_type NOTIFY government_type_changed)
	Q_PROPERTY(bool active_post_amalgamation MEMBER active_post_amalgamation READ is_active_post_amalgamation)

public:
	static constexpr const char *class_identifier = "landed_title";
	static constexpr const char *database_folder = "landed_titles";
	static constexpr const char *barony_prefix = "b_";
	static constexpr const char *county_prefix = "c_";
	static constexpr const char *duchy_prefix = "d_";
	static constexpr const char *kingdom_prefix = "k_";
	static constexpr const char *empire_prefix = "e_";

	//string identifiers for landed title tiers
	static constexpr const char *barony_identifier = "barony";
	static constexpr const char *county_identifier = "county";
	static constexpr const char *duchy_identifier = "duchy";
	static constexpr const char *kingdom_identifier = "kingdom";
	static constexpr const char *empire_identifier = "empire";

	//string identifiers for landed title tier holder title names
	static constexpr const char *baron_identifier = "baron";
	static constexpr const char *count_identifier = "count";
	static constexpr const char *duke_identifier = "duke";
	static constexpr const char *king_identifier = "king";
	static constexpr const char *emperor_identifier = "emperor";

	static const std::vector<landed_title *> &get_tier_titles(const landed_title_tier tier);
	static landed_title *add(const std::string &identifier);

	static const char *get_tier_identifier(const landed_title_tier tier);
	static const char *get_tier_holder_identifier(const landed_title_tier tier);
	static std::string get_tier_name(const landed_title_tier tier);

private:
	static inline std::map<landed_title_tier, std::vector<landed_title *>> titles_by_tier;

public:
	landed_title(const std::string &identifier) : data_entry(identifier)
	{
		connect(this, &landed_title::government_type_changed, this, &landed_title::titled_name_changed);
	}

	virtual void process_gsml_dated_property(const gsml_property &property, const QDateTime &date) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void initialize_history() override;
	virtual void check() const override;
	virtual void check_history() const override;

	virtual std::string get_name() const override;
	std::string get_tier_title_name() const;
	std::string get_titled_name() const;

	QString get_titled_name_qstring() const
	{
		return QString::fromStdString(this->get_titled_name());
	}

	std::string get_holder_title_name() const;

	std::vector<std::vector<std::string>> get_tag_suffix_list_with_fallbacks() const;

	const QColor &get_color() const
	{
		return this->color;
	}

	landed_title_tier get_tier() const
	{
		return this->tier;
	}

	character *get_holder() const
	{
		return this->holder;
	}

	void set_holder(character *character);
	void set_holder_title(landed_title *title);

	metternich::holding_slot *get_holding_slot() const
	{
		return this->holding_slot;
	}

	void set_holding_slot(holding_slot *holding_slot);
	metternich::holding *get_holding() const;

	territory *get_territory() const;

	metternich::province *get_province() const
	{
		return this->province;
	}

	void set_province(province *province)
	{
		this->province = province;
		this->set_capital_province(province);
	}

	metternich::world *get_world() const
	{
		return this->world;
	}

	void set_world(world *world)
	{
		if (world == this->get_world()) {
			return;
		}

		this->world = world;
		this->set_capital_world(world);
	}

	metternich::star_system *get_star_system() const
	{
		return this->star_system;
	}

	void set_star_system(star_system *system)
	{
		if (system == this->get_star_system()) {
			return;
		}

		this->star_system = system;
	}

	landed_title *get_realm() const;
	landed_title *get_liege_title() const;

	landed_title *get_de_jure_liege_title() const
	{
		return this->de_jure_liege_title;
	}

	void set_de_jure_liege_title(landed_title *title);

	const std::vector<landed_title *> &get_de_jure_vassal_titles() const
	{
		return this->de_jure_vassal_titles;
	}

	void add_de_jure_vassal_title(landed_title *title)
	{
		this->de_jure_vassal_titles.push_back(title);
	}

	void remove_de_jure_vassal_title(landed_title *title);

	landed_title *get_tier_title(const landed_title_tier tier) const;
	landed_title *get_tier_de_jure_title(const landed_title_tier tier) const;
	landed_title *get_county() const;
	landed_title *get_de_jure_county() const;
	landed_title *get_duchy() const;
	landed_title *get_de_jure_duchy() const;
	landed_title *get_kingdom() const;
	landed_title *get_de_jure_kingdom() const;
	landed_title *get_empire() const;
	landed_title *get_de_jure_empire() const;

	bool is_titular() const
	{
		//a title is not titular if it has no de jure vassals, or if it is a county belonging to a province, or a barony belonging to a holding
		return this->get_de_jure_vassal_titles().empty() && this->get_territory() == nullptr && this->get_holding_slot() == nullptr && this->get_star_system() == nullptr;
	}

	bool is_primary() const;

	territory *get_capital_territory() const;

	metternich::province *get_capital_province() const
	{
		return this->capital_province;
	}

	void set_capital_province(province *province)
	{
		if (province == this->get_capital_province()) {
			return;
		}

		this->capital_province = province;
		emit capital_province_changed();
	}

	metternich::world *get_capital_world() const
	{
		return this->capital_world;
	}

	void set_capital_world(world *world)
	{
		if (world == this->get_capital_world()) {
			return;
		}

		this->capital_world = world;
		emit capital_world_changed();
	}

	metternich::holding *get_capital_holding() const;

	culture *get_culture() const;
	religion *get_religion() const;

	const std::string &get_flag_tag() const
	{
		if (this->flag_tag.empty()) {
			return this->get_identifier();
		}

		return this->flag_tag;
	}

	void set_flag_tag(const std::string &flag_tag)
	{
		if (flag_tag == this->get_flag_tag()) {
			return;
		}

		this->flag_tag = flag_tag;
	}

	QString get_flag_tag_qstring() const
	{
		return QString::fromStdString(this->get_flag_tag());
	}

	void set_flag_tag_qstring(const QString &flag_tag)
	{
		this->set_flag_tag(flag_tag.toStdString());
	}

	const std::filesystem::path &get_flag_path() const;

	QString get_flag_path_qstring() const
	{
		return "file:///" + QString::fromStdString(this->get_flag_path().string());
	}

	QVariantList get_laws_qvariant_list() const;

	law *get_law(law_group *law_group) const
	{
		auto find_iterator = this->laws.find(law_group);
		if (find_iterator != this->laws.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	bool has_law(const law *law) const;

	Q_INVOKABLE void add_law(law *law);
	Q_INVOKABLE void remove_law(law *law);
	void clear_non_succession_laws();
	void set_missing_laws_to_default();
	void set_missing_law_to_default_for_law_group(law_group *law_group);

	void copy_title_laws_if_missing(const landed_title *title)
	{
		for (const auto &kv_pair : title->laws) {
			if (!this->laws.contains(kv_pair.first)) {
				this->add_law(kv_pair.second);
			}
		}
	}

	government_type *get_government_type() const;

	bool is_active_post_amalgamation() const
	{
		return this->active_post_amalgamation;
	}

signals:
	void titled_name_changed();
	void holder_changed();
	void de_jure_liege_title_changed();
	void realm_changed();
	void capital_province_changed();
	void capital_world_changed();
	void laws_changed();
	void government_type_changed();

private:
	QColor color;
	landed_title_tier tier;
	character *holder = nullptr;
	metternich::holding_slot *holding_slot = nullptr; //the title's holding slot, if it is a non-titular barony or cosmic barony
	metternich::province *province = nullptr; //the title's province, if it is a non-titular county
	metternich::world *world = nullptr; //the title's world, if it is a non-titular cosmic county
	metternich::star_system *star_system = nullptr; //the title's star system, if it is a non-titular cosmic duchy
	landed_title *de_jure_liege_title = nullptr;
	std::vector<landed_title *> de_jure_vassal_titles;
	metternich::province *capital_province = nullptr;
	metternich::world *capital_world = nullptr;
	landed_title *holder_title = nullptr; //title of this title's holder; used only for initialization, and set to null afterwards
	bool random_holder = false; //whether a random holder should be generated for the title upon initialization
	landed_title *liege_title = nullptr; //title of this title's holder's liege; used only for initialization, and set to null afterwards
	std::string flag_tag;
	std::map<law_group *, law *> laws; //the laws pertaining to the title, mapped to the respective law group
	bool active_post_amalgamation = false; //whether the title remains active after the amalgamation of its world
};

}
