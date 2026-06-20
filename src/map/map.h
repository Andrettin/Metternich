#pragma once

#include "util/decimillesimal_int.h"
#include "util/singleton.h"

namespace archimedes {
	class gsml_data;
	class gsml_property;
	enum class direction;
}

namespace metternich {

class province;
class resource;
class site;
class terrain_type;
class tile;

class map final : public QObject, public singleton<map>
{
	Q_OBJECT

	Q_PROPERTY(QSize size READ get_size NOTIFY size_changed)
	Q_PROPERTY(int width READ get_width NOTIFY size_changed)
	Q_PROPERTY(int height READ get_height NOTIFY size_changed)
	Q_PROPERTY(QVariantList provinces READ get_provinces_qvariant_list NOTIFY provinces_changed)
	Q_PROPERTY(QVariantList sites READ get_sites_qvariant_list NOTIFY sites_changed)
	Q_PROPERTY(QSize diplomatic_map_image_size READ get_diplomatic_map_image_size NOTIFY diplomatic_map_image_size_changed)
	Q_PROPERTY(double diplomatic_map_tile_scale_double READ get_diplomatic_map_tile_scale_double NOTIFY diplomatic_map_tile_scale_changed)
	Q_PROPERTY(QSize province_map_image_size READ get_province_map_image_size NOTIFY province_map_image_size_changed)
	Q_PROPERTY(double minimap_tile_scale_double READ get_minimap_tile_scale_double NOTIFY minimap_tile_scale_changed)

public:
	static constexpr bool exploration_enabled = false;

	map();
	~map();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	void create_tiles();
	[[nodiscard]] QCoro::Task<void> initialize(const bool province_post_processing_enabled);
	void do_province_post_processing(std::vector<QPoint> tiles_to_check, const bool only_water_zones);
	[[nodiscard]] QCoro::Task<void> process_border_tiles();
	Q_INVOKABLE void clear();

	const QSize &get_size() const
	{
		return this->size;
	}

	void set_size(const QSize &size)
	{
		if (size == this->get_size()) {
			return;
		}

		this->size = size;
		emit size_changed();
	}

	int get_width() const
	{
		return this->get_size().width();
	}

	int get_height() const
	{
		return this->get_size().height();
	}

	bool contains(const QPoint &pos) const
	{
		return (pos.x() >= 0
			&& pos.y() >= 0
			&& pos.x() < this->get_width()
			&& pos.y() < this->get_height());
	}

	int get_pos_index(const QPoint &pos) const;

	tile *get_tile(const QPoint &pos) const;
	Q_INVOKABLE const metternich::province *get_tile_province(const QPoint &tile_pos) const;
	void set_tile_province(const QPoint &tile_pos, province *province);
	void set_tile_site(const QPoint &tile_pos, const site *site);
	[[nodiscard]] QCoro::Task<void> set_tile_resource_discovered(const QPoint &tile_pos, const bool discovered);

	bool is_tile_near_celestial_body(const QPoint &tile_pos) const;

	bool is_tile_on_province_border(const QPoint &tile_pos) const;
	bool is_tile_on_province_border_with(const QPoint &tile_pos, const province *other_province) const;

	const std::vector<province *> &get_provinces() const
	{
		return this->provinces;
	}

	QVariantList get_provinces_qvariant_list() const;

	void add_province(province *province)
	{
		this->provinces.push_back(province);
	}

	const std::vector<const site *> &get_sites() const
	{
		return this->sites;
	}

	QVariantList get_sites_qvariant_list() const;

	void add_site(const site *site)
	{
		this->sites.push_back(site);
	}

	void initialize_diplomatic_map();
	void initialize_province_map();

	const QImage &get_ocean_diplomatic_map_image() const
	{
		return this->ocean_diplomatic_map_image;
	}

	[[nodiscard]]
	QCoro::Task<void> create_ocean_diplomatic_map_image();

	const QSize &get_diplomatic_map_image_size() const
	{
		return this->diplomatic_map_image_size;
	}

	const decimillesimal_int &get_diplomatic_map_tile_scale() const
	{
		return this->diplomatic_map_tile_scale;
	}

	double get_diplomatic_map_tile_scale_double() const
	{
		return this->get_diplomatic_map_tile_scale().to_double();
	}

	void set_diplomatic_map_tile_scale(const decimillesimal_int &tile_scale)
	{
		this->diplomatic_map_tile_scale = tile_scale;

		emit diplomatic_map_tile_scale_changed();
	}

	const QSize &get_province_map_image_size() const
	{
		return this->province_map_image_size;
	}

	const QImage &get_minimap_image() const
	{
		return this->minimap_image;
	}

	void create_minimap_image();
	void update_minimap_rect(const QRect &tile_rect);

	const decimillesimal_int &get_minimap_tile_scale() const
	{
		return this->minimap_tile_scale;
	}

	double get_minimap_tile_scale_double() const
	{
		return this->get_minimap_tile_scale().to_double();
	}

	void set_minimap_tile_scale(const decimillesimal_int &tile_scale)
	{
		this->minimap_tile_scale = tile_scale;

		emit minimap_tile_scale_changed();
	}

signals:
	void size_changed();
	void tile_prospection_changed(const QPoint &tile_pos);
	void tile_resource_changed(const QPoint &tile_pos);
	void tile_holding_type_changed(const QPoint &tile_pos);
	void provinces_changed();
	void sites_changed();
	void diplomatic_map_image_size_changed();
	void diplomatic_map_tile_scale_changed();
	void province_map_image_size_changed();
	void minimap_tile_scale_changed();

private:
	QSize size;
	std::unique_ptr<std::vector<tile>> tiles;
	std::vector<province *> provinces; //the provinces which are on the map
	std::vector<const site *> sites; //the sites which are on the map
	QImage ocean_diplomatic_map_image;
	QSize diplomatic_map_image_size;
	decimillesimal_int diplomatic_map_tile_scale = decimillesimal_int(1);
	QSize province_map_image_size;
	QImage minimap_image;
	decimillesimal_int minimap_tile_scale = decimillesimal_int(1);
};

}
