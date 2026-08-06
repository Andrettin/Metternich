import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Item {
	id: map_block
	implicitWidth: map_block_width * metternich.defines.province_map_tile_scale * scale_factor
	implicitHeight: map_block_height * metternich.defines.province_map_tile_scale * scale_factor
	clip: true
	
	Item {
		id: unscaled_province_map
		width: map_block_width * metternich.defines.province_map_tile_scale
		height: map_block_height * metternich.defines.province_map_tile_scale
		clip: true
		layer.enabled: true
		
		Repeater {
			model: provinces
			
			Shape {
				id: province_shape
				x: -map_block_start_x * metternich.defines.province_map_tile_scale
				y: -map_block_start_y * metternich.defines.province_map_tile_scale
				visible: province_polygon_path.length > 0
				
				readonly property var province: model.modelData
				readonly property var province_polygon_path: province.map_data.polygon_path
				readonly property var selected: selected_province === province && (selected_garrison === false || province_map.show_site_mode === ProvinceMap.SiteMode.ShowLocations)
				readonly property var interactive: selected_civilian_unit !== null && !selected_civilian_unit.busy && selected_civilian_unit_interactive_provinces.includes(province)
				property int change_count: 0
				
				ShapePath {
					strokeColor: fillColor
					fillColor: selected ? metternich.defines.selected_country_color : (interactive ? "darkGreen" : get_map_mode_color(province_map.mode, province, change_count)) //the change count is there to force a re-evaluation of the binding if a relevant property has changed
					startX: 0
					startY: 0
					
					PathSvg {
						path: province_polygon_path
					}
				}
				
				Connections {
					target: province ? province.game_data : null
					
					function onMap_image_changed() {
						change_count += 1
					}
					
					function onMap_mode_image_changed(map_mode_identifier) {
						switch (mode) {
							case ProvinceMap.Mode.Political:
								return
							case ProvinceMap.Mode.Terrain:
								if (map_mode_identifier !== "terrain") {
									return
								}
								break
							case ProvinceMap.Mode.Cultural:
								if (map_mode_identifier !== "cultural") {
									return
								}
								break
							case ProvinceMap.Mode.Religious:
								if (map_mode_identifier !== "religious") {
									return
								}
								break
							case ProvinceMap.Mode.Technology:
								if (map_mode_identifier !== "technology") {
									return
								}
								break
							case ProvinceMap.Mode.TradeZone:
								if (map_mode_identifier !== "trade_zone") {
									return
								}
								break
							case ProvinceMap.Mode.Temple:
								if (map_mode_identifier !== "temple") {
									return
								}
								break
							case ProvinceMap.Mode.CulturalSociety:
								if (map_mode_identifier !== "cultural_society") {
									return
								}
								break
						}
						
						change_count += 1
					}
				}
			}
		}
	}
	
	ScaledImage {
		id: scaled_province_map
		width: unscaled_province_map.width * scale_factor
		height: unscaled_province_map.height * scale_factor
        source: unscaled_province_map
	}
}
