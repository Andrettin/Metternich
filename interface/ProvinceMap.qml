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
	
	Repeater {
		model: metternich.map.provinces
		
		Image {
			id: province_image
			x: province.game_data.map_image_rect.x
			y: province.game_data.map_image_rect.y
			source: "image://province_map/" + province.identifier + (selected ? "/selected" : get_map_mode_suffix(province_map.mode, province)) + "/" + change_count
			cache: false
			
			readonly property var province: model.modelData
			readonly property var selected: selected_province === province
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
			x: Math.floor(text_rect.x * metternich.map.diplomatic_map_tile_pixel_size * scale_factor)
			y: Math.floor(text_rect.y * metternich.map.diplomatic_map_tile_pixel_size * scale_factor)
			width: Math.floor(text_rect_width)
			height: Math.floor(text_rect_height)
			//visible: contentWidth <= width// && (diplomatic_map.mode === DiplomaticMap.Mode.Political || diplomatic_map.mode === DiplomaticMap.Mode.Diplomatic)
			wrapMode: Text.WordWrap
			horizontalAlignment: contentWidth <= width ? Text.AlignHCenter : (province.game_data.map_image_rect.x === 0 ? Text.AlignLeft : ((province.game_data.map_image_rect.x + province.game_data.map_image_rect.width) >= metternich.map.diplomatic_map_image_size.width * scale_factor ? Text.AlignRight : Text.AlignHCenter))
			verticalAlignment: contentHeight <= height ? Text.AlignVCenter : (province.game_data.map_image_rect.y === 0 ? Text.AlignTop : ((province.game_data.map_image_rect.y + province.game_data.map_image_rect.height) >= metternich.map.diplomatic_map_image_size.height * scale_factor ? Text.AlignBottom : Text.AlignVCenter))
			
			readonly property var province: model.modelData
			readonly property var text_rect: province.game_data.text_rect
			readonly property int text_rect_width: text_rect.width * metternich.map.diplomatic_map_tile_pixel_size * scale_factor
			readonly property int text_rect_height: text_rect.height * metternich.map.diplomatic_map_tile_pixel_size * scale_factor
		}
	}
	
	MouseArea {
		width: province_map.contentWidth
		height: province_map.contentHeight
		hoverEnabled: true
		
		onClicked: function (mouse) {
			var province = metternich.map.get_tile_province(Qt.point(Math.floor(mouse.x / metternich.map.diplomatic_map_tile_pixel_size / scale_factor), Math.floor(mouse.y / metternich.map.diplomatic_map_tile_pixel_size / scale_factor)))
			
			if (province === null || selected_province === province || province.water_zone) {
				select_province(null)
			} else {
				if (metternich.selected_military_units.length > 0) {
					metternich.move_selected_military_units_to(province.provincial_capital.map_data.tile_pos)
					selected_civilian_unit = null
					selected_site = null
					selected_province = null
					metternich.clear_selected_military_units()
					return
				}
				
				select_province(province)
			}
		}
		
		onPositionChanged: function (mouse) {
			var province = metternich.map.get_tile_province(Qt.point(Math.floor(mouse.x / metternich.map.diplomatic_map_tile_pixel_size / scale_factor), Math.floor(mouse.y / metternich.map.diplomatic_map_tile_pixel_size / scale_factor)))
			
			if (province !== null) {
				var text = province.game_data.current_cultural_name
				
				if (province.game_data.owner !== null) {
					text += ", " + province.game_data.owner.name
				}
				
				status_text = text
			} else {
				status_text = ""
			}
			
		}
		
		onContainsMouseChanged: {
			if (!containsMouse) {
				status_text = ""
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
	}
	
	function center_on_tile(tile_x, tile_y) {
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
		
		center_on_tile(capital_x, capital_y)
	}
	
	function get_map_mode_suffix(mode, province) {
		switch (mode) {
			case DiplomaticMap.Mode.Political:
				return "/political"
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
