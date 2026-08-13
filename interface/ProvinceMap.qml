import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import map_grid_model 1.0

TableView {
	id: province_map
	leftMargin: 0
	rightMargin: 0
	topMargin: 0
	bottomMargin: 0
	contentWidth: metternich.map.province_map_image_size.width * scale_factor
	contentHeight: metternich.map.province_map_image_size.height * scale_factor
	boundsBehavior: Flickable.StopAtBounds
	reuseItems: true
	clip: true
	model: MapGridModel {}
	delegate: MapBlockView {}
	
	enum Mode {
		Political,
		Terrain,
		Cultural,
		Religious,
		Technology,
		TradeZone,
		Temple,
		CulturalSociety
	}
	
	enum SiteMode {
		Show,
		ShowLocations,
		Hide
	}
	
	property int mode: ProvinceMap.Mode.Political
	property int show_site_mode: ProvinceMap.SiteMode.Show
	readonly property var reference_country: selected_province ? selected_province.game_data.owner : (metternich.game.player_domain ? metternich.game.player_domain : null)
	property var hovered_site: null
	property int hovered_icon_map_block_index: -1
	
	function select_province(province) {
		selected_civilian_unit = null
		selected_site = null
		selected_province = province
		selected_garrison = false
	}
	
	function get_province_status_text(province) {
		var text = province.game_data.current_cultural_name
		
		if (province.game_data.owner !== null) {
			text += ", " + province.game_data.owner.game_data.name
			
			if (province.game_data.owner.game_data.realm !== province.game_data.owner) {
				text += ", " + province.game_data.owner.game_data.realm.game_data.name
			}
		}
		
		if (province_map.mode === ProvinceMap.Mode.Cultural && province.game_data.culture !== null) {
			text += " (" + province.game_data.culture.name + ")"
		} else if (province_map.mode === ProvinceMap.Mode.Religious && province.game_data.religion !== null) {
			text += " (" + province.game_data.religion.name + ")"
		} else if (province_map.mode === ProvinceMap.Mode.Terrain && province.map_data.terrain !== null) {
			text += " (" + province.map_data.terrain.name + ")"
		} else if (province_map.mode === ProvinceMap.Mode.Technology) {
			if (!province.water_zone) {
				text += " (" + province.game_data.technologies.length + " " + (province.game_data.technologies.length === 1 ? "Technology" : "Technologies") + ")"
			}
		} else if (province_map.mode === ProvinceMap.Mode.TradeZone) {
			var trade_zone_domain = province.game_data.trade_zone_domain
			if (trade_zone_domain !== null) {
				text += " (" + trade_zone_domain.game_data.name + ")"
			}
		} else if (province_map.mode === ProvinceMap.Mode.Temple) {
			var temple_domain = province.game_data.temple_domain
			if (temple_domain !== null) {
				text += " (" + temple_domain.game_data.name + ")"
			}
		} else if (province_map.mode === ProvinceMap.Mode.CulturalSociety) {
			var cultural_society_domain = province.game_data.cultural_society_domain
			if (cultural_society_domain !== null) {
				text += " (" + cultural_society_domain.game_data.name + ")"
			}
		}
		
		return text
	}
	
	function center_on_tile(tile_x, tile_y) {
		var pixel_x = Math.round(tile_x * metternich.defines.province_map_tile_scale * scale_factor - province_map.width / 2)
		var pixel_y = Math.round(tile_y * metternich.defines.province_map_tile_scale * scale_factor - province_map.height / 2)
		
		province_map.contentX = Math.min(Math.max(pixel_x, 0), province_map.contentWidth - province_map.width)
		province_map.contentY = Math.min(Math.max(pixel_y, 0), province_map.contentHeight - province_map.height)
	}
	
	function center_on_province(province) {
		center_on_tile(province.game_data.center_tile_pos.x, province.game_data.center_tile_pos.y)
	}
	
	function center_on_country_capital(country) {
		var capital = country.game_data.capital
		
		if (capital === null) {
			return
		}
		
		var capital_game_data = capital.game_data
		var capital_x = capital_game_data.tile_pos.x
		var capital_y = capital_game_data.tile_pos.y
		
		center_on_tile(capital_x, capital_y)
	}
	
	function get_map_mode_suffix(mode, province) {
		switch (mode) {
			case ProvinceMap.Mode.Political:
				return "/political"
			case ProvinceMap.Mode.Terrain:
				return "/terrain"
			case ProvinceMap.Mode.Cultural:
				return "/cultural"
			case ProvinceMap.Mode.Religious:
				return "/religious"
			case ProvinceMap.Mode.Technology:
				return "/technology"
			case ProvinceMap.Mode.TradeZone:
				return "/trade_zone"
			case ProvinceMap.Mode.Temple:
				return "/temple"
			case ProvinceMap.Mode.CulturalSociety:
				return "/cultural_society"
		}
		
		return ""
	}
	
	function get_map_mode_color(mode, province, change_count) {
		switch (mode) {
			case ProvinceMap.Mode.Political:
				return province.game_data.map_color
			case ProvinceMap.Mode.Terrain:
				return province.map_data.terrain.color
			case ProvinceMap.Mode.Cultural:
				if (province.game_data.culture !== null) {
					return province.game_data.culture.color
				}
				break
			case ProvinceMap.Mode.Religious:
				if (province.game_data.religion !== null) {
					return province.game_data.religion.color
				}
				break
			case ProvinceMap.Mode.Technology:
				return province.game_data.technology_map_color
			case ProvinceMap.Mode.TradeZone:
				if (province.game_data.trade_zone_domain !== null) {
					return province.game_data.trade_zone_domain.color
				}
				break
			case ProvinceMap.Mode.Temple:
				if (province.game_data.temple_domain !== null) {
					return province.game_data.temple_domain.color
				}
				break
			case ProvinceMap.Mode.CulturalSociety:
				if (province.game_data.cultural_society_domain !== null) {
					return province.game_data.cultural_society_domain.color
				}
				break
		}
		
		if (province.water_zone) {
			return metternich.defines.ocean_color
		} else {
			return metternich.defines.map_blank_color
		}
	}
	
	function update_civilian_unit_status_text(civilian_unit, show_text) {
		var text = ""
		
		if (civilian_unit !== null) {
			text = civilian_unit.type.name
			if (civilian_unit.working) {
				text += " (Working)"
			} else if (civilian_unit.moving) {
				text += " (Moving)"
			}
		}
		
		if (show_text) {
			status_text = text
		} else {
			if (status_text === text) {
				status_text = ""
			}
		}
	}
}
