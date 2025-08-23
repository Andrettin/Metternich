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
	
	property int mode: DiplomaticMap.Mode.Political
	readonly property var reference_country: selected_province ? selected_province.game_data.owner : (metternich.game.player_country ? metternich.game.player_country : null)
	
	MouseArea {
		width: province_map.contentWidth
		height: province_map.contentHeight
		
		onClicked: {
			select_province(null)
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
						select_province(null)
					} else {
						select_province(province)
					}
				}
				
				onContainsMouseChanged: {
					var text = province.game_data.current_cultural_name
					
					if (province.game_data.owner !== null) {
						text += ", " + province.game_data.owner.name
					}
					
					if (containsMouse) {
						status_text = text
					} else if (status_text === text) {
						status_text = ""
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
					select_province(other_country.game_data.capital.province)
				}
			}
			
			CustomTooltip {
				text: small_text(consulate.name)
				visible: consulate_mouse_area.containsMouse
			}
		}
	}
	
	function select_province(province) {
		selected_province = province
		if (selected_garrison && (province === null || selected_province.game_data.military_unit_category_counts.length === 0)) {
			selected_garrison = false
		}
	}
	
	function center_on_tile_pos(tile_x, tile_y) {
		var pixel_x = Math.round(tile_x * metternich.map.diplomatic_map_tile_pixel_size * scale_factor - province_map.width / 2)
		var pixel_y = Math.round(tile_y * metternich.map.diplomatic_map_tile_pixel_size * scale_factor - province_map.height / 2)
		
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
		
		center_on_tile_pos(capital_x, capital_y)
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
