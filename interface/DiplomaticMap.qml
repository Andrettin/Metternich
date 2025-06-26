import QtQuick
import QtQuick.Controls
import MaskedMouseArea 1.0

Flickable {
	id: diplomatic_map
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
	
	property string ocean_suffix: ""
	property string country_suffix: "0"
	property var selected_country: null
	property int mode: DiplomaticMap.Mode.Political
	readonly property var reference_country: selected_country ? selected_country : (metternich.game.player_country ? metternich.game.player_country : null)
	
	Image {
		id: ocean_image
		source: ocean_suffix.length > 0 ? ("image://diplomatic_map/ocean/" + ocean_suffix) : "image://empty/"
		cache: false
	}
	
	MouseArea {
		width: diplomatic_map.contentWidth
		height: diplomatic_map.contentHeight
		
		onClicked: {
			diplomatic_map.selected_country = null
		}
	}
	
	Repeater {
		model: metternich.game.countries
		
		Image {
			id: country_image
			x: country.game_data.diplomatic_map_image_rect.x
			y: country.game_data.diplomatic_map_image_rect.y
			source: "image://diplomatic_map/" + country.identifier + (selected ? "/selected" : get_map_mode_suffix(diplomatic_map.mode, country)) + "/" + country_suffix
			cache: false
			
			readonly property var country: model.modelData
			readonly property var selected: selected_country === country
			
			MaskedMouseArea {
				id: country_mouse_area
				anchors.fill: parent
				alphaThreshold: 0.4
				maskSource: parent.source
				
				onClicked: {
					if (selected) {
						diplomatic_map.selected_country = null
					} else {
						diplomatic_map.selected_country = country
					}
				}
			}
			
			CustomTooltip {
				text: country.game_data.titled_name + tooltip_suffix
				visible: country_mouse_area.containsMouse
				
				property string tooltip_suffix: country_mouse_area.containsMouse ? (diplomatic_map.mode === DiplomaticMap.Mode.Terrain ?
					format_text("\n" + small_text(counts_to_percent_strings(country.game_data.tile_terrain_counts)))
				: (diplomatic_map.mode === DiplomaticMap.Mode.Cultural ?
					format_text("\n" + small_text(counts_to_percent_strings(country.game_data.population.culture_counts)))
				: (diplomatic_map.mode === DiplomaticMap.Mode.Religious ?
					format_text("\n" + small_text(counts_to_percent_strings(country.game_data.population.religion_counts)))
				: ""))) : ""
				
				
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
	
	/*
	Repeater {
		model: metternich.game.countries
		
		TinyText {
			id: country_label
			text: country.game_data.titled_name
			x: Math.floor(text_rect.x * metternich.map.diplomatic_map_tile_pixel_size * scale_factor)
			y: Math.floor(text_rect.y * metternich.map.diplomatic_map_tile_pixel_size * scale_factor)
			width: Math.floor(text_rect_width)
			height: Math.floor(text_rect_height)
			visible: contentWidth <= width && (diplomatic_map.mode === DiplomaticMap.Mode.Political || diplomatic_map.mode === DiplomaticMap.Mode.Diplomatic)
			font.pixelSize: 8 * scale_factor
			shadow_offset: 1
			font.bold: true
			wrapMode: Text.WordWrap
			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter
					
			readonly property var country: model.modelData
			readonly property var text_rect: country.game_data.text_rect
			readonly property int text_rect_width: text_rect.width * metternich.map.diplomatic_map_tile_pixel_size * scale_factor
			readonly property int text_rect_height: text_rect.height * metternich.map.diplomatic_map_tile_pixel_size * scale_factor
		}
	}
	*/
	
	Image {
		id: exploration_image
		source: metternich.game.running ? "image://diplomatic_map/exploration" : "image://empty/"
		cache: false
		
		MaskedMouseArea {
			id: exploration_mouse_area
			anchors.fill: parent
			alphaThreshold: 0.4
			maskSource: parent.source
			
			onClicked: {
				diplomatic_map.selected_country = null
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
			visible: !reference_country.game_data.anarchy && !other_country.game_data.anarchy && diplomatic_map.mode === DiplomaticMap.Mode.Treaty
			
			readonly property var other_country: model.modelData.key
			readonly property var other_country_capital: other_country.game_data.capital
			readonly property var consulate: model.modelData.value
			
			MaskedMouseArea {
				id: consulate_mouse_area
				anchors.fill: parent
				alphaThreshold: 0.4
				maskSource: parent.source
				
				onClicked: {
					diplomatic_map.selected_country = other_country
				}
			}
			
			CustomTooltip {
				text: small_text(consulate.name)
				visible: consulate_mouse_area.containsMouse
			}
		}
	}
	
	function center_on_tile_pos(tile_x, tile_y) {
		var pixel_x = Math.round(tile_x * metternich.map.diplomatic_map_tile_pixel_size * scale_factor - diplomatic_map.width / 2)
		var pixel_y = Math.round(tile_y * metternich.map.diplomatic_map_tile_pixel_size * scale_factor - diplomatic_map.height / 2)
		
		diplomatic_map.contentX = Math.min(Math.max(pixel_x, 0), diplomatic_map.contentWidth - diplomatic_map.width)
		diplomatic_map.contentY = Math.min(Math.max(pixel_y, 0), diplomatic_map.contentHeight - diplomatic_map.height)
	}
	
	function center_on_selected_country_capital() {
		var capital = selected_country.game_data.capital
		
		if (capital === null) {
			return
		}
		
		var capital_game_data = capital.game_data
		var capital_x = capital_game_data.tile_pos.x
		var capital_y = capital_game_data.tile_pos.y
		center_on_tile_pos(capital_x, capital_y)
	}
	
	function get_map_mode_suffix(mode, country) {
		switch (mode) {
			case DiplomaticMap.Mode.Treaty:
				if (reference_country !== null) {
					return "/diplomatic/" + reference_country.game_data.get_diplomacy_state_diplomatic_map_suffix(country)
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
