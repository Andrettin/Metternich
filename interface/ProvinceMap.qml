import QtQuick
import QtQuick.Controls
import MaskedMouseArea 1.0

Flickable {
	id: province_map
	contentWidth: metternich.map.province_map_image_size.width * scale_factor
	contentHeight: metternich.map.province_map_image_size.height * scale_factor
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	
	enum Mode {
		Political,
		Terrain,
		Cultural,
		TradeZone,
		Temple
	}
	
	property int mode: ProvinceMap.Mode.Political
	property bool show_sites: false
	readonly property var reference_country: selected_province ? selected_province.game_data.owner : (metternich.game.player_country ? metternich.game.player_country : null)
	property var hovered_site: null
	
	Repeater {
		model: metternich.map.provinces
		
		Image {
			id: province_image
			x: province.game_data.map_image_rect.x
			y: province.game_data.map_image_rect.y
			source: "image://province_map/" + province.identifier + (selected ? "/selected" : get_map_mode_suffix(province_map.mode, province)) + "/" + change_count
			cache: false
			
			readonly property var province: model.modelData
			readonly property var selected: selected_province === province || (selected_site !== null && selected_site.game_data.province === province && !province_map.show_sites)
			property int change_count: 0
			
			Connections {
				target: province.game_data
				
				function onMap_image_changed() {
					change_count += 1
				}
			}
		}
	}
	
	Repeater {
		model: metternich.map.provinces
		
		TinyText {
			id: province_label
			text: province.game_data.current_cultural_name
			x: Math.floor(text_rect.x * metternich.map.province_map_tile_pixel_size * scale_factor)
			y: Math.floor(text_rect.y * metternich.map.province_map_tile_pixel_size * scale_factor)
			width: Math.floor(text_rect_width)
			height: Math.floor(text_rect_height)
			visible: !province_map.show_sites
			wrapMode: Text.WordWrap
			horizontalAlignment: contentWidth <= width ? Text.AlignHCenter : (province.game_data.map_image_rect.x === 0 ? Text.AlignLeft : ((province.game_data.map_image_rect.x + province.game_data.map_image_rect.width) >= metternich.map.province_map_image_size.width * scale_factor ? Text.AlignRight : Text.AlignHCenter))
			verticalAlignment: contentHeight <= height ? Text.AlignVCenter : (province.game_data.map_image_rect.y === 0 ? Text.AlignTop : ((province.game_data.map_image_rect.y + province.game_data.map_image_rect.height) >= metternich.map.province_map_image_size.height * scale_factor ? Text.AlignBottom : Text.AlignVCenter))
			
			readonly property var province: model.modelData
			readonly property var text_rect: province.game_data.text_rect
			readonly property int text_rect_width: text_rect.width * metternich.map.province_map_tile_pixel_size * scale_factor
			readonly property int text_rect_height: text_rect.height * metternich.map.province_map_tile_pixel_size * scale_factor
		}
	}
	
	MouseArea {
		width: province_map.contentWidth
		height: province_map.contentHeight
		hoverEnabled: true
		
		onClicked: function (mouse) {
			var province = metternich.map.get_tile_province(Qt.point(Math.floor(mouse.x / metternich.map.province_map_tile_pixel_size / scale_factor), Math.floor(mouse.y / metternich.map.province_map_tile_pixel_size / scale_factor)))
			
			if (province === null || selected_province === province || province.water_zone) {
				select_province(null)
			} else {
				if (metternich.selected_military_units.length > 0) {
					metternich.move_selected_military_units_to(province.game_data.provincial_capital.map_data.tile_pos)
					select_province(null)
					metternich.clear_selected_military_units()
					return
				}
				
				select_province(province)
			}
		}
		
		onPositionChanged: function (mouse) {
			var province = metternich.map.get_tile_province(Qt.point(Math.floor(mouse.x / metternich.map.province_map_tile_pixel_size / scale_factor), Math.floor(mouse.y / metternich.map.province_map_tile_pixel_size / scale_factor)))
			
			if (province !== null) {
				var text = province.game_data.current_cultural_name
				
				if (province.game_data.owner !== null) {
					text += ", " + province.game_data.owner.name
				}
				
				if (province.game_data.owner.game_data.realm !== province.game_data.owner) {
					text += ", " + province.game_data.owner.game_data.realm.name
				}
				
				if (province_map.mode === ProvinceMap.Mode.Cultural && province.game_data.culture !== null) {
					text += " (" + province.game_data.culture.name + ")"
				} else if (province_map.mode === ProvinceMap.Mode.TradeZone) {
					var trade_zone_domain = province.game_data.get_trade_zone_domain()
					if (trade_zone_domain !== null) {
						text += " (" + trade_zone_domain.name + ")"
					}
				} else if (province_map.mode === ProvinceMap.Mode.Temple) {
					var temple_domain = province.game_data.get_temple_domain()
					if (temple_domain !== null) {
						text += " (" + temple_domain.name + ")"
					}
				}
				
				status_text = text
			} else {
				status_text = ""
			}
			
		}
		
		onContainsMouseChanged: {
			if (!containsMouse && hovered_site === null) {
				status_text = ""
			}
		}
	}
	
	Repeater {
		model: metternich.map.sites
		
		Item {
			id: site_icon_area
			x: site.game_data.tile_pos.x * metternich.map.province_map_tile_pixel_size * scale_factor - Math.floor(width / 2)
			y: site.game_data.tile_pos.y * metternich.map.province_map_tile_pixel_size * scale_factor - Math.floor(height / 2)
			width: site_icon.width + 4 * scale_factor
			height: site_icon.height + 4 * scale_factor
			visible: province_map.show_sites && (site.settlement || dungeon !== null)
			
			readonly property var site: model.modelData
			readonly property var tile_pos: site.map_data.tile_pos
			readonly property var holding_type: site.game_data.holding_type
			readonly property var dungeon: site.game_data.dungeon
			readonly property bool is_visit_target: metternich.game.player_country.game_data.visit_target_site === site
			
			Rectangle {
				id: site_domain_color_circle
				width: site_icon_area.width
				height: site_icon_area.height
				radius: width / 2
				color: site.game_data.owner ? site.game_data.owner.color : "transparent"
				visible: site.game_data.owner !== null && site.game_data.owner !== site.game_data.province.game_data.owner
			}
			
			Image {
				id: site_icon
				anchors.verticalCenter: parent.verticalCenter
				anchors.horizontalCenter: parent.horizontalCenter
				source: "image://icon/" + (holding_type ? holding_type.icon.identifier : (dungeon ? dungeon.icon.identifier : "garrison")) + (site === selected_site ? "/selected" : "")
			}
			
			MouseArea {
				anchors.fill: parent
				hoverEnabled: true
				
				onClicked: {
					selected_civilian_unit = null
					selected_province = null
					if (selected_site === site) {
						selected_site = null
					} else {
						selected_site = site
					}
				}
				
				onContainsMouseChanged: {
					var text = site.game_data.display_text
					
					if (containsMouse) {
						hovered_site = site
						status_text = text
					} else {
						if (status_text === text) {
							status_text = ""
						}
						
						if (hovered_site === site) {
							hovered_site = null
						}
					}
				}
			}
		}
	}
	
	Image {
		id: exploration_image
		source: metternich.game.running ? "image://province_map/exploration" : "image://empty/"
		cache: false
		
		MaskedMouseArea {
			id: exploration_mouse_area
			anchors.fill: parent
			alphaThreshold: 0.4
			maskSource: parent.source
			
			onClicked: {
				select_province(null)
			}
		}
	}
	
	/*
	Repeater {
		model: reference_country ? reference_country.game_data.consulates : []
		
		Image {
			id: consulate_icon
			x: other_country_capital ? (other_country_capital.game_data.tile_pos.x * metternich.map.province_map_tile_pixel_size * scale_factor - width / 2) : 0
			y: other_country_capital ? (other_country_capital.game_data.tile_pos.y * metternich.map.province_map_tile_pixel_size * scale_factor - height / 2) : 0
			source: "image://icon/" + consulate.icon.identifier
			visible: !reference_country.game_data.anarchy && !other_country.game_data.anarchy && province_map.mode === DiplomaticMap.Mode.Treaty
			
			readonly property var other_country: model.modelData.key
			readonly property var other_country_capital: other_country.game_data.capital
			readonly property var consulate: model.modelData.value
			
			MaskedMouseArea {
				id: consulate_mouse_area
				anchors.fill: parent
				alphaThreshold: 0.4
				maskSource: parent.source
				
				onClicked: {
					select_province(other_country.game_data.capital.game_data.province)
				}
			}
			
			CustomTooltip {
				text: small_text(consulate.name)
				visible: consulate_mouse_area.containsMouse
			}
		}
	}
	*/
	
	function select_province(province) {
		selected_civilian_unit = null
		selected_site = null
		selected_province = province
	}
	
	function center_on_tile(tile_x, tile_y) {
		var pixel_x = Math.round(tile_x * metternich.map.province_map_tile_pixel_size * scale_factor - province_map.width / 2)
		var pixel_y = Math.round(tile_y * metternich.map.province_map_tile_pixel_size * scale_factor - province_map.height / 2)
		
		province_map.contentX = Math.min(Math.max(pixel_x, 0), province_map.contentWidth - province_map.width)
		province_map.contentY = Math.min(Math.max(pixel_y, 0), province_map.contentHeight - province_map.height)
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
			case ProvinceMap.Mode.TradeZone:
				return "/trade_zone"
			case ProvinceMap.Mode.Temple:
				return "/temple"
		}
		
		return ""
	}
}
