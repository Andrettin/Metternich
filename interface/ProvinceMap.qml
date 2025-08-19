import QtQuick
import QtQuick.Controls
import MaskedMouseArea 1.0

Flickable {
	id: province_map
	contentWidth: metternich.map.diplomatic_map_image_size.width * scale_factor
	contentHeight: metternich.map.diplomatic_map_image_size.height * scale_factor
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	
	enum Mode {
		Political,
		Treaty,
		Terrain,
		Cultural,
		Religious
	}
	
	property var selected_province: null
	property int mode: DiplomaticMap.Mode.Political
	readonly property var reference_country: selected_province ? selected_province.game_data.owner : (metternich.game.player_country ? metternich.game.player_country : null)
	
	MouseArea {
		width: province_map.contentWidth
		height: province_map.contentHeight
		
		onClicked: {
			province_map.selected_province = null
		}
	}
	
	Repeater {
		model: metternich.map.provinces
		
		Image {
			id: province_image
			x: province.game_data.map_image_rect.x
			y: province.game_data.map_image_rect.y
			source: "image://province_map/" + province.identifier + (selected ? "/selected" : get_map_mode_suffix(province_map.mode, province))
			cache: false
			
			readonly property var province: model.modelData
			readonly property var selected: selected_province === province
			
			MaskedMouseArea {
				id: province_mouse_area
				anchors.fill: parent
				alphaThreshold: 0.4
				maskSource: parent.source
				
				onClicked: {
					if (selected || province.water_zone) {
						province_map.selected_province = null
					} else {
						province_map.selected_province = province
					}
				}
			}
			
			CustomTooltip {
				text: province.game_data.current_cultural_name + tooltip_suffix
				visible: province_mouse_area.containsMouse
				
				property string tooltip_suffix: "" /*province_mouse_area.containsMouse ? (province_map.mode === DiplomaticMap.Mode.Terrain ?
					format_text("\n" + small_text(counts_to_percent_strings(country.game_data.tile_terrain_counts)))
				: (province_map.mode === DiplomaticMap.Mode.Cultural ?
					format_text("\n" + small_text(counts_to_percent_strings(country.game_data.population.culture_counts)))
				: (province_map.mode === DiplomaticMap.Mode.Religious ?
					format_text("\n" + small_text(counts_to_percent_strings(country.game_data.population.religion_counts)))
				: ""))) : ""*/
				
				
				function counts_to_percent_strings(counts) {
					var str = ""
					
					var total_count = 0
					
					for (const kv_pair of counts) {
						var count = kv_pair.value
						total_count += count
					}
					
					var first = true
					
					for (const kv_pair of counts) {
						var key = kv_pair.key
						var count = kv_pair.value
						
						if (first) {
							first = false
						} else {
							str += "\n"
						}
						
						var color_hex_str = color_hex_string(key.color)
						
						str += "<font color=\"#" + color_hex_str + "\">⬤</font> " + (count * 100 / total_count).toFixed(2) + "% " + key.name
					}
					
					return str
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
				province_map.selected_province = null
			}
		}
	}
	
	Repeater {
		model: reference_country ? reference_country.game_data.consulates : []
		
		Image {
			id: consulate_icon
			x: other_country_capital ? (other_country_capital.game_data.tile_pos.x * metternich.map.diplomatic_map_tile_pixel_size * scale_factor - width / 2) : 0
			y: other_country_capital ? (other_country_capital.game_data.tile_pos.y * metternich.map.diplomatic_map_tile_pixel_size * scale_factor - height / 2) : 0
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
					province_map.selected_province = other_country.game_data.capital.province
				}
			}
			
			CustomTooltip {
				text: small_text(consulate.name)
				visible: consulate_mouse_area.containsMouse
			}
		}
	}
	
	function center_on_tile_pos(tile_x, tile_y) {
		var pixel_x = Math.round(tile_x * metternich.map.diplomatic_map_tile_pixel_size * scale_factor - province_map.width / 2)
		var pixel_y = Math.round(tile_y * metternich.map.diplomatic_map_tile_pixel_size * scale_factor - province_map.height / 2)
		
		province_map.contentX = Math.min(Math.max(pixel_x, 0), province_map.contentWidth - province_map.width)
		province_map.contentY = Math.min(Math.max(pixel_y, 0), province_map.contentHeight - province_map.height)
	}
	
	function get_map_mode_suffix(mode, province) {
		switch (mode) {
			case DiplomaticMap.Mode.Treaty:
				if (reference_country !== null && province.game_data.owner !== null) {
					return "/diplomatic/" + reference_country.game_data.get_diplomacy_state_diplomatic_map_suffix(province.game_data.owner)
				}
				break
			case DiplomaticMap.Mode.Terrain:
				return "/terrain"
			case DiplomaticMap.Mode.Cultural:
				return "/cultural"
			case DiplomaticMap.Mode.Religious:
				return "/religious"
		}
		
		return ""
	}
}
