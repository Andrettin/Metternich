#pragma once

#include "economy/resource_container.h"
#include "map/site_container.h"
#include "map/terrain_type_container.h"
#include "util/decimillesimal_int.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("map/province.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("unit/civilian_unit_type.h")

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class character;
class civilian_unit_type;
class domain;
class icon;
class improvement;
class pathway;
class phenotype;
class province;

class civilian_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring CONSTANT)
	Q_PROPERTY(const metternich::civilian_unit_type* type READ get_type NOTIFY type_changed)
	Q_PROPERTY(const metternich::icon* icon READ get_icon NOTIFY icon_changed)
	Q_PROPERTY(const metternich::domain* owner READ get_owner CONSTANT)
	Q_PROPERTY(const metternich::province* province READ get_province NOTIFY province_changed)
	Q_PROPERTY(bool moving READ is_moving NOTIFY original_province_changed)
	Q_PROPERTY(bool working READ is_working NOTIFY work_progress_changed)
	Q_PROPERTY(QString work_progress READ get_work_progress_qstring NOTIFY work_progress_changed)
	Q_PROPERTY(QVariantList buildable_buildings READ get_buildable_buildings_qvariant_list NOTIFY buildable_buildings_changed)
	Q_PROPERTY(const metternich::pathway* buildable_pathway READ get_buildable_pathway NOTIFY buildable_pathway_changed)
	Q_PROPERTY(QVariantList improvable_resource_tiles READ get_improvable_resource_tiles_qvariant_list NOTIFY improvable_resources_changed)
	Q_PROPERTY(QVariantList prospectable_provinces READ get_prospectable_provinces_qvariant_list NOTIFY prospectable_provinces_changed)

public:
	static constexpr int improvement_construction_turns = 2;
	static constexpr int exploration_turns = 1;
	static constexpr int prospection_turns = 1;

	explicit civilian_unit(const civilian_unit_type *type, const domain *owner, const metternich::phenotype *phenotype);
	explicit civilian_unit(const civilian_unit_type *type, const domain *owner, const metternich::character *character);
	explicit civilian_unit(const gsml_data &scope);

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	[[nodiscard]] QCoro::Task<void> do_turn();
	void do_ai_turn();

	const std::string &get_name() const
	{
		return this->name;
	}

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->get_name());
	}

	void generate_name();

	const civilian_unit_type *get_type() const
	{
		return this->type;
	}

	void set_type(const civilian_unit_type *type)
	{
		if (type == this->get_type()) {
			return;
		}

		this->type = type;
		emit type_changed();
	}

	const icon *get_icon() const;

	const domain *get_owner() const
	{
		return this->owner;
	}

	const metternich::culture *get_culture() const;
	const metternich::cultural_group *get_cultural_group() const;

	const metternich::phenotype *get_phenotype() const
	{
		return this->phenotype;
	}

	const metternich::character *get_character() const
	{
		return this->character;
	}

	const province *get_province() const;
	void set_province(const province *province);

	void set_original_province(const metternich::province *province)
	{
		if (province == this->original_province) {
			return;
		}

		this->original_province = province;
		emit original_province_changed();
	}

	Q_INVOKABLE bool can_move_to(const metternich::province *province) const;
	Q_INVOKABLE void move_to(const metternich::province *province);
	Q_INVOKABLE void cancel_move();

	bool is_moving() const
	{
		return this->original_province != nullptr;
	}

	bool is_working() const
	{
		return this->work_progress.has_value();
	}

	bool is_busy() const
	{
		return this->is_moving() || this->is_working();
	}

	site_map<std::vector<const building_type *>> get_buildable_buildings() const;
	QVariantList get_buildable_buildings_qvariant_list() const;
	Q_INVOKABLE void build_building(const metternich::building_type *building_type, const metternich::site *site);

	const metternich::pathway *get_buildable_pathway() const;
	Q_INVOKABLE void build_pathway(const metternich::pathway *pathway);

	Q_INVOKABLE bool can_build_on_tile() const;
	Q_INVOKABLE void build_on_tile();

	bool can_build_improvement(const improvement *improvement) const;
	bool can_build_improvement_on_tile(const improvement *improvement, const QPoint &tile_pos) const;
	void build_improvement(const improvement *improvement);
	Q_INVOKABLE void cancel_work();

	const improvement *get_buildable_resource_improvement_for_tile(const QPoint &tile_pos) const;

	resource_map<std::vector<QPoint>> get_improvable_resource_tiles() const;
	QVariantList get_improvable_resource_tiles_qvariant_list() const;

	bool can_explore_province(const province *province) const;

	bool can_prospect_province(const province *province) const;
	terrain_type_map<std::vector<const province *>> get_prospectable_provinces() const;
	QVariantList get_prospectable_provinces_qvariant_list() const;

	QString get_work_progress_qstring() const;

	void set_work_progress(const std::optional<decimillesimal_int> &progress)
	{
		if (progress == this->work_progress) {
			return;
		}

		if (progress > 100) {
			this->set_work_progress(decimillesimal_int(100));
			return;
		}

		this->work_progress = progress;
		emit work_progress_changed();
	}

	void increment_work_progress();

	[[nodiscard]] QCoro::Task<void> disband(const bool dead);

	Q_INVOKABLE QCoro::QmlTask disband()
	{
		return this->disband(false);
	}

signals:
	void type_changed();
	void icon_changed();
	void province_changed();
	void original_province_changed();
	void buildable_buildings_changed();
	void buildable_pathway_changed();
	void improvable_resources_changed();
	void prospectable_provinces_changed();
	void work_progress_changed();

private:
	std::string name;
	const civilian_unit_type *type = nullptr;
	const domain *owner = nullptr;
	const metternich::phenotype *phenotype = nullptr;
	const metternich::character *character = nullptr;
	const metternich::province *province = nullptr;
	const metternich::province *original_province = nullptr; //the province before moving
	const building_type *under_construction_building = nullptr;
	const site *under_construction_building_site = nullptr;
	const pathway *under_construction_pathway = nullptr;
	const improvement *improvement_under_construction = nullptr;
	bool exploring = false;
	bool prospecting = false;
	std::optional<decimillesimal_int> work_progress;
};

}
